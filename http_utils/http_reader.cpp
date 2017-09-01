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
		if (mess == READING_SHUTDOWNED_ERROR) {\
			is_reading_shutdowned = true;\
		}\
		on_error(mess);\
		return;\
	}

namespace {
	const set<string> not_allowed_fields_in_trailer({
		"Transfer-Encoding", "Content-Length",
		"Host",
		// controls
		"Cache-Control", "Expect", "Host", "Max-Forwards", "Pragma",
		"Range", "TE",
		// Conditionals
		"If-Match", "If-None-Match", "If-Modified-Since",
		"If-Unmodified-Since", "If-Range",
		// authentication 
		"Authorization", "Proxy-Authenticate", "Proxy-Authorization",
		"WWW-Authenticate",
		// ... TODO
		"Age", "Cache-Control", "Expires", "Date",
		"Location", "Retry-After", "Vary", "Warning",
		
		"Content-Encoding", "Content-Type", "Content-Range", "Trailer"
	});
}

void http_reader::read_client_request() {
	my_assert(!is_previous_request_completed(), "previous request not completed");
	my_assert(!*is_alive, "http_reader already closed\n");
	my_assert(is_reading_shutdowned, "http_reader already shutdowned");
	
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
void http_reader::add_method_that_was_sended_to_server(std::string method) {
	methods.push(method);
}
bool http_reader::is_previous_request_completed() {
	return ((forming_server_resp == nullptr) && (forming_client_req == nullptr));
}
bool http_reader::is_it_shutdowned() {
	return is_reading_shutdowned;
}

bool http_reader::is_last_response_available() {
	return reading_until_not_closing;
}
http_response http_reader::get_last_response() {
	if (!is_last_response_available()) {
		throw new runtime_error("last message not available");
	}
	unique_ptr<http_response> temp = move(forming_server_resp);
	
	reading_until_not_closing = false;
	return move(*temp);
}

std::pair<const char*, size_t> http_reader::get_extra_readed_data() {
	return {buff, readed_bytes};
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
//	for (size_t w = 0; w < size; w++) {
//		LOG(buff[w]);
//	}
//	LOG("###############################\n");
	this->buff = buff;
	this->readed_bytes = size;
	
	ERROR_CHECK(size == 0, READING_SHUTDOWNED_ERROR);
	func_to_call_on_r_comp();
}

void http_reader::on_read_response_messadge_body_completion() {
	unique_ptr<http_response> temp_ptr = move(forming_server_resp);
	server_callback(move(*temp_ptr));
}
void http_reader::on_read_request_messadge_body_completion() {
	unique_ptr<http_request> temp_ptr = move(forming_client_req);
	client_callback(move(*temp_ptr));
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
	LOG("readed_buff :\n" << readed_buff << " #########\n\n");
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
	
	while (true) {
		pair<string, string> res;
		try {
			res = read_header(pos);
		} catch (...) {
			ERROR_CHECK(true, SYNTAX_ERROR);
		}
		if (res.first == "") {
			break;
		}
		forming_client_req->headers.insert(res);
	}
	ERROR_CHECK(pos != size, SYNTAX_ERROR);
	
	to_call_on_read_message_body_completion = [this]() {on_read_request_messadge_body_completion();};
	read_message_body(*forming_client_req);
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
	
	while (true) {
		pair<string, string> res;
		try {
			res = read_header(pos);
		} catch (...) {
			ERROR_CHECK(true, SYNTAX_ERROR);
		}
		if (res.first == "") {
			break;
		}
		forming_server_resp->headers.insert(res);
	}
	ERROR_CHECK(pos != size, SYNTAX_ERROR);
	
	to_call_on_read_message_body_completion = [this]() {on_read_response_messadge_body_completion();};
	read_message_body(*forming_server_resp);
}

void http_reader::read_message_body(http_message &mess) {
	multimap<string, string> &headers = mess.headers;
	string prev_method;
	if (!methods.empty()) {
		prev_method = move(methods.front());
		methods.pop();
	}
	if (forming_client_req != nullptr) {
		where_to_write_message_body = &*forming_client_req;
	} else {
		where_to_write_message_body = &*forming_server_resp;
	}
	
	if (forming_server_resp != nullptr) {
		int code = forming_server_resp->status_code;
		// 1
		if ((prev_method == "HEAD") || (code / 100 == 1) || (code == 204) || (code == 304)) {
			to_call_on_read_message_body_completion();
			return;
		}
		// 2
		if ((prev_method == "CONNECT") && (code / 100 == 2)) {
			to_call_on_read_message_body_completion();
			return;
		}
	}
	// 3
	if (headers.count("Transfer-Encoding") > 0) {
		ERROR_CHECK(headers.count("Transfer-Encoding") != 1, SYNTAX_ERROR);
		string codings = (*headers.find("Transfer-Encoding")).second;
		ERROR_CHECK(codings.size() < 7, SYNTAX_ERROR);
		ERROR_CHECK(codings.substr(codings.size() - 7, 7) != "chunked", SYNTAX_ERROR);
		if ((codings.size() > 7) && (codings[codings.size()])) {
			char c = codings[codings.size() - 8];
			ERROR_CHECK((c != ' ') && (c != ','), SYNTAX_ERROR);
		}
		
		codings.erase(codings.size() - 7, 7);
		int pos = codings.size() - 1;
		while ((pos >= 0) && (codings[pos] == ' ')) {
			pos--;
		}
		if ((pos >= 0) && (codings[pos] == ',')) {
			pos--;
		}
		while ((pos >= 0) && (codings[pos] == ' ')) {
			pos--;
		}
		pos++;
		codings.erase(pos, codings.size() - pos);
		
		headers.erase("Transfer-Encoding");
		if (!codings.empty()) {
			headers.emplace("Transfer-Encoding", move(codings));
		}
		
		headers.erase("Content-Length");
		read_message_body_using_chunked_encoding();
		return;
	}
	// 4-5
	if (headers.count("Content-Length") > 0) {
		string str_cont_size = (*headers.find("Content-Length")).second;
		
		auto range = headers.equal_range("Content-Length");
		for (auto w = range.first; w != range.second; ++w) {
			ERROR_CHECK((*w).second != str_cont_size, SYNTAX_ERROR);
		}
		int cont_size;
		try {
			cont_size = stoi(str_cont_size);
		} catch (...) {
			ERROR_CHECK(true, SYNTAX_ERROR);
		}
		remaining_length = cont_size;
		
		read_message_body_using_content_length();
		return;
	}
	// 6
	if (forming_client_req != nullptr) {
		to_call_on_read_message_body_completion();
		return;
	}
	// 7
	read_message_body_until_connection_not_closed();
}
void http_reader::read_message_body_using_content_length() {
	SET_ON_R_COMP(read_message_body_using_content_length);
	
	while (remaining_length > 0) {
		GET_NEXT_CHAR();
		where_to_write_message_body->message_body.push_back(next);
		remaining_length--;
	}
	to_call_on_read_message_body_completion();
}
void http_reader::read_message_body_using_chunked_encoding() {
	LOG("in read_message_body_using_chunked_encoding\n");
	read_chunck();
}

void http_reader::read_chunck() {
	read_line([this]() {read_chunk_size();});
}
void http_reader::read_chunk_size() {
	size_t pos = 0;
	size_t chunck_size = 0;
	bool is_empty = true;
	
	while ((('0' <= readed_buff[pos]) && (readed_buff[pos] <= '9')) ||
		   (('a' <= readed_buff[pos]) && (readed_buff[pos] <= 'f'))) {
		is_empty = false;
		size_t digit;
		if (('0' <= readed_buff[pos]) && (readed_buff[pos] <= '9')) {
			digit = readed_buff[pos] - '0';
		} else {
			digit = 10 + readed_buff[pos] - 'a';
		}
		ERROR_CHECK((chunck_size * 16) / 16 != chunck_size, SYNTAX_ERROR);
		chunck_size = chunck_size * 16 + digit;
		pos++;
	}
	ERROR_CHECK(is_empty, SYNTAX_ERROR);
	
	if (chunck_size == 0) {
		read_trailer();
		return;
	}
	remaining_length = chunck_size + 2;
	read_chunk_data();
}
void http_reader::read_chunk_data() {
	SET_ON_R_COMP(read_chunk_data);
	vector<char> &message = where_to_write_message_body->message_body;
	while (remaining_length > 0) {
		GET_NEXT_CHAR();
		message.push_back(next);
		remaining_length--;
	}
	
	ERROR_CHECK((message[message.size() - 2] != '\r') ||
				(message[message.size() - 1] != '\n'), SYNTAX_ERROR);
	message.pop_back();
	message.pop_back();
	read_chunck();
}
void http_reader::read_trailer() {
	read_line([this]() {
		pair<string, string> res;
		int pos = 0;
		try {
			res = read_header(pos);
		} catch (...) {
			ERROR_CHECK(true, SYNTAX_ERROR);
		}
		if (res.first == "") {
			finish_reading_trailer();
			return;
		}
		if (not_allowed_fields_in_trailer.count(res.first) == 0) {
			if (forming_client_req != nullptr) {
				forming_client_req->headers.insert(res);
			} else {
				forming_server_resp->headers.insert(res);
			}
		}
		read_trailer();
	});
}
void http_reader::finish_reading_trailer() {
	where_to_write_message_body->headers.erase("Trailer");
	where_to_write_message_body->headers.emplace("Content-Length",
		to_string(where_to_write_message_body->message_body.size()));
	
	to_call_on_read_message_body_completion();
}

void http_reader::read_message_body_until_connection_not_closed() {
	SET_ON_R_COMP(read_message_body_until_connection_not_closed);
	reading_until_not_closing = true;
	while (true) {
		GET_NEXT_CHAR();
		where_to_write_message_body->message_body.push_back(next);
	}
}

std::pair<std::string, std::string> http_reader::read_header(int &pos) {
	string name, value;
	int size = readed_buff.size();
	if ((pos + 1 < size) && (readed_buff[pos] == '\r') && (readed_buff[pos + 1] == '\n')) {
		pos += 2;
		return {"", ""};
	}
	while ((pos < size) && (readed_buff[pos] != ':')) {
		name.push_back(readed_buff[pos++]);
	}
	if ((pos == size) || (name.size() == 0)) {
		throw new runtime_error("");
	}
	pos++;
	while ((pos < size) && (readed_buff[pos] == ' ')) {
		pos++;
	}
	while ((pos < size) && ((readed_buff[pos - 1] != '\r') || (readed_buff[pos] != '\n'))) {
		value.push_back(readed_buff[pos++]);
	}
	if (pos == size) {
		throw new runtime_error("");
	}
	pos += 1;
	value.pop_back();
	return {name, value};
}
void http_reader::read_line(func_t to_call) {
	to_call_after_reading_line = to_call;
	readed_buff.clear();
	read_line_cycle();
}
void http_reader::read_line_cycle() {
	SET_ON_R_COMP(read_line_cycle);
	while (true) {
		int size = readed_buff.size();
		if ((size >= 2) && (readed_buff.substr(size - 2, 2) == "\r\n")) {
			break;
		}
		GET_NEXT_CHAR();
		readed_buff.push_back(next);
	}
	to_call_after_reading_line();
}
