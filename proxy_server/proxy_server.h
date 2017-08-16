#ifndef PROXY_SERVER_H
#define PROXY_SERVER_H

#include <set>

#include "../Sockets/sockets.h"
#include "../http_utils/http_utils.h"

struct queue_element {
	queue_element(http_request &&request);
	
	http_request request;
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
		std::queue<http_response> server_responses;
		
		bool to_write_server_req_and_delete = false;
		bool to_delete = false;
	};
	
	proxy_server(std::string addres_of_main_socket, int port, IO_completion_port &comp_port);
	
private:
	void notify_client_about_error(client_data &data, int status_code, std::string reason_phrase);
	
	void on_accept(client_socket_2 client_s) override;
	void on_interruption() override;
	
	void on_client_request_reading_completion(client_data &data, http_request req);
	void on_read_error_from_client(client_data &data, int error);
	
	void on_server_response_writing_completion(client_data &data);
	void on_writing_shutdowning_from_client(client_data &data);
	
	void on_server_response_reading_completion(client_data &data, http_response req);
	void on_read_error_from_server(client_data &data, int error);
	
	void on_client_request_writing_completion(client_data &data);
	void on_writing_shutdowning_from_server(client_data &data);
	
	void on_client_disconnect(client_data &data);
	void on_server_disconnect(client_data &data);
	
	void getaddrinfo_callback(client_data &data, addrinfo *info, std::shared_ptr<queue_element> req, int port);
	
	void delete_client_data(client_data &data);
private:
	bool is_interrupted = false;
	std::map<long long, client_data> clients;
	std::set<long long> keys_in_process_of_deletion;
	getaddrinfo_executer executer;
};

#endif // PROXY_SERVER_H
