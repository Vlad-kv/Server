#ifndef SOCKET_H
#define SOCKET_H

#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>
#include <functional>
#include <memory>

class server_socket;
class client_socket;
class completion_key;
class abstract_socket;
struct abstract_overlapped;

#include "logger.h"
#include "socket_descriptor.h"
#include "IO_completion_port.h"

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
	static int counter_of_items;
	
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
	bool is_registrated = false;
	std::shared_ptr<IO_completion_port*> port_ptr = nullptr;
protected:
	socket_descriptor sd;
};

struct recv_send_overlapped : public abstract_overlapped {
	friend class client_socket;
	
	const static int RECV_KEY = 0;
	const static int SEND_KEY = 1;
	
	const int type_of_operation;
	WSABUF buff;
	void *buff_ptr;
	
	recv_send_overlapped(const abstract_socket &this_socket, int type, char* buff, size_t size, void *buff_ptr);
	void* release_buff_ptr();
	~recv_send_overlapped();
};

class buffer_to_read {
public:
	friend class client_socket;
	
	buffer_to_read(size_t size);
	void resize(size_t new_size);
	~buffer_to_read();
private:
	size_t size;
	char *buffer;
};

class buffer_to_write {
public:
	friend class client_socket;
	
	buffer_to_write();
	size_t get_saved_size();
	void reset();
	void write_to_buffer(const char *buff, size_t size);
	
private:
	size_t start_pos;
	std::vector<char> buffer;
};

class client_socket : public abstract_socket {
public:
	static const size_t DEFAULT_BUFFER_SIZE = 1024;
	
	typedef std::function<void (const char* buff, size_t transmitted_bytes)> func_read_t;
	typedef std::function<void (size_t saved_bytes, size_t transmitted_bytes)> func_write_t;
	typedef std::function<void ()> func_disc_t;
	
	typedef completion_key::key_ptr key_ptr;
	typedef completion_key::on_comp_t on_comp_t;
	
private:
	func_read_t on_read_completion;
	func_write_t on_write_completion;
	func_disc_t on_disconnect;
	
	bool is_connected = false;
	buffer_to_read *b_to_read = nullptr;
	buffer_to_write *b_to_write = nullptr;
	
	friend class IO_completion_port;
	friend class server_socket;
	
	static void on_completion(DWORD transmited_bytes, const key_ptr key,
								abstract_overlapped *overlapped, int error);
public:
	client_socket();
	client_socket(socket_descriptor &&sd, size_t read_buffer_size = DEFAULT_BUFFER_SIZE);
	client_socket(const client_socket &) = delete;
	client_socket(client_socket &&other);
	client_socket(int address_family, int type, int protocol, size_t read_buffer_size = DEFAULT_BUFFER_SIZE);
	
	void set_on_read_completion(func_read_t on_read_completion);
	void set_on_write_completion(func_write_t on_write_completion);
	void set_on_disconnect(func_disc_t on_disconnect);
	
	void read_some();
	void write_some(const char *buff, size_t size);
	void write_some_saved_bytes();
	size_t get_num_of_saved_bytes();
	void connect(short family, const std::string& addr, u_short port);
	
	bool is_reading_available();
	bool is_writing_available();
	
	void shutdown_reading();
	void shutdown_writing();
	
	void reset_write_buffer();
	
	void execute_on_disconnect();
	
	unsigned int get_sd() const;
	
	void close();
	~client_socket();
	
	client_socket& operator=(const client_socket &) = delete;
	client_socket& operator=(client_socket&& socket_d);
};

class server_socket : public abstract_socket {
	friend void bind_socket(const server_socket& sock, short family, u_long addr, u_short port);
	
	typedef std::function<void (client_socket)> func_t;
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
	
	static void on_completion(DWORD transmited_bytes, const key_ptr key,
								abstract_overlapped *overlapped, int error);
public:
	
	server_socket();
	server_socket(socket_descriptor &&sd, func_t on_accept);
	server_socket(const server_socket &) = delete;
	server_socket(server_socket &&other);
	server_socket(int address_family, int type, int protocol, func_t on_accept);
	
	server_socket& operator=(server_socket &&sock);
	
	void bind_and_listen(short address_family, std::string socket_address, int port, int backlog = SOMAXCONN);
	void accept(int address_family, int type, int protocol);
	
	SOCKET get_sd() const;
	sockaddr_in get_sock_address();
	
	void close();
	~server_socket();
};

#endif // SOCKET_H
