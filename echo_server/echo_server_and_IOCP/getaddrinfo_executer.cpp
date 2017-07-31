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

void getaddrinfo_executer::working_thread(getaddrinfo_executer &this_executer, thread_data &data) {
	unique_lock<recursive_mutex> ul(this_executer.m);
	
	while (true) {
		if (this_executer.is_interrupted) {
			return;
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
		if (this_executer.tasks.size() > 0) {
			// TODO
			
			continue;
		}
		this_executer.c_v.wait(ul);
	}
}

getaddrinfo_executer::getaddrinfo_executer(IO_completion_port &port,
					 int max_number_of_free_threads,
					 int max_number_of_threads)
: port(port), max_number_of_free_threads(max_number_of_free_threads),
  max_number_of_threads(max_number_of_threads) {
	assert(0 <= max_number_of_free_threads);
	assert(max(max_number_of_free_threads, 1) <= max_number_of_threads);
	
	for (int w = 0; w < max_number_of_free_threads; w++) {
		add_new_thread();
	}
}

void getaddrinfo_executer::execute(key_t group_id, func_t task) {
	assert(group_id != NO_GROUP);
	
	// TODO
}

void getaddrinfo_executer::add_new_thread() {
	lock_guard<recursive_mutex> lg(m);
	
	threads.emplace_back(*this);
	
	lock_guard<mutex> ld_2(threads.back().m);
	threads.back().pos = (--threads.end());
}
