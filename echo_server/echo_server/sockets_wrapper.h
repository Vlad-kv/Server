#ifndef SOCKETS_WRAPPER
#define SOCKETS_WRAPPER

#include <iostream>
#include <sstream>
#include <string>
#include <winsock2.h>
#include <windows.h>
#include <WinBase.h>

#include <mswsock.h>

#include <memory>

#include <cstddef>

#include "../echo_server_and_IOCP/logger.h"

#include "../echo_server_and_IOCP/socket.h"
#include "../echo_server_and_IOCP/IO_completion_port.h"

using namespace std;

template <typename T>
string to_str(const T& n) {
	ostringstream stm;
	stm << n;
	return stm.str();
}

client_socket create_socket(int af, int type, int protocol);

client_socket accept_socket(const client_socket& sock);

void bind_socket(const client_socket& sock, short family, u_long addr, u_short port);

void connect_to_socket(const client_socket& connectSocket, short family, u_long addr, u_short port);

void send_to_socket(const client_socket& sock, string mess);

void blocking_send(const client_socket& sock, string mess);

string receive_from_socket(const client_socket& sock);

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


#endif // SOCKETS_WRAPPER
