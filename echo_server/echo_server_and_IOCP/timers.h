#ifndef TIMERS_H
#define TIMERS_H

#include <functional>
#include <chrono>
#include <ctime>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <set>
#include <thread>

class timer_thread;
class timer;

class timer_thread {
	struct timer_holder {
		timer *t;
		timer_holder(timer *t);
	};
	
	friend bool operator<(const timer_thread::timer_holder& t_1, const timer_thread::timer_holder& t_2);
	friend class timer;
	
	std::unique_ptr<std::thread> this_thread;
	std::multiset<timer_holder> timers;
	std::recursive_mutex m;
	std::condition_variable_any c_v;
	bool is_initialaised = false;
	bool is_interrupted = false;
	
	bool is_latest_timer_expired();
	static void start_new_thread(timer_thread *t_t);
	
public:
	timer_thread();
	
	void registrate_timer(timer& t);
	void interrupt();
};

class timer {
	typedef std::function<void ()> func_t;
	typedef std::chrono::time_point<std::chrono::steady_clock> time_point_t;
	
	friend class timer_thread;
	friend bool operator<(const timer_thread::timer_holder& t_1, const timer_thread::timer_holder& t_2);
	
	func_t on_time_expiration;
	time_point_t expiration;
	std::chrono::microseconds interval;
	timer_thread *t_t = nullptr;
	std::set<timer_thread::timer_holder>::iterator it;
	// t_t == nullptr - незарегестрирован, t_t != nullptr - если it == t_t->timers.end(),
	// то timer не находится в ожидании, но может возродиться через restart.
	
	void unregistrate();
	
	std::unique_lock<std::recursive_mutex> lock();
	
public:
	
	timer(std::chrono::microseconds interval, func_t on_time_expiration);
	
	void restart();
	
	friend bool operator<(const timer& t_1, const timer& t_2);
	
	~timer();
};

#endif // TIMERS_H
