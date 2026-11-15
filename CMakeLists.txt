cmake_minimum_required(VERSION 3.16)
project(para_cl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(para_cl source/main.cpp)

if(MSVC)
    target_compile_options(para_cl PRIVATE /W4 /permissive-)
else()
    target_compile_options(para_cl PRIVATE -Wall -Wextra -Wpedantic)
endif()
