find_package(Python3 3.7 REQUIRED COMPONENTS Interpreter)

foreach(test_case IN ITEMS factorial fibonacci gcd nested_loops scopes statement_bodies expressions short_circuit empty)
    add_test(NAME "e2e.${test_case}"
        COMMAND Python3::Interpreter
            "${CMAKE_CURRENT_SOURCE_DIR}/test_cli.py" "$<TARGET_FILE:para_cl>" "${test_case}")
    set_tests_properties("e2e.${test_case}" PROPERTIES LABELS e2e TIMEOUT 10)
endforeach()
