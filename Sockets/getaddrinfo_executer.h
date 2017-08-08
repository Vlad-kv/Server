#ifndef GETADDRINFO_EXECUTER_H
#define GETADDRINFO_EXECUTER_H

#include <ws2tcpip.h>
#include <cstring>
#include <thread>
#include <memory>
#include <list>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <map>
#include <cassert>
#include <atomic>

class getaddrinfo_executer;

#include "IO_completion_port.h"

class getaddrinfo_executer {
public:
	// Предже всего берётся getaddrinfo_executer::m, затем thread_data::m
	typedef long long key_t;
	
	struct group_data;
	struct thread_data;
	
	typedef std::unique_ptr<std::thread> thread_ptr;
	typedef IO_completion_port::func_t func_t;
	typedef std::shared_ptr<group_data> group_ptr;
	typedef std::function<void (addrinfo *info)> callback_t;
	
	struct thread_data {
		thread_data(getaddrinfo_executer &this_executer);
		
		thread_ptr thread_p;
		std::mutex m;
		std::list<thread_data>::iterator pos;
	};
	
	struct group_data {
		group_data(key_t id, int tasks_limit);
		
		std::queue<func_t> group_tasks;
		std::atomic_bool is_deleted;
		key_t id;
		int tasks_limit;
	};
	
	getaddrinfo_executer(IO_completion_port &port,
						 size_t max_number_of_free_threads,
						 size_t max_number_of_threads,
						 size_t max_number_of_threads_per_group);
	
	void execute(key_t group_id, std::string pNodeName, std::string pServiceName, const ADDRINFO &pHints, callback_t task);
	void delete_group(key_t group_id);
	void interrupt();
	
	~getaddrinfo_executer();
private:
	static void working_thread(getaddrinfo_executer &this_executer, thread_data &data);
	static void destroy_thread(thread_ptr thread_p = nullptr);
	
	void add_new_thread();
	void move_tasks_to_main_queue(group_ptr group);
	
	void notify();
private:
	std::map<key_t, group_ptr> groups;
	IO_completion_port &port;
	std::list<thread_data> threads;
	std::queue<std::pair<group_ptr, func_t>> tasks;
	
	const size_t max_number_of_free_threads;
	const size_t max_number_of_threads;
	const size_t max_number_of_threads_per_group;
	
	int num_of_busy_threads = 0;
	bool is_interrupted = false;
	
	std::recursive_mutex m;
	std::condition_variable_any c_v;
	
	static thread_ptr thread_to_destroy;
	static std::mutex mutex_to_destroying;
};

#endif // GETADDRINFO_EXECUTER_H
