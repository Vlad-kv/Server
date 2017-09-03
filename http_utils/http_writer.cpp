#include "http_writer.h"
#include "my_assert.h"
using namespace std;
bool http_writer::is_previous_request_completed() {
	return !is_writing_not_completed;
}

void http_writer::close() {
	*is_alive = false;
}
http_writer::~http_writer() {
	close();
}

void http_writer::on_write_completion(size_t saved_bytes, size_t transmitted_bytes) {
	if (transmitted_bytes == 0) {
		is_writing_not_completed = false;
		on_writing_shutdown();
		close();
		return;
	}
	on_event_from_sock();
	
	if (saved_bytes > 0) {
		write_some_saved_bytes();
		return;
	}
	is_writing_not_completed = false;
	callback();
}
