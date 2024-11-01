cmake_minimum_required( VERSION 3.2 )

get_filename_component( PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME )
set( BINARY_NAME ha-${PROJECT_NAME} )
project( ${PROJECT_NAME} )

include_directories(
    ${FIRMWARE_CORE_DIR}
    ${FIRMWARE_COMMON_DIR}
    ${CMAKE_BINARY_DIR}/deps/ubus/include
)

file(GLOB SRC_FILES
    *.cpp
    utils/*.cpp
    issues_monitor/*.cpp
    issues_monitor/handlers/*.cpp
    system_log/*.cpp
)
list(REMOVE_ITEM SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

link_directories(
    ${CMAKE_BINARY_DIR}/deps/ubus/lib/
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
target_link_libraries( ${PROJECT_NAME}
    ubus_interface
    ubus
    ubox
    json-c
    log_utils
    blobmsg_json
    service_interfaces
    service_metrics_data
    dbclient
    )

add_executable( ${BINARY_NAME} main.cpp )
target_link_libraries( ${BINARY_NAME} ${PROJECT_NAME} )

add_dependencies( ${PROJECT_NAME} ubus_ext_project libubox)

HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES("${PROJECT_NAME}" "" "" "")

set( PACKAGE_NAME firmware-core-${PROJECT_NAME})
install( TARGETS ${BINARY_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
install( PROGRAMS ${SOURCES_ROOT}/system/common/service_files/ha-watcher DESTINATION ${PACKAGE_NAME}/etc/init.d )
install( FILES ${SOURCES_ROOT}/system/common/opt/firmware/etc/watcher.cfg DESTINATION ${PACKAGE_NAME}/opt/firmware/etc )
