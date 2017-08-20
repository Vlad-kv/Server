#include "http_message.h"
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

http_message::http_message() {
}
http_message::http_message(http_message &&req)
: headers(move(req.headers)), message_body(move(req.message_body)) {
}
http_message::http_message(std::multimap<std::string, std::string> headers,
					std::vector<char> message_body)
: headers(move(headers)), message_body(move(message_body)) {
}
http_message::http_message(std::multimap<std::string, std::string> headers,
					std::string message_body)
: headers(move(headers)) {
	for (char w : message_body) {
		this->message_body.push_back(w);
	}
}
std::vector<char> to_vector(const http_message& req, bool with_body) {
	vector<char> res;
	for (auto &w : req.headers) {
		string temp_str = w.first + ": " + w.second + "\r\n";
		for (char c : temp_str) {
			res.push_back(c);
		}
	}
	res.push_back('\r');
	res.push_back('\n');
	if (with_body) {
		for (auto c : req.message_body) {
			res.push_back(c);
		}
	}
	return res;
}
std::string to_string(const http_message& req, bool with_body) {
	return to_string(to_vector(req, with_body));
}

http_request::http_request() {
}
http_request::http_request(http_request &&req)
: http_message(static_cast<http_message&&>(req)), method(move(req.method)),
  uri(move(req.uri)), version(move(req.version)) {
}
http_request::http_request(std::string method, std::string uri,
						std::pair<int, int> version,
						std::multimap<std::string, std::string> headers,
						std::vector<char> message_body)
: http_message(move(headers), move(message_body)), method(move(method)),
  uri(move(uri)), version(move(version)) {
}
http_request::http_request(std::string method, std::string uri,
						std::pair<int, int> version,
						std::multimap<std::string, std::string> headers,
						std::string message_body)
: http_message(move(headers), move(message_body)), method(move(method)),
  uri(move(uri)), version(move(version)) {
}
int http_request::extract_port(const std::string& uri) {
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
int http_request::extract_port_number() {
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
std::string http_request::extract_host() {
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

std::vector<char> to_vector(const http_request& req, bool with_body) {
	vector<char> res;
	string temp_str = req.method + " " + req.uri + " HTTP/" + to_string(req.version.first) +
						"." + to_string(req.version.second) + "\r\n";
	for (char c : temp_str) {
		res.push_back(c);
	}
	for (char c : to_vector(static_cast<const http_message&>(req), with_body)) {
		res.push_back(c);
	}
	return res;
}
std::string to_string(const http_request& req, bool with_body) {
	return to_string(to_vector(req, with_body));
}

http_response::http_response() {
}
http_response::http_response(http_response &&req)
: http_message(static_cast<http_message&&>(req)), version(move(req.version)), status_code(req.status_code),
  reason_phrase(move(req.reason_phrase)) {
}
http_response::http_response(std::pair<int, int> version, int status_code,
						std::string reason_phrase,
						std::multimap<std::string, std::string> headers,
						std::vector<char> message_body)
: http_message(move(headers), move(message_body)), version(move(version)),
  status_code(status_code), reason_phrase(move(reason_phrase)) {
}
std::vector<char> to_vector(const http_response& req, bool with_body) {
	vector<char> res;
	string temp_str = "HTTP/" + to_string(req.version.first) + "." + to_string(req.version.second) +
						" " + to_string(req.status_code) + " " + req.reason_phrase + "\r\n";
	for (char c : temp_str) {
		res.push_back(c);
	}
	for (char c : to_vector(static_cast<const http_message&>(req), with_body)) {
		res.push_back(c);
	}
	return res;
}
std::string to_string(const http_response& req, bool with_body) {
	return to_string(to_vector(req, with_body));
}
