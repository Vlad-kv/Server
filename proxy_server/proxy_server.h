#ifndef PROXY_SERVER_H
#define PROXY_SERVER_H

#include <set>

#include "../Sockets/sockets.h"
#include "http_reader.h"
#include "http_writer.h"


struct queue_element {
	queue_element(client_http_request &&request);
	
	client_http_request request;
	bool is_getaddrinfo_completed = false;
};

class proxy_server : public abstract_server {
public:
	struct client_data {
		client_data(proxy_server *this_server, client_socket_2 &&client);
		client_data();
		
		client_socket_2 client;
		std::unique_ptr<http_reader> client_reader;
		std::unique_ptr<http_writer> client_writer;
		std::queue<std::shared_ptr<queue_element>> client_requests;
		
		client_socket server;
		std::unique_ptr<http_reader> server_reader;
		std::unique_ptr<http_writer> server_writer;
		std::queue<server_http_request> server_requests;
		
		bool to_write_server_req_and_delete = false;
		bool to_delete = false;
	};
	
	proxy_server(std::string addres_of_main_socket, int port, IO_completion_port &comp_port);
	
private:
	static int extract_port(const std::string& uri);
	
	void notify_client_about_error(client_data &data, int status_code, std::string reason_phrase);
	
	void on_accept(client_socket_2 client_s) override;
	void on_interruption() override;
	
	void on_client_request_reading_completion(client_data &data, client_http_request req);
	void on_read_error_from_client(client_data &data, int error);
	
	void on_server_request_writing_completion(client_data &data);
	void on_writing_shutdowning_from_client(client_data &data);
	
	void on_server_request_reading_completion(client_data &data, server_http_request req);
	void on_read_error_from_server(client_data &data, int error);
	
	void on_client_request_writing_completion(client_data &data);
	void on_writing_shutdowning_from_server(client_data &data);
	
	void on_client_disconnect(client_data &client_d);
	void on_server_disconnect(client_data &client_d);
	
	void getaddrinfo_callback(client_data &data, addrinfo *info, std::shared_ptr<queue_element> req, int port);
	
	void delete_client_data(client_data &client_d);
private:
	bool is_interrupted = false;
	std::map<long long, client_data> clients;
	std::set<long long> keys_in_process_of_deletion;
	getaddrinfo_executer executer;
};

#endif // PROXY_SERVER_H
