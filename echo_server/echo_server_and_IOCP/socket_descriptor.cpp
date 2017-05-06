#include "socket_descriptor.h"

socket_descriptor::socket_descriptor()
: sd(INVALID_SOCKET) {
}
socket_descriptor::socket_descriptor(int sd)
: sd(sd) {
}
socket_descriptor::socket_descriptor(int address_family, int type, int protocol) {
	sd = socket(address_family, type, protocol);
	
    if (sd == INVALID_SOCKET) {
		throw new socket_exception("Socket function failed with error " + std::to_string(WSAGetLastError()) + "\n");
	}
}
socket_descriptor::socket_descriptor(socket_descriptor &&other) 
: sd(other.sd) {
	other.sd = -1;
}

void socket_descriptor::close() {
	if (sd == INVALID_SOCKET) {
		return;
	}
	LOG("Closing client socket : " << sd << "\n");
	int res = ::closesocket(sd);
	sd = INVALID_SOCKET;
	if (res == -1) {
		throw new socket_exception("Error when closing sd " + std::to_string(sd) + " : " + std::to_string(res) + "\n");
	}
}

socket_descriptor::~socket_descriptor() {
	try {
		close();
	} catch (socket_exception *ex) {
		LOG("Exception in destructor of socket_descriptor.");
	}
}

socket_descriptor& socket_descriptor::operator=(socket_descriptor&& socket_d) {
	close();
	
	sd = socket_d.sd;
	socket_d.sd = INVALID_SOCKET;
	return *this;
}

bool socket_descriptor::is_valid() const {
	return sd != INVALID_SOCKET;
}

void socket_descriptor::invalidate() {
	sd = INVALID_SOCKET;
}

unsigned int socket_descriptor::get_sd() const {
	return sd;
}
