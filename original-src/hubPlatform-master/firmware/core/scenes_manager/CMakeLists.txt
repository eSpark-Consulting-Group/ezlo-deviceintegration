cmake_minimum_required(VERSION 3.2)

get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_ID})


add_subdirectory(scenes_engine)
add_subdirectory(modify_scene)
add_subdirectory(lua_interpreter)

#names
SET(SCENES_ENGINE_LIB "scenes_engine")

file(GLOB SRC_FILES
        *.cpp
        scenes_processor/*.cpp
        scenes_processor/parsers/*.cpp
        scenes_processor/controller_interface/*.cpp
        scenes_processor/response_builders/scenes_block_data_list/*.cpp
        scenes_processor/actions/*.cpp
        scenes_processor/meshbot_templates/*.cpp
        scenes_processor/meshbot_templates/types/*.cpp
        scenes_processor/meshbot_templates/utils/*.cpp
        scenes_processor/meshbot_templates/templates_manager/*.cpp
        scenes_processor/meshbot_templates/meshbot_builder/*.cpp
        )

list(REMOVE_ITEM SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

include_directories(
        ${INCLUDES_ROOT}
        ${SOURCES_ROOT}
        ${DEPS_ROOT}
        ${FIRMWARE_COMMON_DIR}
        ${FIRMWARE_CORE_DIR}
        ${CURRENT_GENERATED_DIR}
        )

add_executable(ha-${PROJECT_NAME} ${SRC_FILES} main.cpp)
target_link_libraries(ha-${PROJECT_NAME}
        tiny
        ${lib_name_atomic}
        service_interfaces
        external_controllers_interface
        ${SCENES_ENGINE_LIB}
        scenes_lua_interpreter
        $<${LIBC_HAS_NSL}:${nslLIB}>
        ${lib_name_rt}
        http
        extensions_manager_interface
        modify_scene
        networkd_interface
        uid_interface
        update_manager_common
        cloud_devices_interface
        retry_command_manager
)

#( remote_interface )
if(${WITH_TESTS} )
    file( GLOB LIB_SRC_FILES
            remote_interface/*.cpp
            test_remote_interface/*.cpp
            tests/resources/*.cpp
            )
else()
    file( GLOB LIB_SRC_FILES
            remote_interface/*.cpp
            )
endif()

add_library( ${PROJECT_NAME}_interface SHARED ${LIB_SRC_FILES} )
target_link_libraries( ${PROJECT_NAME}_interface
        service_interfaces
        cored_interface
        modify_scene
        ${SCENES_ENGINE_LIB}
        )

set(LIBS_SET
    ${PROJECT_NAME}_interface
    ${SCENES_ENGINE_LIB}
    cored_interface
    cored_model
    cored_group_operations
    external_controllers_interface
    modify_scene
    http
    networkd_interface
    uid_interface
    scenes_lua_interpreter
    update_manager_common
    cloud_devices_interface
    retry_command_manager
)

# Tests
file( GLOB EXTRA_TEST_SOURCES ${SRC_FILES} tests/fixtures/*.cpp tests/fakes/*.cpp  )
HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES("ha-${PROJECT_NAME}" "${LIBS_SET}" "" "${EXTRA_TEST_SOURCES}")

# lua
set( service_name "scenes" )
set( SRC_FILES bindings/scenes_manager_lua_binding.cpp
        bindings/resources.cpp)

add_library( lua_${service_name} MODULE ${SRC_FILES} )
target_link_libraries( lua_${service_name}
    lua_binding_interface
    ${PROJECT_NAME}_interface
    )

set( PACKAGE_NAME firmware-core-${PROJECT_NAME})
install( TARGETS ha-${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
install( TARGETS ${PROJECT_NAME}_interface DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
install( TARGETS lua_${service_name} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
install( FILES ${SOURCES_ROOT}/system/common/opt/firmware/etc/scenes_action_metadata.cfg DESTINATION ${PACKAGE_NAME}/opt/firmware/etc )
install( PROGRAMS ${SOURCES_ROOT}/system/common/service_files/ha-08_scenes_manager DESTINATION ${PACKAGE_NAME}/etc/init.d )
