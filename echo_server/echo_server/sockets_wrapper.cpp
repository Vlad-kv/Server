#include "sockets_wrapper.h"

const int BUFFER_SIZE = 1024;
char buffer[BUFFER_SIZE];

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

string receive_from_socket(const socket_descriptor& sock) {
	string result;
	
	int received_bytes;
	
	do {
		received_bytes = recv(sock.get_sd(), buffer, BUFFER_SIZE, 0);
		
		cout << "Received bytes - " << received_bytes << " : ";
		
		for (int w = 0; w < received_bytes; w++) {
			result.push_back(buffer[w]);
			cout << buffer[w];
		}
		cout << "\n";
	} while (received_bytes > 0);
	
	return result;
}


