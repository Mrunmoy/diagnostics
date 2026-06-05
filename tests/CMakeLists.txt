include(FetchContent)

# Pinned GoogleTest release. FetchContent keeps the dependency out of the tree
# and out of any installed/embedded artifact.
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)

# Honor the parent project's runtime settings on Windows.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Test files are C++ (GoogleTest) and link the C99 library under test.
add_executable(diag_tests
    test_context.cpp
    test_features.cpp
)

if(DIAG_FEATURE_CAPSULE)
    target_sources(diag_tests PRIVATE test_capsule.cpp)
endif()

if(DIAG_FEATURE_DTC)
    target_sources(diag_tests PRIVATE test_dtc.cpp)
endif()

if(DIAG_FEATURE_IDENTITY)
    target_sources(diag_tests PRIVATE test_identity.cpp)
endif()

if(DIAG_FEATURE_LIFECYCLE)
    target_sources(diag_tests PRIVATE test_lifecycle.cpp)
endif()

if(DIAG_FEATURE_STORAGE)
    target_sources(diag_tests PRIVATE test_storage.cpp)
endif()

if(DIAG_FEATURE_DTC AND DIAG_FEATURE_STORAGE AND DIAG_FEATURE_CAPSULE)
    target_sources(diag_tests PRIVATE test_persistence.cpp)
endif()

target_link_libraries(diag_tests
    PRIVATE
        diag::diag
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(diag_tests)
