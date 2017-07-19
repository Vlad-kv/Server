#include "abstract_server.h"

long long abstract_server::get_unic_id() {
	return (next_id++);
}

void abstract_server::create_client_socket_2(abstract_server &this_server, client_socket client_s) {
	this_server.comp_port->registrate_socket(client_s);
	
	client_socket_2 client_s_2(std::move(client_s), &this_server, this_server.get_unic_id());
	
	// TODO
	
	this_server.on_accept(std::move(client_s_2));
}

abstract_server::abstract_server(std::string addres_of_main_socket, int address_family, 
									int type, int protocol, int port, IO_completion_port *comp_port)
: address_family(address_family), type(type), protocol(protocol),
  s_soc(address_family, type, protocol, std::bind(create_client_socket_2, std::ref(*this), std::placeholders::_1)),
  comp_port(comp_port) {
	
	s_soc.bind_and_listen(address_family, addres_of_main_socket, port);
	
	if (comp_port == nullptr) {
		this->comp_port = new IO_completion_port();
		thread_where_comp_port_runned = this->comp_port->run_in_new_thread();
		this->comp_port->registrate_socket(s_soc);
	} else {
		this->comp_port->registrate_socket(s_soc);
	}
}

void abstract_server::start() {
	accept();
}

void abstract_server::accept() {
	s_soc.accept(address_family, type, protocol);
}
