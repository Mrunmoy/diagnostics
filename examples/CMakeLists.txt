add_executable(diag_basic_example basic/main.c)
target_link_libraries(diag_basic_example PRIVATE diag::diag)

set(diag_example_targets diag_basic_example)

if(DIAG_FEATURE_DTC)
    add_executable(diag_dtc_example dtc/main.c)
    target_link_libraries(diag_dtc_example PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_dtc_example)
endif()

if(DIAG_FEATURE_LIFECYCLE)
    add_executable(diag_lifecycle_example lifecycle/main.c)
    target_link_libraries(diag_lifecycle_example PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_lifecycle_example)
endif()

if(DIAG_FEATURE_IDENTITY)
    add_executable(diag_identity_example identity/main.c)
    target_link_libraries(diag_identity_example PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_identity_example)
endif()

if(DIAG_FEATURE_STORAGE AND DIAG_FEATURE_TRANSPORT)
    add_executable(diag_adapters_example adapters/main.c)
    target_link_libraries(diag_adapters_example PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_adapters_example)
endif()

if(DIAG_BUILD_TESTS)
    foreach(example_target ${diag_example_targets})
        add_test(NAME ${example_target} COMMAND ${example_target})
    endforeach()
endif()
