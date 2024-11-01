get_filename_component( PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME )
project ( ${PROJECT_ID} )

set( EXTERNAL_INCLUDES_FOLDER ${TOOLCHAIN_INCLUDES_FOLDER} )
set( EXTERNAL_LIB_FOLDER ${TOOLCHAIN_LIB_FOLDER} )

if( NOT "${STAGING_DIR_LIB_FOLDER}" STREQUAL "NOT-DEFINED" )
    set( EXTERNAL_INCLUDES_FOLDER ${STAGING_DIR_INCLUDE_FOLDER} )
    set( EXTERNAL_LIB_FOLDER ${STAGING_DIR_LIB_FOLDER} )
endif()

include_directories(
    ${FIRMWARE_ADDONS_DIR}
    ${FIRMWARE_CORE_DIR}
    ${FIRMWARE_COMMON_DIR}
    ${EXTERNAL_INCLUDES_FOLDER}/dbus-1.0
    ${EXTERNAL_INCLUDES_FOLDER}/glib-2.0
    ${EXTERNAL_LIB_FOLDER}/dbus-1.0/include
    ${EXTERNAL_LIB_FOLDER}/glib-2.0/include
)

include_directories(
    SYSTEM ${EXTERNAL_INCLUDES_FOLDER}/dbus-1.0
    SYSTEM ${EXTERNAL_INCLUDES_FOLDER}/glib-2.0
    SYSTEM ${EXTERNAL_LIB_FOLDER}/dbus-1.0/include
    SYSTEM ${EXTERNAL_LIB_FOLDER}/glib-2.0/include
)

set( PACKAGE_NAME firmware-addons-${PROJECT_NAME})
EXTRACT_PACKAGE_VERSION( FIRMWARE_ADDONS_BLUETOOTH_VERSION ${PACKAGE_NAME} )
CONFIGURE_ARCH_DEPENDENT_FILES(CURRENT_GENERATED_DIR version/bluetooth_addon_version.cpp)

file( GLOB SRC_FILES
    *.cpp
    proxy_method_helpers/*.cpp
    exceptions/*.cpp
    proxies/*.cpp
    entities/*.cpp
    utils/*.cpp
    script_events/*.h script_events/*.cpp
    ${CURRENT_GENERATED_DIR}/version/bluetooth_addon_version.cpp
)
list( REMOVE_ITEM SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp )

link_directories( ${LIBRARIES_DIR} )

HUB_FIND_SHARED_LIBRARY_IN_TOOLCHAIN( libDBUS dbus-1 )
HUB_FIND_SHARED_LIBRARY_IN_TOOLCHAIN( libGLIB glib-2.0 )

HUB_FIND_LIBRARY_IN_TOOLCHAIN( libGDBUS gdbus )

add_library( ${PROJECT_NAME} ${SRC_FILES} )
target_link_libraries( ${PROJECT_NAME}
    ${lib_name_rt}
    ${libGDBUS}
    ${libDBUS}
    ${libGLIB}
    ${ffiLIB}
    addon_interfaces
    jsonrpc
)

add_library( lua_${PROJECT_NAME} SHARED bindings/bluetooth_lua_bindings.cpp )
target_link_libraries( lua_${PROJECT_NAME}
    ${PROJECT_NAME}
    lua_binding_interface
    )

add_executable( ha-${PROJECT_NAME}d main.cpp )
target_link_libraries( ha-${PROJECT_NAME}d
    ${PROJECT_NAME}
)

# Tests
HUB_ADD_TESTS(${PROJECT_NAME})

install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
install( TARGETS lua_${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
install( TARGETS ha-${PROJECT_NAME}d DESTINATION ${PACKAGE_NAME}/opt/firmware/bin/addons )
INSTALL_ADDON_SERVICE( ha-${PROJECT_NAME}d ${PACKAGE_NAME} )
