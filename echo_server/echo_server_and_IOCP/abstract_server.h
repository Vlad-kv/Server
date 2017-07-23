#ifndef ABSTRACT_SERVER_H
#define ABSTRACT_SERVER_H

#include <memory>

class abstract_server;

#include "socket.h"
#include "../echo_server/sockets_wrapper.h"
#include "client_socket_2.h"
#include "IO_completion_port.h"

class abstract_server {
private:
	friend class client_socket_2;
	
	const int address_family, type, protocol;
	server_socket s_soc;
	std::unique_ptr<std::thread> thread_where_comp_port_runned;
	IO_completion_port *comp_port;
	long long next_id = 0;
	
	static void create_client_socket_2(abstract_server &this_server, client_socket client_s);
	
	long long get_unic_id();
public:
	abstract_server(string addres_of_main_socket, int address_family, int type, int protocol, int port, IO_completion_port *comp_port = nullptr);
	
	void start();
	void registrate_timer(timer& t);
protected:
	void accept();
	
	virtual void on_accept(client_socket_2 client_s) = 0;
};

#endif // ABSTRACT_SERVER_H
