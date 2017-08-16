#include "set"

#include "http_reader.h"
#include "my_assert.h"
using namespace std;

#define SET_ON_R_COMP(f)\
	func_to_call_on_r_comp = [this](){f();};

#define GET_NEXT_CHAR()\
	if (readed_bytes > 0) {\
		next = *buff;\
		buff++;\
		readed_bytes--;\
	} else {\
		read_some();\
		return;\
	}

#define ERROR_CHECK(b, mess)\
	if (b) {\
		clear();\
		on_error(mess);\
		return;\
	}

void http_reader::read_client_request() {
	my_assert(!is_previous_request_completed(), "previous request not completed");
	my_assert(!*is_alive, "http_reader already closed\n");
	
	forming_client_req = make_unique<http_request>();
	
	to_call_on_read_main_part = [this]() {parse_request_main_patr();};
	readed_buff.clear();
	
	read_main_part();
}
void http_reader::read_server_response() {
	my_assert(!is_previous_request_completed(), "previous request not completed");
	my_assert(!*is_alive, "http_reader already closed\n");
	
	forming_server_resp = make_unique<http_response>();
	
	to_call_on_read_main_part = [this]() {parse_response_main_patr();};
	readed_buff.clear();
	
	read_main_part();
}

bool http_reader::is_previous_request_completed() {
	return ((forming_server_resp == nullptr) && (forming_client_req == nullptr));
}

void http_reader::close() {
	*is_alive = false;
}
http_reader::~http_reader() {
	close();
}

void http_reader::clear() {
	forming_client_req = nullptr;
	forming_server_resp = nullptr;
}

void http_reader::on_read_completion(const char* buff, size_t size) {
	LOG("in http_reader::on_read_completion: size == " << size << "\n");
//	for (int w = 0; w < size; w++) {
//		cout << buff[w];
//	}
//	cout << "###############################\n";
	this->buff = buff;
	this->readed_bytes = size;
	ERROR_CHECK(size == 0, READING_SHUTDOWNED_ERROR);
	func_to_call_on_r_comp();
}

void http_reader::on_read_server_messadge_body_completion() {
	unique_ptr<http_response> temp_ptr = move(forming_server_resp);
	server_callback(move(*temp_ptr));
}

void http_reader::read_main_part() {
	LOG("in http_reader::read_main_part\n");
	SET_ON_R_COMP(read_main_part);
	while (true) {
		int size = readed_buff.size();
		if ((size >= 4) && (readed_buff.substr(size - 4, 4) == "\r\n\r\n")) {
			break;
		}
		GET_NEXT_CHAR();
		readed_buff.push_back(next);
	}
	to_call_on_read_main_part();
}

void http_reader::parse_request_main_patr() {
	cout << "in http_reader::parse_request_main_patr\n";
	int pos = 0, size = readed_buff.size();
	while ((pos < size) && (readed_buff[pos] != ' ')) {
		forming_client_req->method.push_back(readed_buff[pos++]);
	}
	ERROR_CHECK(pos == size, SYNTAX_ERROR);
	pos++;
	while ((pos < size) && (readed_buff[pos] != ' ')) {
		forming_client_req->uri.push_back(readed_buff[pos++]);
	}
	ERROR_CHECK(pos == size, SYNTAX_ERROR);
	pos++;
	string version;
	while ((pos < size) && (!((readed_buff[pos - 1] == '\r') && (readed_buff[pos] == '\n')))) {
		version.push_back(readed_buff[pos++]);
	}
	ERROR_CHECK(pos == size, SYNTAX_ERROR);
	pos++;
	version.pop_back();
	
	ERROR_CHECK(version.size() != 8, SYNTAX_ERROR);
	int d_1 = version[5] - '0';
	int d_2 = version[7] - '0';
	ERROR_CHECK(!((0 <= min(d_1, d_2)) && (max(d_1, d_2) <= 9)), SYNTAX_ERROR);
	version[5] = version[7] = '*';
	ERROR_CHECK(version != "HTTP/*.*", SYNTAX_ERROR);
	
	forming_client_req->version = {d_1, d_2};
	
	int res = 0;
	while (res == 0) {
		res = read_header(pos);
	}
	ERROR_CHECK(res == 2, SYNTAX_ERROR);
	
	multimap<string, string> &headers = forming_client_req->headers;
	
	if (((headers.count("Content-Length") == 0) || ((*headers.find("Content-Length")).second == "0")) &&
		(headers.count("Transfer-Encoding") == 0)) {
				
		unique_ptr<http_request> temp_ptr = move(forming_client_req);
		client_callback(move(*temp_ptr));
		return;
	}
	assert(0);
}

void http_reader::parse_response_main_patr() {
	LOG("in parse_response_main_patr\n");
	
	int pos = 0, size = readed_buff.size();
	string version;
	while ((pos < size) && (readed_buff[pos] != ' ')) {
		version.push_back(readed_buff[pos++]);
	}
	ERROR_CHECK((pos == size) || (version.size() != 8), SYNTAX_ERROR);
	int d_1 = version[5] - '0';
	int d_2 = version[7] - '0';
	ERROR_CHECK(!((0 <= min(d_1, d_2)) && (max(d_1, d_2) <= 9)), SYNTAX_ERROR);
	version[5] = version[7] = '*';
	ERROR_CHECK(version != "HTTP/*.*", SYNTAX_ERROR);
	
	forming_server_resp->version = {d_1, d_2};
	
	string error;
	pos++;
	while ((pos < size) && (readed_buff[pos] != ' ')) {
		error.push_back(readed_buff[pos++]);
	}
	ERROR_CHECK(pos == size, SYNTAX_ERROR);
	pos++;
	int status_code;
	try {
		status_code = stoi(error);
	} catch (...) {
		ERROR_CHECK(true, SYNTAX_ERROR);
	}
	ERROR_CHECK(status_code >= 1000, SYNTAX_ERROR);
	forming_server_resp->status_code = status_code;
	
	string reason_phrase;
	while ((pos < size) && ((readed_buff[pos - 1] != '\r') ||
							(readed_buff[pos] != '\n'))) {
		reason_phrase.push_back(readed_buff[pos++]);
	}
	ERROR_CHECK(pos == size, SYNTAX_ERROR);
	reason_phrase.pop_back();
	pos++;
	
	forming_server_resp->reason_phrase = move(reason_phrase);
	
	int res = 0;
	while (res == 0) {
		res = read_header(pos);
	}
	ERROR_CHECK(res == 2, SYNTAX_ERROR);
	
	multimap<string, string> &headers = forming_server_resp->headers;
	
	if (headers.count("Content-Length") > 0) {
		string str_cont_size = (*headers.find("Content-Length")).second;
		int cont_size;
		try {
			cont_size = stoi(str_cont_size);
		} catch (...) {
			ERROR_CHECK(true, SYNTAX_ERROR);
		}
		remaining_content_length = cont_size;
		to_call_on_read_message_body_completion = [this]() {on_read_server_messadge_body_completion();};
		where_to_write_message_body = &*forming_server_resp;
		
		read_message_body_using_content_length();
		return;
	}
	
	if (headers.count("Transfer-Encoding") == 0) {
		unique_ptr<http_response> temp_ptr = move(forming_server_resp);
		server_callback(move(*temp_ptr));
		return;
	}
	assert(0);
}

void http_reader::read_message_body_using_content_length() {
	SET_ON_R_COMP(read_message_body_using_content_length);
	
	while (remaining_content_length > 0) {
		GET_NEXT_CHAR();
		where_to_write_message_body->message_body.push_back(next);
		remaining_content_length--;
	}
	to_call_on_read_message_body_completion();
}

int http_reader::read_header(int &pos) {
	string name, value;
	int size = readed_buff.size();
	if ((pos + 1 < size) && (readed_buff[pos] == '\r') && (readed_buff[pos + 1] == '\n')) {
		pos += 2;
		return 1;
	}
	while ((pos < size) && (readed_buff[pos] != ':')) {
		name.push_back(readed_buff[pos++]);
	}
	if (pos == size) {
		return 2;
	}
	pos++;
	while ((pos < size) && (readed_buff[pos] == ' ')) {
		pos++;
	}
	while ((pos < size) && ((readed_buff[pos - 1] != '\r') || (readed_buff[pos] != '\n'))) {
		value.push_back(readed_buff[pos++]);
	}
	if (pos == size) {
		return 2;
	}
	pos += 1;
	value.pop_back();
	
	if (forming_client_req != nullptr) {
		forming_client_req->headers.emplace(name, value);
	} else {
		forming_server_resp->headers.emplace(name, value);
	}
	return 0;
}
