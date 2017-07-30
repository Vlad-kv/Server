#ifndef IO_COMPLETION_PORT_H
#define IO_COMPLETION_PORT_H

#include <winsock2.h>
#include <windows.h>
#include <set>
#include <atomic>
#include <vector>
#include <mutex>
#include <list>

class IO_completion_port;
class timer_holder;

#include "socket.h"
#include "logger.h"
#include "timers.h"

struct timer_holder {
	timer *t;
	timer_holder(timer *t = nullptr);
	
	friend bool operator<(const timer_holder& t_1, const timer_holder& t_2);
};

class IO_completion_port {
public:
	typedef std::function<void ()> func_t;
private:
	friend timer;
	
	static const int MAX_TIME_TO_WAIT = 500;
	static_assert(MAX_TIME_TO_WAIT > 0);
	
	HANDLE iocp_handle;
	std::multiset<timer_holder> timers;
	socket_descriptor to_notify;
	char buff_to_notify[1], buff_to_get_notification[1];
	client_socket notification_socket;
	static std::atomic_bool is_interrapted;
	std::atomic_flag is_already_started;
	std::vector<func_t> on_interrupt_f;
	std::list<func_t> tasks;
	std::mutex m;
	
	struct completion_key_decrementer {
		completion_key* key;
		completion_key_decrementer(completion_key* key);
		~completion_key_decrementer();
	};
	
	static void signal_handler(int signal);
	
	int get_time_to_wait();
	
	void termination();
	
public:
	IO_completion_port(const IO_completion_port &) = delete;
	IO_completion_port(IO_completion_port &&port) = delete;
	IO_completion_port();
	
	void registrate_socket(server_socket& sock);
	void registrate_socket(client_socket& sock);
	void registrate_timer(timer& t);
	void registrate_on_interruption_event(func_t func);
	
	void notify();
	
	void add_task(func_t func);
	
	void start();
private:
	void close();
public:
	~IO_completion_port();
};

#endif // IO_COMPLETION_PORT_H
