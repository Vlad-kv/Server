#ifndef HTTP_READER_H
#define HTTP_READER_H

#include <functional>
#include <memory>

#include "../Sockets/sockets.h"
#include "http_request.h"

class http_reader {
public:
	static const int SYNTAX_ERROR = 1;
	static const int READING_SHUTDOWNED_ERROR = 2;
	
	typedef std::function<void (client_http_request)> client_callback_t;
	typedef std::function<void (server_http_request)> server_callback_t;
	typedef std::function<void (int)> on_read_error_t;
	typedef std::function<void ()> func_t;
	
	template<typename client_sock_t>
	http_reader(client_sock_t *sock, client_callback_t c_f,
				server_callback_t s_f, on_read_error_t err_f)
	: is_alive(std::make_shared<bool>(true)),
	  client_callback(std::move(c_f)), server_callback(std::move(s_f)),
	  on_error(std::move(err_f)) {
		
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
	void read_server_request();
	
	bool is_previous_request_completed();
	
	void close();
	~http_reader();
private:
	void clear();
	
	void read_main_patr();
	int read_header(int &pos);
	
	void parse_client_main_patr();
	void parse_server_main_patr();
	
	void read_message_body_using_content_length();
	
	void on_read_completion(const char* buff, size_t size);
	
	void on_read_server_messadge_body_completion();
	
private:
	std::shared_ptr<bool> is_alive;
	
	std::unique_ptr<client_http_request> forming_client_req = nullptr;
	std::unique_ptr<server_http_request> forming_server_req = nullptr;
	
	client_callback_t client_callback;
	server_callback_t server_callback;
	
	on_read_error_t on_error;
	
	func_t func_to_call_on_r_comp;
	func_t to_call_on_read_main_part;
	func_t to_call_on_read_message_body_completion;
	
	const char *buff = nullptr;
	size_t readed_bytes = 0;
	size_t remaining_content_length;
	http_request *where_to_write_message_body;
	
	char next;
	std::string readed_buff;
	
	func_t read_some;
};

#endif // HTTP_READER_H
