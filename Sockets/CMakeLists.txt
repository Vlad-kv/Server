cmake_minimum_required(VERSION 3.0)

set(LIB_SOURCES 
	abstract_server.cpp
	client_socket_2.cpp
	getaddrinfo_executer.cpp
	IO_completion_port.cpp
	logger.cpp
	socket.cpp
	socket_descriptor.cpp
	timers.cpp
)
add_library(Sockets STATIC ${LIB_SOURCES})

add_library(ws2_32 STATIC IMPORTED)
set_property(TARGET ws2_32 PROPERTY IMPORTED_LOCATION "C:/Program Files (x86)/mingw-w64/i686-7.1.0-posix-dwarf-rt_v5-rev2/mingw32/i686-w64-mingw32/lib/libws2_32.a")
