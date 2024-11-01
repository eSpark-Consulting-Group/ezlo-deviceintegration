cmake_minimum_required( VERSION 3.2 )

get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set( PROJECT_NAME scenes_${PROJECT_ID} )
project( ${PROJECT_NAME} )

include_directories(
        ${FIRMWARE_ADDONS_DIR}
        ${FIRMWARE_CORE_DIR}
        ${FIRMWARE_COMMON_DIR}
        ${FIRMWARE_COMMON_GENERATED_DIR}
)

file( GLOB SRC_FILES *.h *.cpp )
link_directories( ${LIBRARIES_DIR} )

add_library( ${PROJECT_NAME} SHARED ${SRC_FILES} )
target_link_libraries( ${PROJECT_NAME}
        lua
        lua_utils
        lua_binding_interface
        base_utils
        )

set( PACKAGE_NAME firmware-core-scenes_engine )
install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
