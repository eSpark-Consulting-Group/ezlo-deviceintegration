cmake_minimum_required(VERSION 3.2)

get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_ID})

set( PACKAGE_NAME firmware-common-${PROJECT_NAME})

file( GLOB SRC_FILES
    *.cpp
    )

include_directories(
    ${FIRMWARE_COMMON_DIR}
    ${CMAKE_BINARY_DIR}/deps/pahomqtt/include
)

list(APPEND PAHOMQTT_LIBS
    paho-mqtt3a
    paho-mqtt3c
    paho-mqttpp3
    )

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})
target_link_libraries( ${PROJECT_NAME}
    ${lib_name_rt}
    log_utils
    tiny
    ${PAHOMQTT_LIBS}
    )

add_dependencies(${PROJECT_NAME} pahomqttcpp)

install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )

HUB_ADD_TESTS(${PROJECT_NAME})

