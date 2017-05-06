#include "socket.h"

client_socket::client_socket()
: sd(INVALID_SOCKET) {
}
client_socket::client_socket(socket_descriptor &&sd)
: sd(std::move(sd)) {
}
client_socket::client_socket(client_socket &&other) 
: sd(std::move(other.sd)) {
}

void client_socket::set_on_read_completion(func_t on_read_completion) {
	this->on_read_completion = on_read_completion;
}
void client_socket::set_on_write_completion(func_t on_write_completion) {
	this->on_write_completion = on_write_completion;
}

void client_socket::close() {
	LOG("Closing client socket : " << sd.get_sd() << "\n");
	sd.close();
}

client_socket& client_socket::operator=(client_socket&& socket_d) {
	close();
	
	sd = std::move(socket_d.sd);
	return *this;
}

void client_socket::invalidate() {
	sd.invalidate();
}

unsigned int client_socket::get_sd() const {
	return sd.get_sd();
}

///------------------------------------


void server_socket::init_AcceptEx() {
	GuidAcceptEx = new GUID;
	*GuidAcceptEx = WSAID_ACCEPTEX;
	
	DWORD unused;
	int res = WSAIoctl(sd.get_sd(), SIO_GET_EXTENSION_FUNCTION_POINTER,
						 GuidAcceptEx, sizeof(GUID),
						&lpfnAcceptEx, sizeof(lpfnAcceptEx),
						&unused, NULL, NULL);
	if (res == SOCKET_ERROR) {
		delete GuidAcceptEx;
		int error = WSAGetLastError();
		try {
			sd.close();
		} catch (...) {
			LOG("Double fail in init_AcceptEx.");
		}
		throw new socket_exception("WSAIoctl failed with error : " + std::to_string(error) + "\n");
	}
}


server_socket::server_socket() {
}
server_socket::server_socket(socket_descriptor &&sd)
: sd(std::move(sd)) {
	init_AcceptEx();
}
server_socket::server_socket(server_socket &&other) {
	close();
	sd = std::move(other.sd);
	GuidAcceptEx = other.GuidAcceptEx;
	lpfnAcceptEx = other.lpfnAcceptEx;
}


void server_socket::close() {
	if (sd.is_valid()) {
		delete GuidAcceptEx;
		sd.close();
	}
}

server_socket::~server_socket() {
	if (sd.is_valid()) {
		delete GuidAcceptEx;
	}
}
