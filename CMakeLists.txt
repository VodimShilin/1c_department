cmake_minimum_required(VERSION 3.21)
project(1C)

set(CMAKE_CXX_STANDARD 20)

add_executable(1C main.cpp tp/thread_pool.cpp)
add_executable(server main_server.cpp)
add_executable(client main_client.cpp tp/thread_pool.cpp)
