#ifndef HTTP_READER_H
#define HTTP_READER_H

#include <functional>
#include <memory>

#include "../Sockets/sockets.h"
#include "http_message.h"

class http_reader {
public:
	static const int SYNTAX_ERROR = 1;
	static const int READING_SHUTDOWNED_ERROR = 2;
	
	typedef std::function<void (http_request)> client_callback_t;
	typedef std::function<void (http_response)> server_callback_t;
	typedef std::function<void (int)> on_read_error_t;
	typedef std::function<void ()> func_t;
	
	template<typename client_sock_t>
	http_reader(client_sock_t *sock, client_callback_t c_f,
				server_callback_t s_f, on_read_error_t err_f,
				func_t on_event = []() {})
	: is_alive(std::make_shared<bool>(true)),
	  client_callback(std::move(c_f)), server_callback(std::move(s_f)),
	  on_error(std::move(err_f)), on_event_from_sock(std::move(on_event)) {
		
		read_some = [sock](){sock->read_some();};
		std::shared_ptr<bool> temp(is_alive);
		sock->set_on_read_completion(
			[this, temp](const char* buff, size_t size) {
				if (*temp) {
					on_read_completion(buff, size);
				}
			}
		);
	}
	
	void read_client_request();
	void read_server_response();
	
	void add_method_that_was_sended_to_server(std::string method);
	
	bool is_previous_request_completed();
	bool is_it_shutdowned();
	
	bool is_last_response_available();
	bool is_no_bytes_where_readed_after_last_completed_message();
	http_response get_last_response();
	
	std::pair<const char*, size_t> get_extra_readed_data();
	
	void close();
	~http_reader();
private:
	void clear();
	
	void read_main_part();
	std::pair<std::string, std::string> read_header(int &pos);
	void read_line(func_t to_call);
	void read_line_cycle();
	
	void parse_request_main_patr();
	void parse_response_main_patr();
	
	void read_message_body(http_message &mess);
	void read_message_body_using_content_length();
	void read_message_body_using_chunked_encoding();
	
	void read_chunck();
	void read_chunk_size();
	void read_chunk_data();
	void read_trailer();
	void finish_reading_trailer();
	
	void read_message_body_until_connection_not_closed();
	
	void on_read_completion(const char* buff, size_t size);
	
	void on_read_response_messadge_body_completion();
	void on_read_request_messadge_body_completion();
private:
	std::shared_ptr<bool> is_alive;
	bool reading_until_not_closing = false;
	
	bool is_reading_shutdowned = false;
	
	std::unique_ptr<http_request> forming_client_req = nullptr;
	std::unique_ptr<http_response> forming_server_resp = nullptr;
	
	client_callback_t client_callback;
	server_callback_t server_callback;
	
	on_read_error_t on_error;
	
	func_t func_to_call_on_r_comp;
	func_t to_call_on_read_main_part;
	func_t to_call_on_read_message_body_completion;
	func_t to_call_after_reading_line;
	func_t on_event_from_sock;
	
	const char *buff = nullptr;
	size_t readed_bytes = 0;
	size_t readed_bytes_in_this_message = 0;
	
	size_t remaining_length;
	http_message *where_to_write_message_body;
	std::queue<std::string> methods;
	
	char next;
	std::string readed_buff;
	
	func_t read_some;
};

#endif // HTTP_READER_H
