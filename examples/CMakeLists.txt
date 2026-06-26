add_executable(diag_basic_example basic/main.cpp)
target_link_libraries(diag_basic_example PRIVATE diag::diag)

add_executable(diag_dtc_example dtc/main.cpp)
target_link_libraries(diag_dtc_example PRIVATE diag::diag)

add_executable(diag_identity_example identity/main.cpp)
target_link_libraries(diag_identity_example PRIVATE diag::diag)

if(DIAG_BUILD_TESTS)
    add_test(NAME diag_basic_example COMMAND diag_basic_example)
    add_test(NAME diag_dtc_example COMMAND diag_dtc_example)
    add_test(NAME diag_identity_example COMMAND diag_identity_example)
endif()
