get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_ID})

include_directories(
    ${FIRMWARE_COMMON_DIR}
    ${INCLUDES_ROOT}
    ${SOURCES_ROOT}
    ${DEPS_ROOT}
    ${FIRMWARE_CORE_DIR}
    ${CURRENT_GENERATED_DIR}
)

file (  GLOB_RECURSE SRC_FILES *.cpp *.h )

add_library( ${PROJECT_NAME} ${SRC_FILES} )

target_link_libraries(${PROJECT_NAME}
    modify_scene
    jsonrpc
    cored_interface
    extensions_manager_interface
    cored_model
    cored_group_operations
    scenes_lua_interpreter
)

set( PACKAGE_NAME firmware-core-${PROJECT_NAME})
install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )

