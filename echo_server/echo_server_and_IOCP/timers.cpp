#include "timers.h"
#include "cassert"
#include "logger.h"

using namespace std;

timer_thread::timer_holder::timer_holder(timer *t)
: t(t) {
}

bool operator<(const timer_thread::timer_holder& t_1, const timer_thread::timer_holder& t_2) {
	return (t_1.t->expiration < t_2.t->expiration);
}

bool timer_thread::is_latest_timer_expired() {
	if (timers.size() == 0) {
		return false;
	}
	return ((*timers.begin()).t->expiration <= chrono::steady_clock::now());
}

void timer_thread::start_new_thread(timer_thread *t_t) {
	unique_lock<recursive_mutex> ul(t_t->m);
	t_t->is_initialaised = true;
	t_t->c_v.notify_one();
	
	LOG("in start_new_thread, before wait");
	
	t_t->c_v.wait(ul);
	
	while (!t_t->is_interrupted) {
		cout << "????\n";
		if (t_t->is_latest_timer_expired()) {
			timer *t = (*t_t->timers.begin()).t;
			
			assert(t->it == t_t->timers.begin());
			
			t_t->timers.erase(t_t->timers.begin());
			t->it = t_t->timers.end();
			
			timer::func_t temp = t->on_time_expiration;
			try {
				temp();
			} catch (...) {
				LOG("Exception in timer_thread!!!\n");
				/// TODO
			}
			continue;
		}
		if (t_t->timers.size() == 0) {
			t_t->c_v.wait(ul);
		} else {
			cout << "wait_until " << chrono::duration_cast<std::chrono::microseconds>(((*t_t->timers.begin()).t->expiration) - chrono::steady_clock::now()).count() << "\n";
			
			t_t->c_v.wait_until(ul,  (*t_t->timers.begin()).t->expiration);
			cout << "after wait_until\n";
		}
	}
}

timer_thread::timer_thread() {
	unique_lock<recursive_mutex> ul(m);
	this_thread = make_unique<thread>(start_new_thread, this);
	while (!is_initialaised) {
		c_v.wait(ul);
	}
}

void timer_thread::registrate_timer(timer& t) {
	unique_lock<recursive_mutex> lg(m);
	
	t.unregistrate();
	t.t_t = this;
	t.it = timers.emplace(&t);
	
	lg.unlock();
	lg.release();
	
	LOG("in timer_thread::registrate_timer\n");
	
	c_v.notify_one();
	LOG("###\n");
}

void timer_thread::interrupt() {
	lock_guard<recursive_mutex> lg(m);
	is_interrupted = true;
	c_v.notify_one();
}

///-----------------

void timer::unregistrate() {
	if (t_t != nullptr) {
		lock_guard<recursive_mutex> lg(t_t->m);
		if (it != t_t->timers.end()) {
			t_t->timers.erase(it);
		}
		t_t = nullptr;
	}
}

std::unique_lock<std::recursive_mutex> timer::lock() {
	if (t_t == nullptr) {
		return unique_lock<recursive_mutex>();
	} else {
		return unique_lock<recursive_mutex>(t_t->m);
	}
}

timer::timer(std::chrono::microseconds interval, func_t on_time_expiration)
: on_time_expiration(on_time_expiration) {
	this->interval = interval;
	expiration = chrono::steady_clock::now() + this->interval;
}

void timer::restart() {
	time_point_t new_expiration = chrono::steady_clock::now() + this->interval;
	unique_lock<recursive_mutex> ul = lock();
	
	expiration = new_expiration;
	
	if (t_t != nullptr) {
		if (it != t_t->timers.end()) {
			t_t->timers.erase(it);
			it = t_t->timers.end();
		}
		it = t_t->timers.emplace(this);
	}
}

bool operator<(const timer& t_1, const timer& t_2) {
	return (t_1.expiration < t_2.expiration);
}

timer::~timer() {
	unregistrate();
}
