cmake_minimum_required(VERSION 3.10)
project(LiteServe)

set(CMAKE_CXX_STANDARD 17)

add_executable(liteserve src/main.cpp)
add_executable(memory_test src/memory_test.cpp)