cmake_minimum_required(VERSION 3.16)
project(para_cl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_LLVM_IR_GEN "Build the LLVM IR generator" OFF)

include(CTest)
include(cmake/warnings.cmake)

add_library(para_cl_core STATIC
    frontend/source/ast.cpp
    frontend/source/token.cpp
    frontend/source/lexer.cpp
    frontend/source/parser.cpp
    interpreter/source/runtime_context.cpp)
target_include_directories(para_cl_core PUBLIC frontend/include interpreter/include)
para_cl_enable_warnings(para_cl_core)

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

add_executable(para_cl main.cpp frontend/source/input.cpp)
target_link_libraries(para_cl PRIVATE para_cl_core)
para_cl_enable_warnings(para_cl)

if(ENABLE_LLVM_IR_GEN)
    find_package(LLVM REQUIRED CONFIG)
    separate_arguments(para_cl_llvm_flags NATIVE_COMMAND "${LLVM_DEFINITIONS}")

    add_library(para_cl_codegen STATIC backend/source/codegen.cpp)
    target_include_directories(para_cl_codegen PUBLIC backend/include PRIVATE ${LLVM_INCLUDE_DIRS})
    target_compile_options(para_cl_codegen PRIVATE ${para_cl_llvm_flags})
    target_link_libraries(para_cl_codegen PRIVATE para_cl_core LLVMCore LLVMSupport)
    para_cl_enable_warnings(para_cl_codegen)

    target_compile_definitions(para_cl PRIVATE ENABLE_LLVM_IR_GEN)
    target_link_libraries(para_cl PRIVATE para_cl_codegen)
endif()
