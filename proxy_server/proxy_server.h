#ifndef PROXY_SERVER_H
#define PROXY_SERVER_H

#include "../Sockets/sockets.h"
#include "http_reader.h"

class proxy_server : public abstract_server {
public:
	struct client_data {
		client_data(client_socket_2 &&client);
		client_data();
		
		void add_mess(std::string str);
		void write_if_poss();
		
		client_socket_2 client;
		std::queue<std::string> write_mess;
		std::unique_ptr<http_reader> reader;
	};
	
	proxy_server(std::string addres_of_main_socket, int port, IO_completion_port &comp_port);
	
private:
	
	void on_accept(client_socket_2 client_s) override;
	void on_interruption() override;
	
	void on_client_request_reading_completion();
	void on_write_completion(client_data &c, size_t saved_bytes, size_t size);
	void on_disconnect(client_data &client_d);
	
	std::map<long long, client_data> clients;
};

#endif // PROXY_SERVER_H
