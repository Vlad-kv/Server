#include "proxy_server.h"

#include "http_reader.h"
using namespace std;

proxy_server::client_data::client_data(client_socket_2 &&client)
: client(move(client)) {
}
proxy_server::client_data::client_data() {
}
void proxy_server::client_data::add_mess(std::string str) {
	write_mess.push(move(str));
	write_if_poss();
}
void proxy_server::client_data::write_if_poss() {
	if ((client.is_writing_available()) && (write_mess.size() > 0)) {
		string &str = write_mess.front();
		client.write_some(&str[0], str.size());
		write_mess.pop();
	}
}

proxy_server::proxy_server(std::string addres_of_main_socket, int port, IO_completion_port &comp_port)
: abstract_server(addres_of_main_socket, AF_INET, SOCK_STREAM, 0, port, comp_port) {
}

void proxy_server::on_accept(client_socket_2 client_s) {
	LOG("in proxy_server::on_accept");
	long long id = client_s.get_id();
	clients.emplace(id, move(client_s));
	
	client_data &client_d = clients[id];
	client_socket_2 &client = client_d.client;
	
	client_d.reader = make_unique<http_reader>(&client);
	
	client.set_on_write_completion(
		[&](size_t saved_bytes, size_t size) {
			on_write_completion(client_d, saved_bytes, size);
		}
	);
	
	client.set_on_disconnect(
		[&]() {
			on_disconnect(client_d);
		}
	);
	
	client.read_some();
	accept();
}
void proxy_server::on_interruption() {
	LOG("proxy_server interrupted\n");
	clients.clear();
}

void proxy_server::on_write_completion(client_data &c, size_t saved_bytes, size_t size) {
	LOG("in proxy_server::on_write_completion\n");
	if (size == 0) {
		LOG("in on_write_completion size == 0\n");
		c.client.execute_on_disconnect();
		return;
	}
	if (saved_bytes > 0) {
		c.client.write_some_saved_bytes();
	} else {
		c.client.execute_on_disconnect();
//		c.write_if_poss();
	}
}
void proxy_server::on_disconnect(client_data &client_d) {
	LOG("in proxy_server::on_disconnect\n");
	clients.erase(client_d.client.get_id());
}
