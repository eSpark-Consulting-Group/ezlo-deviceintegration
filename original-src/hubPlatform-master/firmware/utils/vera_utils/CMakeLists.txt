cmake_minimum_required(VERSION 3.2)

get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )

include_directories(
    ${FIRMWARE_COMMON_DIR}
    ${FIRMWARE_CORE_DIR}
    ${FIRMWARE_UTILS_DIR}
)

file( GLOB SRC_FILES *.cpp )

add_library( ${PROJECT_ID} STATIC ${SRC_FILES})
target_link_libraries( ${PROJECT_ID}
    ${lib_name_atomic}
    pthread
    dl ${lib_name_rt}
    stdc++
    gcc_s
    )

HUB_ADD_TESTS( ${PROJECT_NAME} )
