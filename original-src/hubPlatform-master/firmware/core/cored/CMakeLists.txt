project(cored)

set( service_name "core" )

add_subdirectory( house_modes )
add_subdirectory( model )
add_subdirectory( group_operations )
add_subdirectory( protect )
add_subdirectory( policies )

include_directories(
    ${FIRMWARE_CORE_DIR}
    ${FIRMWARE_COMMON_DIR}
    ${FIRMWARE_COMMON_GENERATED_DIR}
)

link_directories(${LIBRARIES_DIR})

set( BINARY_NAME ha-${PROJECT_NAME} )

file(GLOB SRC_FILES
    *.cpp *.h
    db_modifier/*.cpp
    script_events/*.cpp
    script_events/resources/*.cpp
    group_operations/handler/*.cpp
    swinger_shutdown/*.cpp
    walk_test/*.cpp
    protect/*.cpp
    policies/handler/*.cpp
)

list(REMOVE_ITEM SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
target_link_libraries(${PROJECT_NAME}
    ${lib_name_rt}
    base_utils
    common_utils
    system_utils
    system_status
    service_interfaces
    update_manager_interface
    ${service_name}d_house_modes
    ${service_name}d_model
    ${service_name}d_group_operations
    ${service_name}d_interface
    ${service_name}d_protect
    ${service_name}d_policies
)

add_executable(${BINARY_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_link_libraries(${BINARY_NAME} ${PROJECT_NAME})

# remote interface
file( GLOB SRC_FILES
    remote_interface/*.cpp
    ${FIRMWARE_COMMON_DIR}/utils/utils.cpp
    ${FIRMWARE_COMMON_DIR}/utils/unit_converter.cpp
    device_adding_wizard.cpp
    what_happened.cpp
    group_operations/remote_interface/*.cpp
    swinger_shutdown/remote_interface/*.cpp
    walk_test/remote_interface/*.cpp
    walk_test/script_events/*.cpp
    protect/remote_interface/*.cpp
    policies/remote_interface/*.cpp
    )

add_library( ${service_name}d_interface SHARED ${SRC_FILES} )
target_link_libraries( ${service_name}d_interface
    ${service_name}d_house_modes
    ${service_name}d_model
    ${service_name}d_group_operations
    ${service_name}d_protect
    ${service_name}d_policies
    base_utils
    service_interfaces
    tiny
    jsonrpc
    dbclient
    device_model
    common_utils
)

# lua
set( SRC_FILES bindings/core_lua_binding.cpp )
add_library( lua_${service_name} MODULE ${SRC_FILES} )
target_link_libraries( lua_${service_name}
    lua_binding_interface
    cored_interface
    ${service_name}d_model
    uid_interface
)

file( GLOB EXTRA_TEST_SOURCES tests/fakes/*.cpp tests/fixtures/*.cpp )
HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES( ${PROJECT_NAME} cored_interface "" "${EXTRA_TEST_SOURCES}" )

# fill settings.cfg file
CONFIGURE_CFG_FILES(CURRENT_GENERATED_CFG_DIR ${SOURCES_ROOT}/system/common/opt/firmware/etc settings.cfg)

# fulfill cored: service executable, etc files, init.d files
set( PACKAGE_NAME firmware-core-${PROJECT_NAME})
install( TARGETS ${BINARY_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )

install( FILES ${CURRENT_GENERATED_CFG_DIR}/settings.cfg DESTINATION ${PACKAGE_NAME}/opt/firmware/etc )
install( FILES ${SOURCES_ROOT}/system/common/opt/firmware/etc/custom_packages/protect.cfg DESTINATION ${PACKAGE_NAME}/opt/firmware/etc/custom_packages )
install( PROGRAMS ${SOURCES_ROOT}/system/common/service_files/ha-02_cored DESTINATION ${PACKAGE_NAME}/etc/init.d )

# fulfill cored_interface: remote interface shared library
set( PACKAGE_NAME firmware-core-${PROJECT_NAME}_interface)
install( TARGETS ${service_name}d_interface DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )

# fulfill lua_core: lua module library
set( LUA_PACKAGE_NAME firmware-core-lua_${PROJECT_NAME})
install( TARGETS lua_${service_name} DESTINATION ${LUA_PACKAGE_NAME}/opt/firmware/lib )
