#include "getaddrinfo_executer.h"

using namespace std;

getaddrinfo_executer::thread_ptr getaddrinfo_executer::thread_to_destroy = nullptr;
std::mutex getaddrinfo_executer::mutex_to_destroying;

void getaddrinfo_executer::destroy_thread(thread_ptr thread_p) {
	lock_guard<mutex> lg(mutex_to_destroying);
	thread_to_destroy = move(thread_p);
}

getaddrinfo_executer::thread_data::thread_data(getaddrinfo_executer &this_executer) {
	thread_p = make_unique<thread>(getaddrinfo_executer::working_thread, ref(this_executer), ref(*this));
}

getaddrinfo_executer::group_data::group_data(key_t id, int tasks_limit)
: is_deleted(false), id(id), tasks_limit(tasks_limit) {
}

void getaddrinfo_executer::working_thread(getaddrinfo_executer &this_executer, thread_data &data) {
	unique_lock<recursive_mutex> ul(this_executer.m);
	
	while (true) {
		if (this_executer.is_interrupted) {
			return;
		}
		auto &tasks = this_executer.tasks;
		if (tasks.size() > 0) {
			group_ptr group = tasks.front().first;
			func_t task = tasks.front().second;
			tasks.pop();
			
			if (group->is_deleted) {
				continue;
			}
			if (tasks.size() > 0) {
				this_executer.c_v.notify_one();
			}
			
			this_executer.num_of_busy_threads++;
			ul.unlock();
			try {
				task();
			} catch (...) {
				LOG("Exception in task in getaddrinfo_executer::working_thread\n");
			}
			ul.lock();
			this_executer.num_of_busy_threads--;
			
			if (this_executer.is_interrupted) {
				return;
			}
			if (group->is_deleted) {
				continue;
			}
			
			group->tasks_limit++;
			this_executer.move_tasks_to_main_queue(group);
			continue;
		}
		if (this_executer.threads.size() - this_executer.num_of_busy_threads > this_executer.max_number_of_free_threads) {
			std::list<thread_data>::iterator pos;
			{
				lock_guard<mutex> lg(data.m);
				destroy_thread(move(data.thread_p));
				pos = data.pos;
			}
			this_executer.threads.erase(pos);
			return;
		}
		this_executer.c_v.wait(ul);
	}
}

getaddrinfo_executer::getaddrinfo_executer(IO_completion_port &port,
					 int max_number_of_free_threads,
					 int max_number_of_threads,
					 int max_number_of_threads_per_group)
: port(port), max_number_of_free_threads(max_number_of_free_threads),
  max_number_of_threads(max_number_of_threads),
  max_number_of_threads_per_group(max_number_of_threads_per_group) {
	assert(0 <= max_number_of_free_threads);
	assert(max(max_number_of_free_threads, 1) <= max_number_of_threads);
	assert(1 <= max_number_of_threads_per_group);
	
	for (int w = 0; w < max_number_of_free_threads; w++) {
		add_new_thread();
	}
}

void getaddrinfo_executer::execute(key_t group_id, std::string pNodeName, std::string pServiceName, ADDRINFO &pHints, callback_t task) {
	lock_guard<recursive_mutex> lg(m);
	
	if (groups.count(group_id) == 0) {
		groups[group_id] = make_shared<group_data>(group_id, max_number_of_threads_per_group);
	}
	group_ptr group = groups[group_id];
	
	group->group_tasks.push(
		[task, this, group, pNodeName, pServiceName, pHints]() {
			addrinfo *result = nullptr;
			int ret_val = getaddrinfo(&pNodeName[0], &pServiceName[0], &pHints, &result);
			if (ret_val != 0) {
				throw new socket_exception("getaddrinfo failed with error : " + to_string(ret_val) + "\n");
			}
			port.add_task(
				[task, group, result]() {
					try {
						if (!group->is_deleted) {
							task(result);
						}
					} catch (...) {
						LOG("Exception in callback in getaddrinfo_executer::execute");
					}
					freeaddrinfo(result);
				}
			);
		}
	);
	move_tasks_to_main_queue(group);
}

void getaddrinfo_executer::delete_group(key_t group_id) {
	lock_guard<recursive_mutex> lg(m);
	groups[group_id]->is_deleted = true;
	groups.erase(group_id);
}

void getaddrinfo_executer::add_new_thread() {
	lock_guard<recursive_mutex> lg(m);
	
	threads.emplace_back(*this);
	
	lock_guard<mutex> ld_2(threads.back().m);
	threads.back().pos = (--threads.end());
}

void getaddrinfo_executer::move_tasks_to_main_queue(group_ptr group) {
	while ((group->group_tasks.size() > 0) && (group->tasks_limit > 0)) {
		tasks.push({group, move(group->group_tasks.front())});
		group->group_tasks.pop();
		group->tasks_limit--;
		
		if (tasks.size() == 1) {
			c_v.notify_one();
		}
	}
}
