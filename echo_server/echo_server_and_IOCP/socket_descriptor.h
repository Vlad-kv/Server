#ifndef SOCKET_DESCRIPTOR_H
#define SOCKET_DESCRIPTOR_H

#include <winsock2.h>
#include <windows.h>

class socket_descriptor;

#include "logger.h"
#include "socket_exception.h"

class socket_descriptor {
	SOCKET sd;
	
public:
	socket_descriptor();
	socket_descriptor(int sd);
	
	socket_descriptor(int address_family, int type, int protocol);
	
	socket_descriptor(const socket_descriptor &) = delete;
	socket_descriptor(socket_descriptor &&other);
	
	void connect(short family, u_long addr, u_short port);
	void connect(short family, std::string addr, u_short port);
	
	void invalidate();
	SOCKET get_sd() const;
	
	bool is_valid() const;
	
	void close();
	~socket_descriptor();
	
	socket_descriptor& operator=(const socket_descriptor &) = delete;
	socket_descriptor& operator=(socket_descriptor&& socket_d);
};

#endif // SOCKET_DESCRIPTOR_H
