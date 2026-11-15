cmake_minimum_required(VERSION 3.16)
project(para_cl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

if(BUILD_TESTING)
    include(FetchContent)

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0)

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    include(GoogleTest)
    add_subdirectory(tests)
endif()

add_executable(para_cl source/main.cpp)

if(MSVC)
    target_compile_options(para_cl PRIVATE /W4 /permissive-)
else()
    target_compile_options(para_cl PRIVATE -Wall -Wextra -Wpedantic)
endif()
