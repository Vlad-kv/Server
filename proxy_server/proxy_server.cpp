#include <tuple>

#include "proxy_server.h"
using namespace std;

queue_element::queue_element(http_request &&request)
: request(move(request)) {
}

proxy_server::client_data::client_data(proxy_server *this_server, client_socket_2 &&client_s)
: client(move(client_s)), server(AF_INET, SOCK_STREAM, 0) {
	client.set_on_connect([this_server, this]() {this_server->on_connect_completion();});
	client.set_on_disconnect([this_server, this]() {this_server->on_client_disconnect(*this);});
	
	client_reader = make_unique<http_reader>(&this->client,
		[this_server, this](http_request req) {this_server->on_client_request_reading_completion(*this, move(req));},
		[this_server, this](http_response req) {throw new runtime_error("server_http_request from client_reader");},
		[this_server, this](int error) {this_server->on_read_error_from_client(*this, error);}
	);
	client_writer = make_unique<http_writer>(&this->client,
		[this_server, this]() {this_server->on_server_response_writing_completion(*this);},
		[this_server, this]() {this_server->on_writing_shutdowning_from_client(*this);}
	);
	server.set_on_disconnect([this_server, this]() {this_server->on_server_disconnect(*this);});
	
	server_reader = make_unique<http_reader>(&this->server,
		[this_server, this](http_request req) {throw new runtime_error("client_http_request from server_reader");},
		[this_server, this](http_response req) {this_server->on_server_response_reading_completion(*this, move(req));},
		[this_server, this](int error) {this_server->on_read_error_from_server(*this, error);}
	);
	server_writer = make_unique<http_writer>(&this->server,
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

void proxy_server::write_to_server(client_data &data) {
	if ((data.server_writer->is_previous_request_completed()) &&
		(!data.client_requests.empty()) &&
		(data.client_requests.front()->is_getaddrinfo_completed)) {
		
		http_request &req = data.client_requests.front()->request;
		
		LOG("writing to server:\n");
		LOG(to_string(req, false) << "\n###############\n");
		
		data.server_reader->add_method_that_was_sended_to_server(req.method);
		
		data.server_writer->write_request(req);
		data.client_requests.pop();
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
		while (!data.server_responses.empty()) {
			data.server_responses.pop();
		}
		data.server_responses.push(http_response({1, 1}, status_code, reason_phrase, {}, {}));
		return;
	}
	
	data.client_writer->write_request(
		http_response({1, 1}, status_code, reason_phrase, {}, {})
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

void proxy_server::on_client_request_reading_completion(client_data &data, http_request req) {
	LOG("request:\n");
	LOG(to_string(req, false));
	
	if ((req.version != pair<int, int>(1, 0)) && (req.version != pair<int, int>(1, 1))) {
		notify_client_about_error(data, 505, "HTTP Version Not Supported");
		return;
	}
	string host = req.extract_host();
	if (host == "") {
		notify_client_about_error(data, 400, "Bad Request");
		return;
	}
	if (req.headers.count("Proxy-Connection") > 0) {
		req.headers.emplace("Connection", (*req.headers.find("Proxy-Connection")).second);
		req.headers.erase(req.headers.find("Proxy-Connection"));
	}
	int port = req.extract_port_number();
	LOG("host : " << host << " port : " << port << "\n");
	
	addrinfo hints;
	SecureZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	
	shared_ptr<queue_element> req_ptr = make_shared<queue_element>(move(req));
	
	data.client_requests.push(req_ptr);
	executer.execute(data.client.get_id(), host, to_string(port), hints,
		[&data, this, req_ptr, port](addrinfo *info) {getaddrinfo_callback(data, info, req_ptr, port);}
	);
	data.client_reader->read_client_request();
}
void proxy_server::on_read_error_from_client(client_data &data, int error) {
	LOG("in proxy_server::on_read_error_from_client\n");
	notify_client_about_error(data, 400, "Bad Request");
}

void proxy_server::on_server_response_writing_completion(client_data &data) {
	LOG("in on_server_response_writing_completion\n");
	if (data.to_delete) {
		delete_client_data(data);
		return;
	}
	if ((data.to_delete_on_empty_server_responses) && (data.server_responses.empty())) {
		delete_client_data(data);
		return;
	}
	if (data.to_write_server_req_and_delete) {
		data.to_delete = true;
		data.client_writer->write_request(data.server_responses.front());
		data.server_responses.pop();
		return;
	}
	if (!data.server_responses.empty()) {
		data.client_writer->write_request(data.server_responses.front());
		data.server_responses.pop();
	}
}
void proxy_server::on_writing_shutdowning_from_client(client_data &data) {
	LOG("in on_writing_shutdowning_from_client\n");
	delete_client_data(data);
}

void proxy_server::on_server_response_reading_completion(client_data &data, http_response resp) {
	LOG("in on_server_response_reading_completion:\n");
	LOG(to_string(resp, false) << "\n################\n");
	
	data.server_responses.push(move(resp));
	if (data.client_writer->is_previous_request_completed()) {
		data.client_writer->write_request(data.server_responses.front());
		data.server_responses.pop();
	}
	data.server_reader->read_server_response();
}
void proxy_server::on_read_error_from_server(client_data &data, int error) {
	LOG("on_read_error_from_server\n");
	
	if (error == http_reader::SYNTAX_ERROR) {
		LOG("SYNTAX_ERROR\n");
		notify_client_about_error(data, 502, "Bad Gateway");
		return;
	}
	if (error == http_reader::READING_SHUTDOWNED_ERROR) {
		LOG("READING_SHUTDOWNED_ERROR");
		if (data.server_reader->is_last_response_available()) {
			on_server_response_reading_completion(data,
						move(data.server_reader->get_last_response()));
			data.to_delete_on_empty_server_responses = true;
			return;
		}
		notify_client_about_error(data, 502, "Bad Gateway");
		return;
	}
	assert(0); // unknown error code
}

void proxy_server::on_client_request_writing_completion(client_data &data) {
	LOG("in on_client_request_writing_completion\n");
	if (data.server_reader->is_previous_request_completed()) {
		data.server_reader->read_server_response();
	}
	write_to_server(data);
}
void proxy_server::on_writing_shutdowning_from_server(client_data &data) {
	LOG("in on_writing_shutdowning_from_server\n");
	
	notify_client_about_error(data, 503, "Service Unavailable");
}

void proxy_server::on_client_disconnect(client_data &data) {
	LOG("in on_client_disconnect\n");
	delete_client_data(data);
}
void proxy_server::on_server_disconnect(client_data &data) {
	LOG("in on_server_disconnect\n");
	if (is_interrupted) {
		delete_client_data(data);
	} else {
		if (data.server_reader->is_last_response_available()) {
			on_server_response_reading_completion(data,
						move(data.server_reader->get_last_response()));
			data.to_delete_on_empty_server_responses = true;
			return;
		}
		notify_client_about_error(data, 503, "Service Unavailable");
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
	
	write_to_server(data);
}
void proxy_server::on_connect_completion(client_data &data, std::shared_ptr<queue_element> req) {
	
}

void proxy_server::delete_client_data(client_data &data) {
	if (!is_interrupted) {
		long long id = data.client.get_id();
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
