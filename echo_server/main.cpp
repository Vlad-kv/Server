#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <string>

#include <winsock2.h> // для WSAData
#include <memory>
using namespace std;
#include "sockets_wrapper.h"

const char* MAIN_SOCKET_ADDRES = "127.0.0.1";


const int BUFFER_SIZE = 1024;
char buffer[BUFFER_SIZE];

int main() {
    WSA_holder wsa_holder(MAKEWORD(2, 2));
    
    socket_descriptor main_socket = create_socket(AF_INET, SOCK_STREAM, 0);
    bind_socket(main_socket, AF_INET, inet_addr(MAIN_SOCKET_ADDRES), htons(8001));
    
    if (listen(main_socket.get_sd(), SOMAXCONN) == SOCKET_ERROR) {
		cout << "listen function failed with error: " << WSAGetLastError() << "\n";
		return 1;
    }
    
    while (1) {
		socket_descriptor client = accept_socket(main_socket);
		
		cout << "Connect to socket " << client.get_sd() << "\n";
		
		int received_bytes;
		
		do {
			received_bytes = recv(client.get_sd(), buffer, BUFFER_SIZE, 0);
			
			cout << "Received bytes - " << received_bytes << " : ";
			
			for (int w = 0; w < received_bytes; w++) {
				cout << buffer[w];
			}
			cout << "\n";
		} while (received_bytes > 0);
		
		cout << "Close connection.\n";
    }
    
    return 0;
}
