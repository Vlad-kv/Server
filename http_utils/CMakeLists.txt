cmake_minimum_required(VERSION 3.0)

project(http_utils)

set(SOURCES http_message.cpp http_reader.cpp http_writer.cpp my_assert.cpp)
add_library(http_utils ${SOURCES})
