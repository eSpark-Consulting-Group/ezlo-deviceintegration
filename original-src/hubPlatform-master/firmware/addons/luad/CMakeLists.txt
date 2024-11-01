cmake_minimum_required( VERSION 3.2 )

get_filename_component( PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME )
project( ${PROJECT_ID} )

include_directories(
    ${FIRMWARE_ADDONS_DIR}
    ${FIRMWARE_CORE_DIR}
    ${FIRMWARE_COMMON_DIR}
    ${FIRMWARE_COMMON_GENERATED_DIR}
)

set( PACKAGE_NAME firmware-addons-lua )
EXTRACT_PACKAGE_VERSION( FIRMWARE_ADDONS_LUAD_VERSION ${PACKAGE_NAME} )
CONFIGURE_ARCH_DEPENDENT_FILES(CURRENT_GENERATED_DIR version/luad_addon_version.cpp)

file( GLOB SRC_FILES
    *.cpp
    addon/*.cpp
    common/*.cpp
    cloud_storage_events/*.cpp
    cloud_storage_events/resources/*.cpp
    interpreter/*.cpp
    interpreter/module_loader/*.cpp
    resources/*.cpp
    ${CURRENT_GENERATED_DIR}/version/luad_addon_version.cpp
)
    
list( REMOVE_ITEM SRC_FILES main.cpp )

file( GLOB_RECURSE OLD_LUUP_SRC
    old_luup/*.cpp
    old_luup/*.h
)

file( GLOB_RECURSE BASE_MODULES_SRC 
    modules/*.cpp
    modules/*.h
    modules/network/dhcp/*.cpp
    modules/network/dhcp/*.h
)

link_directories( ${LIBRARIES_DIR} )

set( BASE_MODULES base_modules )
add_library( ${BASE_MODULES} STATIC ${BASE_MODULES_SRC} )
target_link_libraries( ${BASE_MODULES} events_loop )

add_library( ${PROJECT_NAME} STATIC ${SRC_FILES} ${OLD_LUUP_SRC} )
target_link_libraries( ${PROJECT_NAME}
    lua
    lua_utils
    addon_interfaces
    cored_interface
    common_utils
    system_utils
    system_status
    base_utils
    lua_binding_interface
    extensions_manager_interface
    hardware_manager_interface
    http
    cloud_storage
)

add_executable( ha-${PROJECT_NAME} main.cpp )
target_link_libraries( ha-${PROJECT_NAME} PRIVATE ${PROJECT_NAME} )

# the way we load base lua modules (using `require`) makes us have unreferenced symbols in a static library,
# to prevent the linker from throwing them away a `--whole-archive` flag is used
target_link_libraries( ha-${PROJECT_NAME} PRIVATE
    -Wl,--whole-archive
    ${BASE_MODULES}
    -Wl,--no-whole-archive
)

# tests
HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES( ${PROJECT_NAME} ${BASE_MODULES} ${CURRENT_GENERATED_DIR} "" )

# packaging
install( TARGETS ha-${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin/addons )
install( FILES ${SOURCES_ROOT}/system/common/opt/firmware/etc/luad.cfg DESTINATION ${PACKAGE_NAME}/opt/firmware/etc )
install( PROGRAMS ${SOURCES_ROOT}/system/common/service_files/ha-luad DESTINATION ${PACKAGE_NAME}/etc/init.d )
INSTALL_ADDON_SERVICE( ha-${PROJECT_NAME} ${PACKAGE_NAME} )
