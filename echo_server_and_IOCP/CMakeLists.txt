cmake_minimum_required(VERSION 3.0)
project(echo_server_and_IOCP)

set(SOURCES main.cpp echo_server.cpp)
add_executable(echo_server ${SOURCES})

add_subdirectory(../Sockets ../../Sockets/res)
target_link_libraries(echo_server Sockets ws2_32)
