cmake_minimum_required(VERSION 3.2)

get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_ID})

set(HUB_WWW_PATH ${HUB_LINUX_BASE_PATH}/www)

set( PACKAGE_NAME firmware-addons-${PROJECT_NAME})
EXTRACT_PACKAGE_VERSION( FIRMWARE_ADDONS_LUUP_VERSION ${PACKAGE_NAME} )
CONFIGURE_ARCH_DEPENDENT_FILES(CURRENT_GENERATED_DIR version/luup_addon_version.cpp)

file( GLOB SRC_FILES
    *.cpp
    vera_plugins/*.cpp
    ${CURRENT_GENERATED_DIR}/version/luup_addon_version.cpp
    )

include_directories(
    ${FIRMWARE_COMMON_DIR}
    ${FIRMWARE_CORE_DIR}
    ${FIRMWARE_ADDONS_DIR}
)

link_directories(${TOOLCHAIN_DEPS_LIBS_FOLDER})
link_directories(
    ${LIBRARIES_DIR}
    )

HUB_FIND_SHARED_LIBRARY_IN_TOOLCHAIN( upnpLIB upnp )
HUB_FIND_SHARED_LIBRARY_IN_TOOLCHAIN( ixmlLIB ixml )
HUB_FIND_SHARED_LIBRARY_IN_TOOLCHAIN( threadutilLIB threadutil )
HUB_FIND_SHARED_LIBRARY_IN_TOOLCHAIN( stdcLIB stdc++ )

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
target_link_libraries(${PROJECT_NAME}
    ${threadutilLIB}
    ${upnpLIB}
    ${ixmlLIB}
    ${threadutilLIB}
    ${mongooseLIB}
    ${curlLIB}
    ${stdcLIB}
    base_utils
    addon_interfaces
    common_utils
    scenes_manager_interface
    jsonrpc
    git_version
)

CONFIGURE_ARCH_DEPENDENT_FILES( CURRNET_GENERATED_DIR main.cpp )

add_executable(ha-luupd ${CURRNET_GENERATED_DIR}/main.cpp)
target_link_libraries(ha-luupd
    ${PROJECT_NAME}
    ${lib_name_rt}
)

# Tests
HUB_ADD_TESTS(${PROJECT_ID})

install( TARGETS ha-luupd DESTINATION ${PACKAGE_NAME}/opt/firmware/bin/addons )
INSTALL_ADDON_SERVICE( ha-luupd ${PACKAGE_NAME} )
