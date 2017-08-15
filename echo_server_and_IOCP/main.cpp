#include <iostream>

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

#include "../Sockets/sockets.h"
#include "test_server.h"
#include "echo_server.h"
using namespace std;

const char* MAIN_SOCKET_ADDRES = "127.0.0.1";

int main(int argc, char* argv[]) {
	WSA_holder wsa_holder(MAKEWORD(2, 2));
	{
		IO_completion_port port;
//		echo_server server(MAIN_SOCKET_ADDRES, AF_INET, SOCK_STREAM, 0, 8001, port);
		
		test_server test_s(MAIN_SOCKET_ADDRES, AF_INET, SOCK_STREAM, 0, 8002, port);
		port.start();
	}
	Sleep(2000);
	return 0;
}
