cmake_minimum_required(VERSION 3.2)

get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_ID})

include_directories(
        ${FIRMWARE_COMMON_DIR}
        ${FIRMWARE_CORE_DIR}
)

link_directories(${LIBRARIES_DIR})

file( GLOB SRC_FILES
    *.cpp
    )

add_library( ${PROJECT_NAME} SHARED ${SRC_FILES})
target_link_libraries( ${PROJECT_NAME}
    plugin_utils
    dbid
    json_utils
    system_utils
    cored_model
    )

add_library( ${PROJECT_NAME}_lib STATIC ${SRC_FILES})
target_link_libraries( ${PROJECT_NAME}_lib
    plugin_utils_lib
    dbid_lib
    json_utils_lib
    system_utils_static
    )

set( PACKAGE_NAME firmware-core-extensions_manager_interface )
install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
