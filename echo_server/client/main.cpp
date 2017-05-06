#include <iostream>
#include <thread>
#include <iomanip>
#include <ctime>
#include <chrono>

#include "../echo_server/sockets_wrapper.h"
using namespace std;

const char* MAIN_SOCKET_ADDRES = "127.0.0.1";

int main() {
	
//	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
//	cout << std::chrono::duration_cast<std::chrono::microseconds>(now).count() << "\n";
	
	{
		WSA_holder wsa_holder(MAKEWORD(2, 2));
		
		socket_descriptor client = create_socket(AF_INET, SOCK_STREAM, 0);
		
		connect_to_socket(client, AF_INET, inet_addr(MAIN_SOCKET_ADDRES), htons(8001));
		
		string test;
		
		for (int w = 0; w < 2000; w++) {
			test += "w";
		}
		
		for (int w = 0; w < 5; w++) {
			Sleep(1000);
			
//			send_to_socket(client, test);
			
			blocking_send(client, test);
			
			cout << "--\n";
		}
		
		Sleep(1000);
		int res;
		res = shutdown(client.get_sd(), SD_SEND);
		if (res == SOCKET_ERROR) {
			cout << "Error in shutdowning : " << WSAGetLastError() << "\n";
			return 0;
		}
		
		Sleep(1000);
		string result = receive_from_socket(client);
		
		cout << "result : " << result << "\n";
	}
	
	Sleep(5000);
    return 0;
}
