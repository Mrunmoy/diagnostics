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
   AND DIAG_FEATURE_IDENTITY
   AND DIAG_FEATURE_STORAGE
   AND DIAG_FEATURE_CAPSULE)
    add_executable(diag_catalog_reader_example
        catalog_reader/main.c
        common/example_diag_tool.c
        diagnostic_session/device.c
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

if(DIAG_BUILD_SOCKETCAN_EXAMPLE
   AND DIAG_FEATURE_DTC
   AND DIAG_FEATURE_IDENTITY
   AND DIAG_FEATURE_STORAGE
   AND DIAG_FEATURE_CAPSULE)
    add_library(diag_socketcan_example_socketcan INTERFACE)
    target_include_directories(diag_socketcan_example_socketcan
        INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}/socketcan_transport/third_party/socket-can
    )

    add_executable(diag_socketcan_device_cpp
        common/example_diag_tool.c
        diagnostic_session/device.c
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
        common/example_diag_tool.c
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

if(DIAG_BUILD_SERIAL_EXAMPLE
   AND DIAG_FEATURE_DTC
   AND DIAG_FEATURE_IDENTITY
   AND DIAG_FEATURE_STORAGE
   AND DIAG_FEATURE_CAPSULE)
    add_executable(diag_serial_device_cpp
        common/example_diag_tool.c
        diagnostic_session/device.c
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
        common/example_diag_tool.c
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

if(DIAG_BUILD_MODBUS_RTU_EXAMPLE
   AND DIAG_FEATURE_DTC
   AND DIAG_FEATURE_IDENTITY
   AND DIAG_FEATURE_STORAGE
   AND DIAG_FEATURE_CAPSULE)
    add_executable(diag_modbus_rtu_device_cpp
        common/example_diag_tool.c
        diagnostic_session/device.c
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
        common/example_diag_tool.c
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

if(DIAG_FEATURE_DTC
   AND DIAG_FEATURE_IDENTITY
   AND DIAG_FEATURE_STORAGE
   AND DIAG_FEATURE_CAPSULE)
    add_executable(diag_diagnostic_viewer_device
        common/example_diag_tool.c
        diagnostic_viewer/device.c
        diagnostic_viewer/main.c
    )
    target_include_directories(diag_diagnostic_viewer_device
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/common
            ${CMAKE_CURRENT_SOURCE_DIR}/diagnostic_viewer
    )
    target_link_libraries(diag_diagnostic_viewer_device PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_diagnostic_viewer_device)
endif()

if(DIAG_FEATURE_DTC
   AND DIAG_FEATURE_IDENTITY
   AND DIAG_FEATURE_STORAGE
   AND DIAG_FEATURE_CAPSULE)
    add_executable(diag_grafana_reader_example
        common/example_diag_tool.c
        grafana_reader/device.c
        grafana_reader/exporter.c
        grafana_reader/main.c
    )
    target_include_directories(diag_grafana_reader_example
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/common
            ${CMAKE_CURRENT_SOURCE_DIR}/grafana_reader
    )
    target_link_libraries(diag_grafana_reader_example PRIVATE diag::diag)
    list(APPEND diag_example_targets diag_grafana_reader_example)
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

if(DIAG_BUILD_TESTS AND DIAG_TEST_EXAMPLES)
    foreach(example_target ${diag_example_targets})
        add_test(NAME ${example_target} COMMAND ${example_target})
    endforeach()

    if(TARGET diag_diagnostic_viewer_device)
        find_package(Python3 COMPONENTS Interpreter QUIET)
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

    if(TARGET diag_socketcan_device_cpp AND TARGET diag_socketcan_tester_cpp)
        find_package(Python3 COMPONENTS Interpreter QUIET)
        if(Python3_Interpreter_FOUND)
            add_test(
                NAME diag_socketcan_transport_vcan_demo
                COMMAND
                    ${Python3_EXECUTABLE}
                    ${CMAKE_CURRENT_SOURCE_DIR}/socketcan_transport/run_vcan_demo.py
                    --device $<TARGET_FILE:diag_socketcan_device_cpp>
                    --tester $<TARGET_FILE:diag_socketcan_tester_cpp>
            )
        else()
            message(STATUS "Python3 interpreter not found; skipping SocketCAN transport demo test")
        endif()
    endif()

    if(TARGET diag_serial_device_cpp AND TARGET diag_serial_tester_cpp)
        find_package(Python3 COMPONENTS Interpreter QUIET)
        if(Python3_Interpreter_FOUND)
            add_test(
                NAME diag_serial_transport_pty_demo
                COMMAND
                    ${Python3_EXECUTABLE}
                    ${CMAKE_CURRENT_SOURCE_DIR}/serial_transport/run_pty_demo.py
                    --device $<TARGET_FILE:diag_serial_device_cpp>
                    --tester $<TARGET_FILE:diag_serial_tester_cpp>
            )
        else()
            message(STATUS "Python3 interpreter not found; skipping serial transport demo test")
        endif()
    endif()

    if(TARGET diag_modbus_rtu_device_cpp AND TARGET diag_modbus_rtu_tester_cpp)
        find_package(Python3 COMPONENTS Interpreter QUIET)
        if(Python3_Interpreter_FOUND)
            add_test(
                NAME diag_modbus_rtu_transport_pty_demo
                COMMAND
                    ${Python3_EXECUTABLE}
                    ${CMAKE_CURRENT_SOURCE_DIR}/modbus_rtu_transport/run_pty_demo.py
                    --device $<TARGET_FILE:diag_modbus_rtu_device_cpp>
                    --tester $<TARGET_FILE:diag_modbus_rtu_tester_cpp>
            )
        else()
            message(STATUS "Python3 interpreter not found; skipping Modbus RTU transport demo test")
        endif()
    endif()
endif()
