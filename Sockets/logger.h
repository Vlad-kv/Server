#ifndef LOGGER_H
#define LOGGER_H

#include <sstream>
#include <iostream>
#include <mutex>

extern std::mutex _log_mutex;

#define LOG(s) \
	{\
	/*	std::lock_guard<std::mutex> _LOG_lg(_log_mutex);*/\
		std::cout << s;\
	}

#endif // LOGGER_H
