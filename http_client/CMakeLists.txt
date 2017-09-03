cmake_minimum_required(VERSION 3.0)
project(http_client)

set(SOURCES main.cpp)
add_executable(http_client ${SOURCES})

add_subdirectory(../Sockets ../../Sockets/res)
add_subdirectory(../http_utils ../../http_utils/res)

target_link_libraries(http_client Sockets http_utils ws2_32)
