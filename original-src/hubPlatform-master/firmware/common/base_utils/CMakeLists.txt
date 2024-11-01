get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )

include_directories(
    ${FIRMWARE_COMMON_DIR}
)

file ( GLOB SRC_FILES *.cpp
    spi/*.cpp
    uart/*.cpp
    enums/*.cpp
    zwave/*.cpp
    zigbee/*.cpp
    )

HUB_FIND_SHARED_LIBRARY_IN_TOOLCHAIN( minilzo2LIB minilzo2 )

add_library( ${PROJECT_NAME} ${SRC_FILES} )
target_link_libraries( ${PROJECT_NAME}
                        device_model
                        tiny
                        log_utils
                        dbclient
                        bus
                        jsonrpc
                        ${curlLIB}
                        ${minilzo2LIB}
                        )

# Tests
HUB_ADD_TESTS("${PROJECT_NAME}")

set( PACKAGE_NAME firmware-common-${PROJECT_NAME})
install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
