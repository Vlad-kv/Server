#include "abstract_server.h"

using namespace std;

long long abstract_server::get_unic_id() {
	return (next_id++);
}

void abstract_server::create_client_socket_2(abstract_server &this_server, client_socket client_s) {
	this_server.comp_port.registrate_socket(client_s);
	
	client_socket_2 client_s_2(move(client_s), this_server.get_unic_id());
	
	this_server.on_accept(move(client_s_2));
}

abstract_server::abstract_server(std::string addres_of_main_socket, int address_family, 
									int type, int protocol, int port, IO_completion_port &comp_port)
: address_family(address_family), type(type), protocol(protocol),
  s_soc(address_family, type, protocol, bind(create_client_socket_2, ref(*this), placeholders::_1)),
  comp_port(comp_port) {
	on_int_reg = move(comp_port.registrate_on_interruption_event(
		[this]() {
			s_soc.close();
			on_interruption();
		}
	));
	
	s_soc.bind_and_listen(address_family, addres_of_main_socket, port);
	comp_port.registrate_socket(s_soc);
	accept();
}

void abstract_server::registrate_timer(timer& t) {
	comp_port.registrate_timer(t);
}
void abstract_server::add_task(func_t func) {
	comp_port.add_task(func);
}

void abstract_server::accept() {
	s_soc.accept(address_family, type, protocol);
}
