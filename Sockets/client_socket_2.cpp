#include "client_socket_2.h"

client_socket_2::client_socket_2()
: id(-1) {
	
}
client_socket_2::client_socket_2(client_socket_2 &&c_s_2)
: id(-1) {
	*this = std::move(c_s_2);
}
client_socket_2::client_socket_2(client_socket &&c_s, long long id)
: c_s(std::move(c_s)), id(id) {
}
client_socket_2& client_socket_2::operator=(client_socket_2 &&c_s_2) {
	c_s = std::move(c_s_2.c_s);
	id = c_s_2.id;
	
	c_s_2.id = -1;
	return *this;
}

void client_socket_2::set_on_read_completion(func_read_t on_read_completion) {
	c_s.set_on_read_completion(on_read_completion);
}
void client_socket_2::set_on_write_completion(func_write_t on_write_completion) {
	c_s.set_on_write_completion(on_write_completion);
}
void client_socket_2::set_on_disconnect(func_t on_disconnect) {
	c_s.set_on_disconnect(on_disconnect);
}

void client_socket_2::read_some() {
	c_s.read_some();
}
void client_socket_2::write_some(const char *buff, size_t size) {
	c_s.write_some(buff, size);
}
void client_socket_2::write_some_saved_bytes() {
	c_s.write_some_saved_bytes();
}
size_t client_socket_2::get_num_of_saved_bytes() {
	return c_s.get_num_of_saved_bytes();
}

bool client_socket_2::is_reading_available() {
	return c_s.is_reading_available();
}
bool client_socket_2::is_writing_available() {
	return c_s.is_writing_available();
}

void client_socket_2::shutdown_reading() {
	c_s.shutdown_reading();
}
void client_socket_2::shutdown_writing() {
	c_s.shutdown_writing();
}

void client_socket_2::execute_on_disconnect() {
	c_s.execute_on_disconnect();
}

long long client_socket_2::get_id() const {
	return id;
}
