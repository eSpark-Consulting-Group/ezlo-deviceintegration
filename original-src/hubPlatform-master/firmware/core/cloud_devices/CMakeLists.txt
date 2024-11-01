get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )

if( WITH_TESTS )
    set(WITH_CLOUD_DEVICES_LEGACY_CODE ON CACHE BOOL "Enable cloud devices legacy code")
    add_definitions( -D__WITH_CLOUD_DEVICES_LEGACY_CODE__=1 )
else()
    set(WITH_CLOUD_DEVICES_LEGACY_CODE OFF CACHE BOOL "Enable cloud devices legacy code")
endif()

include_directories(
    ${FIRMWARE_ADDONS_DIR}
    ${FIRMWARE_CORE_DIR}
    ${FIRMWARE_COMMON_DIR}
)

set( BINARY_NAME ha-${PROJECT_NAME} )

set( PACKAGE_NAME firmware-core-${PROJECT_NAME})

file ( GLOB SRC_FILES
    *.cpp
    resources/*.cpp
    subscription_manager/*.cpp
    local_notifications/*.cpp
    local_notifications/types/*.cpp
    )

file ( GLOB LEGACY_SRC_FILES
    script_events/*.cpp
    )

list( REMOVE_ITEM SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp )

link_directories(
    ${LIBRARIES_DIR}
    )

add_library( ${PROJECT_NAME} ${SRC_FILES} ${LEGACY_SRC_FILES} )
target_link_libraries( ${PROJECT_NAME}
    ${lib_name_rt}
    uid_interface
    cored_interface
    addon_interfaces
    config_reader
    jsonrpc
    http
)

# remote interface
file( GLOB SRC_FILES
    remote_interface/*.cpp
    resources/*.cpp
    local_notifications/resources.cpp
    local_notifications/utils.cpp
    local_notifications/types/meshbot_template_record.cpp
    local_notifications/types/local_notifiction_data_record.cpp
    )

add_library( ${PROJECT_NAME}_interface SHARED ${SRC_FILES} )
target_link_libraries( ${PROJECT_NAME}_interface
    service_interfaces
    tiny
    jsonrpc
    dbclient
)

add_library( lua_${PROJECT_NAME} SHARED bindings/cloud_devices_lua_bindings.cpp )
target_link_libraries( lua_${PROJECT_NAME}
    ${PROJECT_NAME}
    lua_binding_interface
    )

add_executable( ${BINARY_NAME} main.cpp )
target_link_libraries( ${BINARY_NAME}
    ${PROJECT_NAME}
    )

# Tests
file( GLOB EXTRA_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/utils/*.cpp )
HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES( ${PROJECT_NAME} "" "" ${EXTRA_TEST_SOURCES} )

install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
install( TARGETS ${PROJECT_NAME}_interface DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
install( TARGETS ${BINARY_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )

install( TARGETS lua_${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
install( PROGRAMS ${SOURCES_ROOT}/system/common/service_files/ha-07_cloud_devices DESTINATION ${PACKAGE_NAME}/etc/init.d )
