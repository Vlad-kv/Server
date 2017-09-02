#include "http_message.h"
#include <iostream>
#include <cassert>
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

uri_authority::uri_authority(const std::string &str) {
	size_t pos = 0;
	string temp;
	while ((pos < str.size()) && (str[pos] != '@')
							  && (str[pos] != ':')) {
		temp += str[pos++];
	}
	if (str[pos] == '@') {
		userinfo = temp;
		pos++;
		
		while ((pos < str.size()) && (str[pos] != ':')) {
			host.push_back(str[pos++]);
		}
	} else {
		host = temp;
	}
	if (str[pos] == ':') {
		pos++;
		while (pos < str.size()) {
			if (!(('0' <= str[pos]) && (str[pos] <= '9'))) {
				throw runtime_error("invalid port");
			}
			int new_port = port * 10 + str[pos] - '0';
			if (new_port / 10 != port) {
				throw runtime_error("too big port number");
			}
			port = new_port;
			pos++;
		}
	}
}
uri_authority::uri_authority(uri_authority &&auth) {
	userinfo = auth.userinfo;
	host = auth.host;
	port = auth.port;
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
uri_authority http_request::extract_authority(const std::string& uri) {
	size_t pos = 0;
	while ((pos < uri.size()) && (uri[pos] != ':')) {
		pos++;
	}
	pos++;
	if ((pos + 2 >= uri.size()) || (uri.substr(pos, 2) != "//")) {
		return uri_authority("");
	}
	pos += 2;
	size_t start_pos = pos;
	while ((pos < uri.size()) && (uri[pos] != '/')
			&& (uri[pos] != '?') && (uri[pos] != '#')) {
		pos++;
	}
	return uri_authority(uri.substr(start_pos, pos - start_pos));
}
std::string http_request::extract_scheme(const std::string& uri) {
	size_t pos = 0;
	string res;
	while ((pos < uri.size()) && (uri[pos] != ':')) {
		char c = uri[pos];
		if (('A' <= c) && (c <= 'Z')) {
			c += 'a' - 'A';
		}
		res.push_back(c);
		pos++;
	}
	return res;
}
int http_request::extract_port_number() {
	int port = 80;
	if (extract_scheme(uri) == "https") {
		port = 443;
	}
	try {
		int res;
		if (method == "CONNECT") {
			res = uri_authority(uri).port;
		} else {
			res = extract_authority(uri).port;
		}
		if (res > 0) {
			port = res;
		}
		if (headers.count("Host") > 0) {
			uri_authority authority((*headers.find("Host")).second);
			if (authority.port > 0) {
				port = authority.port;
			}
		}
	} catch (...) {
		cout << "Error in extract_port_number!!!\n";
		assert(0);
	}
	return port;
}
std::string http_request::extract_host() {
	string host;
	if (headers.count("Host") > 0) {
		return uri_authority((*headers.find("Host")).second).host;
	}
	try {
		if (method == "CONNECT") {
			host = uri_authority(uri).host;
		} else {
			host = extract_authority(uri).host;
		}
	} catch (...) {
		cout << "Error in extract_host!!!\n";
		assert(0);
	}
	return host;
}
void http_request::convert_uri_to_origin_form() {
	if (uri.size() == 0) {
		uri = "/";
		return;
	}
	if ((uri[0] == '/') || (uri[0] == '*')) {
		return;
	}
	size_t pos = 0;
	while ((pos < uri.size()) && (uri[pos] != ':')) {
		pos++;
	}
	if (pos == uri.size()) {
		uri = "/";
		return;
	}
	pos++;
	if ((uri.size() - pos >= 2) && (uri.substr(pos, 2) == "//")) {
		pos += 2;
	}
	while ((pos < uri.size()) && (uri[pos] != '/')
			&& (uri[pos] != '?') && (uri[pos] != '#')) {
		pos++;
	}
	uri = uri.substr(pos, uri.size() - pos);
	if ((uri.size() == 0) || (uri[0] != '/')) {
		uri = "/" + uri;
	}
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
