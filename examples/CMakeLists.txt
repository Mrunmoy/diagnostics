add_executable(diag_basic_example basic/main.cpp)
target_link_libraries(diag_basic_example PRIVATE diag::diag)

if(DIAG_BUILD_TESTS)
    add_test(NAME diag_basic_example COMMAND diag_basic_example)
endif()
