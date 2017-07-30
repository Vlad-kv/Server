#include <iostream>

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

#include "logger.h"
#include "../echo_server/sockets_wrapper.h"
#include "echo_server.h"
#include "socket.h"

#include "timers.h"

using namespace std;

const char* MAIN_SOCKET_ADDRES = "127.0.0.1";
const int DEFAULT_BUFFER_SIZE = 1024;

int main(int argc, char* argv[]) {
	WSA_holder wsa_holder(MAKEWORD(2, 2));
	{
		IO_completion_port port;
		
		echo_server server(MAIN_SOCKET_ADDRES, AF_INET, SOCK_STREAM, 0, 8001, port);
		
		server.add_task([](){cout << "Tasks works!\n";});
		
		timer t(chrono::milliseconds(2000), 
				[](timer *this_timer) {
					cout << "Timers works!\n";
					this_timer->restart();
				}
		);
		server.registrate_timer(t);
		
		port.start();
	}
	Sleep(5000);
	return 0;
}
