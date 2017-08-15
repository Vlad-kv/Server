#include "http_request.h"
using namespace std;

namespace {
	string to_string(const vector<char> v) {
		string res;
		for (auto c : v) {
			res.push_back(c);
		}
		return res;
	}
}

http_request::http_request() {
}
http_request::http_request(http_request &&req)
: headers(move(req.headers)), message_body(move(req.message_body)) {
}
http_request::http_request(std::multimap<std::string, std::string> headers,
					std::vector<char> message_body)
: headers(move(headers)), message_body(move(message_body)) {
}
std::vector<char> to_vector(const http_request& req) {
	vector<char> res;
	for (auto &w : req.headers) {
		string temp_str = w.first + ": " + w.second + "\r\n";
		for (char c : temp_str) {
			res.push_back(c);
		}
	}
	res.push_back('\r');
	res.push_back('\n');
	for (auto c : req.message_body) {
		res.push_back(c);
	}
	return res;
}
std::string to_string(const http_request& req) {
	return to_string(to_vector(req));
}

client_http_request::client_http_request() {
}
client_http_request::client_http_request(client_http_request &&req)
: http_request(static_cast<http_request&&>(req)), method(move(req.method)),
  uri(move(req.uri)), version(move(req.version)) {
}
client_http_request::client_http_request(std::string method, std::string uri,
						std::pair<int, int> version,
						std::multimap<std::string, std::string> headers,
						std::vector<char> message_body)
: http_request(move(headers), move(message_body)), method(move(method)),
  uri(move(uri)), version(move(version)) {
}
int client_http_request::extract_port(const std::string& uri) {
	size_t pos = 0;
	if ((uri.size() >= 7) && (uri.substr(0, 7) == "http://")) {
		pos = 7;
	}
	while ((pos < uri.size()) && (uri[pos] != ':')) {
		pos++;
	}
	if (pos == uri.size()) {
		return 0;
	}
	pos++;
	string str_port;
	while ((pos < uri.size()) && (('0' <= uri[pos]) && (uri[pos] <= '9'))) {
		str_port.push_back(uri[pos++]);
	}
	try {
		return stoi(str_port);
	} catch (...) {
		return 0;
	}
}
int client_http_request::extract_port_number() {
	int int_port = extract_port(uri);
	if (headers.count("Host") == 0) {
		if (int_port == 0) {
			return 80;
		}
		return int_port;
	}
	string &str = (*headers.find("Host")).second;
	string port;
	size_t pos = 0;
	while ((pos < str.size()) && (str[pos] != ':')) {
		pos++;
	}
	if (pos != str.size()) {
		pos++;
		while (pos < str.size()) {
			port.push_back(str[pos++]);
		}
	}
	try {
		int_port = max(int_port, stoi(port));
	} catch (...) {
	}
	if (int_port == 0) {
		int_port = 80;
	}
	return int_port;
}
std::string client_http_request::extract_host() {
	string host;
	if (headers.count("Host") > 0) {
		string &str = (*headers.find("Host")).second;
		size_t pos = 0;
		while ((pos < str.size()) && (str[pos] != ':')) {
			host.push_back(str[pos++]);
		}
		return host;
	}
	size_t pos = 0;
	if ((uri.size() >= 7) && (uri.substr(0, 7) == "http://")) {
		pos = 7;
	}
	while ((pos < uri.size()) && (uri[pos] != ':') && (uri[pos] != '/')) {
		host.push_back(uri[pos++]);
	}
	return host;
}

std::vector<char> to_vector(const client_http_request& req) {
	vector<char> res;
	string temp_str = req.method + " " + req.uri + " HTTP/" + to_string(req.version.first) +
						"." + to_string(req.version.second) + "\r\n";
	for (char c : temp_str) {
		res.push_back(c);
	}
	for (char c : to_vector(static_cast<const http_request&>(req))) {
		res.push_back(c);
	}
	return res;
}
std::string to_string(const client_http_request& req) {
	return to_string(to_vector(req));
}

server_http_request::server_http_request() {
}
server_http_request::server_http_request(server_http_request &&req)
: http_request(static_cast<http_request&&>(req)), version(move(req.version)), status_code(req.status_code),
  reason_phrase(move(req.reason_phrase)) {
}
server_http_request::server_http_request(std::pair<int, int> version, int status_code,
						std::string reason_phrase,
						std::multimap<std::string, std::string> headers,
						std::vector<char> message_body)
: http_request(move(headers), move(message_body)), version(move(version)),
  status_code(status_code), reason_phrase(move(reason_phrase)) {
}
std::vector<char> to_vector(const server_http_request& req) {
	vector<char> res;
	string temp_str = "HTTP/" + to_string(req.version.first) + "." + to_string(req.version.second) +
						" " + to_string(req.status_code) + " " + req.reason_phrase + "\r\n";
	for (char c : temp_str) {
		res.push_back(c);
	}
	for (char c : to_vector(static_cast<const http_request&>(req))) {
		res.push_back(c);
	}
	return res;
}
std::string to_string(const server_http_request& req) {
	return to_string(to_vector(req));
}
