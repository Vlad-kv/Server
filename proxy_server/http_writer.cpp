#include "http_writer.h"
#include "my_assert.h"

void http_writer::clean() {
	callback = nullptr;
	on_writing_shutdown = nullptr;
}
void http_writer::close() {
	clean();
	*is_alive = false;
}
http_writer::~http_writer() {
	close();
}

void http_writer::on_write_completion(size_t saved_bytes, size_t transmitted_bytes) {
	if (transmitted_bytes == 0) {
		on_writing_shutdown();
		close();
		return;
	}
	if (saved_bytes > 0) {
		write_some_saved_bytes();
		return;
	}
	func_t temp = move(callback);
	clean();
	callback();
}
