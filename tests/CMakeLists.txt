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
    test_capsule.cpp
    test_context.cpp
)

target_link_libraries(diag_tests
    PRIVATE
        diag::diag
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(diag_tests)
