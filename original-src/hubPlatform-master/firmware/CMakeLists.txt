cmake_minimum_required(VERSION 3.2)
message( STATUS "-- CMAKE is working in \"${CMAKE_CURRENT_SOURCE_DIR}\" " )

if( NOT CMAKE_BUILD_TYPE )
    set( CMAKE_BUILD_TYPE "Release" )
    message( INFO "${COLOR_VAR_NAME}CMAKE_BUILD_TYPE${COLOR_INFO} was not specified in command line parameters. Sorry, but we set it for you to the ${COLOR_VAR_NAME}${CMAKE_BUILD_TYPE}${COLOR_INFO} value.${COLOR_RESET}" )
endif()

set( CMAKE_CXX_STANDARD 11 )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-result -Wno-strict-aliasing -Wno-sequence-point -Winvalid-pch -Werror=return-type " )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WEBSOCKETPP_CPP11_STL_ -D_WEBSOCKETPP_NO_CPP11_REGEX_" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_REENTRANT -D_GNU_SOURCE -D_THREAD_SAFE" )

if( IS_NEED_COVERAGE )
    set( WITH_UNIT_TESTS 1 )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -coverage" )
endif()

message( STATUS "${COLOR_INFO}--->${COLOR_NOTICE} current ${COLOR_VAR_NAME}CMAKE_CXX_FLAGS${COLOR_NOTICE}=${COLOR_VAR_VAL}${CMAKE_CXX_FLAGS}${COLOR_RESET}" )

####################################################################

message( STATUS "${COLOR_NOTICE}**** ${COLOR_ATTENTION}Configuring firmware${COLOR_DOTS}...${COLOR_RESET}" )
message( STATUS "${COLOR_NOTICE}**** ${COLOR_ATTENTION}Building for hub type: ${HUB_TYPE} ${COLOR_RESET}" )

add_definitions( -D__BUILDING_FIRMWARE__=1 )

####################################################################
message( STATUS "${COLOR_INFO}--->${COLOR_NOTICE} current ${COLOR_VAR_NAME}CMAKE_CXX_FLAGS${COLOR_NOTICE}=${COLOR_VAR_VAL}${CMAKE_CXX_FLAGS}${COLOR_RESET}" )

if("${ARCH}" STREQUAL "arm ")
    set(HUB_LINUX_BASE_PATH "")
else()
    set(HUB_LINUX_BASE_PATH "$ENV{HOME}/.hubPlatform")
endif()

set( FIRMWARE_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/core )
set( FIRMWARE_COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/common )
set( FIRMWARE_ADDONS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/addons )
set( FIRMWARE_UTILS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/utils )
set( FIRMWARE_COMPONENT_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/component_tests )

set( FIRMWARE_GENERATED_DIR "${GENERATED_DIR}/firmware" )
set( FIRMWARE_COMMON_GENERATED_DIR "${FIRMWARE_GENERATED_DIR}/common" )

if (NOT HUB_ADDONS_LIB_DIR)
    message( WARNING "HUB_ADDONS_LIB_DIR is not set, CMAKE_LIBRARY_OUTPUT_DIRECTORY will be used" )
    set( HUB_ADDONS_LIB_DIR ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
endif()

if ( CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT )
    set (CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/installed" CACHE PATH "default install path" FORCE )
    message( INFO "${COLOR_VAR_NAME}CMAKE_INSTALL_PREFIX${COLOR_INFO} was not specified in command line parameters. Sorry, but we set it for you to the ${COLOR_VAR_NAME}${CMAKE_INSTALL_PREFIX}${COLOR_INFO} value.${COLOR_RESET}" )
else()
    message( INFO "${COLOR_VAR_NAME}CMAKE_INSTALL_PREFIX${COLOR_INFO} was set to ${COLOR_VAR_NAME}${CMAKE_INSTALL_PREFIX}${COLOR_INFO} value.${COLOR_RESET}" )
endif()

add_custom_target(
    opkg
    COMMAND "${SOURCES_ROOT}/misc/scripts/opkg_mks.sh" --no-clean update
)

add_custom_target(
    opkg_check
    COMMAND "${SOURCES_ROOT}/misc/scripts/opkg_mks.sh" --no-clean check
)

set(DEPS_DIR ${SOURCES_ROOT}/deps)
set(TINY_DIR ${DEPS_DIR}/tiny)
set(LIBZWAY_ZNET_DIR ${DEPS_DIR}/ZNet)
set(EMBER_ZNET_DIR ${DEPS_ROOT}/Zigbee/v2.7)

set(EMBER_ZNET_INCLUDES
        ${EMBER_ZNET_DIR}
        ${EMBER_ZNET_DIR}/app/builder/zigbeegw
        ${EMBER_ZNET_DIR}/platform/base
        ${EMBER_ZNET_DIR}/platform/base/hal
        ${EMBER_ZNET_DIR}/protocol/zigbee
        ${EMBER_ZNET_DIR}/protocol/zigbee/app/framework/include
        ${EMBER_ZNET_DIR}/protocol/zigbee/stack
        ${EMBER_ZNET_DIR}/util/third_party/mbedtls/include
)

include_directories( SYSTEM ${TINY_DIR}/include )
include_directories( SYSTEM ${DEPS_ROOT}/plog/include )

add_subdirectory( common )
add_subdirectory( core )
add_subdirectory( addons )

add_subdirectory( utils )

if (WITH_TESTS)
    add_subdirectory( component_tests )
endif(WITH_TESTS)


set( DAEMONS at88util at88serial ha-infocmd ha-ledcmd fb-zwflash ha-backupd ha-cored ha-cloud_devices ha-extensions_manager ha-external_controllers ha-hardware_manager ha-metrics ha-watcher ha-networkd ha-scenes_manager ha-uid ha-update_manager logcmd migration_tool updatecmd )

message( STATUS "${COLOR_INFO}--->${COLOR_NOTICE} Adding custom target ${COLOR_BROWN}daemons${COLOR_DOTS}...${COLOR_RESET}" )
message( STATUS "${COLOR_INFO}--->${COLOR_NOTICE} Have ${COLOR_VAR_NAME}DAEMONS${COLOR_NOTICE}=${COLOR_VAR_VAL}${DAEMONS}${COLOR_RESET}" )
add_custom_target( daemons DEPENDS ${DAEMONS} )

message( STATUS "${COLOR_NOTICE}**** ${COLOR_ATTENTION}Done, firmware configured${COLOR_DOTS}.${COLOR_RESET}" )
message( STATUS "${COLOR_NOTICE}    ${COLOR_RESET}" )

add_custom_target( pretty
                    COMMAND ${SOURCES_ROOT}/misc/scripts/code_style.py --action format )

add_custom_target( pretty_check
                    COMMAND ${SOURCES_ROOT}/misc/scripts/code_style.py --action check )

set( PACKAGE_NAME firmware-common-tiny)
install( FILES ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libtiny.so DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
install( FILES ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libtiny_consolecolors.so DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
install( FILES ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libtiny_url.so DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )

set( PACKAGE_NAME hub-platform-firmware)

install( PROGRAMS ${SOURCES_ROOT}/system/common/opt/firmware/bin/hz-daemon.sh DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
install( PROGRAMS ${SOURCES_ROOT}/system/common/opt/firmware/bin/spread.sh DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
install( PROGRAMS ${SOURCES_ROOT}/system/common/opt/firmware/bin/spread_watchdog.sh DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
install( PROGRAMS ${SOURCES_ROOT}/system/common/opt/firmware/bin/mosquitto.sh DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
install( PROGRAMS ${SOURCES_ROOT}/system/common/opt/firmware/bin/start.sh DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
install( PROGRAMS ${SOURCES_ROOT}/system/common/opt/firmware/bin/stop.sh DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
install( PROGRAMS ${SOURCES_ROOT}/system/common/opt/firmware/bin/connection-checker.sh DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )

install( CODE "execute_process( COMMAND bash -c \"ln -s /home/data $DESTDIR/${CMAKE_INSTALL_PREFIX}/${PACKAGE_NAME}/opt/firmware/data\" )" )

install( DIRECTORY ${SOURCES_ROOT}/system/common/etc DESTINATION ${PACKAGE_NAME} USE_SOURCE_PERMISSIONS )
install( FILES ${SOURCES_ROOT}/system/common/etc/logrotate.d/firmware DESTINATION ${PACKAGE_NAME}/etc/logrotate.d PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ )
install( DIRECTORY ${SOURCES_ROOT}/system/${HUB_TYPE}/etc/crontabs DESTINATION ${PACKAGE_NAME}/etc USE_SOURCE_PERMISSIONS )

if( ${HUB_TYPE} MATCHES "vhub" )
    install( PROGRAMS ${SOURCES_ROOT}/system/${HUB_TYPE}/usr/sbin/start_services DESTINATION ${PACKAGE_NAME}/usr/sbin )
    install( DIRECTORY ${SOURCES_ROOT}/system/${HUB_TYPE}/etc/init.d DESTINATION ${PACKAGE_NAME}/etc USE_SOURCE_PERMISSIONS )
    install( FILES ${SOURCES_ROOT}/system/common/opt/firmware/etc/spread.conf DESTINATION ${PACKAGE_NAME}/opt/firmware/etc )
endif()

#file( GLOB OPKG_FFMPEG_FILES ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libav* )
#install( FILES ${OPKG_FFMPEG_FILES} DESTINATION firmware-common-libs/lib )

#file( GLOB OPKG_LIBNETLINK_FILES ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libnetlink.so )
#install( FILES ${OPKG_LIBNETLINK_FILES} DESTINATION firmware-common-libs/lib )
