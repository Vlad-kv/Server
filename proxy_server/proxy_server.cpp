#include <tuple>

#include "proxy_server.h"
using namespace std;

proxy_server::client_data::client_data(proxy_server *this_server, client_socket_2 &&client)
: client(move(client)) {
	client_reader = make_unique<http_reader>(&this->client,
								[this_server, this](client_http_request req){this_server->on_client_request_reading_completion(*this, move(req));},
								[this_server, this](server_http_request req){throw new runtime_error("server_http_request from client_reader");},
								[this_server, this](int error){this_server->on_read_error_from_client(*this, error);}
	);
	client_writer = make_unique<http_writer>(&this->client,
								[this_server, this](){this_server->on_server_request_writing_completion(*this);},
								[this_server, this](){this_server->on_writing_shutdowning_from_client(*this);}
	);
}
proxy_server::client_data::client_data() {
}

proxy_server::proxy_server(std::string addres_of_main_socket, int port, IO_completion_port &comp_port)
: abstract_server(addres_of_main_socket, AF_INET, SOCK_STREAM, 0, port, comp_port) {
}

void proxy_server::on_accept(client_socket_2 client_s) {
	LOG("in proxy_server::on_accept");
	long long id = client_s.get_id();
	clients.emplace(piecewise_construct, tuple<long long>(id), tuple<proxy_server*, client_socket_2>(this, move(client_s)));
	
	client_data &client_d = clients[id];
	client_socket_2 &client = client_d.client;
	
	client.set_on_disconnect([&]() {on_disconnect(client_d);});
	
//	client_d.reader.
	
	accept();
}
void proxy_server::on_interruption() {
	LOG("proxy_server interrupted\n");
	clients.clear();
}

void proxy_server::on_client_request_reading_completion(client_data &data, client_http_request req) {
	LOG("request:\n");
	LOG(to_string(req));
	data.client_writer->write_request(
		move(server_http_request({1, 1}, 500, "Internal Server Error", {}, {}))
	);
}
void proxy_server::on_read_error_from_client(client_data &data, int error) {
	data.client_writer->write_request(
		move(server_http_request({1, 1}, 400, "Bad Request", {}, {}))
	);
}

void proxy_server::on_server_request_writing_completion(client_data &data) {
	on_disconnect(data);
}
void proxy_server::on_writing_shutdowning_from_client(client_data &data) {
	on_disconnect(data);
}

void proxy_server::on_disconnect(client_data &client_d) {
	LOG("in proxy_server::on_disconnect\n");
	clients.erase(client_d.client.get_id());
}
