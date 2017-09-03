#ifndef IO_COMPLETION_PORT_H
#define IO_COMPLETION_PORT_H

#include <winsock2.h>
#include <windows.h>
#include <set>
#include <atomic>
#include <vector>
#include <mutex>
#include <queue>
#include <memory>

class IO_completion_port;
class timer_holder;
class registration;

#include "socket.h"
#include "logger.h"
#include "timers.h"

struct timer_holder {
	timer *t;
	timer_holder(timer *t = nullptr);
	
	friend bool operator<(const timer_holder& t_1, const timer_holder& t_2);
};

class registration {
public:
	friend class IO_completion_port;
	
	registration& operator=(registration &&reg);
	
	registration(registration &&reg);
	registration();
	~registration();
private:
	registration(std::shared_ptr<bool> data_ptr);
	
	std::shared_ptr<bool> data_ptr;
};

const int MAX_TIME_TO_WAIT = 500;
static_assert(MAX_TIME_TO_WAIT > 0);

class IO_completion_port {
public:
	typedef std::function<void ()> func_t;
private:
	friend timer;
	friend struct abstract_overlapped;
	
	HANDLE iocp_handle;
	std::multiset<timer_holder> timers;
	socket_descriptor to_notify;
	char buff_to_notify[1];
	std::unique_ptr<client_socket> notification_socket;
	static std::atomic_bool is_interrapted;
	bool is_terminated = false;
	std::atomic_flag is_already_started;
	std::vector<func_t> on_interrupt_f;
	std::queue<func_t> tasks;
	std::mutex m;
	
	std::shared_ptr<IO_completion_port*> port_ptr = std::make_shared<IO_completion_port*>(this);
	
	static void signal_handler(int signal);
	
	int get_time_to_wait();
	void wait_to_comp_and_exec(int time_to_wait);
	
	void termination();
	
public:
	IO_completion_port(const IO_completion_port &) = delete;
	IO_completion_port(IO_completion_port &&port) = delete;
	IO_completion_port();
	
	void registrate_socket(abstract_socket& sock);
	void registrate_timer(timer& t);
	
	registration registrate_on_interruption_event(func_t func);
	
	void interrupt();
	void notify();
	
	void add_task(func_t func);
	
	void start();
private:
	void close();
public:
	~IO_completion_port();
};

#endif // IO_COMPLETION_PORT_H
