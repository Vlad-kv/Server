#ifndef IO_COMPLETION_PORT_H
#define IO_COMPLETION_PORT_H

#include <winsock2.h>
#include <windows.h>
#include <memory>
#include <thread>

class IO_completion_port;

#include "socket.h"
#include "logger.h"

class IO_completion_port {
	HANDLE iocp_handle;
	
	struct completion_key_decrementer {
		completion_key* key;
		completion_key_decrementer(completion_key* key);
		~completion_key_decrementer();
	};
public:
	IO_completion_port(HANDLE handle);
	IO_completion_port(const IO_completion_port &) = delete;
	IO_completion_port(IO_completion_port &&port);
	IO_completion_port();
	
	HANDLE get_handle() const;
	
	void registrate_socket(server_socket& sock);
	void registrate_socket(client_socket& sock);
	
	void run_in_this_thread();
	std::unique_ptr<std::thread> run_in_new_thread();
	
	void close();
	~IO_completion_port();
};

#endif // IO_COMPLETION_PORT_H
