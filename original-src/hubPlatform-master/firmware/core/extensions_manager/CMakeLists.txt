cmake_minimum_required(VERSION 3.2)

get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_ID})

add_subdirectory( plugin_model )


file(GLOB SRC_FILES
        *.cpp
        extensions/*.cpp
        common/*.cpp
        resources/*.cpp
        )
file(GLOB_RECURSE PLUGIN_TYPES_SRC extensions/plugins/*.cpp)

list(APPEND SRC_FILES ${PLUGIN_TYPES_SRC})
list(REMOVE_ITEM SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

include_directories(
        ${FIRMWARE_COMMON_DIR}
        ${FIRMWARE_CORE_DIR}
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
target_link_libraries(${PROJECT_NAME}
        tiny
        ${lib_name_atomic}
        $<${LIBC_HAS_NSL}:${nslLIB}>
        base_utils
        service_interfaces
        cored_interface
        cored_model
        update_manager_interface
        common_utils
        scenes_manager_interface
        plugin_model
        plugin_utils
)

add_executable(ha-${PROJECT_NAME} main.cpp)
target_link_libraries(ha-${PROJECT_NAME}
    ${PROJECT_NAME}
    ${lib_name_rt}
     plugin_model
)

# remote interface
file( GLOB SRC_FILES
        remote_interface/*.cpp
        common/*.cpp
        resources/*.cpp
        )
add_library( ${PROJECT_NAME}_interface SHARED ${SRC_FILES} )
target_link_libraries( ${PROJECT_NAME}_interface
        service_interfaces
        common_utils
        plugin_model
        plugin_utils
)

# lua
set( SRC_FILES bindings/em_lua_binding.cpp )
add_library( lua_${PROJECT_NAME} MODULE ${SRC_FILES} )
target_link_libraries( lua_${PROJECT_NAME}
    lua_binding_interface
    ${PROJECT_NAME}_interface
)

# Tests
HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES(${PROJECT_NAME} "${PROJECT_NAME}_interface" "" "")

set( PACKAGE_NAME firmware-core-${PROJECT_NAME})
install( TARGETS ha-${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )

set( PACKAGE_NAME firmware-core-${PROJECT_NAME}_interface)
install( TARGETS ${PROJECT_NAME}_interface DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )

set( LUA_PACKAGE_NAME firmware-core-lua_${PROJECT_NAME})
install( TARGETS lua_${PROJECT_NAME} DESTINATION ${LUA_PACKAGE_NAME}/opt/firmware/lib )
install( PROGRAMS ${SOURCES_ROOT}/system/common/service_files/ha-09_extensions_manager DESTINATION ${PACKAGE_NAME}/etc/init.d )
