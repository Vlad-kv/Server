#ifndef GETADDRINFO_EXECUTER_H
#define GETADDRINFO_EXECUTER_H

#include <thread>
#include <memory>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <map>
#include <cassert>

class getaddrinfo_executer;

#include "IO_completion_port.h"

class getaddrinfo_executer {
	// Предже всего берётся getaddrinfo_executer::m, затем 
	typedef long long key_t;
	
	struct group_data;
	struct thread_data;
	
	typedef std::unique_ptr<std::thread> thread_ptr;
	typedef IO_completion_port::func_t func_t;
	typedef std::shared_ptr<group_data> group_ptr;
	
	static const key_t NO_GROUP = -1;
	
	struct thread_data {
		thread_data(getaddrinfo_executer &this_executer);
		
		thread_ptr thread_p;
		std::mutex m;
		std::list<thread_data>::iterator pos;
	};
	
	struct group_data {
		std::queue<func_t> group_tasks;
		bool is_deleted = false;
		key_t id;
	};
	
	static void working_thread(getaddrinfo_executer &this_executer, thread_data &data);
	
	static void destroy_thread(thread_ptr thread_p = nullptr);
	
	getaddrinfo_executer(IO_completion_port &port,
						 int max_number_of_free_threads,
						 int max_number_of_threads);
	
	void execute(key_t group_id, func_t task);
	
private:
	void add_new_thread();
	
private:
	std::map<key_t, group_ptr> groups;
	IO_completion_port &port;
	std::list<thread_data> threads;
	std::queue<std::pair<key_t, func_t>> tasks;
	
	const int max_number_of_free_threads;
	const int max_number_of_threads;
	
	int num_of_busy_threads = 0;
	bool is_interrupted = false;
	
	std::recursive_mutex m;
	std::condition_variable_any c_v;
	
	static thread_ptr thread_to_destroy;
	static std::mutex mutex_to_destroying;
};

#endif // GETADDRINFO_EXECUTER_H
