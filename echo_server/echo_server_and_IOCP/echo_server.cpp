#include "echo_server.h"

using namespace std;

echo_server::client_data::client_data(client_socket_2 &&c_s)
: c_s(move(c_s)), buff(nullptr) {
}

echo_server::client_data::client_data()
: buff(nullptr) {
}

echo_server::client_data& echo_server::client_data::operator=(client_data &&c_d) {
	c_s = move(c_d.c_s);
	buff = c_d.buff;
	data_size = c_d.data_size;
	
	c_d.buff = nullptr;
	c_d.data_size = 0;
	return *this;
}

echo_server::client_data::~client_data() {
	if (buff != nullptr) {
		delete []buff;
	}
}

///-------

echo_server::echo_server(string addres_of_main_socket, int address_family, int type, int protocol, int port, IO_completion_port &comp_port)
: abstract_server(addres_of_main_socket, address_family, type, protocol, port, comp_port) {
}

void echo_server::on_accept(client_socket_2 client_soc) {
	int id = client_soc.get_id();
	client_sockets[id] = client_data(move(client_soc));
	
	client_data &client_d = client_sockets[id];
	
	client_d.c_s.set_on_read_completion(bind(on_read_completion, this, &client_d, placeholders::_1));
	client_d.c_s.set_on_write_completion(bind(on_write_completion, this, &client_d, placeholders::_1));
	client_d.c_s.set_on_disconnect(function<void ()>(bind(on_disconnect, this, &client_d)));
	
	client_d.buff = new char[BUFFER_SIZE];
	client_d.data_size = BUFFER_SIZE;
	
	client_d.c_s.read_some(client_d.buff, client_d.data_size);
	
	accept();
}

void echo_server::on_interruption() {
	LOG("Echo server interrupted.\n");
}

void echo_server::on_read_completion(echo_server *this_server, client_data *client_d, size_t size) {
	LOG("in on_read_completion : " << size << "\n");
	if (size == 0) {
		client_d->c_s.execute_on_disconnect();
		return;
	}
	client_d->data_size = size;
	client_d->sended_size = 0;
	
	client_d->c_s.write_some(client_d->buff, size);
}

void echo_server::on_write_completion(echo_server *this_server, client_data *c, size_t size) {
	LOG("in on_write_completion : " << size << "\n");
	if (size == 0) {
		c->c_s.execute_on_disconnect();
		return;
	}
	c->sended_size += size;
	if (c->sended_size == c->data_size) {
		c->c_s.read_some(c->buff, BUFFER_SIZE);
	} else {
		c->c_s.write_some(c->buff + c->sended_size, c->data_size - c->sended_size);
	}
}

void echo_server::on_disconnect(echo_server *this_server, client_data *client_d) {
	LOG("in on_disconnect\n");
	long long id = client_d->c_s.get_id();
	this_server->client_sockets.erase(id);
}
