#ifndef CLIENT_SOCKET_2_H
#define CLIENT_SOCKET_2_H

#include <functional>
#include <cassert>

class client_socket_2;

#include "socket.h"
#include "abstract_server.h"

class client_socket_2 {
	friend class abstract_server;
	
	typedef client_socket::func_read_t  func_read_t;
	typedef client_socket::func_write_t func_write_t;
	typedef client_socket::func_t  func_t;
	
	client_socket c_s;
	long long id = -1;
	
	client_socket_2(client_socket &&c_s, long long id);
	client_socket_2(const client_socket_2 &) = delete;
	
public:
	client_socket_2();
	client_socket_2(client_socket_2 &&c_s_2);
	client_socket_2& operator=(client_socket_2 &&c_s_2);
	
	void set_on_read_completion(func_read_t on_read_completion);
	void set_on_write_completion(func_write_t on_write_completion);
	void set_on_disconnect(func_t on_disconnect);
	
	void read_some();
	void write_some(const char *buff, size_t size);
	void write_some_saved_bytes();
	size_t get_num_of_saved_bytes();
	
	bool is_reading_available();
	bool is_writing_available();
	
	void shutdown_reading();
	void shutdown_writing();
	
	void execute_on_disconnect();
	
	long long get_id() const;
};

#endif // CLIENT_SOCKET_2_H
