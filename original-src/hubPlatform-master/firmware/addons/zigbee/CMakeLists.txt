get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )

include_directories(
    ${FIRMWARE_ADDONS_DIR}
    ${FIRMWARE_CORE_DIR}
    ${FIRMWARE_COMMON_DIR}
    ${EMBER_ZNET_INCLUDES}
    ${GENERATED_DIR}/deps
)

set( BINARY_NAME  ${HUB_ADDONS_PROJECT_NAME_PREFIX}${PROJECT_NAME}${HUB_ADDONS_PROJECT_NAME_SUFFIX} )
set( PACKAGE_NAME firmware-addons-${PROJECT_NAME})
EXTRACT_PACKAGE_VERSION( FIRMWARE_ADDONS_ZIGBEE_VERSION ${PACKAGE_NAME} )
CONFIGURE_ARCH_DEPENDENT_FILES(CURRENT_GENERATED_DIR version/zigbee_addon_version.cpp)

file ( GLOB SRC_FILES
    *.cpp
    model/*.cpp
    model/data_structures/*.cpp
    model/enums/*.cpp
    model/enums/attributes/*.cpp
    model/enums/commands/*.cpp
    handlers/*.cpp
    interview_configurations/*.cpp
    backup/*.cpp
    resources/*.cpp
    utils/*.cpp
    script_events/*.h script_events/*.cpp
    ${CURRENT_GENERATED_DIR}/version/zigbee_addon_version.cpp
    )

list( REMOVE_ITEM SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp )
list( REMOVE_ITEM SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/zigbee_lua_bindings.cpp )

file ( GLOB_RECURSE SRC_FILES_EVENTS events_subsystem/*.cpp events_subsystem/*.h )

link_directories(
    ${LIBRARIES_DIR}
    )

add_library( ${PROJECT_NAME} ${SRC_FILES} ${SRC_FILES_EVENTS} )
target_link_libraries( ${PROJECT_NAME}
    ${lib_name_rt}
    ember_znet
    addon_interfaces
    common_utils
    config_reader
    jsonrpc
    extensions_manager_interface
)

add_library( lua_${PROJECT_NAME} SHARED bindings/zigbee_lua_bindings.cpp )
target_link_libraries( lua_${PROJECT_NAME}
    ${PROJECT_NAME}
    lua_binding_interface
    )

add_executable( ${BINARY_NAME} main.cpp )
target_link_libraries( ${BINARY_NAME}
    ${PROJECT_NAME}
    )

# Tests
HUB_ADD_TESTS(${PROJECT_NAME})

install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
install( TARGETS lua_${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
install( TARGETS ${BINARY_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin/addons )
install( FILES ${SOURCES_ROOT}/system/common/opt/firmware/etc/zigbee.cfg DESTINATION ${PACKAGE_NAME}/opt/firmware/etc )
install( PROGRAMS ${SOURCES_ROOT}/system/common/service_files/ha-zigbeed DESTINATION ${PACKAGE_NAME}/etc/init.d )
INSTALL_ADDON_SERVICE( ${BINARY_NAME} ${PACKAGE_NAME} )
