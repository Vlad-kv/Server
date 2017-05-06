#include "IO_completion_port.h"

IO_completion_port::IO_completion_port(HANDLE handle)
: iocp_handle(handle) {
}

IO_completion_port::IO_completion_port(IO_completion_port &&port)
: iocp_handle(port.iocp_handle) {
	port.iocp_handle = INVALID_HANDLE_VALUE;
}

IO_completion_port::IO_completion_port()
: iocp_handle(INVALID_HANDLE_VALUE) {
}

HANDLE IO_completion_port::get_handle() const {
	return iocp_handle;
}

void IO_completion_port::close() {
	if (iocp_handle != INVALID_HANDLE_VALUE) {
		LOG("~~~~~~~~~~~~~\n");
		BOOL res = CloseHandle(iocp_handle);
		if (res == 0) {
			throw new socket_exception("Error in closing completion port : " + std::to_string(GetLastError()));
		}
	}
}

IO_completion_port::~IO_completion_port() {
	try {
		close();
	} catch (const socket_exception *ex) {
		LOG("Exception in destructor of IO_completion_port");
	}
}
