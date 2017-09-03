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

timer::timer()
: on_time_expiration(nullptr) {
}
timer::timer(std::chrono::microseconds interval, func_t on_time_expiration)
: on_time_expiration(on_time_expiration) {
	this->interval = interval;
	expiration = chrono::steady_clock::now() + this->interval;
}
timer::timer(timer &&t)
: on_time_expiration(move(t.on_time_expiration)),
  expiration(t.expiration), interval(t.interval) {
	t.unregistrate();
	port->registrate_timer(*this);
}

void timer::restart() {
	time_point_t new_expiration = chrono::steady_clock::now() + this->interval;
	
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
timer& timer::operator=(timer &&t) {
	unregistrate();
	
	on_time_expiration = move(t.on_time_expiration);
	expiration = t.expiration;
	interval = t.interval;
	port = t.port;
	
	t.unregistrate();
	if (port != nullptr) {
		port->registrate_timer(*this);
	}
	return *this;
}
timer::~timer() {
	unregistrate();
}
