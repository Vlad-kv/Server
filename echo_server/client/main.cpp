#include <iostream>
#include <thread>
#include <chrono>

#include "../echo_server/sockets_wrapper.h"
using namespace std;

const char* MAIN_SOCKET_ADDRES = "127.0.0.1";

int main() {
	{
		WSA_holder wsa_holder(MAKEWORD(2, 2));
		
		socket_descriptor client = create_socket(AF_INET, SOCK_STREAM, 0);
		
		connect_to_socket(client, AF_INET, inet_addr(MAIN_SOCKET_ADDRES), htons(8001));
		
		string test;
		
		for (int w = 0; w < 10000; w++) {
			test += "w";
		}
		
//		send_to_socket(client, test);
		
		int res = send(client.get_sd(), &test[0], test.length(), 0);
		
		if (res == SOCKET_ERROR) {
			cout << "Error in sending data " << WSAGetLastError() << "\n";
			return 0;
		}
		
		res = shutdown(client.get_sd(), SD_SEND);
		if (res == SOCKET_ERROR) {
			cout << "Error in shutdowning : " << WSAGetLastError() << "\n";
			return 0;
		}
		
		cout << "Sended bytes : " << res << "\n";
		
		string result = receive_from_socket(client);
		
		cout << "result : " << result << "\n";
	}
	
	Sleep(2000);
    return 0;
}
