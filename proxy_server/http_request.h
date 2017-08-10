#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <map>
#include <vector>

struct http_request {
	http_request();
	http_request(http_request &&req);
	
	std::multimap<std::string, std::string> headers;
	std::vector<char> message_body;
};
std::vector<char> to_vector(const http_request& req);
std::string to_string(const http_request& req);

struct client_http_request : http_request {
	client_http_request();
	client_http_request(client_http_request &&req);
	
	std::string method, uri;
	std::pair<int, int> version;
};
std::vector<char> to_vector(const client_http_request& req);
std::string to_string(const client_http_request& req);

struct server_http_request : http_request {
	server_http_request();
	server_http_request(server_http_request &&req);
	
	std::pair<int, int> version;
	int status_code;
	std::string reason_phrase;
};

#endif // HTTP_REQUEST_H
