#include <iostream>
#include "../Sockets/sockets.h"
#include "proxy_server.h"
using namespace std;
const char* MAIN_SOCKET_ADDRES = "127.0.0.1";

int main() {
	WSA_holder wsa_holder(MAKEWORD(2, 2));
	
	IO_completion_port port;
	proxy_server server(MAIN_SOCKET_ADDRES, 80, port);
	
	port.start();
	
	Sleep(1000);
	
	return 0;
}