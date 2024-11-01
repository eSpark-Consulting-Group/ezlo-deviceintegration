project( networkd )
message( STATUS "${COLOR_INFO}--->${COLOR_NOTICE} Generating project ${COLOR_PROJECT_NAME}${PROJECT_NAME}${COLOR_DOTS}...${COLOR_RESET}" )

file( GLOB NETWORKD_SOURCES
    *.cpp
    configuration/*.cpp
    connection_service/*.cpp
    ssh_tunnel/*.cpp
    interfaces/*.cpp
    monitoring/*.cpp
    resources/*.cpp
    routes/*.cpp
    scan/*.cpp
    gsm/*.cpp
    gsm/ec25/*.cpp
    utils/*.cpp
    communication_test/*.cpp
    )

list( REMOVE_ITEM NETWORKD_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/network.cpp )

if( (${HUB_TYPE} MATCHES "g150") OR
    (${HUB_TYPE} MATCHES "g450") OR
    (${HUB_TYPE} MATCHES "g550")
    )
    list( APPEND NETWORKD_SOURCES scan/nm_vera_wifi_passive_scan_task.cpp )
endif()

include_directories( SYSTEM "${CMAKE_BINARY_DIR}/deps/libnetlink/src/include" )

include_directories(
        ${FIRMWARE_COMMON_DIR}
        ${FIRMWARE_CORE_DIR}
        ${CMAKE_BINARY_DIR}/deps/libnetlink/src/include )

link_directories( ${LIBRARIES_DIR} )

add_library( ${PROJECT_NAME} STATIC ${NETWORKD_SOURCES} )
target_link_libraries( ${PROJECT_NAME}
    git_version
    jsonrpc
    common_utils
    mongoose
    ${allOpenSslLibraries}
    ${lib_name_atomic}
    service_interfaces
    service_metrics_data
    hardware_manager_interface
    cored_interface
    daemon_module
    $<${LIBC_HAS_NSL}:${nslLIB}>
    netlink
    base_utils
    system_utils
    uci_utils
    dbclient
    bus
    ${PROJECT_NAME}_interface
    config_reader
    cloud_storage
)

add_executable( ha-${PROJECT_NAME} network.cpp)
target_link_libraries( ha-${PROJECT_NAME} ${PROJECT_NAME} )

#remote_interface
file( GLOB SRC_FILES remote_interface/*.cpp resources/nm_resources.cpp )
add_library( ${PROJECT_NAME}_interface SHARED ${SRC_FILES} )
target_link_libraries( ${PROJECT_NAME}_interface
    service_interfaces
    common_utils
)

file( GLOB EXTRA_TEST_SOURCES tests/fakes/*.cpp )
HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES( ${PROJECT_NAME} "" "" ${EXTRA_TEST_SOURCES} )

set( PACKAGE_NAME firmware-core-${PROJECT_NAME})
install( TARGETS ha-${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
install( TARGETS ${PROJECT_NAME}_interface DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
install( FILES ${SOURCES_ROOT}/system/common/opt/firmware/etc/network.cfg DESTINATION ${PACKAGE_NAME}/opt/firmware/etc )
install( PROGRAMS ${SOURCES_ROOT}/system/common/service_files/ha-04_networkd DESTINATION ${PACKAGE_NAME}/etc/init.d )
