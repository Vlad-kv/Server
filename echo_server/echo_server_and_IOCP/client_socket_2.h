#ifndef CLIENT_SOCKET_2_H
#define CLIENT_SOCKET_2_H

#include <functional>
#include <cassert>

class client_socket_2;

#include "socket.h"
#include "abstract_server.h"

class client_socket_2 {
	friend class abstract_server;
	
	typedef client_socket::func_real_rw_t   func_real_rw_t;
	typedef client_socket::func_rw_t        func_rw_t;
	typedef client_socket::func_real_disc_t func_real_disc_t;
	typedef client_socket::func_disc_t      func_disc_t;
	
	client_socket c_s;
	abstract_server *abs_server;
	long long id = -1;
	
	client_socket_2(client_socket &&c_s, abstract_server *abs_server, long long id);
	client_socket_2(const client_socket_2 &) = delete;
	
public:
	client_socket_2();
	client_socket_2(client_socket_2 &&c_s_2);
	client_socket_2& operator=(client_socket_2 &&c_s_2);
	
	void set_on_read_completion(func_real_rw_t on_read_completion);
	void set_on_read_completion(func_rw_t on_read_completion);
	
	void set_on_write_completion(func_real_rw_t on_write_completion);
	void set_on_write_completion(func_rw_t on_write_completion);
	
	void set_on_disconnect(func_real_disc_t on_disconnect);
	void set_on_disconnect(func_disc_t on_disconnect);
	
	void read_some(char *buff, size_t size);
	void write_some(const char *buff, size_t size);
	
	void execute_on_disconnect();
	
	long long get_id() const;
	
	void add_timer() {
		// TODO
	}
	
	~client_socket_2() {
		// TODO
	}
};

#endif // CLIENT_SOCKET_2_H
