#ifndef IO_COMPLETION_PORT_H
#define IO_COMPLETION_PORT_H

#include <winsock2.h>
#include <windows.h>

#include "socket.h"
#include "logger.h"

class IO_completion_port {
	HANDLE iocp_handle;
	
public:
	IO_completion_port(HANDLE handle);
	IO_completion_port(const IO_completion_port &) = delete;
	IO_completion_port(IO_completion_port &&port);
	IO_completion_port();
	
	HANDLE get_handle() const;
	
	void close();
	
	void registrate_socket() {
	}
	
	void run() {
	}
	
	~IO_completion_port();
};

#endif // IO_COMPLETION_PORT_H
