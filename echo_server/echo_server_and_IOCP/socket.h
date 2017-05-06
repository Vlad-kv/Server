#ifndef SOCKET_H
#define SOCKET_H

#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>
#include <functional>

#include "logger.h"
#include "socket_descriptor.h"

class server_socket;
class client_socket;

class completion_key {
	public:
	
	static const int CLIENT_SOCKET_KEY = 0;
	static const int SERVER_SOCKET_KEY = 1;
	
	size_t num_referenses;
	int type;
	void *ptr;
	
	completion_key(client_socket* client);
	completion_key(server_socket* server);
};

class client_socket {
	typedef std::function<void (client_socket&, size_t)> func_real_rw_t;
	typedef std::function<void (size_t)> func_rw_t;
	typedef std::function<void (client_socket&)> func_real_disc_t;
	typedef std::function<void ()> func_disc_t;
	
	socket_descriptor sd;
	
	func_rw_t on_read_completion;
	func_rw_t on_write_completion;
	func_disc_t on_disconnect;
	
	completion_key *key;
	
	struct client_socket_overlapped {
		const static int RECV_KEY = 0;
		const static int SEND_KEY = 1;
		
		OVERLAPPED overlapped;
		int type_of_operation;
		WSABUF buff;
		
		client_socket_overlapped(int type, char* buff, size_t size);
	};
	friend class IO_completion_port;
public:
	client_socket();
	client_socket(socket_descriptor &&sd);
	client_socket(const client_socket &) = delete;
	client_socket(client_socket &&other);
	
	void set_on_read_completion(func_real_rw_t on_read_completion);
	void set_on_write_completion(func_real_rw_t on_write_completion);
	void set_on_disconnect(func_real_disc_t on_disconnect);
	
	void read_some(char *buff, size_t size);
	void write_some(const char *buff, size_t size);
	
	void invalidate();
	unsigned int get_sd() const;
	
	void close();
	~client_socket();
	
	client_socket& operator=(const client_socket &) = delete;
	client_socket& operator=(client_socket&& socket_d);
};

class server_socket {
	typedef std::function<void (client_socket)> func_t;
	func_t on_accept;
	
	socket_descriptor sd;
	GUID *GuidAcceptEx;
	LPFN_ACCEPTEX lpfnAcceptEx = NULL;
	completion_key *key;
	
	void init_AcceptEx();
	
	struct server_socket_overlapped {
		OVERLAPPED overlapped;
		char buffer[(sizeof (sockaddr_in) + 16) * 2];
		socket_descriptor sd;
		
		server_socket_overlapped();
	};
	
	friend class IO_completion_port;
public:
	
	server_socket();
	server_socket(socket_descriptor &&sd, func_t on_accept);
	server_socket(const server_socket &) = delete;
	server_socket(server_socket &&other);
	
	void accept(int address_family, int type, int protocol);
	
	void close();
	~server_socket();
};

#endif // SOCKET_H
