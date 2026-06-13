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

if(DIAG_FEATURE_DTC AND DIAG_FEATURE_IDENTITY)
    add_executable(diag_sensor_node_example sensor_node/main.c common/example_diag_tool.c)
    target_include_directories(diag_sensor_node_example
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/common
    )
    target_link_libraries(diag_sensor_node_example PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_sensor_node_example)
endif()

if(DIAG_FEATURE_DTC
   AND DIAG_FEATURE_IDENTITY
   AND DIAG_FEATURE_STORAGE
   AND DIAG_FEATURE_CAPSULE)
    add_executable(diag_diagnostic_session_example
        common/example_diag_tool.c
        diagnostic_session/device.c
        diagnostic_session/main.c
    )
    target_include_directories(diag_diagnostic_session_example
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/common
            ${CMAKE_CURRENT_SOURCE_DIR}/diagnostic_session
    )
    target_link_libraries(diag_diagnostic_session_example PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_diagnostic_session_example)
endif()

if(DIAG_FEATURE_DTC
   AND DIAG_FEATURE_LIFECYCLE
   AND DIAG_FEATURE_IDENTITY
   AND DIAG_FEATURE_STORAGE
   AND DIAG_FEATURE_CAPSULE)
    add_executable(diag_industrial_oven_example industrial_oven/main.c common/example_diag_tool.c)
    target_include_directories(diag_industrial_oven_example
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/common
    )
    target_link_libraries(diag_industrial_oven_example PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_industrial_oven_example)
endif()

if(DIAG_FEATURE_DTC
   AND DIAG_FEATURE_LIFECYCLE
   AND DIAG_FEATURE_IDENTITY
   AND DIAG_FEATURE_STORAGE
   AND DIAG_FEATURE_CAPSULE)
    add_executable(diag_process_controller_example process_controller/main.c common/example_diag_tool.c)
    target_include_directories(diag_process_controller_example
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/common
    )
    target_link_libraries(diag_process_controller_example PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_process_controller_example)
endif()

if(DIAG_FEATURE_IDENTITY AND DIAG_FEATURE_TRANSPORT)
    add_executable(diag_io_module_example io_module/main.c)
    target_link_libraries(diag_io_module_example PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_io_module_example)
endif()

if(DIAG_FEATURE_DTC
   AND DIAG_FEATURE_LIFECYCLE
   AND DIAG_FEATURE_IDENTITY
   AND DIAG_FEATURE_STORAGE
   AND DIAG_FEATURE_TRANSPORT
   AND DIAG_FEATURE_CAPSULE)
    add_executable(diag_ecu_node_example ecu_node/main.c common/example_diag_tool.c)
    target_include_directories(diag_ecu_node_example
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/common
    )
    target_link_libraries(diag_ecu_node_example PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_ecu_node_example)
endif()

if(DIAG_FEATURE_DTC
   AND DIAG_FEATURE_LIFECYCLE
   AND DIAG_FEATURE_STORAGE
   AND DIAG_FEATURE_CAPSULE)
    add_executable(diag_bootloader_app_shared_example bootloader_app_shared/main.c)
    target_link_libraries(diag_bootloader_app_shared_example PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_bootloader_app_shared_example)
endif()

if(DIAG_BUILD_TESTS)
    foreach(example_target ${diag_example_targets})
        add_test(NAME ${example_target} COMMAND ${example_target})
    endforeach()
endif()
