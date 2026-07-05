include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
    GIT_SHALLOW TRUE
)

if(MSVC)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()
FetchContent_MakeAvailable(googletest)

set(diag_test_sources
    test_context.cpp
    test_features.cpp
    test_result.cpp
    test_types.cpp
)

if(DIAG_FEATURE_CAPSULE)
    list(APPEND diag_test_sources test_capsule.cpp)
endif()

if(DIAG_FEATURE_DTC)
    list(APPEND diag_test_sources test_dtc.cpp)
endif()

if(DIAG_FEATURE_IDENTITY)
    list(APPEND diag_test_sources test_identity.cpp)
endif()

if(DIAG_FEATURE_LIFECYCLE)
    list(APPEND diag_test_sources test_lifecycle.cpp)
endif()

if(DIAG_FEATURE_DTC
   AND DIAG_FEATURE_LIFECYCLE
   AND DIAG_FEATURE_STORAGE
   AND DIAG_FEATURE_CAPSULE)
    list(APPEND diag_test_sources test_storage.cpp)
endif()

add_executable(diag_tests ${diag_test_sources})

target_link_libraries(diag_tests
    PRIVATE
        diag::diag
        GTest::gtest_main
)

include(GoogleTest)

if(DIAG_TEST_SPLIT_GTEST)
    gtest_add_tests(TARGET diag_tests SOURCES ${diag_test_sources} TEST_LIST diag_tests_TESTS)
else()
    add_test(NAME diag_tests COMMAND diag_tests)
endif()
