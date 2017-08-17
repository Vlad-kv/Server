#ifndef HTTP_MESSAGE_H
#define HTTP_MESSAGE_H

#include <map>
#include <vector>

struct http_message {
	http_message();
	http_message(http_message &&req);
	http_message(std::multimap<std::string, std::string> headers,
					std::vector<char> message_body);
	http_message(std::multimap<std::string, std::string> headers,
					std::string message_body);
	
	std::multimap<std::string, std::string> headers;
	std::vector<char> message_body;
};
std::vector<char> to_vector(const http_message& req);
std::string to_string(const http_message& req);

struct http_request : http_message {
	http_request();
	http_request(http_request &&req);
	http_request(std::string method, std::string uri,
						std::pair<int, int> version,
						std::multimap<std::string, std::string> headers,
						std::vector<char> message_body);
	http_request(std::string method, std::string uri,
						std::pair<int, int> version,
						std::multimap<std::string, std::string> headers,
						std::string message_body);
	int extract_port_number();
	std::string extract_host();
private:
	static int extract_port(const std::string& uri);
public:
	
	std::string method, uri;
	std::pair<int, int> version;
};
std::vector<char> to_vector(const http_request& req);
std::string to_string(const http_request& req);

struct http_response : http_message {
	http_response();
	http_response(http_response &&req);
	http_response(std::pair<int, int> version, int status_code,
						std::string reason_phrase,
						std::multimap<std::string, std::string> headers,
						std::vector<char> message_body);
	
	std::pair<int, int> version;
	int status_code;
	std::string reason_phrase;
};
std::vector<char> to_vector(const http_response& req);
std::string to_string(const http_response& req);

#endif // HTTP_MESSAGE_H
