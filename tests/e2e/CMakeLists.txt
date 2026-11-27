find_package(Python3 3.7 REQUIRED COMPONENTS Interpreter)

set(test_script "${CMAKE_CURRENT_SOURCE_DIR}/test_cli.py")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${test_script}")

execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${test_script}" --list-cases
    RESULT_VARIABLE list_result
    OUTPUT_VARIABLE case_names
    ERROR_VARIABLE list_error)
string(REGEX MATCHALL "[^\r\n]+" test_cases "${case_names}")
if(NOT list_result STREQUAL "0" OR NOT test_cases)
    message(FATAL_ERROR "Cannot list Python e2e cases (${list_result}): ${list_error}")
endif()

foreach(test_case IN LISTS test_cases)
    add_test(NAME "e2e.${test_case}"
        COMMAND Python3::Interpreter "${test_script}" "$<TARGET_FILE:para_cl>" "${test_case}")
    set_tests_properties("e2e.${test_case}" PROPERTIES LABELS e2e TIMEOUT 10)
endforeach()

if(ENABLE_LLVM_IR_GEN)
    add_test(NAME e2e.emit_llvm
        COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/test_emit_llvm.py" "$<TARGET_FILE:para_cl>")
    set_tests_properties(e2e.emit_llvm PROPERTIES LABELS e2e TIMEOUT 15)
    add_test(NAME e2e.emit_llvm_print_literals
        COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/test_emit_llvm.py" "$<TARGET_FILE:para_cl>" --run-print "$<TARGET_FILE:para_cl_runtime>")
    set_tests_properties(e2e.emit_llvm_print_literals PROPERTIES LABELS e2e TIMEOUT 15 SKIP_RETURN_CODE 77)
    add_test(NAME e2e.emit_llvm_while_iterations
        COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/test_emit_llvm.py" "$<TARGET_FILE:para_cl>" --run-loops "$<TARGET_FILE:para_cl_runtime>")
    set_tests_properties(e2e.emit_llvm_while_iterations PROPERTIES LABELS e2e TIMEOUT 25 SKIP_RETURN_CODE 77)
endif()
