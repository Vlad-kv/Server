#ifndef ABSTRACT_SERVER_H
#define ABSTRACT_SERVER_H

class abstract_server;

#include "socket.h"
#include "../echo_server/sockets_wrapper.h"
#include "servers_client_socket_2.h"
#include "IO_completion_port.h"

class abstract_server {
public:
	typedef IO_completion_port::func_t func_t;
private:
	friend class servers_client_socket_2;
	
	const int address_family, type, protocol;
	server_socket s_soc;
	IO_completion_port &comp_port;
	long long next_id = 0;
	registration on_int_reg;
	
	static void create_client_socket_2(abstract_server &this_server, servers_client_socket client_s);
	
	long long get_unic_id();
public:
	abstract_server(std::string addres_of_main_socket, int address_family, int type, int protocol, int port, IO_completion_port &comp_port);
	
	void registrate_timer(timer& t);
	void add_task(func_t func);
protected:
	void accept();
	
	virtual void on_accept(servers_client_socket_2 client_s) = 0;
	virtual void on_interruption() = 0;
};

#endif // ABSTRACT_SERVER_H
