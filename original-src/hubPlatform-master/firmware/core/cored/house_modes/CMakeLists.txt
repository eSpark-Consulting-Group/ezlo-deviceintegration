get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${service_name}d_${PROJECT_ID})

include_directories(
        ${FIRMWARE_COMMON_DIR}
        ${INCLUDES_ROOT}
        ${SOURCES_ROOT}
        ${DEPS_ROOT}
        ${FIRMWARE_CORE_DIR}
        ${CURRENT_GENERATED_DIR}
)

file ( GLOB SRC_FILES *.cpp *.h resources/*.cpp script_events/*.cpp )

add_library( ${PROJECT_NAME} ${SRC_FILES} )
target_link_libraries( ${PROJECT_NAME} jsonrpc config_reader dbclient cored_model cored_group_operations )

set( SRC_FILES bindings/house_modes_lua_binding.cpp )
add_library( lua_house_modes MODULE ${SRC_FILES} )
target_link_libraries( lua_house_modes
    lua_binding_interface
    cored_interface
    ${service_name}d_model
    uid_interface
)

set( PACKAGE_NAME firmware-core-${PROJECT_NAME})
install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
install( FILES ${SOURCES_ROOT}/system/common/opt/firmware/etc/house_modes.cfg DESTINATION ${PACKAGE_NAME}/opt/firmware/etc )

set( LUA_PACKAGE_NAME firmware-core-lua_house_modes)
install( TARGETS lua_house_modes DESTINATION ${LUA_PACKAGE_NAME}/opt/firmware/lib )
