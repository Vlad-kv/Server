#ifndef PROXY_SERVER_H
#define PROXY_SERVER_H

#include <set>

#include "../Sockets/sockets.h"
#include "../http_utils/http_utils.h"

const std::chrono::milliseconds TIMEOUT = std::chrono::milliseconds(3000);

class proxy_server : public abstract_server {
public:
	struct client_data {
		client_data(proxy_server *this_server, client_socket_2 &&client);
		client_data();
		void init_server_part(proxy_server *this_server);
		
		void reset_server(proxy_server *this_server);
		
		timer t;
		
		client_socket_2 client;
		std::unique_ptr<http_reader> client_reader;
		std::unique_ptr<http_writer> client_writer;
		std::queue<http_request> client_requests;
		
		client_socket server;
		std::unique_ptr<http_reader> server_reader;
		std::unique_ptr<http_writer> server_writer;
		std::queue<http_response> server_responses;
		
		std::string current_host;
		bool is_previous_connect_completed = true;
		
		int num_of_responses_to_read_from_server = 0;
		
		bool is_server_now_reseting = false;
		bool tunnel_creating = false;
		
		bool to_write_server_req_and_delete = false;
		bool to_delete = false;
		bool to_delete_on_empty_server_responses = false;
		
		bool is_timer_expired_at_past = false;
		
		bool is_reading_from_client_shutdowned = false;
	};
	
	proxy_server(std::string addres_of_main_socket, int port, IO_completion_port &comp_port);
	
private:
	void write_to_server(client_data &data);
	void write_to_client(client_data &data);
	
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
	
	void getaddrinfo_callback(client_data &data, addrinfo *info, int port);
	void on_connect_completion(client_data &data);
	
	void on_event_from_socket(client_data &data);
	void on_timer_expiration(client_data &data);
	
	void delete_client_data(client_data &data);
	
	void establish_tunnel(client_data &data);
private:
	bool is_interrupted = false;
	std::map<long long, client_data> clients;
	std::set<long long> keys_in_process_of_deletion;
	getaddrinfo_executer executer;
};

#endif // PROXY_SERVER_H
