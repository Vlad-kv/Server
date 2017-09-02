#include <cassert>
#include <cstring>
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
	if (!this_socket.is_registrated) {
		throw socket_exception("socket is not registrated\n");
	}
	if ((*this_socket.port_ptr) == nullptr) {
		throw socket_exception("IO_completion_port already closed\n");
	}
	if ((*this_socket.port_ptr)->is_terminated) {
		throw socket_exception("IO_completion_port already terminated\n");
	}
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

recv_send_overlapped::recv_send_overlapped(const abstract_socket &this_socket, int type, char* buff, size_t size, void *buff_ptr)
: abstract_overlapped(this_socket), type_of_operation(type), buff_ptr(buff_ptr) {
	this->buff.buf = buff;
	this->buff.len = size;
}
void* recv_send_overlapped::release_buff_ptr() {
	void *temp = buff_ptr;
	buff_ptr = nullptr;
	return temp;
}
recv_send_overlapped::~recv_send_overlapped() {
	if (type_of_operation == RECV_KEY) {
		delete (buffer_to_read*)buff_ptr;
	} else {
		delete (buffer_to_write*)buff_ptr;
	}
}

///------------------------------------------

buffer_to_read::buffer_to_read(size_t size)
: size(size) {
	assert(size > 0);
	buffer = new char[size];
}
void buffer_to_read::resize(size_t new_size) {
	assert(new_size > 0);
	char *new_buffer = new char[new_size];
	delete []buffer;
	buffer = new_buffer;
	size = new_size;
}
buffer_to_read::~buffer_to_read() {
	delete []buffer;
}

///------------------------------------------

buffer_to_write::buffer_to_write()
: start_pos(0) {
}
size_t buffer_to_write::get_saved_size() {
	return buffer.size() - start_pos;
}

void buffer_to_write::reset() {
	start_pos = 0;
	buffer.clear();
}
void buffer_to_write::write_to_buffer(const char *buff, size_t size) {
	reset();
	for (size_t w = 0; w < size; w++) {
		buffer.push_back(buff[w]);
	}
}

///------------------------------------------

void client_socket::init_ConnectEx() {
	GuidConnectEx = new GUID;
	*GuidConnectEx = WSAID_CONNECTEX;
	
	DWORD unused;
	int res = WSAIoctl(sd.get_sd(), SIO_GET_EXTENSION_FUNCTION_POINTER,
						 GuidConnectEx, sizeof(GUID),
						&lpfnConnectEx, sizeof(lpfnConnectEx),
						&unused, NULL, NULL);
	if (res == SOCKET_ERROR) {
		delete GuidConnectEx;
		int error = WSAGetLastError();
		try {
			sd.close();
		} catch (...) {
			LOG("Double fail in init_ConnectEx.");
		}
		throw socket_exception("WSAIoctl failed with error : " + to_string(error) + "\n");
	}
}

void client_socket::on_completion(DWORD transmited_bytes,
		const key_ptr key, abstract_overlapped *overlapped, int error) {
	recv_send_overlapped *real_overlaped = 
							reinterpret_cast<recv_send_overlapped*>(overlapped);
	int type = real_overlaped->type_of_operation;
	void *buff_ptr = real_overlaped->release_buff_ptr();
	delete real_overlaped;
	
	client_socket *real_ptr = (client_socket*)key->get_ptr();
	if (real_ptr == nullptr) {
		LOG("socket already closed.");
		return;
	}
	if (type == recv_send_overlapped::RECV_KEY) {
		real_ptr->b_to_read = (buffer_to_read*)buff_ptr;
	}
	if (type == recv_send_overlapped::SEND_KEY) {
		real_ptr->b_to_write = (buffer_to_write*)buff_ptr;
	}
	
	if (error == WSA_OPERATION_ABORTED) {
		LOG("client_socket operation aborted\n");
		return;
	}
	if (error != 0) {
		if (error == 64) {
			LOG("connection interrupted\n");
			real_ptr->execute_on_disconnect();
			return;
		}
		throw socket_exception("Error in GetQueuedCompletionStatus (in client_socket) : " +
									to_string(error) + "\n");
	}
	if (type == recv_send_overlapped::RECV_KEY) {
		real_ptr->on_read_completion(real_ptr->b_to_read->buffer, transmited_bytes);
		return;
	}
	if (type == recv_send_overlapped::SEND_KEY) {
		real_ptr->b_to_write->start_pos += transmited_bytes;
		real_ptr->on_write_completion(real_ptr->b_to_write->get_saved_size(), transmited_bytes);
		return;
	}
	if (type == recv_send_overlapped::CONNECT_KEY) {
		if (0 != setsockopt(real_ptr->get_sd(), SOL_SOCKET,
							SO_UPDATE_CONNECT_CONTEXT, nullptr, 0)) {
			error = WSAGetLastError();
			throw socket_exception("setsockopt failed with error : " + to_string(error) + "\n");
		}
		real_ptr->is_it_connected = true;
		real_ptr->on_connect();
		return;
	}
	LOG(type << " !!!\n");
	throw socket_exception("Unknown operation code : " + to_string(type) + "\n");
}

client_socket::client_socket()
: abstract_socket(on_completion) {
}
client_socket::client_socket(socket_descriptor &&sd, size_t read_buffer_size)
: abstract_socket(on_completion, move(sd)) {
	b_to_read = new buffer_to_read(read_buffer_size);
	try {
		b_to_write = new buffer_to_write();
	} catch (...) {
		delete b_to_read;
		b_to_read = nullptr;
		throw;
	}
	init_ConnectEx();
}
client_socket::client_socket(client_socket &&c_s)
: abstract_socket(static_cast<abstract_socket&&>(c_s)) {
	swap(on_read_completion, c_s.on_read_completion);
	swap(on_write_completion, c_s.on_write_completion);
	swap(on_disconnect, c_s.on_disconnect);
	
	swap(b_to_read, c_s.b_to_read);
	swap(b_to_write, c_s.b_to_write);
	
	swap(is_it_connected, c_s.is_it_connected);
	swap(is_bound, c_s.is_bound);
	
	swap(GuidConnectEx, c_s.GuidConnectEx);
	swap(lpfnConnectEx, c_s.lpfnConnectEx);
}

client_socket::client_socket(int address_family, int type, int protocol, size_t read_buffer_size)
: abstract_socket(on_completion, address_family, type, protocol) {
	b_to_read = new buffer_to_read(read_buffer_size);
	try {
		b_to_write = new buffer_to_write();
	} catch (...) {
		delete b_to_read;
		b_to_read = nullptr;
		throw;
	}
	init_ConnectEx();
}

void client_socket::set_on_read_completion(func_read_t on_read_completion) {
	this->on_read_completion = on_read_completion;
}
void client_socket::set_on_write_completion(func_write_t on_write_completion) {
	this->on_write_completion = on_write_completion;
}
void client_socket::set_on_connect(func_t on_connect) {
	this->on_connect = on_connect;
}
void client_socket::set_on_disconnect(func_t on_disconnect) {
	this->on_disconnect = on_disconnect;
}

void client_socket::read_some() {
	if (b_to_read == nullptr) {
		
		throw socket_exception("previous operation uncompleted\n");
	}
	DWORD received_bytes;
	DWORD flags = 0;
	
	recv_send_overlapped* overlapped = new recv_send_overlapped(*this, recv_send_overlapped::RECV_KEY,
																b_to_read->buffer, b_to_read->size, b_to_read);
	b_to_read = nullptr;
	
	if (WSARecv(sd.get_sd(), &overlapped->buff, 1, &received_bytes/*unused*/, &flags,
			&overlapped->overlapped, NULL) == SOCKET_ERROR) {
					int error = WSAGetLastError();
					if ((error == 10054) || (error == 10053)) {
						LOG("In WSARecv connection broken.\n");
						
						b_to_read = (buffer_to_read*)overlapped->release_buff_ptr();
						delete overlapped;
						execute_on_disconnect();
						return;
					}
					if (error != ERROR_IO_PENDING) {
						b_to_read = (buffer_to_read*)overlapped->release_buff_ptr();
						delete overlapped;
						execute_on_disconnect();
						throw socket_exception("Error in WSARecv : " + to_string(error) + "\n");
					}
	}
}

void client_socket::write_some(const char *buff, size_t size) {
	assert(size > 0);
	{
		LOG("Sending bytes : ");
		LOG(size << "\n");
	}
	b_to_write->write_to_buffer(buff, size);
	write_some_saved_bytes();
}

void client_socket::write_some_saved_bytes() {
	assert(b_to_write->buffer.size() > 0);
	
	DWORD received_bytes;
	recv_send_overlapped *overlapped = new recv_send_overlapped(*this, recv_send_overlapped::SEND_KEY,
																&(b_to_write->buffer[b_to_write->start_pos]),
																b_to_write->buffer.size() - b_to_write->start_pos,
																b_to_write);
	b_to_write = nullptr;
	
	if (WSASend(sd.get_sd(), &overlapped->buff, 1, &received_bytes/*unused*/, 0, 
				&overlapped->overlapped, NULL) == SOCKET_ERROR) {
					int error = WSAGetLastError();
					if ((error == WSAECONNABORTED) ||
						(error == WSAECONNRESET)) { // подключение разорвано
						
						LOG("In WSASend connection broken.\n");
						
						b_to_write = (buffer_to_write*)overlapped->release_buff_ptr();
						delete overlapped;
						execute_on_disconnect();
						return;
					}
					if (error != ERROR_IO_PENDING) {
						b_to_write = (buffer_to_write*)overlapped->release_buff_ptr();
						delete overlapped;
						execute_on_disconnect();
						throw socket_exception("Error in WSASend : " + to_string(error) + "\n");
					}
	}
}
size_t client_socket::get_num_of_saved_bytes() {
	if (b_to_write == nullptr) {
		throw socket_exception("write operation is not completed\n");
	}
	return b_to_write->get_saved_size();
}



void client_socket::connect(short family, const std::string& addr, u_short port) {
	connect(family, inet_addr(&addr[0]), port);
}
void client_socket::connect(short family, unsigned long addr, u_short port) {
	if (is_it_connected) {
		throw socket_exception("client_socket already connected\n");
	}
	sockaddr_in addres;
	
	if (!is_bound) {
		addres.sin_family = family;
		addres.sin_addr.s_addr = ADDR_ANY;
		addres.sin_port = 0;
		
		if (SOCKET_ERROR == ::bind(sd.get_sd(), (SOCKADDR*) &addres, sizeof(addres))) {
			int error = WSAGetLastError();
			throw socket_exception("bind failed with error : " + to_string(error) + "\n");
		}
		is_bound = true;
	}
	
	addres.sin_family = family;
	addres.sin_addr.s_addr = addr;
	addres.sin_port = htons(port);
	
	recv_send_overlapped *overlapped = new recv_send_overlapped(*this, recv_send_overlapped::CONNECT_KEY,
																nullptr, 0, nullptr);
	
	bool res = lpfnConnectEx(sd.get_sd(), (SOCKADDR*) &addres, sizeof(addres),
							nullptr, 0, nullptr, &overlapped->overlapped);
	if (res == FALSE) {
		int error = WSAGetLastError();
		if (error != ERROR_IO_PENDING) {
			delete overlapped;
			throw socket_exception("ConnectEx failed with error : " + to_string(error) + "\n");
		}
	} else {
		is_it_connected = true;
	}
}
bool client_socket::is_connected() const {
	return is_it_connected;
}

bool client_socket::is_reading_available() {
	return (b_to_read != nullptr);
}
bool client_socket::is_writing_available() {
	return (b_to_write != nullptr);
}

void client_socket::shutdown_reading() {
	if (!is_it_connected) {
		throw socket_exception("client_socket is not connected\n");
	}
	if (shutdown(sd.get_sd(), SD_RECEIVE)) {
		int error = WSAGetLastError();
		throw socket_exception("shutdown failed with error : " + to_string(error) + "\n");
	}
}
void client_socket::shutdown_writing() {
	if (!is_it_connected) {
		throw socket_exception("client_socket is not connected\n");
	}
	if (shutdown(sd.get_sd(), SD_SEND)) {
		int error = WSAGetLastError();
		throw socket_exception("shutdown failed with error : " + to_string(error) + "\n");
	}
}

void client_socket::execute_on_disconnect() {
	if (on_disconnect != nullptr) {
		func_t temp = move(on_disconnect);
		on_disconnect = nullptr;
		temp();
	}
}

void client_socket::close() {
	if (sd.is_valid()) {
		delete b_to_read;
		delete b_to_write;
		b_to_read = nullptr;
		b_to_write = nullptr;
		
		is_it_connected = false;
		is_bound = false;
		
		delete GuidConnectEx;
		GuidConnectEx = nullptr;
		lpfnConnectEx = NULL;
		
		sd.close();
		close_comp_key();
		execute_on_disconnect();
	}
}
client_socket::~client_socket() {
	try {
		close();
	} catch (...) {
		LOG("Exception in destructor of client_socket.\n");
	}
}

client_socket& client_socket::operator=(client_socket&& c_s) {
	close();
	
	on_read_completion = move(c_s.on_read_completion);
	on_write_completion = move(c_s.on_write_completion);
	on_disconnect = move(c_s.on_disconnect);
	
	b_to_read = c_s.b_to_read;
	b_to_write = c_s.b_to_write;
	
	swap(is_it_connected, c_s.is_it_connected);
	swap(is_bound, c_s.is_bound);
	
	swap(GuidConnectEx, c_s.GuidConnectEx);
	swap(lpfnConnectEx, c_s.lpfnConnectEx);
	
	*(abstract_socket*)this = move(c_s);
	return *this;
}

unsigned int client_socket::get_sd() const {
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
	
	LOG("in server_socket::on_completion\n");
	
	client_socket client(move(real_overlaped->sd));
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
		throw socket_exception("Error in GetQueuedCompletionStatus (in server_socket) : " +
									to_string(error) + "\n");
	}
	client.is_it_connected = true;
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
		throw socket_exception("WSAIoctl failed with error : " + to_string(error) + "\n");
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

void server_socket::bind_and_listen(short address_family, std::string socket_address, int port, int backlog) {
	sockaddr_in addres;
	
	addres.sin_family = address_family;
	addres.sin_addr.s_addr = inet_addr(&socket_address[0]);
	addres.sin_port = htons(port);
	
	if (SOCKET_ERROR == ::bind(sd.get_sd(), (SOCKADDR*)&addres, sizeof(addres))) {
		throw socket_exception("bind failed with error " + to_string(WSAGetLastError()) + "\n");
	}
	if (listen(sd.get_sd(), backlog) == SOCKET_ERROR) {
		throw socket_exception("listen failed with error " + to_string(WSAGetLastError()) + "\n");
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
			throw socket_exception("lpfnAcceptEx failed with error : " + to_string(error) + "\n");
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
		throw socket_exception("getsockname failed with error : " + to_string(error) + "\n");
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
