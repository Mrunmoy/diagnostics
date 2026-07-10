add_executable(diag_basic_example basic/main.cpp)
target_link_libraries(diag_basic_example PRIVATE diag::diag)

set(diag_example_targets diag_basic_example)

add_executable(diag_capsule_example capsule/main.cpp)
target_link_libraries(diag_capsule_example PRIVATE diag::diag)
list(APPEND diag_example_targets diag_capsule_example)

if(DIAG_FEATURE_DTC)
    add_executable(diag_dtc_example dtc/main.cpp)
    target_link_libraries(diag_dtc_example PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_dtc_example)
endif()

if(DIAG_FEATURE_IDENTITY)
    add_executable(diag_identity_example identity/main.cpp)
    target_link_libraries(diag_identity_example PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_identity_example)
endif()

if(DIAG_FEATURE_LIFECYCLE)
    add_executable(diag_lifecycle_example lifecycle/main.cpp)
    target_link_libraries(diag_lifecycle_example PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_lifecycle_example)
endif()

if(DIAG_FEATURE_DTC
   AND DIAG_FEATURE_LIFECYCLE
   AND DIAG_FEATURE_STORAGE
   AND DIAG_FEATURE_CAPSULE)
    add_executable(diag_storage_example storage/main.cpp)
    target_link_libraries(diag_storage_example PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_storage_example)
endif()

if(DIAG_BUILD_TESTS)
    foreach(example_target IN LISTS diag_example_targets)
        add_test(NAME ${example_target} COMMAND ${example_target})
    endforeach()
endif()
