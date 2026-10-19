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

if(DIAG_FEATURE_DTC
   AND DIAG_FEATURE_IDENTITY
   AND DIAG_FEATURE_STORAGE
   AND DIAG_FEATURE_CAPSULE)
    add_executable(diag_diagnostic_session_example
        common/example_diag_client.cpp
        common/example_diag_tool.cpp
        diagnostic_session/device.cpp
        diagnostic_session/main.cpp
    )
    target_include_directories(diag_diagnostic_session_example
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/common
            ${CMAKE_CURRENT_SOURCE_DIR}/diagnostic_session
    )
    target_link_libraries(diag_diagnostic_session_example PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_diagnostic_session_example)

    add_executable(diag_catalog_reader_example
        catalog_reader/main.cpp
        common/example_diag_tool.cpp
        diagnostic_session/device.cpp
    )
    target_include_directories(diag_catalog_reader_example
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/catalog_reader
            ${CMAKE_CURRENT_SOURCE_DIR}/common
            ${CMAKE_CURRENT_SOURCE_DIR}/diagnostic_session
    )
    target_link_libraries(diag_catalog_reader_example PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_catalog_reader_example)
endif()

if(DIAG_FEATURE_STORAGE AND DIAG_FEATURE_TRANSPORT)
    add_executable(diag_adapters_example adapters/main.cpp)
    target_link_libraries(diag_adapters_example PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_adapters_example)
endif()

if(DIAG_FEATURE_DTC AND DIAG_FEATURE_IDENTITY)
    add_executable(diag_sensor_node_example
        common/example_diag_tool.cpp
        sensor_node/main.cpp
    )
    target_include_directories(diag_sensor_node_example
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/common
    )
    target_link_libraries(diag_sensor_node_example PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_sensor_node_example)
endif()

if(DIAG_FEATURE_IDENTITY AND DIAG_FEATURE_TRANSPORT)
    add_executable(diag_io_module_example io_module/main.cpp)
    target_link_libraries(diag_io_module_example PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_io_module_example)
endif()

if(DIAG_FEATURE_DTC
   AND DIAG_FEATURE_LIFECYCLE
   AND DIAG_FEATURE_IDENTITY
   AND DIAG_FEATURE_STORAGE
   AND DIAG_FEATURE_CAPSULE)
    add_executable(diag_industrial_oven_example
        common/example_diag_tool.cpp
        industrial_oven/main.cpp
    )
    target_include_directories(diag_industrial_oven_example
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/common
    )
    target_link_libraries(diag_industrial_oven_example PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_industrial_oven_example)

    add_executable(diag_process_controller_example
        common/example_diag_tool.cpp
        process_controller/main.cpp
    )
    target_include_directories(diag_process_controller_example
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/common
    )
    target_link_libraries(diag_process_controller_example PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_process_controller_example)
endif()

if(DIAG_FEATURE_DTC
   AND DIAG_FEATURE_LIFECYCLE
   AND DIAG_FEATURE_STORAGE
   AND DIAG_FEATURE_CAPSULE)
    add_executable(diag_bootloader_app_shared_example bootloader_app_shared/main.cpp)
    target_link_libraries(diag_bootloader_app_shared_example PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_bootloader_app_shared_example)
endif()

if(DIAG_FEATURE_DTC
   AND DIAG_FEATURE_LIFECYCLE
   AND DIAG_FEATURE_IDENTITY
   AND DIAG_FEATURE_STORAGE
   AND DIAG_FEATURE_TRANSPORT
   AND DIAG_FEATURE_CAPSULE)
    add_executable(diag_ecu_node_example
        common/example_diag_tool.cpp
        ecu_node/main.cpp
    )
    target_include_directories(diag_ecu_node_example
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/common
    )
    target_link_libraries(diag_ecu_node_example PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_ecu_node_example)
endif()

set(diag_session_link_features_ok FALSE)
if(DIAG_FEATURE_DTC
   AND DIAG_FEATURE_IDENTITY
   AND DIAG_FEATURE_STORAGE
   AND DIAG_FEATURE_CAPSULE)
    set(diag_session_link_features_ok TRUE)
endif()

if(DIAG_BUILD_SERIAL_EXAMPLE AND diag_session_link_features_ok)
    add_executable(diag_serial_device_cpp
        common/example_diag_tool.cpp
        diagnostic_session/device.cpp
        serial_transport/device.cpp
        serial_transport/serial_diag_link.cpp
    )
    target_include_directories(diag_serial_device_cpp
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/common
            ${CMAKE_CURRENT_SOURCE_DIR}/diagnostic_session
            ${CMAKE_CURRENT_SOURCE_DIR}/serial_transport
    )
    target_link_libraries(diag_serial_device_cpp PRIVATE diag::diag)

    add_executable(diag_serial_tester_cpp
        common/example_diag_client.cpp
        common/example_diag_tool.cpp
        serial_transport/serial_diag_link.cpp
        serial_transport/tester.cpp
    )
    target_include_directories(diag_serial_tester_cpp
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/common
            ${CMAKE_CURRENT_SOURCE_DIR}/serial_transport
    )
    target_link_libraries(diag_serial_tester_cpp PRIVATE diag::diag)
endif()

if(DIAG_BUILD_MODBUS_RTU_EXAMPLE AND diag_session_link_features_ok)
    add_executable(diag_modbus_rtu_device_cpp
        common/example_diag_tool.cpp
        diagnostic_session/device.cpp
        modbus_rtu_transport/device.cpp
        modbus_rtu_transport/modbus_rtu_diag_link.cpp
        serial_transport/serial_diag_link.cpp
    )
    target_include_directories(diag_modbus_rtu_device_cpp
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/common
            ${CMAKE_CURRENT_SOURCE_DIR}/diagnostic_session
            ${CMAKE_CURRENT_SOURCE_DIR}/modbus_rtu_transport
            ${CMAKE_CURRENT_SOURCE_DIR}/serial_transport
    )
    target_link_libraries(diag_modbus_rtu_device_cpp PRIVATE diag::diag)

    add_executable(diag_modbus_rtu_tester_cpp
        common/example_diag_client.cpp
        common/example_diag_tool.cpp
        modbus_rtu_transport/modbus_rtu_diag_link.cpp
        modbus_rtu_transport/tester.cpp
        serial_transport/serial_diag_link.cpp
    )
    target_include_directories(diag_modbus_rtu_tester_cpp
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/common
            ${CMAKE_CURRENT_SOURCE_DIR}/modbus_rtu_transport
            ${CMAKE_CURRENT_SOURCE_DIR}/serial_transport
    )
    target_link_libraries(diag_modbus_rtu_tester_cpp PRIVATE diag::diag)
endif()

if(DIAG_BUILD_SOCKETCAN_EXAMPLE AND diag_session_link_features_ok)
    add_library(diag_socketcan_example_socketcan INTERFACE)
    target_include_directories(diag_socketcan_example_socketcan
        INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}/socketcan_transport/third_party/socket-can
    )

    add_executable(diag_socketcan_device_cpp
        common/example_diag_tool.cpp
        diagnostic_session/device.cpp
        socketcan_transport/device.cpp
        socketcan_transport/socketcan_diag_link.cpp
    )
    target_include_directories(diag_socketcan_device_cpp
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/common
            ${CMAKE_CURRENT_SOURCE_DIR}/diagnostic_session
            ${CMAKE_CURRENT_SOURCE_DIR}/socketcan_transport
    )
    target_link_libraries(diag_socketcan_device_cpp
        PRIVATE
            diag::diag
            diag_socketcan_example_socketcan
    )

    add_executable(diag_socketcan_tester_cpp
        common/example_diag_client.cpp
        common/example_diag_tool.cpp
        socketcan_transport/socketcan_diag_link.cpp
        socketcan_transport/tester.cpp
    )
    target_include_directories(diag_socketcan_tester_cpp
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/common
            ${CMAKE_CURRENT_SOURCE_DIR}/socketcan_transport
    )
    target_link_libraries(diag_socketcan_tester_cpp
        PRIVATE
            diag::diag
            diag_socketcan_example_socketcan
    )
endif()

if(DIAG_FEATURE_DTC
   AND DIAG_FEATURE_IDENTITY
   AND DIAG_FEATURE_STORAGE
   AND DIAG_FEATURE_CAPSULE)
    add_executable(diag_diagnostic_viewer_device
        common/example_diag_tool.cpp
        diagnostic_viewer/device.cpp
        diagnostic_viewer/main.cpp
    )
    target_include_directories(diag_diagnostic_viewer_device
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/common
            ${CMAKE_CURRENT_SOURCE_DIR}/diagnostic_viewer
    )
    target_link_libraries(diag_diagnostic_viewer_device PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_diagnostic_viewer_device)
endif()

if(DIAG_BUILD_TESTS)
    foreach(example_target IN LISTS diag_example_targets)
        add_test(NAME ${example_target} COMMAND ${example_target})
    endforeach()

    find_package(Python3 COMPONENTS Interpreter QUIET)

    if(TARGET diag_serial_device_cpp AND TARGET diag_serial_tester_cpp)
        if(Python3_Interpreter_FOUND)
            add_test(
                NAME diag_serial_transport_pty_demo
                COMMAND
                    ${Python3_EXECUTABLE}
                    ${CMAKE_CURRENT_SOURCE_DIR}/serial_transport/run_pty_demo.py
                    --device $<TARGET_FILE:diag_serial_device_cpp>
                    --tester $<TARGET_FILE:diag_serial_tester_cpp>
            )
        endif()
    endif()

    if(TARGET diag_modbus_rtu_device_cpp AND TARGET diag_modbus_rtu_tester_cpp)
        if(Python3_Interpreter_FOUND)
            add_test(
                NAME diag_modbus_rtu_transport_pty_demo
                COMMAND
                    ${Python3_EXECUTABLE}
                    ${CMAKE_CURRENT_SOURCE_DIR}/modbus_rtu_transport/run_pty_demo.py
                    --device $<TARGET_FILE:diag_modbus_rtu_device_cpp>
                    --tester $<TARGET_FILE:diag_modbus_rtu_tester_cpp>
            )
        endif()
    endif()

    if(TARGET diag_socketcan_device_cpp AND TARGET diag_socketcan_tester_cpp)
        if(Python3_Interpreter_FOUND)
            add_test(
                NAME diag_socketcan_transport_vcan_demo
                COMMAND
                    ${Python3_EXECUTABLE}
                    ${CMAKE_CURRENT_SOURCE_DIR}/socketcan_transport/run_vcan_demo.py
                    --device $<TARGET_FILE:diag_socketcan_device_cpp>
                    --tester $<TARGET_FILE:diag_socketcan_tester_cpp>
            )
        endif()
    endif()

    if(TARGET diag_diagnostic_viewer_device)
        if(Python3_Interpreter_FOUND)
            add_test(
                NAME diag_diagnostic_viewer_server_self_test
                COMMAND
                    ${Python3_EXECUTABLE}
                    ${CMAKE_CURRENT_SOURCE_DIR}/diagnostic_viewer/viewer_server.py
                    --binary $<TARGET_FILE:diag_diagnostic_viewer_device>
                    --self-test
            )
        else()
            message(STATUS "Python3 interpreter not found; skipping diagnostic viewer server self-test")
        endif()
    endif()
endif()
