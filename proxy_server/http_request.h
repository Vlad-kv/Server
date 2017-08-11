#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <map>
#include <vector>

struct http_request {
	http_request();
	http_request(http_request &&req);
	http_request(std::multimap<std::string, std::string> headers,
					std::vector<char> message_body);
	
	std::multimap<std::string, std::string> headers;
	std::vector<char> message_body;
};
std::vector<char> to_vector(const http_request& req);
std::string to_string(const http_request& req);

struct client_http_request : http_request {
	client_http_request();
	client_http_request(client_http_request &&req);
	client_http_request(std::string method, std::string uri,
						std::pair<int, int> version,
						std::multimap<std::string, std::string> headers,
						std::vector<char> message_body);
	
	std::string method, uri;
	std::pair<int, int> version;
};
std::vector<char> to_vector(const client_http_request& req);
std::string to_string(const client_http_request& req);

struct server_http_request : http_request {
	server_http_request();
	server_http_request(server_http_request &&req);
	server_http_request(std::pair<int, int> version, int status_code,
						std::string reason_phrase,
						std::multimap<std::string, std::string> headers,
						std::vector<char> message_body);
	
	std::pair<int, int> version;
	int status_code;
	std::string reason_phrase;
};
std::vector<char> to_vector(const server_http_request& req);
std::string to_string(const server_http_request& req);

#endif // HTTP_REQUEST_H
