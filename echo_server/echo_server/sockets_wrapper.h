#ifndef SOCKETS_WRAPPER
#define SOCKETS_WRAPPER

#include <iostream>
#include <sstream>
#include <string>
#include <winsock2.h>
#include <windows.h>
#include <WinBase.h>
#include <memory>

using namespace std;

namespace patch {
    template <typename T>
    string to_string(const T& n) {
        ostringstream stm;
        stm << n;
        return stm.str();
    }
}

class socket_exception {
	public:
	string mess;
	
	socket_exception(string mess)
	: mess(mess) {
	}
};

class socket_descriptor {
	unsigned int sd;
	
	socket_descriptor(socket_descriptor &to_move) {
	}
public:
	class socket_descriptor_exception {
	};
	
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
		cout << "Closing socket : " << sd << "\n";
		int res = ::closesocket(sd);
		if (res == -1) {
			std::cout << "Error when closing sd " << sd << " : " << res << "\n";
			throw new socket_descriptor_exception;
		}
		sd = INVALID_SOCKET;
	}
	
	~socket_descriptor() {
		close();
	}
};

socket_descriptor create_socket(int af, int type, int protocol);

socket_descriptor accept_socket(const socket_descriptor& sock);

void bind_socket(const socket_descriptor& sock, short family, u_long addr, u_short port);

void connect_to_socket(const socket_descriptor& connectSocket, short family, u_long addr, u_short port);

void send_to_socket(const socket_descriptor& sock, string mess);

string receive_from_socket(const socket_descriptor& sock);

class WSA_holder {
	public:
	WSAData wsaData;
	
	WSA_holder(WORD version) {
		int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    	if (res != 0) {
			throw new socket_exception("WSAStartup failed " + patch::to_string(res) + "\n");
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
			cout << "~~~~~~~~~~~~~\n";
			BOOL res = CloseHandle(iocp_handle);
			if (res == 0) {
				throw new socket_exception("Error in closing completion port : " + patch::to_string(GetLastError()));
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
	OVERLAPPED overlapped;
	
	WSABUF data_buff;
	
	char *buffer;
	
	DWORD received_bytes; // long unsigned int
	DWORD sended_bytes;
	int buff_size;
	
	static additional_info* create(int buff_size) {
		
		additional_info* result = (additional_info*)GlobalAlloc(GPTR, sizeof(additional_info));
		if (result == NULL) {
			throw new socket_exception("GlobalAlloc failed with error : " + patch::to_string(GetLastError()) + "\n");
		}
		
		result->buffer = (char*)GlobalAlloc(GPTR, buff_size);
		
		if (result->buffer == NULL) {
			GlobalFree(result);
			throw new socket_exception("GlobalAlloc failed with error : " + patch::to_string(GetLastError()) + "\n");
		}
		
		result->buff_size = buff_size;
		
		result->clear();
		return result;
	}
	
	void clear() {
		ZeroMemory(&overlapped, sizeof(OVERLAPPED));
		received_bytes = sended_bytes = 0;
		
		data_buff.len = buff_size;
		data_buff.buf = buffer;
	}
	
	static void destroy(additional_info* obj) {
		GlobalFree(obj->buffer);
		GlobalFree(obj);
	}
	
};

#endif // SOCKETS_WRAPPER
