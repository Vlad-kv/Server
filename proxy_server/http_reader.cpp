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
	
void http_reader::read_client_request(client_callback_t f, on_read_error_t err_f) {
	my_assert((forming_server_req) || (forming_client_req), "previous request not completed");
	
	forming_client_req = make_unique<client_http_request>();
	client_callback = move(f);
	on_error = move(err_f);
	
	to_call_on_read_main_part = [this](){parse_client_main_patr();};
	read_main_patr();
}
void http_reader::read_server_request(server_callback_t f, on_read_error_t err_f) {
	my_assert((forming_server_req) || (forming_client_req), "previous request not completed");
	
}

void http_reader::clear() {
	forming_client_req = nullptr;
	forming_server_req = nullptr;
	
	on_error = nullptr;
}

void http_reader::on_read_completion(const char* buff, size_t transmitted_bytes) {
	this->buff = buff;
	this->readed_bytes = transmitted_bytes;
	func_to_call_on_r_comp();
}

void http_reader::read_main_patr() {
	SET_ON_R_COMP(read_main_patr);
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

void http_reader::parse_client_main_patr() {
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
	ERROR_CHECK(version != "http/*.*", SYNTAX_ERROR);
	
	forming_client_req->version = {d_1, d_2};
	int res = 0;
	while (res == 0) {
		res = read_header(pos);
	}
	ERROR_CHECK(res == 2, SYNTAX_ERROR);
	
	if ((forming_client_req->headers.count("Content-Length") == 0) &&
		(forming_client_req->headers.count("Transfer-Encoding") == 0)) {
				
		unique_ptr<client_http_request> temp_ptr = move(forming_client_req);
		client_callback_t temp_c = move(client_callback);
		clear();
		temp_c(move(*temp_ptr));
		return;
	}
	assert(0);
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
	pos += 2;
	if (forming_client_req != nullptr) {
		forming_client_req->headers.emplace(name, value);
	} else {
		forming_server_req->headers.emplace(name, value);
	}
	return 0;
}
