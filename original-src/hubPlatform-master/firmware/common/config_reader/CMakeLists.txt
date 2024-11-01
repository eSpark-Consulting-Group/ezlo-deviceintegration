cmake_minimum_required(VERSION 3.2)

get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )

link_directories(${LIBRARIES_DIR})
include_directories(
    ${FIRMWARE_COMMON_DIR}
    ${CURRENT_GENERATED_DIR}
)

file ( GLOB SRC_FILES *.cpp )

add_library( ${PROJECT_NAME} ${SRC_FILES} )
target_link_libraries( ${PROJECT_NAME}
    system_utils
)

add_library( ${PROJECT_NAME}_lib STATIC ${SRC_FILES} )
target_link_libraries( ${PROJECT_NAME}_lib
    system_utils_static
)

HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES(${PROJECT_NAME} "" ${FIRMWARE_COMMON_DIR} "")

set( PACKAGE_NAME firmware-common-${PROJECT_NAME})
install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
