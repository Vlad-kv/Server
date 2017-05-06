#ifndef SOCKET_H
#define SOCKET_H

#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>
#include <functional>

#include "logger.h"
#include "boost/variant.hpp"
#include "socket_descriptor.h"

class client_socket {
	typedef std::function<void ()> func_t;
	
	socket_descriptor sd;
	
	func_t on_read_completion;
	func_t on_write_completion;
	
public:
	client_socket();
	client_socket(socket_descriptor &&sd);
	client_socket(const client_socket &) = delete;
	client_socket(client_socket &&other);
	
	void set_on_read_completion(func_t on_read_completion);
	void set_on_write_completion(func_t on_write_completion);
	
	void read_some(char *buff, size_t size) {
		// TODO
	}
	
	void write_some(const char *buff, size_t size) {
		// TODO
	}
	
	void invalidate();
	unsigned int get_sd() const;
	
	void close();
	
	client_socket& operator=(const client_socket &) = delete;
	client_socket& operator=(client_socket&& socket_d);
};

class server_socket {
	socket_descriptor sd;
	GUID *GuidAcceptEx;
	LPFN_ACCEPTEX lpfnAcceptEx = NULL;
	
	void init_AcceptEx();
	
public:
	
	server_socket();
	server_socket(socket_descriptor &&sd);
	server_socket(const server_socket &) = delete;
	server_socket(server_socket &&other);
	
	void accept() {
		// TODO
	}
	
	void close();
	
	~server_socket();
};

class completion_key {
	boost::variant<client_socket> var;
	
//	client_socket sd;
	
};

#endif // SOCKET_H
