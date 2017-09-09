#ifndef HTTP_WRITER_H
#define HTTP_WRITER_H

#include <functional>
#include <memory>

#include "http_message.h"
#include "my_assert.h"

class http_writer {
public:
	typedef std::function<void ()> func_t;
	
	template<typename client_sock_t>
	http_writer(client_sock_t *sock, func_t f, func_t on_writing_shutdown, func_t on_event = []() {})
	: is_alive(std::make_shared<bool>(true)), callback(std::move(f)),
	  on_writing_shutdown(on_writing_shutdown),
	  on_event_from_sock(std::move(on_event)) {
	  	
		write_some = [sock](const char* buff, size_t size) {
			sock->write_some(buff, size);
		};
		write_some_saved_bytes = [sock]() {
			sock->write_some_saved_bytes();
		};
		std::shared_ptr<bool> temp(is_alive);
		sock->set_on_write_completion(
			[this, temp](size_t saved_bytes, size_t transmitted_bytes) {
				if (*temp) {
					on_write_completion(saved_bytes, transmitted_bytes);
				}
			}
		);
	}
	
	template<typename http_request_t>
	void write_request(const http_request_t& req) {
		my_assert(is_writing_not_completed, "previous request not writed");
		my_assert(!*is_alive, "http_writer already closed\n");
		
		std::vector<char> temp_v = to_vector(req);
		
		is_writing_not_completed = true;
		write_some(&temp_v[0], temp_v.size());
	}
	
	bool is_previous_request_completed();
	
	void close();
	~http_writer();
private:
	void on_write_completion(size_t saved_bytes, size_t transmitted_bytes);
	
private:
	std::shared_ptr<bool> is_alive;
	
	func_t callback;
	func_t on_writing_shutdown;
	func_t on_event_from_sock;
	bool is_writing_not_completed = false;
	
	std::function<void (const char* buff, size_t size)> write_some;
	func_t write_some_saved_bytes;
};

#endif // HTTP_WRITER_H
