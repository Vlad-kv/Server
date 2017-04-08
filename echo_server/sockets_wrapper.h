#ifndef SOCKETS_WRAPPER
#define SOCKETS_WRAPPER

#include <sstream>
#include <string>
#include <winsock2.h>
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
	int sd;
	
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
	
	int get_sd() const {
		return sd;
	}
	
	void close() {
		if (sd == INVALID_SOCKET) {
			return;
		}
		int res = ::closesocket(sd);
		if (res == -1) {
			cout << "Error when closing sd " << sd << " : " << res << "\n";
			throw new socket_descriptor_exception;
		}
		sd = INVALID_SOCKET;
	}
	
	~socket_descriptor() {
		close();
	}
};

socket_descriptor create_socket(int af, int type, int protocol) {
	int res = socket(af, type, protocol);
	
    if (res == INVALID_SOCKET) {
		throw new socket_exception("Socket function failed with error " + patch::to_string(WSAGetLastError()) + "\n");
	}
	return socket_descriptor(res);
}

socket_descriptor accept_socket(const socket_descriptor& sock) {
	int res = accept(sock.get_sd(), 0, 0);
	if (res == INVALID_SOCKET) {
		throw new socket_exception("Accept failed with error " + patch::to_string(WSAGetLastError()) + "\n");
	}
	return socket_descriptor(res);
}

void bind_socket(const socket_descriptor& sock, short family, u_long addr, u_short port) {
	sockaddr_in addres;
	
	addres.sin_family = family;
	addres.sin_addr.s_addr = addr;
	addres.sin_port = port;
	
	int res = bind(sock.get_sd(), (SOCKADDR *) &addres, sizeof(addres));
	
	if (res == SOCKET_ERROR) {
		throw new socket_exception("Bind failed with error " + patch::to_string(WSAGetLastError()) + "\n");
	}
}

void connect_to_socket(const socket_descriptor& connectSocket, short family, u_long addr, u_short port) {
	sockaddr_in addres;
	
	addres.sin_family = family;
	addres.sin_addr.s_addr = addr;
	addres.sin_port = port;
	
	int res = connect(connectSocket.get_sd(), (SOCKADDR *) &addres, sizeof(addres));
	if (res == SOCKET_ERROR) {
		throw new socket_exception("Connect function failed with error " + patch::to_string(WSAGetLastError()) + "\n");
	}
}

void send_to_socket(const socket_descriptor& sock, string mess) {
	int res = send(sock.get_sd(), &mess[0], mess.length(), 0);
	if (res == SOCKET_ERROR) {
		throw new socket_exception("Send failed with error " + patch::to_string(WSAGetLastError()) + "\n");
	}
}

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

#endif // SOCKETS_WRAPPER
