cmake_minimum_required(VERSION 3.20)
project(itch_handler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(itch_handler src/main.cpp)
