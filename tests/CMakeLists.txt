foreach(test_target IN ITEMS ast_tests runtime_context_tests)
    add_executable(${test_target} unit/${test_target}.cpp)
    target_include_directories(${test_target} PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_link_libraries(${test_target} PRIVATE GTest::gtest_main)

    if(MSVC)
        target_compile_options(${test_target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${test_target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    gtest_discover_tests(${test_target})
endforeach()
