#ifndef PROXY_SERVER_H
#define PROXY_SERVER_H

#include "../Sockets/sockets.h"
#include "http_reader.h"
#include "http_writer.h"

class proxy_server : public abstract_server {
public:
	struct client_data {
		client_data(proxy_server *this_server, client_socket_2 &&client);
		client_data();
		
		client_socket_2 client;
		std::unique_ptr<http_reader> client_reader;
		std::unique_ptr<http_writer> client_writer;
	};
	
	proxy_server(std::string addres_of_main_socket, int port, IO_completion_port &comp_port);
	
private:
	
	void on_accept(client_socket_2 client_s) override;
	void on_interruption() override;
	
	void on_client_request_reading_completion(client_data &data, client_http_request req);
	void on_read_error_from_client(client_data &data, int error);
	
	void on_server_request_writing_completion(client_data &data);
	void on_writing_shutdowning_from_client(client_data &data);
	
	void on_disconnect(client_data &client_d);
	
	std::map<long long, client_data> clients;
};

#endif // PROXY_SERVER_H
