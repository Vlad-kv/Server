#include <cassert>
#include "socket.h"
using namespace std;

int completion_key::counter_of_items = 0;

completion_key::completion_key(abstract_socket *ptr, on_comp_t on_comp)
: ptr(ptr), on_comp(on_comp) {
	counter_of_items++;
	cout << "(+) " << counter_of_items << "\n";
}
completion_key::~completion_key() {
	counter_of_items--;
	cout << "(-) " << counter_of_items << "\n";
}

abstract_socket* completion_key::get_ptr() const {
	return ptr;
}

abstract_overlapped::abstract_overlapped(const abstract_socket &this_socket)
: key(this_socket.key) {
	assert(this_socket.is_registrated); // socket is not registrated
	assert((*this_socket.port_ptr) != nullptr); //IO_completion_port already closed
	assert(!(*this_socket.port_ptr)->is_terminated); //IO_completion_port already terminated
	
	SecureZeroMemory((PVOID) &overlapped, sizeof(OVERLAPPED));
}

abstract_socket::abstract_socket(on_comp_t f)
: key(make_shared<completion_key>(this, f)) {
}
abstract_socket::abstract_socket(on_comp_t f, socket_descriptor &&sd)
: key(make_shared<completion_key>(this, f)), sd(move(sd)) {
}
abstract_socket::abstract_socket(on_comp_t f, int address_family, int type, int protocol)
: key(make_shared<completion_key>(this, f)), sd(address_family, type, protocol) {
}
abstract_socket::abstract_socket(abstract_socket &&abs_socket)
: key(move(abs_socket.key)), is_registrated(abs_socket.is_registrated),
  port_ptr(move(abs_socket.port_ptr)), sd(move(abs_socket.sd)) {
	key->ptr = this;
	abs_socket.is_registrated = false;
}

abstract_socket& abstract_socket::operator=(abstract_socket &&abs_socket) {
	close_comp_key();
	key = move(abs_socket.key);
	key->ptr = this;
	
	is_registrated = abs_socket.is_registrated;
	abs_socket.is_registrated = false;
	port_ptr = move(abs_socket.port_ptr);
	
	sd = move(abs_socket.sd);
	return *this;
}

void abstract_socket::close_comp_key() {
	if (key != nullptr) {
		key->ptr = nullptr;
		key = nullptr;
	}
}
abstract_socket::~abstract_socket() {
	close_comp_key();
}
///------------------------------------------

servers_client_socket::servers_client_socket_overlapped::servers_client_socket_overlapped(const abstract_socket &this_socket, int type, char* buff, size_t size)
: abstract_overlapped(this_socket), type_of_operation(type) {
	this->buff.buf = buff;
	this->buff.len = size;
}

void servers_client_socket::on_completion(DWORD transmited_bytes,
		const key_ptr key, abstract_overlapped *overlapped, int error) {
	servers_client_socket_overlapped *real_overlaped = 
							reinterpret_cast<servers_client_socket_overlapped*>(overlapped);
	int type = real_overlaped->type_of_operation;
	delete real_overlaped;
	
	if (error == WSA_OPERATION_ABORTED) {
		LOG("servers_client_socket operation aborted\n");
		return;
	}
	
	servers_client_socket *real_ptr = (servers_client_socket*)key->get_ptr();
	
	if (real_ptr == nullptr) {
		LOG("socket already closed.");
		return;
	}
	
	if (error != 0) {
		if (error == 64) {
			LOG("connection interrupted\n");
			real_ptr->execute_on_disconnect();
			return;
		}
		throw new socket_exception("Error in GetQueuedCompletionStatus : " +
									to_string(GetLastError()) + "\n");
	}
	
	if (type == servers_client_socket::servers_client_socket_overlapped::RECV_KEY) {
		real_ptr->on_read_completion(transmited_bytes);
		return;
	}
	if (type == servers_client_socket::servers_client_socket_overlapped::SEND_KEY) {
		real_ptr->on_write_completion(transmited_bytes);
		return;
	}
	throw new socket_exception("Uncnown operation code : " + to_string(type) + "\n");
}

servers_client_socket::servers_client_socket()
: abstract_socket(on_completion) {
}
servers_client_socket::servers_client_socket(socket_descriptor &&sd)
: abstract_socket(on_completion, move(sd)) {
}
servers_client_socket::servers_client_socket(servers_client_socket &&other)
: abstract_socket(on_completion) {
	*this = move(other);
}
servers_client_socket::servers_client_socket(int address_family, int type, int protocol)
: abstract_socket(on_completion, address_family, type, protocol) {
}

void servers_client_socket::set_on_read_completion(func_rw_t on_read_completion) {
	this->on_read_completion = on_read_completion;
}
void servers_client_socket::set_on_write_completion(func_rw_t on_write_completion) {
	this->on_write_completion = on_write_completion;
}
void servers_client_socket::set_on_disconnect(func_disc_t on_disconnect) {
	this->on_disconnect = on_disconnect;
}

void servers_client_socket::read_some(char *buff, size_t size) {
	DWORD received_bytes;
	DWORD flags = 0;
	servers_client_socket_overlapped* overlapped = new servers_client_socket_overlapped(*this, servers_client_socket_overlapped::RECV_KEY,
																		buff, size);
	
	if (WSARecv(sd.get_sd(), &overlapped->buff, 1, &received_bytes/*unused*/, &flags,
			&overlapped->overlapped, NULL) == SOCKET_ERROR) {
					int error = WSAGetLastError();
					if ((error == 10054) || (error == 10053)) {
						LOG("In WSARecv connection broken.\n");
						
						delete overlapped;
						on_disconnect();
						return;
					}
					if (error != ERROR_IO_PENDING) {
						delete overlapped;
						on_disconnect();
						throw new socket_exception("Error in WSARecv : " + to_string(error) + "\n");
					}
	}
}

void servers_client_socket::write_some(const char *buff, size_t size) {
	{
		LOG("Sending bytes : ");
		LOG(size << "\n");
	}
	
	DWORD received_bytes;
	servers_client_socket_overlapped *overlapped = new servers_client_socket_overlapped(*this, servers_client_socket_overlapped::SEND_KEY,
																		const_cast<char*>(buff), size);
	
	if (WSASend(sd.get_sd(), &overlapped->buff, 1, &received_bytes/*unused*/, 0, 
				&overlapped->overlapped, NULL) == SOCKET_ERROR) {
					int error = WSAGetLastError();
					if (error == 10054) { // подключение разорвано
						LOG("In WSASend connection broken.\n");
						
						delete overlapped;
						on_disconnect();
						return;
					}
					if (error != ERROR_IO_PENDING) {
						delete overlapped;
						on_disconnect();
						throw new socket_exception("Error in WSASend : " + to_string(error) + "\n");
					}
	}
}

void servers_client_socket::execute_on_disconnect() {
	if (on_disconnect != nullptr) {
		func_disc_t temp = on_disconnect;
		on_disconnect = nullptr;
		temp();
	}
}

void servers_client_socket::close() {
	if (sd.is_valid()) {
		sd.close();
		close_comp_key();
		execute_on_disconnect();
	}
}
servers_client_socket::~servers_client_socket() {
	try {
		close();
	} catch (...) {
		LOG("Exception in destructor of client_socket.\n");
	}
}

servers_client_socket& servers_client_socket::operator=(servers_client_socket&& c_s) {
	close();
	
	on_read_completion = c_s.on_read_completion;
	on_write_completion = c_s.on_write_completion;
	on_disconnect = c_s.on_disconnect;
	
	c_s.on_read_completion = nullptr;
	c_s.on_write_completion = nullptr;
	c_s.on_disconnect = nullptr;
	
	*(abstract_socket*)this = move(c_s);
	return *this;
}

unsigned int servers_client_socket::get_sd() const {
	return sd.get_sd();
}

///------------------------------------

server_socket::server_socket_overlapped::server_socket_overlapped(const abstract_socket &this_socket)
: abstract_overlapped(this_socket) {
}

void server_socket::on_completion(DWORD transmited_bytes,
		const key_ptr key, abstract_overlapped *overlapped, int error) {
	server_socket_overlapped *real_overlaped = 
								reinterpret_cast<server_socket_overlapped*>(overlapped);
	
	socket_descriptor client = move(real_overlaped->sd);
	delete real_overlaped;
	
	if (error == WSA_OPERATION_ABORTED) {
		LOG("server_socket operation aborted\n");
		return;
	}
	if (key->get_ptr() == nullptr) {
		LOG("socket already closed.");
		return;
	}
	if (error != 0) {
		throw new socket_exception("Error in GetQueuedCompletionStatus : " +
									to_string(GetLastError()) + "\n");
	}
	((server_socket*)key->get_ptr())->on_accept(move(client));
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
		throw new socket_exception("WSAIoctl failed with error : " + to_string(error) + "\n");
	}
}

server_socket::server_socket()
: abstract_socket(on_completion) {
}
server_socket::server_socket(socket_descriptor &&sd, func_t on_accept)
: abstract_socket(on_completion, move(sd)), on_accept(on_accept) {
	init_AcceptEx();
}
server_socket::server_socket(server_socket &&other)
: abstract_socket(static_cast<abstract_socket&&>(other)) {
	swap(GuidAcceptEx, other.GuidAcceptEx);
	swap(lpfnAcceptEx, other.lpfnAcceptEx);
	swap(on_accept, other.on_accept);
}
server_socket::server_socket(int address_family, int type, int protocol, func_t on_accept)
: abstract_socket(on_completion, address_family, type, protocol),
  on_accept(on_accept) {
	init_AcceptEx();
}
server_socket& server_socket::operator=(server_socket &&sock) {
	close();
	*(abstract_socket*)this = move(sock);
	swap(GuidAcceptEx, sock.GuidAcceptEx);
	swap(lpfnAcceptEx, sock.lpfnAcceptEx);
	swap(on_accept, sock.on_accept);
	return *this;
}

void server_socket::bind_and_listen(int address_family, std::string addres_of_main_socket, int port, int backlog) {
	bind_socket(*this, address_family, inet_addr(&addres_of_main_socket[0]), htons(port));
	
	if (listen(sd.get_sd(), backlog) == SOCKET_ERROR) {
		throw new socket_exception("listen failed with error " + to_string(WSAGetLastError()) + "\n");
	}
}

void server_socket::accept(int address_family, int type, int protocol) {
	socket_descriptor client(address_family, type, protocol);
	
	server_socket_overlapped *overlapped = new server_socket_overlapped(*this);
	overlapped->sd = move(client);
	
	DWORD unused;
	int res = lpfnAcceptEx(sd.get_sd(), overlapped->sd.get_sd(), overlapped->buffer,
							0, sizeof (sockaddr_in) + 16, sizeof (sockaddr_in) + 16,
							&unused, &overlapped->overlapped);
	if (res == FALSE) {
		int error = WSAGetLastError();
		if (error != ERROR_IO_PENDING) {
			delete overlapped;
			throw new socket_exception("lpfnAcceptEx failed with error : " + to_string(error) + "\n");
		}
	}
}

SOCKET server_socket::get_sd() const {
	return sd.get_sd();
}
sockaddr_in server_socket::get_sock_address() {
	sockaddr_in result;
	int size = sizeof(result);
	int res = getsockname(sd.get_sd(), (sockaddr*)&result, &size);
	if (res != 0) {
		int error = WSAGetLastError();
		throw new socket_exception("getsockname failed with error : " + to_string(error) + "\n");
	}
	return result;
}

void server_socket::close() {
	if (sd.is_valid()) {
		delete GuidAcceptEx;
		GuidAcceptEx = nullptr;
		lpfnAcceptEx = NULL;
		sd.close();
		close_comp_key();
	}
}

server_socket::~server_socket() {
	close();
}
