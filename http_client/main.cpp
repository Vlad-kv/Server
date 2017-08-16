#include <iostream>

#include "../Sockets/sockets.h"
#include "../http_utils/http_utils.h"
using namespace std;
int main() {
	WSA_holder wsa_holder(MAKEWORD(2, 2));
	IO_completion_port port;
	
	client_socket client(AF_INET, SOCK_STREAM, 0);
	client.connect(AF_INET, "127.0.0.1", 80);
	
	port.registrate_socket(client);
	
	http_reader reader(&client,
						[&](http_request req) {},
						[&](http_response req) {
							cout << to_string(req) << "\n";
							client.execute_on_disconnect();
						},
						[&](int error) {client.execute_on_disconnect();}
	);
	http_writer writer(&client,
						[&]() {
							reader.read_server_response();
						},
						[&]() {
							cout << "shut downing from writer\n";
							client.execute_on_disconnect();
						}
	);
	
	client.set_on_disconnect(
		[&]() {
			client.close();
			port.interrupt();
		}
	);
	
	writer.write_request(http_request(
		"TRACE", "/", {1, 1}, {
			{"User-Agent", "Mozilla/4.0 (compatible; MSIE5.01; Windows NT)"},
			{"Host", "www.tutorialspoint.com"}
		}, {}
	));
	
	port.start();
	
	cout << "before sleeping\n";
	Sleep(100000);
	return 0;
}
