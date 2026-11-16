foreach(test_target IN ITEMS ast_tests runtime_context_tests statement_tests lexer_tests)
    add_executable(${test_target} unit/${test_target}.cpp)
    target_link_libraries(${test_target} PRIVATE para_cl_core GTest::gtest_main)

    para_cl_enable_warnings(${test_target})

    gtest_discover_tests(${test_target} PROPERTIES TIMEOUT 10)
endforeach()
