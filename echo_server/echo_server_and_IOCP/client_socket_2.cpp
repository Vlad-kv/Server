#include "client_socket_2.h"

client_socket_2::client_socket_2()
: abs_server(nullptr), id(-1) {
	
}
client_socket_2::client_socket_2(client_socket_2 &&c_s_2)
: abs_server(nullptr), id(-1) {
	*this = std::move(c_s_2);
}
client_socket_2::client_socket_2(client_socket &&c_s, abstract_server *abs_server, long long id)
: c_s(std::move(c_s)), abs_server(abs_server), id(id) {
}
client_socket_2& client_socket_2::operator=(client_socket_2 &&c_s_2) {
	c_s = std::move(c_s_2.c_s);
	abs_server = c_s_2.abs_server;
	id = c_s_2.id;
	
	c_s_2.abs_server = nullptr;
	c_s_2.id = -1;
	return *this;
}

void client_socket_2::set_on_read_completion(func_real_rw_t on_read_completion) {
	c_s.set_on_read_completion(on_read_completion);
}
void client_socket_2::set_on_read_completion(func_rw_t on_read_completion) {
	c_s.set_on_read_completion(on_read_completion);
}

void client_socket_2::set_on_write_completion(func_real_rw_t on_write_completion) {
	c_s.set_on_write_completion(on_write_completion);
}
void client_socket_2::set_on_write_completion(func_rw_t on_write_completion) {
	c_s.set_on_write_completion(on_write_completion);
}

void client_socket_2::set_on_disconnect(func_real_disc_t on_disconnect) {
	c_s.set_on_disconnect(on_disconnect);
}
void client_socket_2::set_on_disconnect(func_disc_t on_disconnect) {
	c_s.set_on_disconnect(on_disconnect);
}

void client_socket_2::read_some(char *buff, size_t size) {
	c_s.read_some(buff, size);
}
void client_socket_2::write_some(const char *buff, size_t size) {
	c_s.write_some(buff, size);
}

void client_socket_2::execute_on_disconnect() {
	c_s.execute_on_disconnect();
}

long long client_socket_2::get_id() const {
	return id;
}
