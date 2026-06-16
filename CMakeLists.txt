cmake_minimum_required(VERSION 3.22)

project(generic_diagnostics_cpp
    VERSION 0.1.0
    DESCRIPTION "Modern C++17 transport-agnostic diagnostics library for embedded systems"
    LANGUAGES CXX
)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(DIAG_BUILD_TESTS "Build diagnostics tests" ON)
option(DIAG_BUILD_EXAMPLES "Build diagnostics examples" ON)
option(DIAG_TEST_SPLIT_GTEST "Register each GoogleTest case separately" ON)

add_library(diag
    src/context.cpp
)

add_library(diag::diag ALIAS diag)

set_target_properties(diag PROPERTIES
    EXPORT_NAME diag
    OUTPUT_NAME diag
)

target_include_directories(diag
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(diag PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(diag PRIVATE /W4 /WX /GR- /EHs-c-)
else()
    target_compile_options(diag PRIVATE -Wall -Wextra -Wpedantic -Werror -fno-exceptions -fno-rtti)
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
