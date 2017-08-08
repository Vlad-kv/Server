#ifndef TIMERS_H
#define TIMERS_H

#include <functional>
#include <chrono>
#include <ctime>
#include <set>

class timer;

#include "IO_completion_port.h"

class timer {
	typedef std::function<void (timer *this_timer)> func_t;
	typedef std::chrono::time_point<std::chrono::steady_clock> time_point_t;
	
	friend class IO_completion_port;
	friend class timer_holder;
	
	func_t on_time_expiration;
	time_point_t expiration;
	std::chrono::microseconds interval;
	IO_completion_port *port = nullptr;
	std::set<timer_holder>::iterator it;
	// port == nullptr - незарегестрирован, port != nullptr - если it == port->timers.end(),
	// то timer не находится в ожидании, но может возродиться через restart.
	
public:
	
	timer(std::chrono::microseconds interval, func_t on_time_expiration);
	timer(timer &&t);
	
	void unregistrate();
	void restart();
	
	friend bool operator<(const timer& t_1, const timer& t_2);
	timer& operator=(timer &&t);
	timer& operator=(const timer& t) = delete;
	
	~timer();
};

#endif // TIMERS_H
