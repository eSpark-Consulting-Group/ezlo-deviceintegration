cmake_minimum_required(VERSION 3.2)

project( device_lib )

include_directories(
        ${FIRMWARE_COMMON_DIR}
)

file( GLOB SRC_FILES
    "${PROJECT_SOURCE_DIR}/*.cpp"
    "${PROJECT_SOURCE_DIR}/*.h"
)

add_library( ${PROJECT_NAME}
    ${SRC_FILES}
)
target_link_libraries( ${PROJECT_NAME} tiny log_utils )
