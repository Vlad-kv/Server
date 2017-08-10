#include <iostream>
#include <thread>
#include <iomanip>
#include <ctime>
#include <chrono>

#include "../Sockets/sockets.h"
using namespace std;

const char* MAIN_SOCKET_ADDRES = "127.0.0.1";

int main() {
	{
		WSA_holder wsa_holder(MAKEWORD(2, 2));
		
		IO_completion_port port;
		client_socket client(AF_INET, SOCK_STREAM, 0);
		client.connect(AF_INET, MAIN_SOCKET_ADDRES, 8001);
		
		int sended_bytes = 0;
		
		string recv_str, test_str;
		
		client.set_on_read_completion(
			[&](const char* buff, size_t transmited_bytes) {
				if (transmited_bytes == 0) {
					client.execute_on_disconnect();
				} else {
					for (int w = 0; w < transmited_bytes; w++) {
						recv_str.push_back(buff[w]);
					}
					client.read_some();
				}
			}
		);
		
		client.set_on_write_completion(
			[&](size_t saved_bytes, size_t transmited_bytes) {
				Sleep(4000);
				if (transmited_bytes == 0) {
					client.execute_on_disconnect();
				} else {
					if (saved_bytes == 0) {
						cout << "client.shutdown_writing\n";
						client.shutdown_writing();
					} else {
						client.write_some_saved_bytes();
					}
				}
			}
		);
		
		client.set_on_disconnect(
			[&]() {
				client.close();
				if (recv_str == test_str) {
					cout << "OK\n";
				} else {
					cout << "ERROR:\n" << test_str << "\n"
						<< recv_str << "\n";
				}
				port.interrupt();
			}
		);
		
		for (int w = 0; w < 400; w++) {
			test_str += "_" + to_string(w);
		}
		cout << "size : " << test_str.size() << "\n";
		
		port.registrate_socket(client);
		
		client.write_some(&test_str[0], test_str.size());
		client.read_some();
		
		port.start();
	}
	Sleep(50000);
    return 0;
}
