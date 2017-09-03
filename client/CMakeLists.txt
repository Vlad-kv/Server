cmake_minimum_required(VERSION 3.0)
project(client)

set(SOURCES main.cpp)
add_executable(client ${SOURCES})

add_subdirectory(../Sockets ../../Sockets/res)
target_link_libraries(client Sockets ws2_32)
