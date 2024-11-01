get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )

include_directories(
    ${FIRMWARE_ADDONS_DIR}
    ${FIRMWARE_CORE_DIR}
    ${FIRMWARE_COMMON_DIR}
    ${FIRMWARE_COMMON_GENERATED_DIR}
)

if (WITH_ZW_NVM_CONVERTER)
    include_directories(${ZIPGATEWAY_INCLUDE_DIRS})
endif()

set( BINARY_NAME  ${HUB_ADDONS_PROJECT_NAME_PREFIX}${PROJECT_NAME}${HUB_ADDONS_PROJECT_NAME_SUFFIX} )

set( PACKAGE_NAME firmware-addons-${PROJECT_NAME})
EXTRACT_PACKAGE_VERSION( FIRMWARE_ADDONS_ZWAVE_VERSION ${PACKAGE_NAME} )
CONFIGURE_ARCH_DEPENDENT_FILES(CURRENT_GENERATED_DIR version/zwave_addon_version.cpp)

file ( GLOB SRC_FILES
    *.cpp
    znet_handlers/*.cpp
    common/*.cpp
    provisioning/*.cpp
    resources/*.cpp
    serialization/*.cpp
    script_events/*.h script_events/*.cpp
    ${CURRENT_GENERATED_DIR}/version/zwave_addon_version.cpp
    )

if (WITH_ZW_NVM_CONVERTER)
    list(APPEND SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/zw_nvm_converter_utils/zw_nvm_converter_utils.cpp)
else()
    list(APPEND SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/zw_nvm_converter_utils/zw_nvm_converter_utils_fake.cpp)
endif()

list(REMOVE_ITEM SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
list(REMOVE_ITEM SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/zwave_lua_bindings.cpp)

file ( GLOB_RECURSE SRC_FILES_EVENTS events_subsystem/*.cpp events_subsystem/*.h )

link_directories(
    ${LIBRARIES_DIR}
    )

add_library( ${PROJECT_NAME} STATIC ${SRC_FILES} ${SRC_FILES_EVENTS} )
target_link_libraries( ${PROJECT_NAME}
    ${lib_name_rt}
    addon_interfaces
    device_firmware_update_manager
    backupd_interface
    hardware_manager_interface
    extensions_manager_interface
    system_utils
    jsonrpc
    znet
)

if (WITH_ZW_NVM_CONVERTER)
    target_link_libraries( ${PROJECT_NAME} zw_nvm_converter_api json-c )
    add_dependencies(${PROJECT_NAME} zipgateway-ext-project)
endif()

add_library( lua_${PROJECT_NAME} SHARED bindings/zwave_lua_bindings.cpp )
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

install( TARGETS lua_${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
install( TARGETS ${BINARY_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin/addons )
install( FILES ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libznet.so DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
install( PROGRAMS ${SOURCES_ROOT}/system/common/service_files/ha-zwaved DESTINATION ${PACKAGE_NAME}/etc/init.d )
INSTALL_ADDON_SERVICE( ${BINARY_NAME} ${PACKAGE_NAME} )
