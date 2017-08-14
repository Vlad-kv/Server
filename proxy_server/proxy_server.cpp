#include <tuple>

#include "proxy_server.h"
using namespace std;

queue_element::queue_element(client_http_request &&request)
: request(move(request)) {
}

proxy_server::client_data::client_data(proxy_server *this_server, client_socket_2 &&client_s)
: client(move(client_s)), server(AF_INET, SOCK_STREAM, 0) {
	client.set_on_disconnect([this_server, this]() {this_server->on_client_disconnect(*this);});
	
	client_reader = make_unique<http_reader>(&this->client,
		[this_server, this](client_http_request req) {this_server->on_client_request_reading_completion(*this, move(req));},
		[this_server, this](server_http_request req) {throw new runtime_error("server_http_request from client_reader");},
		[this_server, this](int error) {this_server->on_read_error_from_client(*this, error);}
	);
	client_writer = make_unique<http_writer>(&this->client,
		[this_server, this]() {this_server->on_server_request_writing_completion(*this);},
		[this_server, this]() {this_server->on_writing_shutdowning_from_client(*this);}
	);
	server.set_on_disconnect([this_server, this]() {this_server->on_server_disconnect(*this);});
	
	server_reader = make_unique<http_reader>(&this->server,
		[this_server, this](client_http_request req) {throw new runtime_error("client_http_request from server_reader");},
		[this_server, this](server_http_request req) {this_server->on_server_request_reading_completion(*this, move(req));},
		[this_server, this](int error) {this_server->on_read_error_from_server(*this, error);}
	);
	server_writer = make_unique<http_writer>(&this->client,
		[this_server, this]() {this_server->on_client_request_writing_completion(*this);},
		[this_server, this]() {this_server->on_writing_shutdowning_from_server(*this);}
	);
	this_server->registrate_socket(server);
}
proxy_server::client_data::client_data() {
}

proxy_server::proxy_server(std::string addres_of_main_socket, int port, IO_completion_port &comp_port)
: abstract_server(addres_of_main_socket, AF_INET, SOCK_STREAM, 0, port, comp_port),
  executer(comp_port, 3, 10, 2) {
}

int proxy_server::extract_port(const std::string& uri) {
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

void proxy_server::notify_client_about_error(client_data &data, int status_code, std::string reason_phrase) {
	if ((data.to_write_server_req_and_delete) || (data.to_delete)) {
		return;
	}
	LOG("in notify_client_about_error : " << status_code << " " << reason_phrase << "\n");
	data.client_reader->close();
	data.server_reader->close();
	data.server_writer->close();
	
	if (!data.client_writer->is_previous_request_completed()) {
		data.to_write_server_req_and_delete = true;
		while (!data.server_requests.empty()) {
			data.server_requests.pop();
		}
		data.server_requests.push(server_http_request({1, 1}, status_code, reason_phrase, {}, {}));
		return;
	}
	
	data.client_writer->write_request(
		server_http_request({1, 1}, status_code, reason_phrase, {}, {})
	);
	data.to_delete = true;
}

void proxy_server::on_accept(client_socket_2 client_s) {
	LOG("in proxy_server::on_accept");
	long long id = client_s.get_id();
	clients.emplace(piecewise_construct, tuple<long long>(id), tuple<proxy_server*, client_socket_2>(this, move(client_s)));
	
	clients[id].client_reader->read_client_request();
	
	accept();
}
void proxy_server::on_interruption() {
	LOG("proxy_server interrupted\n");
	is_interrupted = true;
	executer.interrupt();
	LOG("before clearing\n");
	clients.clear();
}

void proxy_server::on_client_request_reading_completion(client_data &data, client_http_request req) {
	LOG("request:\n");
	LOG(to_string(req));
	
	if ((req.version != pair<int, int>(1, 0)) && (req.version != pair<int, int>(1, 1))) {
		notify_client_about_error(data, 505, "HTTP Version Not Supported");
		return;
	}
	if (req.headers.count("Host") == 0) {
		notify_client_about_error(data, 400, "Bad Request");
		return;
	}
	
	{
		if (req.headers.count("Proxy-Connection") > 0) {
			req.headers.emplace("Connection", (*req.headers.find("Proxy-Connection")).second);
			req.headers.erase(req.headers.find("Proxy-Connection"));
		}
	}
	
	string &str = (*req.headers.find("Host")).second;
	string host, port;
	size_t pos = 0;
	while ((pos < str.size()) && (str[pos] != ':')) {
		host.push_back(str[pos++]);
	}
	if (pos != str.size()) {
		pos++;
		while (pos < str.size()) {
			port.push_back(str[pos++]);
		}
	}
	
	int int_port = extract_port(req.uri);
	try {
		int_port = max(int_port, stoi(port));
	} catch (...) {
	}
	if (int_port == 0) {
		int_port = 80;
	}
	LOG("int_port " << int_port << "\n");
	
	addrinfo hints;
	SecureZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	
	shared_ptr<queue_element> req_ptr = make_shared<queue_element>(move(req));
	
	executer.execute(data.client.get_id(), host, port, hints,
		[&data, this, req_ptr, int_port](addrinfo *info) {getaddrinfo_callback(data, info, req_ptr, int_port);}
	);
	data.client_requests.push(req_ptr);
}
void proxy_server::on_read_error_from_client(client_data &data, int error) {
	LOG("in proxy_server::on_read_error_from_client\n");
	notify_client_about_error(data, 400, "Bad Request");
}

void proxy_server::on_server_request_writing_completion(client_data &data) {
	LOG("in on_server_request_writing_completion\n");
	if (data.to_delete) {
		delete_client_data(data);
		return;
	}
	if (data.to_write_server_req_and_delete) {
		data.to_delete = true;
		data.client_writer->write_request(data.server_requests.front());
		data.server_requests.pop();
		return;
	}
	if (!data.server_requests.empty()) {
		data.client_writer->write_request(data.server_requests.front());
		data.server_requests.pop();
	}
}
void proxy_server::on_writing_shutdowning_from_client(client_data &data) {
	LOG("in on_writing_shutdowning_from_client\n");
	delete_client_data(data);
}

void proxy_server::on_server_request_reading_completion(client_data &data, server_http_request req) {
	LOG("in on_server_request_reading_completion\n");
	
	notify_client_about_error(data, 503, "Service Unavailable");
	
	// TODO
}
void proxy_server::on_read_error_from_server(client_data &data, int error) {
	LOG("on_read_error_from_server\n");
	
	if (error == http_reader::SYNTAX_ERROR) {
		notify_client_about_error(data, 500, "Internal Server Error");
		return;
	}
	if (error == http_reader::READING_SHUTDOWNED_ERROR) {
		notify_client_about_error(data, 503, "Service Unavailable");
		return;
	}
	assert(0); // unknown error code
}

void proxy_server::on_client_request_writing_completion(client_data &data) {
	LOG("in on_client_request_writing_completion\n");
	if (data.server_reader->is_previous_request_completed()) {
		data.server_reader->read_client_request();
	}
	if ((data.server_writer->is_previous_request_completed()) &&
		(!data.client_requests.empty()) &&
		(data.client_requests.front()->is_getaddrinfo_completed)) {
		
		data.server_writer->write_request(data.client_requests.front()->request);
		data.client_requests.pop();
	}
}
void proxy_server::on_writing_shutdowning_from_server(client_data &data) {
	LOG("in on_writing_shutdowning_from_server\n");
	
	notify_client_about_error(data, 503, "Service Unavailable");
}

void proxy_server::on_client_disconnect(client_data &client_d) {
	LOG("in on_client_disconnect\n");
	delete_client_data(client_d);
}
void proxy_server::on_server_disconnect(client_data &client_d) {
	LOG("in on_server_disconnect\n");
	if (is_interrupted) {
		delete_client_data(client_d);
	} else {
		notify_client_about_error(client_d, 503, "Service Unavailable");
	}
}

void proxy_server::getaddrinfo_callback(client_data &data, addrinfo *info, std::shared_ptr<queue_element> req, int port) {
	LOG("in getaddrinfo_callback\n");
	if ((data.to_write_server_req_and_delete) || (data.to_delete)) {
		return;
	}
	if (info == nullptr) {
		notify_client_about_error(data, 503, "Service Unavailable");
		return;
	}
	
	LOG(inet_ntoa(((sockaddr_in*)info->ai_addr)->sin_addr) << " " << port << "\n");
	
	try {
		// TODO сделать ассинхронно
		data.server.connect(info->ai_family, ((sockaddr_in*)info->ai_addr)->sin_addr.s_addr, port);
	} catch (...) {
		notify_client_about_error(data, 503, "Service Unavailable");
		return;
	}
	req->is_getaddrinfo_completed = true;
	
	if ((data.server_writer->is_previous_request_completed()) &&
		(data.client_requests.front()->is_getaddrinfo_completed)) {
		
		LOG("writing to server\n");
		
		data.server_writer->write_request(data.client_requests.front()->request);
		data.client_requests.pop();
	}
}

void proxy_server::delete_client_data(client_data &client_d) {
	if (!is_interrupted) {
		long long id = client_d.client.get_id();
		if (keys_in_process_of_deletion.count(id) != 0) {
			return;
		}
		keys_in_process_of_deletion.insert(id);
		LOG("in proxy_server::delete_client_data " << id << "\n");
		
		executer.delete_group(id);
		clients.erase(id);
		keys_in_process_of_deletion.erase(id);
	}
}
