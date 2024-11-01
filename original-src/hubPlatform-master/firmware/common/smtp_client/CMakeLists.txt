cmake_minimum_required(VERSION 3.2)

get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_ID})

set( PACKAGE_NAME firmware-common-${PROJECT_NAME})

file( GLOB SRC_FILES
    *.cpp
    )

include_directories(
    ${FIRMWARE_COMMON_DIR}
)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})
target_link_libraries( ${PROJECT_NAME} PRIVATE
    ${lib_name_rt}
    log_utils
    tiny
    system_utils
    ${curlLIB}
    )

add_compile_definitions(SMTP_CERTIFICATES=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/files/certificates\")
add_compile_definitions(SMTP_SERVER_ROOT=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/files/smtp_server\")

install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )

HUB_ADD_TESTS(${PROJECT_NAME})

