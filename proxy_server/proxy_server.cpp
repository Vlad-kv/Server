#include <tuple>

#include "proxy_server.h"
using namespace std;

proxy_server::client_data::client_data(proxy_server *this_server, client_socket_2 &&client_s)
: client(move(client_s)), server(AF_INET, SOCK_STREAM, 0) {
	t = timer(TIMEOUT, [this_server, this](timer *t) {this_server->on_timer_expiration(*this);});
	this_server->registrate_timer(t);
	
	client.set_on_disconnect([this_server, this]() {this_server->on_client_disconnect(*this);});
	
	client_reader = make_unique<http_reader>(&this->client,
		[this_server, this](http_request req) {this_server->on_client_request_reading_completion(*this, move(req));},
		[this_server, this](http_response req) {throw new runtime_error("server_http_request from client_reader");},
		[this_server, this](int error) {this_server->on_read_error_from_client(*this, error);},
		[this_server, this]() {this_server->on_event_from_socket(*this);}
	);
	client_writer = make_unique<http_writer>(&this->client,
		[this_server, this]() {this_server->on_server_response_writing_completion(*this);},
		[this_server, this]() {this_server->on_writing_shutdowning_from_client(*this);},
		[this_server, this]() {this_server->on_event_from_socket(*this);}
	);
	init_server_part(this_server);
}
proxy_server::client_data::client_data() {
}
void proxy_server::client_data::init_server_part(proxy_server *this_server) {
	server.set_on_connect([this_server, this]() {this_server->on_connect_completion(*this);});
	server.set_on_disconnect([this_server, this]() {this_server->on_server_disconnect(*this);});
	
	server_reader = make_unique<http_reader>(&this->server,
		[this_server, this](http_request req) {throw new runtime_error("client_http_request from server_reader");},
		[this_server, this](http_response req) {this_server->on_server_response_reading_completion(*this, move(req));},
		[this_server, this](int error) {
			if (!is_server_now_reseting) {
				this_server->on_read_error_from_server(*this, error);
			}
		},
		[this_server, this]() {this_server->on_event_from_socket(*this);}
	);
	server_writer = make_unique<http_writer>(&this->server,
		[this_server, this]() {this_server->on_client_request_writing_completion(*this);},
		[this_server, this]() {
			if (!is_server_now_reseting) {
				this_server->on_writing_shutdowning_from_server(*this);
			}
		},
		[this_server, this]() {this_server->on_event_from_socket(*this);}
	);
	this_server->registrate_socket(server);
}
void proxy_server::client_data::reset_server(proxy_server *this_server) {
	is_server_now_reseting = true;
	
	server_reader = nullptr;
	server_writer = nullptr;
	server.set_on_disconnect([]() {});
	server.close();
	
	server = client_socket(AF_INET, SOCK_STREAM, 0);
	init_server_part(this_server);
	
	is_server_now_reseting = false;
}

proxy_server::proxy_server(std::string addres_of_main_socket, int port, IO_completion_port &comp_port)
: abstract_server(addres_of_main_socket, AF_INET, SOCK_STREAM, 0, port, comp_port),
  executer(comp_port, 3, 10, 2) {
}

void proxy_server::write_to_server(client_data &data) {
	if ((data.server_writer->is_previous_request_completed()) &&
//		(!data.client_requests.empty()) &&
		(data.is_previous_connect_completed)) {
		
		if (data.client_requests.empty()) {
			if (data.is_reading_from_client_shutdowned) {
				if (!data.to_delete_on_empty_server_responses) {
					if (!data.server.is_connected()) {
						delete_client_data(data);
						return;
					}
					data.server.shutdown_writing();
					data.to_delete_on_empty_server_responses = true;
				}
			}
			return;
		}
		
		http_request &req = data.client_requests.front();
		string host = req.extract_host();
		
		if (host == data.current_host) {
			if (req.method == "CONNECT") {
				data.tunnel_creating = true;
				if (data.num_of_responses_to_read_from_server > 0) {
					return;
				}
				data.client_requests.pop();
				data.server_responses.push(http_response(
					{1, 1}, 200, "Connection established", {}, {}
				));
				write_to_client(data);
				return;
			}
//			LOG("writing to server:\n");
//			LOG(to_string(req, false) << "\n###############\n");
//			
			req.convert_uri_to_origin_form();
			
			data.server_reader->add_method_that_was_sended_to_server(req.method);
			data.server_writer->write_request(req);
			
			data.num_of_responses_to_read_from_server++;
			
			data.client_requests.pop();
			return;
		}
		if (data.num_of_responses_to_read_from_server > 0) {
			return;
		}
		
		data.is_previous_connect_completed = false;
		data.reset_server(this);
		
		LOG("prev host : " << data.current_host << "\n");
		
		data.current_host = host;
		
		int port = req.extract_port_number();
		LOG("host : " << host << " port : " << port << "\n");
		
		addrinfo hints;
		SecureZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		
		executer.execute(data.client.get_id(), host, to_string(port), hints,
			[&data, this, port](addrinfo *info) {getaddrinfo_callback(data, info, port);},
			[&data, this]() {on_error_in_getaddrinfo(data);}
		);
	}
}
void proxy_server::write_to_client(client_data &data) {
	if ((!data.server_responses.empty()) &&
		(data.client_writer->is_previous_request_completed())) {
		
		data.client_writer->write_request(data.server_responses.front());
		data.server_responses.pop();
	}
}

void proxy_server::notify_client_about_error(client_data &data, int status_code, std::string reason_phrase) {
	if ((data.to_write_server_req_and_delete) || (data.to_delete)) {
		return;
	}
	LOG("in notify_client_about_error : " << status_code << " " << reason_phrase << "\n");
	if (data.client_reader != nullptr) {
		data.client_reader->close();
	}
	if (data.server_reader != nullptr) {
		data.server_reader->close();
	}
	if (data.server_writer != nullptr) {
		data.server_writer->close();
	}
	http_response resp({1, 1}, status_code, reason_phrase, {
			{"Connection", "Closed"},
			{"Content-length", "0"}}, {}
	);
	if (!data.client_writer->is_previous_request_completed()) {
		data.to_write_server_req_and_delete = true;
		while (!data.server_responses.empty()) {
			data.server_responses.pop();
		}
		data.server_responses.push(move(resp));
		return;
	}
	
	data.client_writer->write_request(move(resp));
	data.to_delete = true;
}

void proxy_server::on_accept(client_socket_2 client_s) {
	LOG("in proxy_server::on_accept\n");
	long long id = client_s.get_id();
	clients.emplace(piecewise_construct, tuple<long long>(id), tuple<proxy_server*, client_socket_2>(this, move(client_s)));
	
	clients[id].client_reader->read_client_request();
	accept();
}
void proxy_server::on_interruption() {
	LOG("proxy_server interrupted\n");
	is_interrupted = true;
	executer.interrupt();
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
	
//	LOG("res request:\n" << to_string(req) << "\n");
	
	string method = req.method;
	
	data.client_requests.push(move(req));
	write_to_server(data);
	if (method != "CONNECT") {
		data.client_reader->read_client_request();
	}
}
void proxy_server::on_read_error_from_client(client_data &data, int error) {
	if (error == http_reader::READING_SHUTDOWNED_ERROR) {
		LOG("in proxy_server::on_read_error_from_client : READING_SHUTDOWNED_ERROR\n");
		data.is_reading_from_client_shutdowned = true;
		write_to_server(data);
		return;
	}
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
	if ((data.num_of_responses_to_read_from_server == 0) && (data.client_reader->is_it_shutdowned())) {
		delete_client_data(data);
		return;
	}
	if (!data.server_responses.empty()) {
		data.client_writer->write_request(data.server_responses.front());
		data.server_responses.pop();
	}
	if (data.tunnel_creating) {
		establish_tunnel(data);
	}
}
void proxy_server::on_writing_shutdowning_from_client(client_data &data) {
	LOG("in on_writing_shutdowning_from_client\n");
	delete_client_data(data);
}

void proxy_server::on_server_response_reading_completion(client_data &data, http_response resp) {
	LOG("in on_server_response_reading_completion:\n");
	LOG(to_string(resp, false) << "\n################\n");
	
	data.num_of_responses_to_read_from_server--;
	write_to_server(data);
	
	data.server_responses.push(move(resp));
	write_to_client(data);
	
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
		LOG("READING_SHUTDOWNED_ERROR\n");
		if (data.server_reader->is_last_response_available()) {
			on_server_response_reading_completion(data,
						move(data.server_reader->get_last_response()));
			data.to_delete_on_empty_server_responses = true;
		} else {
			if ((data.server_reader->is_no_bytes_where_readed_after_last_completed_message()) && 
				(data.num_of_responses_to_read_from_server == 0)) {
				
				data.to_delete_on_empty_server_responses = true;
			} else {
				notify_client_about_error(data, 502, "Bad Gateway");
			}
		}
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

void proxy_server::getaddrinfo_callback(client_data &data, addrinfo *info, int port) {
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
		data.server.connect(info->ai_family, ((sockaddr_in*)info->ai_addr)->sin_addr.s_addr, port);
	} catch (...) {
		notify_client_about_error(data, 503, "Service Unavailable");
		return;
	}
}
void proxy_server::on_error_in_getaddrinfo(client_data &data) {
	cout << "in on_error_in_getaddrinfo\n";
	LOG("in on_error_in_getaddrinfo\n");
	notify_client_about_error(data, 502, "Bad Gateway");
}
void proxy_server::on_connect_completion(client_data &data) {
	data.is_previous_connect_completed = true;
	write_to_server(data);
}

void proxy_server::on_event_from_socket(client_data &data) {
	data.t.restart();
}
void proxy_server::on_timer_expiration(client_data &data) {
	if (!data.is_timer_expired_at_past) {
		cout << "timer expired\n";
		data.is_timer_expired_at_past = true;
		data.t.restart();
		if (data.client_writer == nullptr) { 
			cout << "              expired in tunnel\n";
			delete_client_data(data);
			return;
		}
		notify_client_about_error(data, 504, "Gateway Timeout");
	} else {
		cout << "timer expited at second time\n";
		delete_client_data(data);
	}
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
		data.client.set_on_disconnect(nullptr);
		data.server.set_on_disconnect(nullptr);
		
		clients.erase(id);
		keys_in_process_of_deletion.erase(id);
	}
}

void proxy_server::establish_tunnel(client_data &data) {
	pair<const char*, size_t> extra_data = data.client_reader->get_extra_readed_data();
	
	data.client_reader = nullptr;
	data.client_writer = nullptr;
	data.server_reader = nullptr;
	data.server_writer = nullptr;
	
	data.client.set_on_disconnect([this, &data]() {this->delete_client_data(data);});
	data.client.set_on_read_completion(
		[&data](const char* buff, size_t size) {
			data.t.restart();
			if (size > 0) {
				data.server.write_some(buff, size);
			} else {
				data.server.shutdown_writing();
			}
		}
	);
	data.client.set_on_write_completion(
		[&data](size_t saved_bytes, size_t transmitted_bytes) {
			data.t.restart();
			if (transmitted_bytes == 0) {
				data.client.execute_on_disconnect();
				return;
			}
			if (saved_bytes > 0) {
				data.server.write_some_saved_bytes();
			} else {
				data.server.read_some();
			}
		}
	);
	
	data.server.set_on_disconnect([this, &data]() {this->delete_client_data(data);});
	data.server.set_on_read_completion(
		[&data](const char* buff, size_t size) {
			data.t.restart();
			if (size == 0) {
				data.client.execute_on_disconnect();
				return;
			}
			data.client.write_some(buff, size);
		}
	);
	data.server.set_on_write_completion(
		[&data](size_t saved_bytes, size_t transmitted_bytes) {
			data.t.restart();
			if (transmitted_bytes == 0) {
				data.client.shutdown_reading();
				return;
			}
			if (saved_bytes > 0) {
				data.server.write_some_saved_bytes();
			} else {
				data.client.read_some();
			}
		}
	);
	
	if (extra_data.second > 0) {
		data.server.write_some(extra_data.first, extra_data.second);
	} else {
		data.client.read_some();
	}
	data.server.read_some();
}
