#ifndef SOCKET_EXCEPTION_H
#define SOCKET_EXCEPTION_H

#include "logger.h"

class socket_exception : std::runtime_error {
	public:
	
	socket_exception(const std::string& mess)
	: std::runtime_error(mess) {
		LOG(mess);
	}
};

#endif // SOCKET_EXCEPTION_H
