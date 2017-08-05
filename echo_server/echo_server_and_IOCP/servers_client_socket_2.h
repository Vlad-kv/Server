#ifndef SERVERS_CLIENT_SOCKET_2_H
#define SERVERS_CLIENT_SOCKET_2_H

#include <functional>
#include <cassert>

class servers_client_socket_2;

#include "socket.h"
#include "abstract_server.h"

class servers_client_socket_2 {
	friend class abstract_server;
	
	typedef servers_client_socket::func_rw_t    func_rw_t;
	typedef servers_client_socket::func_disc_t  func_disc_t;
	
	servers_client_socket c_s;
	abstract_server *abs_server;
	long long id = -1;
	
	servers_client_socket_2(servers_client_socket &&c_s, abstract_server *abs_server, long long id);
	servers_client_socket_2(const servers_client_socket_2 &) = delete;
	
public:
	servers_client_socket_2();
	servers_client_socket_2(servers_client_socket_2 &&c_s_2);
	servers_client_socket_2& operator=(servers_client_socket_2 &&c_s_2);
	
	
	void set_on_read_completion(func_rw_t on_read_completion);
	void set_on_write_completion(func_rw_t on_write_completion);
	void set_on_disconnect(func_disc_t on_disconnect);
	
	void read_some(char *buff, size_t size);
	void write_some(const char *buff, size_t size);
	
	void execute_on_disconnect();
	
	long long get_id() const;
};

#endif // SERVERS_CLIENT_SOCKET_2_H
