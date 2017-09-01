#ifndef HTTP_MESSAGE_H
#define HTTP_MESSAGE_H

#include <map>
#include <vector>
#include "../Sockets/sockets.h"

struct uri_authority {
	uri_authority(const std::string &str);
	uri_authority(uri_authority &&auth);
	
	std::string userinfo, host;
	int port = 0;
};

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
std::vector<char> to_vector(const http_message& req, bool with_body = true);
std::string to_string(const http_message& req, bool with_body = true);

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
	static uri_authority extract_authority(const std::string& uri);
	static std::string extract_scheme(const std::string& uri);
public:
	
	std::string method, uri;
	std::pair<int, int> version;
};
std::vector<char> to_vector(const http_request& req, bool with_body = true);
std::string to_string(const http_request& req, bool with_body = true);

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
std::vector<char> to_vector(const http_response& req, bool with_body = true);
std::string to_string(const http_response& req, bool with_body = true);

#endif // HTTP_MESSAGE_H
