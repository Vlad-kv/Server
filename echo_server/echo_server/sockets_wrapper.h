#ifndef SOCKETS_WRAPPER
#define SOCKETS_WRAPPER

#include <iostream>
#include <sstream>
#include <string>
#include <winsock2.h>
#include <windows.h>
#include <WinBase.h>
#include <memory>

#include <cstddef>

#include "../echo_server_and_IOCP/logger.h"

using namespace std;

template <typename T>
string to_str(const T& n) {
	ostringstream stm;
	stm << n;
	return stm.str();
}

class socket_exception {
	public:
	string mess;
	
	socket_exception(string mess)
	: mess(mess) {
		LOG(mess);
	}
};

class socket_descriptor {
	unsigned int sd;
	
	socket_descriptor(socket_descriptor &to_move) {
	}
public:
	class socket_descriptor_exception {
	};
	
	socket_descriptor()
	: sd(INVALID_SOCKET) {
	}
	
	socket_descriptor(int sd)
	: sd(sd) {
	}
	
	socket_descriptor(socket_descriptor &&to_move) 
	: sd(to_move.sd) {
		to_move.sd = -1;
	}
	
	unsigned int get_sd() const {
		return sd;
	}
	
	void close() {
		if (sd == INVALID_SOCKET) {
			return;
		}
		LOG("Closing socket : " << sd << "\n");
		int res = ::closesocket(sd);
		if (res == -1) {
			LOG("Error when closing sd " << sd << " : " << res << "\n");
			throw new socket_descriptor_exception;
		}
		sd = INVALID_SOCKET;
	}
	
	~socket_descriptor() {
		close();
	}
	
	socket_descriptor& operator=(socket_descriptor&& socket_d) {
		close();
		
		sd = socket_d.sd;
		socket_d.sd = INVALID_SOCKET;
		return *this;
	}
	
	friend bool operator==(const socket_descriptor& sd_1, const socket_descriptor& sd_2) {
		return (sd_1.sd == sd_2.sd);
	}
	friend bool operator!=(const socket_descriptor& sd_1, const socket_descriptor& sd_2) {
		return (sd_1.sd != sd_2.sd);
	}
};

socket_descriptor create_socket(int af, int type, int protocol);

socket_descriptor accept_socket(const socket_descriptor& sock);

void bind_socket(const socket_descriptor& sock, short family, u_long addr, u_short port);

void connect_to_socket(const socket_descriptor& connectSocket, short family, u_long addr, u_short port);

void send_to_socket(const socket_descriptor& sock, string mess);

void blocking_send(const socket_descriptor& sock, string mess);

string receive_from_socket(const socket_descriptor& sock);

class WSA_holder {
	public:
	WSAData wsaData;
	
	WSA_holder(WORD version) {
		int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    	if (res != 0) {
			throw new socket_exception("WSAStartup failed " + to_str(res) + "\n");
    	}
	}
	
	~WSA_holder() {
		WSACleanup();
	}
};

class IO_completion_port {
	HANDLE iocp_handle;
	
public:
	IO_completion_port(HANDLE handle)
	: iocp_handle(handle) {
	}
	
	IO_completion_port(IO_completion_port &&port)
	: iocp_handle(port.iocp_handle) {
		port.iocp_handle = NULL;
	}
	
	HANDLE get_handle() {
		return iocp_handle;
	}
	
	~IO_completion_port() {
		if (iocp_handle != NULL) {
			LOG("~~~~~~~~~~~~~\n");
			BOOL res = CloseHandle(iocp_handle);
			if (res == 0) {
				LOG("Error in closing completion port : " + to_str(GetLastError()));
			}
		}
	}
};

class my_completion_key {
public:
	socket_descriptor sd;
	// TODO
	
	
	my_completion_key(socket_descriptor &&sd) 
	: sd(std::move(sd)) {
	}
	
};

class additional_info {
public:
	
	static int NO_OPERATION_KEY, RECV_KEY, SEND_KEY;
	
//	OVERLAPPED overlapped;
	
	WSABUF data_buff;
	
	char *buffer;
	int buff_size;
	
	int last_operation_type;
	
	DWORD received_bytes; // long unsigned int
	DWORD sended_bytes;
	
	
	additional_info(int buff_size) 
	: buffer(new char[buff_size]), buff_size(buff_size) {
		clear();
	}
	
	int get_operation_type() {
		return last_operation_type;
	}
	
	void clear() {
//		ZeroMemory(&overlapped, sizeof(OVERLAPPED));
		received_bytes = sended_bytes = 0;
		
		data_buff.len = buff_size;
		data_buff.buf = buffer;
		
		last_operation_type = NO_OPERATION_KEY;
	}
	
	~additional_info() {
		delete [] buffer;
	}
};

struct my_OVERLAPED {
	OVERLAPPED overlapped;
	additional_info* info;
	
	my_OVERLAPED(additional_info* info)
	: info(info) {
		SecureZeroMemory((PVOID) &overlapped, sizeof(OVERLAPPED));
	}
};

#endif // SOCKETS_WRAPPER
