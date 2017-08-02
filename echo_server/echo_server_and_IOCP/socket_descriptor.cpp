#include "socket_descriptor.h"

using namespace std;

socket_descriptor::socket_descriptor()
: sd(INVALID_SOCKET) {
}
socket_descriptor::socket_descriptor(int sd)
: sd(sd) {
}
socket_descriptor::socket_descriptor(int address_family, int type, int protocol) {
	sd = socket(address_family, type, protocol);
    if (sd == INVALID_SOCKET) {
		throw new socket_exception("Socket function failed with error " + to_string(WSAGetLastError()) + "\n");
	}
}
socket_descriptor::socket_descriptor(socket_descriptor &&other) 
: sd(other.sd) {
	other.sd = -1;
}

void socket_descriptor::connect(short family, u_long addr, u_short port) {
	sockaddr_in addres;
	
	addres.sin_family = family;
	addres.sin_addr.s_addr = addr;
	addres.sin_port = htons(port);
	
	int res = ::connect(sd, (SOCKADDR *) &addres, sizeof(addres));
	if (res == SOCKET_ERROR) {
		throw new socket_exception("Connect function failed with error " + to_string(WSAGetLastError()) + "\n");
	}
}

void socket_descriptor::connect(short family, std::string addr, u_short port) {
	connect(family, inet_addr(&addr[0]), port);
}

void socket_descriptor::close() {
	if (sd == INVALID_SOCKET) {
		return;
	}
	LOG("Closing socket : " << sd << "\n");
	int res = ::closesocket(sd);
	sd = INVALID_SOCKET;
	if (res == -1) {
		throw new socket_exception("Error when closing sd " + to_string(sd) + " : " + to_string(res) + "\n");
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

SOCKET socket_descriptor::get_sd() const {
	return sd;
}
