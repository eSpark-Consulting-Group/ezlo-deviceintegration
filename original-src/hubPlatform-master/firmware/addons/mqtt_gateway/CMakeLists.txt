cmake_minimum_required(VERSION 3.2)

get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_ID})

set( PACKAGE_NAME firmware-addons-${PROJECT_NAME})
EXTRACT_PACKAGE_VERSION( FIRMWARE_ADDONS_MQTT_GATEWAY_VERSION ${PACKAGE_NAME} )
CONFIGURE_ARCH_DEPENDENT_FILES(CURRENT_GENERATED_DIR version/mqtt_gateway_addon_version.cpp)

set( ADDON_NAME ha-mqtt_gatewayd )

file( GLOB SRC_FILES
    *.cpp
    ./mqtt_client/*.cpp
    ./spread_bus_wrapper/*.cpp
    ./utils/*.cpp
    ./resources/*.cpp
    ./request_handlers/*.cpp
    ${CURRENT_GENERATED_DIR}/version/mqtt_gateway_addon_version.cpp
    )

include_directories(
    ${FIRMWARE_ADDONS_DIR}
    ${FIRMWARE_COMMON_DIR}
    ${FIRMWARE_CORE_DIR}
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
target_link_libraries( ${PROJECT_NAME}
    addon_interfaces
    mqtt_client
    lmdb
    common_utils
    cored_interface
    log_utils
    tiny
    )

add_executable( ${ADDON_NAME} main.cpp )
target_link_libraries( ${ADDON_NAME}
    ${PROJECT_NAME}
    ${lib_name_rt}
)

HUB_ADD_TESTS(${PROJECT_ID})

install( TARGETS ${ADDON_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin/addons )
install( PROGRAMS ${SOURCES_ROOT}/system/common/service_files/ha-mqtt_gatewayd DESTINATION ${PACKAGE_NAME}/etc/init.d )
INSTALL_ADDON_SERVICE( ${ADDON_NAME} ${PACKAGE_NAME} )

