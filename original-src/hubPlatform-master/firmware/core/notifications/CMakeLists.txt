get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )

include_directories(
    ${FIRMWARE_CORE_DIR}
    ${FIRMWARE_COMMON_DIR}
)

set( BINARY_NAME ha-notificationsd )

set( PACKAGE_NAME firmware-core-${PROJECT_NAME})

file ( GLOB SRC_FILES
    *.cpp
    resources/*.cpp
    notifications_manager/*.cpp
    database_settings/*.cpp
    )

list( REMOVE_ITEM SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp )

link_directories(
    ${LIBRARIES_DIR}
    )

add_library( ${PROJECT_NAME} ${SRC_FILES} )
target_link_libraries( ${PROJECT_NAME}
    ${lib_name_rt}
    service_interfaces
    config_reader
    data_utils
    jsonrpc
    smtp_client
)

# remote interface
file( GLOB INTERFACE_SRC_FILES
    remote_interface/*.cpp
    resources/*.cpp
    database_settings/*.cpp
    )

add_library( ${PROJECT_NAME}_interface SHARED ${INTERFACE_SRC_FILES} )
target_link_libraries( ${PROJECT_NAME}_interface
    service_interfaces
    tiny
    jsonrpc
    dbclient
)

add_executable( ${BINARY_NAME} main.cpp )
target_link_libraries( ${BINARY_NAME}
    ${PROJECT_NAME}
    )

# Tests
HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES( ${PROJECT_NAME} "" "" "" )

install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
install( TARGETS ${PROJECT_NAME}_interface DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
install( TARGETS ${BINARY_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
install( FILES ${SOURCES_ROOT}/system/common/opt/firmware/etc/notifications.cfg DESTINATION ${PACKAGE_NAME}/opt/firmware/etc )

install( PROGRAMS ${SOURCES_ROOT}/system/common/service_files/ha-07_notifications DESTINATION ${PACKAGE_NAME}/etc/init.d )

