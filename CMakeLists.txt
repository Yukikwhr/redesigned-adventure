cmake_minimum_required(VERSION 3.1)

project(syspro9)

find_package(Threads REQUIRED)

add_executable(server logutil.c logutil.h server.c)

target_link_libraries(server Threads::Threads)