#include "timers.h"
#include "cassert"
#include "logger.h"

using namespace std;

void timer::unregistrate() {
	if (port != nullptr) {
		if (it != port->timers.end()) {
			port->timers.erase(it);
		}
		port = nullptr;
	}
}

timer::timer(std::chrono::microseconds interval, func_t on_time_expiration)
: on_time_expiration(on_time_expiration) {
	this->interval = interval;
	expiration = chrono::steady_clock::now() + this->interval;
}

void timer::restart() {
	time_point_t new_expiration = chrono::steady_clock::now() + this->interval;
//	unique_lock<recursive_mutex> ul = lock();
	
	expiration = new_expiration;
	
	if (port != nullptr) {
		if (it != port->timers.end()) {
			port->timers.erase(it);
			it = port->timers.end();
		}
		it = port->timers.emplace(this);
	}
}

bool operator<(const timer& t_1, const timer& t_2) {
	return (t_1.expiration < t_2.expiration);
}

timer::~timer() {
	unregistrate();
}
