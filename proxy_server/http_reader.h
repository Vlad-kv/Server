#ifndef HTTP_READER_H
#define HTTP_READER_H

#include <functional>
#include <memory>

#include "../Sockets/sockets.h"
#include "http_request.h"

class http_reader {
public:
	static const int SYNTAX_ERROR = 1;
	
	typedef std::function<void (client_http_request)> client_callback_t;
	typedef std::function<void (server_http_request)> server_callback_t;
	typedef std::function<void (int)> on_read_error_t;
	typedef std::function<void ()> func_t;
	
	template<typename client_sock_t>
	http_reader(client_sock_t *sock) {
		read_some = [sock](){sock->read_some();};
		sock->set_on_read_completion(
			[this](const char* buff, size_t transmitted_bytes) {
				on_read_completion(buff, transmitted_bytes);
			}
		);
	}
	
	void read_client_request(client_callback_t f, on_read_error_t err_f);
	void read_server_request(server_callback_t f, on_read_error_t err_f);
	
private:
	void clear();
	
	void read_main_patr();
	
	void parse_client_main_patr();
	
	int read_header(int &pos);
	
	std::unique_ptr<client_http_request> forming_client_req = nullptr;
	std::unique_ptr<server_http_request> forming_server_req = nullptr;
	
	client_callback_t client_callback;
	server_callback_t server_callback;
	
	on_read_error_t on_error = nullptr;
	
	func_t func_to_call_on_r_comp;
	func_t to_call_on_read_main_part;
	
	const char *buff = nullptr;
	size_t readed_bytes = 0;
	
	char next;
	std::string readed_buff;
	
	func_t read_some;
	
	void on_read_completion(const char* buff, size_t transmitted_bytes);
};

#endif // HTTP_READER_H
