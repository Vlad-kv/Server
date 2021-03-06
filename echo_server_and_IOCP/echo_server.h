#ifndef ECHO_SERVER_H
#define ECHO_SERVER_H

#include <map>

class echo_server;

#include "../Sockets/sockets.h"

class echo_server : public abstract_server {
public:
	echo_server(std::string addres_of_main_socket, int address_family, int type, int protocol, int port, IO_completion_port &comp_port);
private:
	struct client_data {
		client_socket_2 c_s;
		int data_size, sended_size;
		std::unique_ptr<timer> t;
		
		client_data(client_socket_2 &&c_s);
		client_data(client_data &&c_d);
		client_data();
		
		client_data& operator=(client_data &&c_d);
	};
	
	std::map<long long, client_data> client_sockets;
	
	void on_accept(client_socket_2 client_soc) override;
	void on_interruption() override;
	
	static void on_read_completion(echo_server *this_server, client_data *client_d, const char* buff, size_t size);
	static void on_write_completion(echo_server *this_server, client_data *c, size_t saved_bytes, size_t size);
	static void on_disconnect(echo_server *this_server, client_data *client_d);
};

#endif // ECHO_SERVER_H
