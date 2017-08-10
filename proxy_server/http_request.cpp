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
: method(move(req.method)), uri(move(req.method)) {
}
std::vector<char> to_vector(const client_http_request& req) {
	vector<char> res;
	string temp_str = req.method + " " + req.uri + " http/" + to_string(req.version.first) +
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
: version(move(req.version)), status_code(req.status_code),
  reason_phrase(move(req.reason_phrase)) {
}
