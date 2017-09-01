#ifndef LOGGER_H
#define LOGGER_H

#include <sstream>
#include <iostream>
#include <mutex>
#include <fstream>

extern std::mutex _log_mutex;

#define LOG(s) \
	{\
		std::lock_guard<std::mutex> _LOG_lg(_log_mutex);\
		std::ofstream out("log.txt", std::ios_base::app);\
		out << s;\
		out.close();\
	}

void clear_log_file();
	
#endif // LOGGER_H
