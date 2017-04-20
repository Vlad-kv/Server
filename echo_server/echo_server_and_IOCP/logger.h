#ifndef LOGGER_H
#define LOGGER_H

#include <sstream>

#define LOG(s) std::cout << s

#define LOG_AND_RET(s) (LOG(s), s)

#endif // LOGGER_H
