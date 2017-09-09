#include "logger.h"

const bool _LOG_TO_CONSOLE = 0;
std::mutex _log_mutex;

void clear_log_file() {
	std::lock_guard<std::mutex> _LOG_lg(_log_mutex);
	std::ofstream out("log.txt");
	out.close();
}
