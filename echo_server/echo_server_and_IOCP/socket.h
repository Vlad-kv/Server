#ifndef SOCKET_H
#define SOCKET_H

#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>
#include <functional>
#include <memory>

class server_socket;
class servers_client_socket;
class completion_key;
class abstract_socket;
struct abstract_overlapped;

#include "logger.h"
#include "socket_descriptor.h"

/**
	
*/

class completion_key {
public:
	friend class abstract_socket;
	friend class IO_completion_port;
	
	typedef std::shared_ptr<completion_key> key_ptr;
	typedef std::function<void (DWORD transmited_bytes,
								const key_ptr key,
								abstract_overlapped *overlapped,
								int error)> on_comp_t;
	
	completion_key(abstract_socket *ptr, on_comp_t on_comp);
	abstract_socket* get_ptr() const;
	~completion_key();
private:
	static int debug_counter;
	
	abstract_socket *ptr;
	on_comp_t on_comp;
};

struct abstract_overlapped {
	friend class IO_completion_port;
	
	typedef completion_key::key_ptr key_ptr;
	
	abstract_overlapped(const abstract_socket &this_socket);
public:
	OVERLAPPED overlapped;
private:
	completion_key::key_ptr key;
};

class abstract_socket {
public:
	friend struct abstract_overlapped;
	friend struct IO_completion_port;
	
	typedef completion_key::key_ptr key_ptr;
	typedef completion_key::on_comp_t on_comp_t;
	
	abstract_socket(on_comp_t f);
	abstract_socket(on_comp_t f, socket_descriptor &&sd);
	abstract_socket(on_comp_t f, int address_family, int type, int protocol);
	abstract_socket(abstract_socket &&abs_socket);
	abstract_socket(const abstract_socket &abs_socket) = delete;
	
	abstract_socket& operator=(abstract_socket &&abs_socket);
	void close_comp_key();
	
	~abstract_socket();
	
private:
	key_ptr key;
protected:
	socket_descriptor sd;
};

class servers_client_socket : public abstract_socket {
public:
	typedef std::function<void (size_t)> func_rw_t;
	typedef std::function<void ()> func_disc_t;
	
	typedef completion_key::key_ptr key_ptr;
	typedef completion_key::on_comp_t on_comp_t;
private:
	func_rw_t on_read_completion;
	func_rw_t on_write_completion;
	func_disc_t on_disconnect;
	
	struct servers_client_socket_overlapped : public abstract_overlapped {
		const static int RECV_KEY = 0;
		const static int SEND_KEY = 1;
		
		int type_of_operation;
		WSABUF buff;
		
		servers_client_socket_overlapped(const abstract_socket &this_socket, int type, char* buff, size_t size);
	};
	friend class IO_completion_port;
	
	static void on_completion(DWORD transmited_bytes,
								const key_ptr key,
								abstract_overlapped *overlapped,
								int error);
public:
	servers_client_socket();
	servers_client_socket(socket_descriptor &&sd);
	servers_client_socket(const servers_client_socket &) = delete;
	servers_client_socket(servers_client_socket &&other);
	servers_client_socket(int address_family, int type, int protocol);
	
	void set_on_read_completion(func_rw_t on_read_completion);
	void set_on_write_completion(func_rw_t on_write_completion);
	void set_on_disconnect(func_disc_t on_disconnect);
	
	void read_some(char *buff, size_t size);
	void write_some(const char *buff, size_t size);
	
	void execute_on_disconnect();
	
	unsigned int get_sd() const;
	
	void close();
	~servers_client_socket();
	
	servers_client_socket& operator=(const servers_client_socket &) = delete;
	servers_client_socket& operator=(servers_client_socket&& socket_d);
};

class server_socket : public abstract_socket {
	friend void bind_socket(const server_socket& sock, short family, u_long addr, u_short port);
	
	typedef std::function<void (servers_client_socket)> func_t;
	func_t on_accept = nullptr;
	
	GUID *GuidAcceptEx = nullptr;
	LPFN_ACCEPTEX lpfnAcceptEx = NULL;
	
	void init_AcceptEx();
	
	struct server_socket_overlapped : public abstract_overlapped {
		char buffer[(sizeof (sockaddr_in) + 16) * 2];
		socket_descriptor sd;
		
		server_socket_overlapped(const abstract_socket &this_socket);
	};
	
	friend class IO_completion_port;
	
	static void on_completion(DWORD transmited_bytes,
								const key_ptr key,
								abstract_overlapped *overlapped,
								int error);
public:
	
	server_socket();
	server_socket(socket_descriptor &&sd, func_t on_accept);
	server_socket(const server_socket &) = delete;
	server_socket(server_socket &&other);
	server_socket(int address_family, int type, int protocol, func_t on_accept);
	
	server_socket& operator=(server_socket &&sock);
	
	void bind_and_listen(int address_family, std::string addres_of_main_socket, int port, int backlog = SOMAXCONN);
	void accept(int address_family, int type, int protocol);
	
	SOCKET get_sd() const;
	sockaddr_in get_sock_address();
	
	void close();
	~server_socket();
};

#endif // SOCKET_H
