#include <iostream>
#include "proxy_server.h"
using namespace std;
const char* MAIN_SOCKET_ADDRES = "127.0.0.1";

int main() {
	clear_log_file();
	{
		WSA_holder wsa_holder(MAKEWORD(2, 2));
		
		IO_completion_port port;
		proxy_server server(MAIN_SOCKET_ADDRES, 80, port);
		
		port.start();
	}
	cout << "before sleep\n";
	Sleep(100000);
	return 0;
}
