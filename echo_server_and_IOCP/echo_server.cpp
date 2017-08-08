#include "echo_server.h"
using namespace std;

echo_server::client_data::client_data(client_socket_2 &&c_s)
: c_s(move(c_s)) {
}
echo_server::client_data::client_data(client_data &&c_d)
: c_s(move(c_d.c_s)), t(move(c_d.t)) {
}
echo_server::client_data::client_data() {
}
echo_server::client_data& echo_server::client_data::operator=(client_data &&c_d) {
	c_s = move(c_d.c_s);
	t = move(c_d.t);
	data_size = c_d.data_size;
	sended_size = c_d.sended_size;
}
///-------

echo_server::echo_server(std::string addres_of_main_socket, int address_family, int type, int protocol, int port, IO_completion_port &comp_port)
: abstract_server(addres_of_main_socket, address_family, type, protocol, port, comp_port) {
}

void echo_server::on_accept(client_socket_2 client_soc) {
	long long id = client_soc.get_id();
	LOG("in on_accept : " << id << "\n");
	
	client_sockets[id] = client_data(move(client_soc));
	
	client_data &client_d = client_sockets[id];
	client_d.t = make_unique<timer>(
		chrono::milliseconds(3000),
		[&](timer *t) {
			LOG("time expired\n");
			client_d.c_s.execute_on_disconnect();
		}
	);
	registrate_timer(*client_d.t);
	
	using namespace placeholders;
	
	client_d.c_s.set_on_read_completion(bind(on_read_completion, this, &client_d, _1, _2));
	client_d.c_s.set_on_write_completion(bind(on_write_completion, this, &client_d, _1, _2));
	client_d.c_s.set_on_disconnect(function<void ()>(bind(on_disconnect, this, &client_d)));
	
	client_d.c_s.read_some();
	
	accept();
}

void echo_server::on_interruption() {
	LOG("Echo server interrupted.\n");
}

void echo_server::on_read_completion(echo_server *this_server, client_data *client_d, const char* buff, size_t size) {
	LOG("in on_read_completion : " << size << "\n");
	client_d->t->restart();
	if (size == 0) {
		client_d->c_s.execute_on_disconnect();
		return;
	}
	client_d->data_size = size;
	client_d->sended_size = 0;
	
	client_d->c_s.write_some(buff, size);
}

void echo_server::on_write_completion(echo_server *this_server, client_data *c, size_t saved_bytes, size_t size) {
	LOG("in on_write_completion : " << size << "\n");
	c->t->restart();
	if (size == 0) {
		c->c_s.execute_on_disconnect();
		return;
	}
	c->sended_size += size;
	if (saved_bytes == 0) {
		c->c_s.read_some();
	} else {
		c->c_s.write_some_saved_bytes();
	}
}

void echo_server::on_disconnect(echo_server *this_server, client_data *client_d) {
	LOG("in on_disconnect\n");
	long long id = client_d->c_s.get_id();
	this_server->client_sockets.erase(id);
}
