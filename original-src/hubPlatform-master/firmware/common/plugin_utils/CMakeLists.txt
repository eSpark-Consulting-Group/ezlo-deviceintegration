cmake_minimum_required( VERSION 3.2 )

get_filename_component( PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME )
project ( ${PROJECT_ID} )

include_directories(
    ${FIRMWARE_COMMON_DIR}
)

file( GLOB SRC_FILES *.cpp )

add_library( ${PROJECT_NAME} ${SRC_FILES} )
target_link_libraries( ${PROJECT_NAME}
    system_utils
    json_utils
    log_utils
)

add_library( ${PROJECT_NAME}_lib STATIC ${SRC_FILES} )
target_link_libraries( ${PROJECT_NAME}_lib
    system_utils_static
    json_utils_lib
    log_utils_lib
)

set( PACKAGE_NAME firmware-common-${PROJECT_NAME} )
install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
