cmake_minimum_required(VERSION 3.2)

get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )

include_directories(
    ${FIRMWARE_COMMON_DIR}
    ${CURRENT_GENERATED_DIR}
)

file ( GLOB SRC_FILES *.cpp )

add_library( ${PROJECT_NAME} STATIC ${SRC_FILES} )
link_directories(${LIBRARIES_DIR})

target_link_libraries( ${PROJECT_NAME} crypto tiny_consolecolors )

HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES(${PROJECT_NAME} "" ${FIRMWARE_COMMON_DIR} "")
