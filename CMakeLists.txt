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
option(DIAG_BUILD_SOCKETCAN_EXAMPLE "Build Linux SocketCAN transport example" OFF)
option(DIAG_FEATURE_DTC "Enable Diagnostic Trouble Code APIs" ON)
option(DIAG_FEATURE_LIFECYCLE "Enable lifecycle/reset counter APIs" ON)
option(DIAG_FEATURE_IDENTITY "Enable device identity APIs" ON)
option(DIAG_FEATURE_STORAGE "Enable storage adapter APIs" ON)
option(DIAG_FEATURE_TRANSPORT "Enable transport adapter APIs" ON)
option(DIAG_FEATURE_CAPSULE "Enable serialized capsule APIs" ON)

# The diagnostics core is C99. C++17 is enabled only for host-side tests and
# optional C++ transport examples. Embedded builds with tests/examples off never
# pull in a C++ toolchain.
if(DIAG_BUILD_TESTS OR DIAG_BUILD_SOCKETCAN_EXAMPLE)
    enable_language(CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(DIAG_BUILD_TESTS)
    enable_testing()
endif()

add_library(diag
    src/diag_context.c
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

foreach(feature
    DTC
    LIFECYCLE
    IDENTITY
    STORAGE
    TRANSPORT
    CAPSULE
)
    if(DIAG_FEATURE_${feature})
        target_compile_definitions(diag PUBLIC DIAG_FEATURE_${feature}=1)
    else()
        target_compile_definitions(diag PUBLIC DIAG_FEATURE_${feature}=0)
    endif()
endforeach()

if(DIAG_FEATURE_CAPSULE)
    target_sources(diag PRIVATE src/diag_capsule.c)
endif()

if(DIAG_FEATURE_DTC)
    target_sources(diag PRIVATE src/diag_dtc.c)
endif()

if(DIAG_FEATURE_IDENTITY)
    target_sources(diag PRIVATE src/diag_identity.c)
endif()

if(DIAG_FEATURE_LIFECYCLE)
    target_sources(diag PRIVATE src/diag_lifecycle.c)
endif()

if(DIAG_FEATURE_STORAGE)
    target_sources(diag PRIVATE src/diag_storage.c)
endif()

if(DIAG_FEATURE_TRANSPORT)
    target_sources(diag PRIVATE src/diag_transport.c)
endif()

if(MSVC)
    target_compile_options(diag PRIVATE /W4)
else()
    target_compile_options(diag PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(DIAG_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(DIAG_BUILD_TESTS)
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
