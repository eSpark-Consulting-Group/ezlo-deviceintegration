cmake_minimum_required(VERSION 3.2)

project( retry_command_manager )

include_directories(
        ${FIRMWARE_COMMON_DIR}
)

file( GLOB SRC_FILES
        *.cpp
        )

add_library( ${PROJECT_NAME}
        ${SRC_FILES}
        )

target_link_libraries( ${PROJECT_NAME} tiny log_utils base_utils service_interfaces )


set( PACKAGE_NAME firmware-common-${PROJECT_NAME})
install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )

# Tests
HUB_ADD_TESTS(${PROJECT_NAME})
