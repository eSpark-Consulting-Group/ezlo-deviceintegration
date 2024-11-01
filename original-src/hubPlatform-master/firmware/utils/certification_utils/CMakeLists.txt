cmake_minimum_required(VERSION 3.2)

get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )

if( NOT DEFINED HUB_DEPLOY_ZIGBEE )
    message( FATAL_ERROR "CMake variable HUB_DEPLOY_ZIGBEE should be defined" )
endif()

set( WIFI_TEST "wifi_test" )
set( ZWAVE_TEST "zwave_test" )

if( ${HUB_DEPLOY_ZIGBEE} )
set( ZIGBEE_TEST "zigbee_test" )
else()
unset( ZIGBEE_TEST )
endif()

set( SRC_COMMON
        man_page.cpp
        common.cpp
        )

link_directories( ${LIBRARIES_DIR} )

include_directories(
        ${FIRMWARE_COMMON_DIR}
        ${FIRMWARE_UTILS_DIR}
        ${FIRMWARE_CORE_DIR}
        ${EMBER_ZNET_INCLUDES}
        ${GENERATED_DIR}/deps
        )

set( PROJECT_LIBRARIES
        base_utils
        "$<$<BOOL:${HUB_DEPLOY_ZIGBEE}>:ember_znet>"
        )

add_executable( ${WIFI_TEST} wifi_test.cpp ${SRC_COMMON} )
target_link_libraries( ${WIFI_TEST} ${PROJECT_LIBRARIES} )

add_executable( ${ZWAVE_TEST} zwave_test.cpp ${SRC_COMMON} )
target_link_libraries( ${ZWAVE_TEST} ${PROJECT_LIBRARIES} )

if( ${HUB_DEPLOY_ZIGBEE} )
add_executable( ${ZIGBEE_TEST} zigbee_test.cpp ${SRC_COMMON} )
target_link_libraries( ${ZIGBEE_TEST} ${PROJECT_LIBRARIES} )
endif()

set( TARGET_UTILS
    ${WIFI_TEST}
    ${ZWAVE_TEST}
    ${ZIGBEE_TEST}
    )

set( PACKAGE_NAME firmware-dev-${PROJECT_ID})
install( TARGETS ${TARGET_UTILS} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )

if( (${HUB_TYPE} MATCHES "h2.1") OR
    (${HUB_TYPE} MATCHES "h2_secure.1")
    )
    install( PROGRAMS ${SOURCES_ROOT}/system/${HUB_TYPE}/opt/firmware/bin/bt_test DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
    install( PROGRAMS ${SOURCES_ROOT}/system/${HUB_TYPE}/opt/firmware/bin/ble_test DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
endif()