cmake_minimum_required(VERSION 3.0)
project(proxy_server)

set(SOURCES main.cpp proxy_server.cpp)
add_executable(proxy_server ${SOURCES})

add_subdirectory(../Sockets ../../Sockets/res)
add_subdirectory(../http_utils ../../http_utils/res)

target_link_libraries(proxy_server Sockets http_utils ws2_32)
