#include "socket.h"

completion_key::completion_key(client_socket* client) 
: num_referenses(0), type(CLIENT_SOCKET_KEY), ptr((void*)client) {
}
completion_key::completion_key(server_socket* server) 
: num_referenses(0), type(SERVER_SOCKET_KEY), ptr((void*)server) {
}

///------------------------------------------

client_socket::client_socket_overlapped::client_socket_overlapped(int type, char* buff, size_t size)
: type_of_operation(type) {
	SecureZeroMemory((PVOID) &overlapped, sizeof(OVERLAPPED));
	this->buff.buf = buff;
	this->buff.len = size;
}

client_socket::client_socket()
: sd(INVALID_SOCKET), key(nullptr) {
}
client_socket::client_socket(socket_descriptor &&sd)
: sd(std::move(sd)), key(nullptr) {
}
client_socket::client_socket(client_socket &&other) 
: sd(std::move(other.sd)), key(other.key) {
	other.key = nullptr;
}

void client_socket::set_on_read_completion(func_real_rw_t on_read_completion) {
	this->on_read_completion = std::bind(on_read_completion, std::ref(*this), std::placeholders::_1);
}
void client_socket::set_on_write_completion(func_real_rw_t on_write_completion) {
	this->on_write_completion = std::bind(on_write_completion, std::ref(*this), std::placeholders::_1);
}
void client_socket::set_on_disconnect(func_real_disc_t on_disconnect) {
	this->on_disconnect = std::bind(on_disconnect, std::ref(*this));
}

void client_socket::read_some(char *buff, size_t size) {
	DWORD received_bytes;
	DWORD flags = 0;
	
	client_socket_overlapped* overlapped = new client_socket_overlapped(client_socket_overlapped::RECV_KEY,
																		buff, size);
	key->num_referenses++;
	
	if (WSARecv(sd.get_sd(), &overlapped->buff, 1, &received_bytes/*unused*/, &flags,
			&overlapped->overlapped, NULL) == SOCKET_ERROR) {
					if ((WSAGetLastError() == 10054) || (WSAGetLastError() == 10053)) {
						LOG("In WSARecv connection broken.\n");
						
						delete overlapped;
						key->num_referenses--;
						on_disconnect();
						return;
					}
					if (WSAGetLastError() != ERROR_IO_PENDING) {
						delete overlapped;
						key->num_referenses--;
						on_disconnect();
						throw new socket_exception("Error in WSARecv : " + std::to_string(WSAGetLastError()) + "\n");
					}
	}
}

void client_socket::write_some(const char *buff, size_t size) {
	{
		LOG("Sending bytes : ");
		LOG(size << "\n");
	}
	
	DWORD received_bytes;
	client_socket_overlapped *overlapped = new client_socket_overlapped(client_socket_overlapped::SEND_KEY,
																		const_cast<char*>(buff), size);
	key->num_referenses++;
	
	if (WSASend(sd.get_sd(), &overlapped->buff, 1, &received_bytes/*unused*/, 0, 
				&overlapped->overlapped, NULL) == SOCKET_ERROR) {
					if (WSAGetLastError() == 10054) { // подключение разорвано
						LOG("In WSASend connection broken.\n");
						
						delete overlapped;
						key->num_referenses--;
						on_disconnect();
						return;
					}
					if (WSAGetLastError() != ERROR_IO_PENDING) {
						delete overlapped;
						key->num_referenses--;
						on_disconnect();
						throw new socket_exception("Error in WSASend : " + std::to_string(WSAGetLastError()) + "\n");
					}
	}
}

void client_socket::close() {
	if (sd.is_valid()) {
		LOG("Closing client socket : " << sd.get_sd() << "\n");
		sd.close();
		if (key) {
			key->ptr = nullptr;
			key->num_referenses--;
			if (key->num_referenses == 0) {
				delete key;
			}
		}
	}
	key = nullptr;
}
client_socket::~client_socket() {
	try {
		close();
	} catch (...) {
		LOG("Exception in destructor of client_socket.\n");
	}
}

client_socket& client_socket::operator=(client_socket&& socket_d) {
	close();
	
	sd = std::move(socket_d.sd);
	std::swap(key, socket_d.key);
	return *this;
}

void client_socket::invalidate() {
	sd.invalidate();
}

unsigned int client_socket::get_sd() const {
	return sd.get_sd();
}

///------------------------------------

server_socket::server_socket_overlapped::server_socket_overlapped() {
	SecureZeroMemory((PVOID) &overlapped, sizeof(OVERLAPPED));
}

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


server_socket::server_socket() 
: key(nullptr) {
}
server_socket::server_socket(socket_descriptor &&sd, func_t on_accept)
: on_accept(on_accept), sd(std::move(sd)), key(nullptr) {
	init_AcceptEx();
}
server_socket::server_socket(server_socket &&other) {
	close();
	sd = std::move(other.sd);
	GuidAcceptEx = other.GuidAcceptEx;
	lpfnAcceptEx = other.lpfnAcceptEx;
	on_accept = other.on_accept;
	std::swap(key, other.key);
}

void server_socket::accept(int address_family, int type, int protocol) {
	socket_descriptor client(address_family, type, protocol);
	
	server_socket_overlapped *overlapped = new server_socket_overlapped;
	overlapped->sd = std::move(client);
	
	key->num_referenses++;
	
	DWORD unused;
	int res = lpfnAcceptEx(sd.get_sd(), overlapped->sd.get_sd(), overlapped->buffer,
							0, sizeof (sockaddr_in) + 16, sizeof (sockaddr_in) + 16,
							&unused, &overlapped->overlapped);
	if (res == FALSE) {
		if (WSAGetLastError() != ERROR_IO_PENDING) {
			delete overlapped;
			key->num_referenses--;
			throw new socket_exception("lpfnAcceptEx failed with error : " + std::to_string(WSAGetLastError()) + "\n");
		}
	}
}

void server_socket::close() {
	if (sd.is_valid()) {
		delete GuidAcceptEx;
		sd.close();
		if (key) {
			key->ptr = nullptr;
			key->num_referenses--;
			if (key->num_referenses == 0) {
				delete key;
			}
		}
	}
	key = nullptr;
}

server_socket::~server_socket() {
	if (sd.is_valid()) {
		delete GuidAcceptEx;
	}
}
