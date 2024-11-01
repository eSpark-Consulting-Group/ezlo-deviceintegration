get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )

include_directories(
    ${FIRMWARE_COMMON_DIR}
)

CONFIGURE_ARCH_DEPENDENT_FILES( CURRENT_GENERATED_DIR test_environment.cpp )

file ( GLOB SRC_FILES *.cpp
                      component_tests/*.cpp
                      component_tests/ws_server/*.cpp
                      component_tests/http_server/*.cpp
                      component_tests/mqtt_broker/*.cpp
                      ${CURRENT_GENERATED_DIR}/test_environment.cpp )

add_library( ${PROJECT_NAME} ${SRC_FILES} )
target_link_libraries( ${PROJECT_NAME}
    gtest
    system_utils
    service_interfaces
    config_reader
    jsonrpc
    tiny
    http_bridge
)
