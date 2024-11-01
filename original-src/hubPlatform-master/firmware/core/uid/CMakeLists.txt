cmake_minimum_required(VERSION 3.2)

get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_ID})

include_directories(
    ${FIRMWARE_COMMON_DIR}
    ${FIRMWARE_CORE_DIR}
    ${FIRMWARE_UTILS_DIR}
    ${FIRMWARE_COMMON_GENERATED_DIR}
    ${FIRMWARE_ADDONS_DIR}
)
link_directories(
    ${LIBRARIES_DIR}
    ${DEPS_ROOT}/network-functionality/build/lib
)

add_subdirectory( response_builders )

file(GLOB SRC_FILES
    *.cpp
    handlers/*.cpp
    ui_preferences/*.cpp
    server/*.cpp
    cluster_manager/*.cpp
    icc_manager/*.cpp
    mdns_config/*.cpp
    cloud_storage/*.cpp
    extensions/*.cpp
    extensions/**/*.cpp
    extensions/**/**/*.cpp
    custom_nodes/*.cpp
    reporter/*.cpp
)

list(REMOVE_ITEM SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
target_link_libraries(${PROJECT_NAME}
        device_model
        service_interfaces
        cored_interface
        cored_model
        networkd_interface
        scenes_manager_interface
        extensions_manager_interface
        backupd_interface
        hardware_manager_interface
        update_manager_interface
        cloud_devices_interface
        notifications_interface
        common_utils
        data_utils
        system_utils
        ssl_utils
        http_bridge
        http
        cloud_storage
        uid_response_builders
)

if( (${HUB_TYPE} MATCHES "g150") OR
    (${HUB_TYPE} MATCHES "g450") OR
    (${HUB_TYPE} MATCHES "g550")
    )
    target_link_libraries( ${PROJECT_NAME} vera_utils )
endif()

add_executable(ha-${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_link_libraries(ha-${PROJECT_NAME}
        ${PROJECT_NAME}
)


# remote interface
file(GLOB SRC_FILES
    remote_interface/*.cpp
    common_methods.cpp
    )
add_library( ${PROJECT_NAME}_interface SHARED ${SRC_FILES} )
target_link_libraries( ${PROJECT_NAME}_interface
    service_interfaces
    hardware_manager_interface
    networkd_interface
    cored_interface
    common_utils
    base_utils
    data_utils
    uid_response_builders
    )

# ICC remote interface
file(GLOB SRC_FILES
        icc_interface/*.cpp
        )
add_library( icc_interface SHARED ${SRC_FILES} )
target_link_libraries( icc_interface
        service_interfaces
        )

# lua: http_server lua bindings
file(GLOB SRC_FILES
        bindings/*.cpp
        )
add_library( lua_http_server SHARED ${SRC_FILES} )
target_link_libraries( lua_http_server
        lua_binding_interface
        uid_interface
        extensions_manager_interface
        )

# fill firmware.cfg file
CONFIGURE_CFG_FILES(CURRENT_GENERATED_CFG_DIR ${SOURCES_ROOT}/system/common/opt/firmware/etc firmware.cfg)

# Tests
CONFIGURE_ARCH_DEPENDENT_FILES(CURRENT_GENERATED_DIR tests/uid_tests_defs.cpp)
HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES("${PROJECT_NAME}" "" "${CURRENT_GENERATED_DIR}/tests" "${CURRENT_GENERATED_DIR}/tests/uid_tests_defs.cpp")

set( PACKAGE_NAME firmware-core-${PROJECT_NAME})
install( TARGETS ha-${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
install( TARGETS lua_http_server DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
install( FILES ${CURRENT_GENERATED_CFG_DIR}/firmware.cfg DESTINATION ${PACKAGE_NAME}/opt/firmware/etc )

set( PACKAGE_NAME firmware-core-${PROJECT_NAME}_interface)
install( TARGETS ${PROJECT_NAME}_interface DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )

set( PACKAGE_NAME firmware-core-icc_interface)
install( TARGETS icc_interface DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
install( PROGRAMS ${SOURCES_ROOT}/system/common/service_files/ha-05_uid DESTINATION ${PACKAGE_NAME}/etc/init.d )
