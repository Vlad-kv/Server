#ifndef IO_COMPLETION_PORT_H
#define IO_COMPLETION_PORT_H

#include <winsock2.h>
#include <windows.h>
#include <memory>
#include <thread>
#include <set>

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
	friend timer;
	
	static const int MAX_TIME_TO_WAIT = 500;
	static_assert(MAX_TIME_TO_WAIT > 0);
	
	HANDLE iocp_handle;
	std::multiset<timer_holder> timers;
	socket_descriptor to_notify;
	char buff_to_notify[1];
	client_socket notification_socket;
	
	struct completion_key_decrementer {
		completion_key* key;
		completion_key_decrementer(completion_key* key);
		~completion_key_decrementer();
	};
	
	int get_time_to_wait();
public:
	IO_completion_port(const IO_completion_port &) = delete;
	IO_completion_port(IO_completion_port &&port);
	IO_completion_port();
	
	HANDLE get_handle() const;
	
	void registrate_socket(server_socket& sock);
	void registrate_socket(client_socket& sock);
	void registrate_timer(timer& t);
	
	void run_in_this_thread();
	std::unique_ptr<std::thread> run_in_new_thread();
	
	void close();
	~IO_completion_port();
};

#endif // IO_COMPLETION_PORT_H
