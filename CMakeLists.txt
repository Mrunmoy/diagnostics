cmake_minimum_required(VERSION 3.22)

project(generic_diagnostics
    VERSION 0.1.0
    DESCRIPTION "Transport-agnostic diagnostics library for embedded and host systems"
    LANGUAGES C
)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(DIAG_BUILD_TESTS "Build diagnostics tests" ON)
option(DIAG_BUILD_EXAMPLES "Build diagnostics examples" ON)

# The diagnostics core is C99. C++17 is enabled only when building the test
# suite, which uses GoogleTest. Cross/embedded builds with -DDIAG_BUILD_TESTS=OFF
# never pull in a C++ toolchain.
if(DIAG_BUILD_TESTS)
    enable_language(CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

add_library(diag
    src/diag_capsule.c
    src/diag_context.c
    src/diag_dtc.c
    src/diag_storage.c
    src/diag_transport.c
)

add_library(diag::diag ALIAS diag)

set_target_properties(diag PROPERTIES
    EXPORT_NAME diag
    OUTPUT_NAME diag
)

target_include_directories(diag
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(diag PUBLIC c_std_99)

if(MSVC)
    target_compile_options(diag PRIVATE /W4)
else()
    target_compile_options(diag PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(DIAG_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(DIAG_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

install(TARGETS diag
    EXPORT diagTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/diag
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT diagTargets
    FILE diagTargets.cmake
    NAMESPACE diag::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/diag
)

configure_package_config_file(
    cmake/diagConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/diagConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/diag
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/diagConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/diagConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/diagConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/diag
)
