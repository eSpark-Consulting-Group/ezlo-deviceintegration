cmake_minimum_required(VERSION 3.13)

project(HubPlatform)

# what is going to build
option( WITH_FFMPEG        "Build ffmpeg"                              ON )
option( WITH_X264          "Build x264"                                ON )
option( IS_NEED_COVERAGE   "Add coverage target"                       OFF )
option( HUB_CMAKE_COLORED_OUTPUT "Use special characters for coloring CMake output" ON )
option( HUB_FIRMWARE_COLORED_OUTPUT "Use special characters for coloring firmware output" OFF )
option( HUB_DEPLOY_BLE     "Deploy Bluetooth addon"                    OFF )

option( HUB_ENABLE_ACL    "Check permissions for API requests"         OFF )

set( HUB_WITH_SPEAKER OFF CACHE BOOL "Speaker support")

set( HUB_LOCAL_DIRECTORY          "$ENV{HOME}/.hubPlatform" CACHE STRING "Local directory for working locally" )
set( HUB_LOCAL_DEPLOY_PATH        "${HUB_LOCAL_DIRECTORY}/opt/firmware" CACHE STRING "Directory for deploying firmware locally")
set( HUB_ARM_DEPLOY_IP            "127.0.0.1"               CACHE STRING "IP of ezlo hub for deploying firmware")
set( HUB_ARM_DEPLOY_PORT          "22"                      CACHE STRING "Port of ezlo hub for deploying firmware")
set( HUB_ARM_DEPLOY_ROOT_PASSWORD ""                        CACHE STRING "root password for ssh access to ezlo hub")

set( CMAKE_C_STANDARD 11 )
set( CMAKE_CXX_STANDARD 11 )

set( CMAKE_BUILD_WITH_INSTALL_RPATH    OFF )
set( CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF )
set( CMAKE_SKIP_RPATH                  ON )

# basic folders
get_filename_component( SOURCES_ROOT "." ABSOLUTE )
get_filename_component( DEPS_ROOT "./deps" ABSOLUTE )
get_filename_component( HUB_PLUGINS_ROOT "./deps/HubPlugins" ABSOLUTE )

include("misc/cmake/utils.cmake")

set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
set(TESTS_POLIGON_DIR "${CMAKE_BINARY_DIR}/tests_poligon")

file(MAKE_DIRECTORY "${GENERATED_DIR}")

file(REMOVE_RECURSE "${TESTS_POLIGON_DIR}")
file(MAKE_DIRECTORY "${TESTS_POLIGON_DIR}")

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message( FATAL_ERROR "Please, set CMAKE_TOOLCHAIN_FILE variable to one of toolchaine files: ${SOURCES_ROOT}/misc/cmake/amd64.toolchain.cmake, ${SOURCES_ROOT}/misc/cmake/arm.toolchain.cmake, ${SOURCES_ROOT}/misc/cmake/arm-openwrt.toolchain.cmake, ${SOURCES_ROOT}/misc/cmake/mips.toolchain.cmake, ${SOURCES_ROOT}/misc/cmake/mips-musl.toolchain.cmake." )
endif()

if (NOT BUILD_SHARED_LIBS AND NOT BUILD_STATIC_LIBS)
    set (BUILD_SHARED_LIBS ON)
endif ()

# colors/basic
if(HUB_CMAKE_COLORED_OUTPUT)
    string( ASCII 27 Esc )
    set( COLOR_RESET "${Esc}[0m" ) # No Color
    set( COLOR_BLACK "${Esc}[0;30m" )
    set( COLOR_DARK_GRAY "${Esc}[1;30m" )
    set( COLOR_BLUE "${Esc}[0;34m" )
    set( COLOR_LIGHT_BLUE "${Esc}[1;34m" )
    set( COLOR_GREEN "${Esc}[0;32m" )
    set( COLOR_LIGHT_GREEN "${Esc}[1;32m" )
    set( COLOR_CYAN "${Esc}[0;36m" )
    set( COLOR_LIGHT_CYAN "${Esc}[1;36m" )
    set( COLOR_RED "${Esc}[0;31m" )
    set( COLOR_LIGHT_RED "${Esc}[1;31m" )
    set( COLOR_MAGENTA "${Esc}[0;35m" )
    set( COLOR_LIGHT_MAGENTA "${Esc}[1;35m" )
    set( COLOR_BROWN "${Esc}[0;33m" )
    set( COLOR_YELLOW "${Esc}[1;33m" )
    set( COLOR_LIGHT_GRAY "${Esc}[0;37m" )
    set( COLOR_WHITE "${Esc}[1;37m" )
else()
    set( COLOR_RESET "" )
    set( COLOR_BLACK "" )
    set( COLOR_DARK_GRAY "" )
    set( COLOR_BLUE "" )
    set( COLOR_LIGHT_BLUE "" )
    set( COLOR_GREEN "" )
    set( COLOR_LIGHT_GREEN "" )
    set( COLOR_CYAN "" )
    set( COLOR_LIGHT_CYAN "" )
    set( COLOR_RED "" )
    set( COLOR_LIGHT_RED "" )
    set( COLOR_MAGENTA "" )
    set( COLOR_LIGHT_MAGENTA "" )
    set( COLOR_BROWN "" )
    set( COLOR_YELLOW "" )
    set( COLOR_LIGHT_GRAY "" )
    set( COLOR_WHITE "" )
endif()

# colors/variables
set( COLOR_ERROR "${COLOR_RED}" )
set( COLOR_WARN "${COLOR_YELLOW}" )
set( COLOR_ATTENTION "${COLOR_LIGHT_CYAN}" )
set( COLOR_SUCCESS "${COLOR_GREEN}" )
set( COLOR_INFO "${COLOR_BLUE}" )
set( COLOR_NOTICE "${COLOR_MAGENTA}" )
set( COLOR_DOTS "${COLOR_DARK_GRAY}" )
set( COLOR_SEPARATOR "${COLOR_DARK_GRAY}" )
set( COLOR_VAR_NAME "${COLOR_BLUE}" )
set( COLOR_VAR_DESC "${COLOR_BROWN}" )
set( COLOR_VAR_VAL "${COLOR_LIGHT_GRAY}" )
set( COLOR_PROJECT_NAME "${COLOR_LIGHT_BLUE}" )

if(NOT DEFINED ARCH)
    set(ARCH "amd64" CACHE STRING "Target building architecture")
    set(ARCHValues "amd64;arm;arm-openwrt;mips;mips-musl")
    set_property(CACHE ARCH PROPERTY STRINGS ${ARCHValues})
endif()

# default value is <empty> for any builds than amd64
set(HUB_SERIAL_DEFAULT "")
if(${ARCH} STREQUAL "amd64" )
    set( HUB_SERIAL_DEFAULT "00000000" )
endif()

set(HUB_SERIAL ${HUB_SERIAL_DEFAULT} CACHE STRING "Serial number of local hub ( must be redefined )")
set(HUB_TYPE "a20.1" CACHE STRING "Type of hub which we need to use")
set(HUB_MAC "" CACHE STRING "Mac address of hub which we need to use")
set(HUB_TYPEValues "local;vhub;a20.1;h2.1;h2_secure.1;g150;g450;g550;beewave.1")
set(FIRMWARE_BUILD_NUMBER "0" CACHE STRING "Build number that will be hardcoded to firmware and addons")
set_property( CACHE HUB_TYPE PROPERTY STRINGS ${HUB_TYPEValues} )

set(HUB_FIRMWARE_LOGS_LEVELValues ERROR WARNING INFO DEBUG TRACE)
set(HUB_FIRMWARE_LOGS_LEVEL "DEBUG" CACHE STRING "Default logs level for firmware")
set_property( CACHE HUB_FIRMWARE_LOGS_LEVEL PROPERTY STRINGS ${HUB_FIRMWARE_LOGS_LEVELValues} )

list(FIND HUB_FIRMWARE_LOGS_LEVELValues ${HUB_FIRMWARE_LOGS_LEVEL} index)
if(index EQUAL -1)
    message(FATAL_ERROR "HUB_FIRMWARE_LOGS_LEVEL must be one of ${HUB_FIRMWARE_LOGS_LEVELValues}")
endif()

set( HUB_FIRMWARE_NETWORK_PING_TIMEOUT 10 )

set(ZWAVE_REGION "us" CACHE STRING "Z-Wave region configuration")
set(ZWAVE_REGIONValues "us;eu")
set_property( CACHE ZWAVE_REGION PROPERTY STRINGS ${ZWAVE_REGIONValues} )
add_definitions(-DCONFIG_ZNET_LOG_LEVEL=4)

set(CLOUD_ENVIRONMENT "qa" CACHE STRING "Cloud environment which hub need to use")
set(CLOUD_ENVIRONMENTValues "dev;tiny;qa;stage;production")
set_property( CACHE CLOUD_ENVIRONMENT PROPERTY STRINGS ${CLOUD_ENVIRONMENTValues} )

set( TOOLCHAIN_BIN_FOLDER ${TOOLCHAIN_FOLDER}/bin )
set( TOOLCHAIN_LIB_FOLDER ${TOOLCHAIN_FOLDER}/lib )
set( TOOLCHAIN_INCLUDES_FOLDER ${TOOLCHAIN_FOLDER}/include )

set( TOOLCHAIN_SYSTEM_BIN_FOLDER ${TOOLCHAIN_FOLDER}/${TOOLCHAIN_TARGET_NAME}/bin )
set( TOOLCHAIN_SYSTEM_LIB_FOLDER ${TOOLCHAIN_FOLDER}/${TOOLCHAIN_TARGET_NAME}/lib )
set( TOOLCHAIN_SYSTEM_INCLUDES_FOLDER ${TOOLCHAIN_FOLDER}/${TOOLCHAIN_TARGET_NAME}/include )

set( TOOLCHAIN_DEPS_LIBS_FOLDER ${TOOLCHAIN_FOLDER}/usr/lib )
set( TOOLCHAIN_DEPS_INCLUDES_FOLDER ${TOOLCHAIN_FOLDER}/usr/include )

set( TESTS_DIR ${CMAKE_BINARY_DIR}/tests )

set( CMAKE_PREFIX_PATH ${TOOLCHAIN_FOLDER}
                       ${TOOLCHAIN_FOLDER}/${TOOLCHAIN_TARGET_NAME}
                       ${TOOLCHAIN_FOLDER}/usr)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # Place for binaries and win dlls
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) # Place for libs and shared libs
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) # Place for static libs and win dll import libs
set(CMAKE_SERVICE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/services) # Place for services

set( CMAKE_LINKER       ${TOOLCHAIN_BIN_FOLDER}/${TOOLCHAIN_TOOLS_PREFIX}ld  )
set( CMAKE_AR           ${TOOLCHAIN_BIN_FOLDER}/${TOOLCHAIN_TOOLS_PREFIX}ar       )
set( CMAKE_OBJCOPY      ${TOOLCHAIN_BIN_FOLDER}/${TOOLCHAIN_TOOLS_PREFIX}objcopy  )
set( CMAKE_OBJDUMP      ${TOOLCHAIN_BIN_FOLDER}/${TOOLCHAIN_TOOLS_PREFIX}objdump  )
set( CMAKE_RANLIB       ${TOOLCHAIN_BIN_FOLDER}/${TOOLCHAIN_TOOLS_PREFIX}ranlib   )
set( CMAKE_NM           ${TOOLCHAIN_BIN_FOLDER}/${TOOLCHAIN_TOOLS_PREFIX}nm )
set( STAGING_DIR_INCLUDE_FOLDER "NOT-DEFINED" CACHE STRING "Will be set for deploy environment from cmdline")
set( STAGING_DIR_LIB_FOLDER "NOT-DEFINED" CACHE STRING "Will be set for deploy environment from cmdline")

set( TOOLCHAIN_LIBRARIES_DIR
    "${TOOLCHAIN_LIB_FOLDER}"
    "${TOOLCHAIN_SYSTEM_LIB_FOLDER}"
    "${TOOLCHAIN_DEPS_LIBS_FOLDER}"
)

set( FIRMWARE_LIBRARIES_DIR
    "${CMAKE_BINARY_DIR}/lib/"
)

set( LIBRARIES_DIR
    ${TOOLCHAIN_LIBRARIES_DIR}
    ${FIRMWARE_LIBRARIES_DIR}
)

if( NOT "${STAGING_DIR_LIB_FOLDER}" STREQUAL "NOT-DEFINED" )
    list( APPEND TOOLCHAIN_LIBRARIES_DIR "${STAGING_DIR_LIB_FOLDER}" )
    list( APPEND LIBRARIES_DIR "${STAGING_DIR_LIB_FOLDER}" )
endif()

add_definitions( -D__HUB_SERIAL_STR__="${HUB_SERIAL}" )
add_definitions( -D__HUB_TYPE_STR__="${HUB_TYPE}" )
add_definitions( -D__MAC_ADDRESS_STR__="${HUB_MAC}" )

if( HUB_FIRMWARE_COLORED_OUTPUT )
    add_definitions( -D__EZLO_CONSOLE_COLORS_BY_DEFAULT_ON__ )
    set( HUB_FIRMWARE_LOGS_INDENT 4 )
    set( HUB_FIRMWARE_LOGS_COLOR true )
else()
    set( HUB_FIRMWARE_LOGS_INDENT -1 )
    set( HUB_FIRMWARE_LOGS_COLOR false )
endif()

if( ${HUB_TYPE} MATCHES "g150" OR ${HUB_TYPE} MATCHES "beewave.1" )
    set( HUB_FIRMWARE_LOGS_ROTATION_INTERVAL 1 )
    set( HUB_FIRMWARE_LOGS_ROTATION_SIZE_LIMIT 1 ) # In Mb
    set( HUB_FIRMWARE_LOGS_FILE_SIZE_LIMIT 1024 ) # In Kb
    set( HUB_FIRMWARE_LOGS_PENDING_FOLDER_SIZE_LIMIT 200 ) # In Mb
else()
    set( HUB_FIRMWARE_LOGS_ROTATION_INTERVAL 12 )
    set( HUB_FIRMWARE_LOGS_ROTATION_SIZE_LIMIT 500 ) # In Mb
    set( HUB_FIRMWARE_LOGS_FILE_SIZE_LIMIT 2048 ) # In Kb
    set( HUB_FIRMWARE_LOGS_PENDING_FOLDER_SIZE_LIMIT 256 ) # In Mb
endif()

if( ${HUB_TYPE} MATCHES "h2.1" OR ${HUB_TYPE} MATCHES "h2_secure.1" OR ${HUB_TYPE} MATCHES "beewave.1" )
    set(HUB_DEPLOY_ZIGBEE ON CACHE BOOL "Deploy ZigBee addon")
else()
    set(HUB_DEPLOY_ZIGBEE OFF CACHE BOOL "Deploy ZigBee addon")
endif()

if( ${HUB_TYPE} MATCHES "h2.1" OR ${HUB_TYPE} MATCHES "h2_secure.1" )
    set(WITH_WEBRTC OFF CACHE BOOL "Build WebRTC service")
else()
    set(WITH_WEBRTC OFF CACHE BOOL "Build WebRTC service")
endif()

if(${HUB_TYPE} MATCHES "h2_secure.1")
    set(HUB_DEPLOY_2GIG ON CACHE BOOL "Deploy 2GIG addon")
    set(HUB_WITH_LTE ON CACHE BOOL "Enable LTE support")
    set(HUB_WITH_SPEAKER ON CACHE BOOL "Enable Speaker" FORCE)
else()
    set(HUB_DEPLOY_2GIG OFF CACHE BOOL "Deploy 2GIG addon")
    set(HUB_WITH_LTE OFF CACHE BOOL "Enable LTE support")
endif()

if( ${HUB_TYPE} MATCHES "a20.1" )
    add_definitions( -D__HUB_TYPE__=0x0a20d1 )
    add_definitions( -D__HUB_TYPE_A20__=1 )

elseif(${HUB_TYPE} MATCHES "h2.1" )
    add_definitions( -D__HUB_TYPE__=0x2d1 )
    add_definitions( -D__HUB_TYPE_H2__=1 )

elseif(${HUB_TYPE} MATCHES "h2_secure.1" )
    add_definitions( -D__HUB_TYPE__=0x273d1 )  # 0x02 - h2, 0x73 - s, 0x0d - ., 0x01 - 1
    add_definitions( -D__HUB_TYPE_H2_SECURE__=1 )
    add_definitions( -D__HUB_WITH_SPEAKER__=1)

elseif(${HUB_TYPE} MATCHES "g150" )
    add_definitions( -D__HUB_TYPE__=0x96 )
    add_definitions( -D__HUB_TYPE_G150__=1 )

elseif(${HUB_TYPE} MATCHES "g450" )
    add_definitions( -D__HUB_TYPE__=0x1c2 )
    add_definitions( -D__HUB_TYPE_G450__=1 )

elseif(${HUB_TYPE} MATCHES "g550" )
    add_definitions( -D__HUB_TYPE__=0x226 )
    add_definitions( -D__HUB_TYPE_G550__=1 )

elseif(${HUB_TYPE} MATCHES "beewave.1" )
    add_definitions( -D__HUB_TYPE__=0x6277 )
    add_definitions( -D__HUB_TYPE_BEEWAVE__=1 )

elseif(${HUB_TYPE} MATCHES "local" )
    add_definitions( -D__HUB_TYPE__=0x0a20d2 )
    add_definitions( -D__HUB_TYPE_LOCAL__=1 )
    if(${HUB_WITH_SPEAKER})
        add_definitions( -D__HUB_WITH_SPEAKER__=1)
    endif()

elseif(${HUB_TYPE} MATCHES "vhub" )
    add_definitions( -D__HUB_TYPE__=0x7668 ) # 0x76 - v, 0x68 - h
    add_definitions( -D__HUB_TYPE_VHUB__=1 )

else()
    message( FATAL_ERROR "${COLOR_VAR_NAME}HUB_TYPE${COLOR_ERROR} is set to strange unsupported value: ${COLOR_VAR_VAL}${HUB_TYPE}${COLOR_RESET}" )
endif()

set( HUB_DEPLOY_COMMON_PARAMS BUILD_DIR=${CMAKE_BINARY_DIR} ZWAVE_REGION=${ZWAVE_REGION} HUB_TYPE=${HUB_TYPE} CLOUD_ENVIRONMENT=${CLOUD_ENVIRONMENT} TOOLCHAIN_PATH=${TOOLCHAIN_FOLDER} TOOLCHAIN_TARGET_NAME=${TOOLCHAIN_TARGET_NAME} )

if ( HUB_DEPLOY_BLE )
    set( HUB_DEPLOY_COMMON_PARAMS ${HUB_DEPLOY_COMMON_PARAMS} WITH_BLE=1 )
else()
    set( HUB_DEPLOY_COMMON_PARAMS ${HUB_DEPLOY_COMMON_PARAMS} WITH_BLE=0 )
endif()

if ( HUB_DEPLOY_ZIGBEE )
    set( HUB_DEPLOY_COMMON_PARAMS ${HUB_DEPLOY_COMMON_PARAMS} WITH_ZIGBEE=1 )
else()
    set( HUB_DEPLOY_COMMON_PARAMS ${HUB_DEPLOY_COMMON_PARAMS} WITH_ZIGBEE=0 )
endif()

if ( HUB_DEPLOY_2GIG )
    set( HUB_DEPLOY_COMMON_PARAMS ${HUB_DEPLOY_COMMON_PARAMS} WITH_2GIG=1 )
else()
    set( HUB_DEPLOY_COMMON_PARAMS ${HUB_DEPLOY_COMMON_PARAMS} WITH_2GIG=0 )
endif()

if ( HUB_ENABLE_ACL )
    add_definitions( -D__HUB_ENABLE_ACL__=1 )
endif()

if ( WITH_WEBRTC )
    add_definitions( -D__HUB_WITH_WEBRTC__=1 )
endif()

set( CROSSPLATFORM_COMPILE_FLAGS "-Wno-psabi -pthread -ldl" )
if( ${ARCH} STREQUAL "arm" )
    set(WITH_TESTS OFF CACHE BOOL "Build tests")

    add_definitions( -D__EZLO_BUILDING_TARGET_ARCH_ARM__=1 )
    add_definitions( -D__HUB_BUILDING_TARGET_ARCH_ARM__=1 )

    include_directories( SYSTEM "${TOOLCHAIN_INCLUDES_FOLDER}" )

    set( LIBC_HAS_NSL 1 )

    set( COMMON_COMPILE_FLAGS "${CROSSPLATFORM_COMPILE_FLAGS} -Wl,--as-needed -Wl,--no-undefined -Wl,--no-allow-shlib-undefined -Wl,-rpath=/opt/firmware/lib -Wl,-rpath=/lib/arm-linux-gnueabihf" )
    set( HUB_FIRMWARE_DIR /opt/firmware )
    set( HUB_ADDONS_LIB_DIR /opt/firmware/lib )

    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_COMPILE_FLAGS}" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_COMPILE_FLAGS}" )

    set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s -ffunction-sections -fdata-sections -Wl,--gc-sections")

    set( CONFIG_DIR "/opt/firmware/etc" )
    set( LOG_DIR "/var/log/firmware" )

    set( DEPLOY_PASSWORD_OPTIONS "" )
    if( NOT "${HUB_ARM_DEPLOY_ROOT_PASSWORD}" STREQUAL "" )
        set( DEPLOY_PASSWORD_OPTIONS "PASSWORD='${HUB_ARM_DEPLOY_ROOT_PASSWORD}'" )
    endif()

    add_custom_target(
            deploy
            COMMAND echo "deploy.sh ${HUB_DEPLOY_COMMON_PARAMS} HUB_IP=${HUB_ARM_DEPLOY_IP} HUB_PORT=${HUB_ARM_DEPLOY_PORT} ${DEPLOY_PASSWORD_OPTIONS}"
            COMMAND "${SOURCES_ROOT}/misc/scripts/deploy.sh" ${HUB_DEPLOY_COMMON_PARAMS} HUB_IP=${HUB_ARM_DEPLOY_IP} HUB_PORT=${HUB_ARM_DEPLOY_PORT} ${DEPLOY_PASSWORD_OPTIONS}
    )

elseif(${ARCH} STREQUAL "amd64" )
    add_definitions( -D__EZLO_BUILDING_TARGET_ARCH_X86_OR_X64__=1 )
    add_definitions( -D__HUB_BUILDING_TARGET_ARCH_AMD64__=1 )

    set( LIBC_HAS_NSL 1 )

    if(${HUB_TYPE} MATCHES "vhub" )
        set(WITH_TESTS OFF CACHE BOOL "Build tests")

        include_directories( SYSTEM "${TOOLCHAIN_INCLUDES_FOLDER}" )
        set( COMMON_COMPILE_FLAGS "${CROSSPLATFORM_COMPILE_FLAGS} -Wl,--as-needed -Wl,--no-undefined -Wl,--no-allow-shlib-undefined -fPIC -Wl,-rpath=/opt/firmware/lib -Wl,-rpath=/lib" )
        set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s -ffunction-sections -fdata-sections -Wl,--gc-sections")

        set( HUB_FIRMWARE_DIR /opt/firmware )
        set( CONFIG_DIR "/opt/firmware/etc" )
        set( LOG_DIR "/var/log/firmware" )

    else() # local
        set(WITH_TESTS ON CACHE BOOL "Build tests")

        set( COVERAGE_COMPILE_FLAGS "" )
        if ( ENABLE_COVERAGE )
            set( COVERAGE_COMPILE_FLAGS "--coverage" )
        endif()
        set( COMMON_COMPILE_FLAGS "${CROSSPLATFORM_COMPILE_FLAGS} ${COVERAGE_COMPILE_FLAGS} -Wl,--as-needed -Wl,--no-undefined -Wl,--no-allow-shlib-undefined -fPIC -Wl,-rpath=${CMAKE_LIBRARY_OUTPUT_DIRECTORY} -Wl,-rpath=${TOOLCHAIN_SYSTEM_LIB_FOLDER} -Wl,-rpath=${TOOLCHAIN_DEPS_LIBS_FOLDER}" )

        set( HUB_FIRMWARE_DIR ${HUB_LOCAL_DEPLOY_PATH} )
        set( HUB_ADDONS_LIB_DIR ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
        set( CONFIG_DIR "${HUB_LOCAL_DEPLOY_PATH}/etc" )
        set( LOG_DIR "${HUB_LOCAL_DIRECTORY}/var/log/firmware" )

        add_custom_target(
                deploy
                COMMAND echo "deploy.sh ${HUB_DEPLOY_COMMON_PARAMS} LOCAL_DEPLOY_PATH=${HUB_LOCAL_DEPLOY_PATH} LOG_DIR=${LOG_DIR}"
                COMMAND "${SOURCES_ROOT}/misc/scripts/deploy.sh" ${HUB_DEPLOY_COMMON_PARAMS} LOCAL_DEPLOY_PATH=${HUB_LOCAL_DEPLOY_PATH} LOG_DIR=${LOG_DIR}
        )
    endif()

    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_COMPILE_FLAGS}" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_COMPILE_FLAGS}" )

    set(WITH_JSON_C 1)
    set(WITH_ZW_NVM_CONVERTER 1)

    if (WITH_TESTS)
        #HubPlugins tests

        set(PLUGINS_BUILD_DIR ${CMAKE_BINARY_DIR}/deps/HubPlugins/)
        set(PLUGINS_MAKE $(MAKE) BUILD_DIR=${PLUGINS_BUILD_DIR})

        add_custom_target(plugins_tests_target ALL
            COMMAND mkdir -p ${PLUGINS_BUILD_DIR}
            WORKING_DIRECTORY ${HUB_PLUGINS_ROOT}
            COMMAND ${PLUGINS_MAKE} devices 
            COMMAND ${PLUGINS_MAKE} compile_tests_runner
        )

        list(TRANSFORM PLUGINS_MAKE REPLACE "\\$\\(MAKE\\)" "make")
        add_test(NAME plugins_test 
                 COMMAND ${PLUGINS_MAKE} run_tests
                 WORKING_DIRECTORY ${HUB_PLUGINS_ROOT})
    endif(WITH_TESTS)

elseif(${ARCH} STREQUAL "arm-openwrt" )
    set(WITH_TESTS OFF CACHE BOOL "Build tests")

    add_definitions( -D__EZLO_BUILDING_TARGET_ARCH_ARM__=1 )
    add_definitions( -D__HUB_BUILDING_TARGET_ARCH_ARM__=1 )

    add_definitions( -D_LIBCPP_HAS_MUSL_LIBC=1 )
    set( LIBC_HAS_NSL 0 )
    set( COMMON_COMPILE_FLAGS "-mword-relocations ${CROSSPLATFORM_COMPILE_FLAGS} -Wl,--as-needed -Wl,--no-undefined -Wl,--no-allow-shlib-undefined -Wl,-rpath=/opt/firmware/lib -Wl,-rpath=/lib -Wl,-rpath=/usr/lib -L${TOOLCHAIN_LIB_FOLDER} -L${TOOLCHAIN_SYSTEM_LIB_FOLDER} -L${TOOLCHAIN_DEPS_LIBS_FOLDER} -isystem ${TOOLCHAIN_DEPS_INCLUDES_FOLDER}" )

    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_COMPILE_FLAGS}" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_COMPILE_FLAGS}" )
    set( CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -pipe -mcpu=cortex-a7 -mfpu=neon-vfpv4 -fno-caller-saves -fno-plt -fhonour-copts -Wno-error=unused-but-set-variable -Wno-error=unused-result -mfloat-abi=hard -Wformat -Werror=format-security -fstack-protector -D_FORTIFY_SOURCE=1 -Wl,-z,now -Wl,-z,relro -s -ffunction-sections -fdata-sections -Wl,--gc-sections -znow -zrelro -Wl,-Bsymbolic-functions" )
    set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s -ffunction-sections -fdata-sections -Wl,--gc-sections")

    set( HUB_FIRMWARE_DIR /opt/firmware )
    set( HUB_ADDONS_LIB_DIR /opt/firmware/lib )
    set( CONFIG_DIR "/opt/firmware/etc" )
    set( LOG_DIR "/var/log/firmware" )

    set( DEPLOY_PASSWORD_OPTIONS "" )
    if( NOT "${HUB_ARM_DEPLOY_ROOT_PASSWORD}" STREQUAL "" )
        set( DEPLOY_PASSWORD_OPTIONS "PASSWORD='${HUB_ARM_DEPLOY_ROOT_PASSWORD}'" )
    endif()

    add_custom_target(
        deploy
        COMMAND echo "deploy.sh ${HUB_DEPLOY_COMMON_PARAMS} HUB_IP=${HUB_ARM_DEPLOY_IP} HUB_PORT=${HUB_ARM_DEPLOY_PORT} ${DEPLOY_PASSWORD_OPTIONS}"
        COMMAND "${SOURCES_ROOT}/misc/scripts/deploy.sh" ${HUB_DEPLOY_COMMON_PARAMS} HUB_IP=${HUB_ARM_DEPLOY_IP} HUB_PORT=${HUB_ARM_DEPLOY_PORT} ${DEPLOY_PASSWORD_OPTIONS}
    )

    set(WITH_JSON_C 1)
    set(WITH_ZW_NVM_CONVERTER 1)

elseif(${ARCH} STREQUAL "mips" )
    set(WITH_TESTS OFF CACHE BOOL "Build tests")

    add_definitions( -D__HUB_BUILDING_TARGET_ARCH_MIPS__=1 )
    add_definitions( -D__STDC_FORMAT_MACROS )
    add_definitions( -D_GLIBCXX_USE_C99 )

    set( LIBC_HAS_NSL 1 )

    set( OPENWRT_COMPILE_FLAGS "-pipe -mno-branch-likely -mips32r2 -mtune=24kec -mdsp -fno-caller-saves -fhonour-copts -Wno-error=unused-but-set-variable -msoft-float")

    set( COMMON_COMPILE_FLAGS "${OPENWRT_COMPILE_FLAGS} ${CROSSPLATFORM_COMPILE_FLAGS} -fPIC -Wl,--as-needed -Wl,--no-undefined -Wl,-rpath=/opt/firmware/lib -Wl,-rpath=/lib -Wl,-rpath=/usr/lib -L${TOOLCHAIN_LIB_FOLDER} -L${TOOLCHAIN_SYSTEM_LIB_FOLDER} -L${TOOLCHAIN_DEPS_LIBS_FOLDER} -isystem ${TOOLCHAIN_DEPS_INCLUDES_FOLDER}" )

    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_COMPILE_FLAGS}" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_COMPILE_FLAGS}" )
    set( CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Wformat -Werror=format-security -s -ffunction-sections -fdata-sections -Wl,--gc-sections -Wl,-Bsymbolic-functions" )
    set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s -ffunction-sections -fdata-sections -Wl,--gc-sections")

    set( HUB_FIRMWARE_DIR /opt/firmware )
    set( HUB_ADDONS_LIB_DIR /opt/firmware/lib )
    set( CONFIG_DIR "/opt/firmware/etc" )
    set( LOG_DIR "/var/log/firmware" )

    set( DEPLOY_PASSWORD_OPTIONS "" )
    if( NOT "${HUB_ARM_DEPLOY_ROOT_PASSWORD}" STREQUAL "" )
        set( DEPLOY_PASSWORD_OPTIONS "PASSWORD='${HUB_ARM_DEPLOY_ROOT_PASSWORD}'" )
    endif()

    add_custom_target(
        deploy
        COMMAND echo "deploy.sh ${HUB_DEPLOY_COMMON_PARAMS} HUB_IP=${HUB_ARM_DEPLOY_IP} HUB_PORT=${HUB_ARM_DEPLOY_PORT} ${DEPLOY_PASSWORD_OPTIONS}"
        COMMAND "${SOURCES_ROOT}/misc/scripts/deploy.sh" ${HUB_DEPLOY_COMMON_PARAMS} HUB_IP=${HUB_ARM_DEPLOY_IP} HUB_PORT=${HUB_ARM_DEPLOY_PORT} ${DEPLOY_PASSWORD_OPTIONS}
    )
elseif(${ARCH} STREQUAL "mips-musl" )
    set(WITH_TESTS OFF CACHE BOOL "Build tests")

    add_definitions( -D__HUB_BUILDING_TARGET_ARCH_MIPS_MUSL__=1 )
    add_definitions( -D__STDC_FORMAT_MACROS )
    add_definitions( -D_GLIBCXX_USE_C99 )

    add_definitions( -D_LIBCPP_HAS_MUSL_LIBC=1 )
    set( LIBC_HAS_NSL 0 )

    set( OPENWRT_COMPILE_FLAGS "-pipe -mno-branch-likely -mips32r2 -mtune=24kec -mdsp -fno-caller-saves -fhonour-copts -Wno-error=unused-but-set-variable -msoft-float")

    set( COMMON_COMPILE_FLAGS "${OPENWRT_COMPILE_FLAGS} ${CROSSPLATFORM_COMPILE_FLAGS} -fPIC -Wl,--as-needed -Wl,--no-undefined -Wl,--no-allow-shlib-undefined -Wl,-rpath=/opt/firmware/lib -Wl,-rpath=/lib -Wl,-rpath=/usr/lib -L${TOOLCHAIN_LIB_FOLDER} -L${TOOLCHAIN_SYSTEM_LIB_FOLDER} -L${TOOLCHAIN_DEPS_LIBS_FOLDER} -isystem ${TOOLCHAIN_DEPS_INCLUDES_FOLDER}" )

    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_COMPILE_FLAGS}" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_COMPILE_FLAGS}" )
    set( CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -fno-plt -Wno-error=unused-result -Wformat -Werror=format-security -fstack-protector -D_FORTIFY_SOURCE=1 -Wl,-z,now -Wl,-z,relro -s -ffunction-sections -fdata-sections -Wl,--gc-sections -znow -zrelro -Wl,-Bsymbolic-functions" )
    set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s -ffunction-sections -fdata-sections -Wl,--gc-sections")

    set( HUB_FIRMWARE_DIR /opt/firmware )
    set( HUB_ADDONS_LIB_DIR /opt/firmware/lib )
    set( CONFIG_DIR "/opt/firmware/etc" )
    set( LOG_DIR "/var/log/firmware" )

    set( DEPLOY_PASSWORD_OPTIONS "" )
    if( NOT "${HUB_ARM_DEPLOY_ROOT_PASSWORD}" STREQUAL "" )
        set( DEPLOY_PASSWORD_OPTIONS "PASSWORD='${HUB_ARM_DEPLOY_ROOT_PASSWORD}'" )
    endif()

    add_custom_target(
        deploy
        COMMAND echo "deploy.sh ${HUB_DEPLOY_COMMON_PARAMS} HUB_IP=${HUB_ARM_DEPLOY_IP} HUB_PORT=${HUB_ARM_DEPLOY_PORT} ${DEPLOY_PASSWORD_OPTIONS}"
        COMMAND "${SOURCES_ROOT}/misc/scripts/deploy.sh" ${HUB_DEPLOY_COMMON_PARAMS} HUB_IP=${HUB_ARM_DEPLOY_IP} HUB_PORT=${HUB_ARM_DEPLOY_PORT} ${DEPLOY_PASSWORD_OPTIONS}
    )

else()
    message( FATAL_ERROR "${COLOR_VAR_NAME}ARCH${COLOR_ERROR} is set to strange unsupported value: ${ARCH}" )
endif()

message( STATUS "-- CMAKE is working in \"${CMAKE_CURRENT_SOURCE_DIR}\" " )

# CMAKE_BUILD_TYPE needs to be known in any case
if( NOT CMAKE_BUILD_TYPE )
    set( CMAKE_BUILD_TYPE "Release" )
    message( STATUS "${COLOR_VAR_NAME}CMAKE_BUILD_TYPE${COLOR_RESET} was not specified in command line parameters. Sorry, but we set it for you to the ${COLOR_VAR_NAME}${CMAKE_BUILD_TYPE}${COLOR_RESET} value." )
endif()

# system we are building on
execute_process( COMMAND uname OUTPUT_VARIABLE UNIX_SYSTEM_NAME )
string( STRIP "${UNIX_SYSTEM_NAME}" UNIX_SYSTEM_NAME )
message( STATUS "The ${COLOR_VAR_NAME}UNIX_SYSTEM_NAME${COLOR_RESET} variable was programmatically set to ${COLOR_VAR_VAL}${UNIX_SYSTEM_NAME}${COLOR_RESET}" )

# debug or release
set( DIR_PART_AS_BUILD_TYPE "Release" )
if( CMAKE_BUILD_TYPE MATCHES Debug )
    add_definitions( -D_DEBUG=1 )
    set( DIR_PART_AS_BUILD_TYPE "Debug" )
endif()
if( CMAKE_BUILD_TYPE MATCHES Release )
    add_definitions( -DNDEBUG=1 )
endif()

# logging options
option( DEBUG_LOGGING "Enable debug messages" FALSE )
if( DEBUG_LOGGING )
    add_definitions( -D__DEBUG_LOGGING__=1 )
endif()

option( ERROR_LOGGING "Enable error messages" FALSE )
if( ERROR_LOGGING OR CMAKE_BUILD_TYPE MATCHES Debug )
    add_definitions( -D__ERROR_LOGGING__=1 )
endif()

option( NETWORK_TRAFFIC_LOGGING "Enable network traffic messages" FALSE )
if( NETWORK_TRAFFIC_LOGGING )
    add_definitions( -D__NETWORK_TRAFFIC_LOGGING__=1 )
endif()

option( LOGGER_LOG_THREAD_ID "Print a tid (thread id) for each log message line" FALSE )
if( LOGGER_LOG_THREAD_ID )
    add_definitions( -D__LOGGER_LOG_THREAD_ID__=1 )
endif()

# ARCH-specific dependent definitions

if( "${STAGING_DIR_INCLUDE_FOLDER}" STREQUAL "NOT-DEFINED" )
    include_directories(
            SYSTEM "${TOOLCHAIN_DEPS_INCLUDES_FOLDER}"
            SYSTEM "${DEPS_ROOT}" )
else()
    include_directories(
            SYSTEM "${TOOLCHAIN_DEPS_INCLUDES_FOLDER}"
            SYSTEM "${STAGING_DIR_INCLUDE_FOLDER}"
            SYSTEM "${DEPS_ROOT}" )
endif()

if( ${WITH_TESTS} )
    ENABLE_TESTING()
    list( APPEND LIBRARIES_DIR "${TESTS_DIR}" )
endif()

link_directories( ${LIBRARIES_DIR} )

# let compiler know we are building with CMAKE
add_definitions( -D__EZLO_BUILDING_WITH_CMAKE__=1 )

# parallel build
if( NOT "${PARALLEL_COUNT}" )
    set( PARALLEL_COUNT "1" )
endif()

# show what we have configured
message( STATUS "${COLOR_VAR_NAME}UNIX_SYSTEM_NAME${COLOR_DOTS}..........................${COLOR_VAR_DESC}Building on host${COLOR_DOTS}.............${COLOR_VAR_VAL}${UNIX_SYSTEM_NAME}${COLOR_RESET}" )
message( STATUS "${COLOR_VAR_NAME}ARCH${COLOR_DOTS}......................................${COLOR_VAR_DESC}Architecture${COLOR_DOTS}.................${COLOR_VAR_VAL}${ARCH}${COLOR_RESET}" )
message( STATUS "${COLOR_VAR_NAME}CMAKE_TOOLCHAIN_FILE${COLOR_DOTS}......................${COLOR_VAR_VAL}${CMAKE_TOOLCHAIN_FILE}${COLOR_RESET}" )
message( STATUS "${COLOR_VAR_NAME}PARALLEL_COUNT${COLOR_DOTS}............................${COLOR_VAR_VAL}${PARALLEL_COUNT}${COLOR_RESET}" )
message( STATUS "${COLOR_VAR_NAME}CMAKE_BUILD_TYPE${COLOR_DOTS}..........................${COLOR_VAR_VAL}${CMAKE_BUILD_TYPE}${COLOR_RESET}" )
message( STATUS "${COLOR_VAR_NAME}CMAKE_CURRENT_SOURCE_DIR${COLOR_DOTS}..................${COLOR_VAR_VAL}${CMAKE_CURRENT_SOURCE_DIR}${COLOR_RESET}" )
message( STATUS "${COLOR_VAR_NAME}SOURCES_ROOT${COLOR_DOTS}..............................${COLOR_VAR_DESC}Libraries source directory${COLOR_DOTS}...${COLOR_VAR_VAL}${SOURCES_ROOT}${COLOR_RESET}" )
message( STATUS "${COLOR_VAR_NAME}DEPS_ROOT${COLOR_DOTS}.................................${COLOR_VAR_VAL}${DEPS_ROOT}${COLOR_RESET}" )
message( STATUS "${COLOR_VAR_NAME}HUB_TYPE${COLOR_DOTS}..................................${COLOR_VAR_VAL}${HUB_TYPE}${COLOR_RESET}" )
message( STATUS "${COLOR_VAR_NAME}HUB_FIRMWARE_LOGS_LEVEL${COLOR_DOTS}...................${COLOR_VAR_VAL}${HUB_FIRMWARE_LOGS_LEVEL}${COLOR_RESET}" )
message( STATUS "${COLOR_VAR_NAME}FIRMWARE_BUILD_NUMBER${COLOR_DOTS}.....................${COLOR_VAR_VAL}${FIRMWARE_BUILD_NUMBER}${COLOR_RESET}" )
message( STATUS "${COLOR_VAR_NAME}ZWAVE_REGION${COLOR_DOTS}..............................${COLOR_VAR_VAL}${ZWAVE_REGION}${COLOR_RESET}" )
message( STATUS "${COLOR_VAR_NAME}CMAKE_C_COMPILER${COLOR_DOTS}..........................${COLOR_VAR_VAL}${CMAKE_C_COMPILER}${COLOR_RESET}" )
message( STATUS "${COLOR_VAR_NAME}CMAKE_CXX_COMPILER${COLOR_DOTS}........................${COLOR_VAR_VAL}${CMAKE_CXX_COMPILER}${COLOR_RESET}" )
message( STATUS "${COLOR_VAR_NAME}CMAKE_CXX_LINK_EXECUTABLE${COLOR_DOTS}.................${COLOR_VAR_VAL}${CMAKE_CXX_LINK_EXECUTABLE}${COLOR_RESET}" )
message( STATUS "${COLOR_VAR_NAME}CMAKE_LINKER${COLOR_DOTS}..............................${COLOR_VAR_VAL}${CMAKE_LINKER}${COLOR_RESET}" )
message( STATUS "${COLOR_VAR_NAME}CMAKE_AR${COLOR_DOTS}..................................${COLOR_VAR_VAL}${CMAKE_AR}${COLOR_RESET}" )
message( STATUS "${COLOR_VAR_NAME}CMAKE_RANLIB${COLOR_DOTS}..............................${COLOR_VAR_VAL}${CMAKE_RANLIB}${COLOR_RESET}" )
message( STATUS "${COLOR_VAR_NAME}CMAKE_OBJCOPY${COLOR_DOTS}.............................${COLOR_VAR_VAL}${CMAKE_OBJCOPY}${COLOR_RESET}" )
message( STATUS "${COLOR_VAR_NAME}CMAKE_OBJDUMP${COLOR_DOTS}.............................${COLOR_VAR_VAL}${CMAKE_OBJDUMP}${COLOR_RESET}" )
message( STATUS "${COLOR_VAR_NAME}CMAKE_NM${COLOR_DOTS}..................................${COLOR_VAR_VAL}${CMAKE_NM}${COLOR_RESET}" )
message( STATUS "${COLOR_VAR_NAME}CMAKE_C_FLAGS${COLOR_DOTS}.............................${COLOR_VAR_VAL}${CMAKE_C_FLAGS}${COLOR_RESET}" )
message( STATUS "${COLOR_VAR_NAME}CMAKE_CXX_FLAGS${COLOR_DOTS}...........................${COLOR_VAR_VAL}${CMAKE_CXX_FLAGS}${COLOR_RESET}" )
message( STATUS "${COLOR_VAR_NAME}WITH_TESTS${COLOR_DOTS}................................${COLOR_VAR_VAL}${WITH_TESTS}${COLOR_RESET}" )
message( STATUS "${COLOR_VAR_NAME}HUB_ENABLE_ACL${COLOR_DOTS}............................${COLOR_VAR_VAL}${HUB_ENABLE_ACL}${COLOR_RESET}" )
message( STATUS "${COLOR_VAR_NAME}LIBRARIES_DIR:${COLOR_RESET}" )
foreach(libDir ${LIBRARIES_DIR})
    message( STATUS "${COLOR_DOTS}              ............................${COLOR_VAR_VAL}${libDir}${COLOR_RESET}" )
endforeach(libDir)
message( STATUS "${COLOR_VAR_NAME}INCLUDES_DIR:${COLOR_RESET}" )
get_property(inc_dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(inc_dir ${inc_dirs})
    message( STATUS "${COLOR_DOTS}              ............................${COLOR_VAR_VAL}${inc_dir}${COLOR_RESET}" )
endforeach()

HUB_FIND_LIBRARY_IN_TOOLCHAIN( lib_name_rt rt )

HUB_FIND_SHARED_LIBRARY_IN_TOOLCHAIN( lib_name_atomic atomic )
HUB_FIND_SHARED_LIBRARY_IN_TOOLCHAIN( ffiLIB ffi )
HUB_FIND_SHARED_LIBRARY_IN_TOOLCHAIN( sslLIB ssl )
HUB_FIND_SHARED_LIBRARY_IN_TOOLCHAIN( cryptoLIB crypto )
HUB_FIND_SHARED_LIBRARY_IN_TOOLCHAIN( curlLIB curl )
HUB_FIND_SHARED_LIBRARY_IN_TOOLCHAIN( uvLIB uv )
HUB_FIND_SHARED_LIBRARY_IN_TOOLCHAIN( websocketsLIB websockets )
HUB_FIND_SHARED_LIBRARY_IN_TOOLCHAIN( zLIB z )
HUB_FIND_SHARED_LIBRARY_IN_TOOLCHAIN( mongooseLIB mongoose )

if( ${LIBC_HAS_NSL} )
    HUB_FIND_LIBRARY_IN_TOOLCHAIN( nslLIB nsl )
endif()

set( allOpenSslLibraries ${sslLIB} ${cryptoLIB} )

option( NOT_SMART_USERS "Enable simple user adding" OFF )
if( NOT_SMART_USERS STREQUAL ON )
    set( NMA_USERS_ADDITIONAL_DEFINE "-D__EZLO_DEBUG_SIMPLE_VERSION_OF_ADD_USER__=1" )
else()
    set( NMA_USERS_ADDITIONAL_DEFINE "" )
endif()

set( BUILD_SHARED_LIBS_SAVED "${BUILD_SHARED_LIBS}" )
set( BUILD_STATIC_LIBS_SAVED "${BUILD_STATIC_LIBS}" )

# deps
message( STATUS "${COLOR_NOTICE}*** ${COLOR_RESET}" )
message( STATUS "${COLOR_NOTICE}*** ${COLOR_ATTENTION}Adding ${COLOR_VAR_VAL}dependencies${COLOR_ATTENTION} projects${COLOR_DOTS}...${COLOR_RESET}" )
message( STATUS "${COLOR_NOTICE}*** ${COLOR_RESET}" )

if(NOT ${HUB_TYPE} MATCHES "g150" AND NOT ${HUB_TYPE} MATCHES "vhub")
	add_subdirectory( deps/protobuf )
	set(PROTOBUF_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/deps/protobuf/include")
endif()

add_subdirectory( deps/gtest )
target_compile_definitions(gtest PRIVATE _POSIX_PATH_MAX=256)

add_subdirectory( deps/tiny )
target_compile_options(tiny PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-narrowing>)

#if( NOT ${HUB_TYPE} MATCHES "vhub" )
    add_subdirectory( deps/ZNet )
    add_subdirectory( deps/Zigbee )

    target_compile_options(znet PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-w>)
    target_compile_options(znet PRIVATE $<$<COMPILE_LANGUAGE:C>:-w>)

    target_compile_options(ember_znet PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-w>)
    target_compile_options(ember_znet PRIVATE $<$<COMPILE_LANGUAGE:C>:-w>)
#endif()

if( ${HUB_DEPLOY_2GIG} OR ${WITH_TESTS} )
    set(WITH_2GIG ON CACHE BOOL "Build 2GIG")
    add_subdirectory( deps/2GIG )
endif()

if( NOT ${HUB_TYPE} MATCHES "g150" )
    add_subdirectory( deps/pahomqtt )
    add_subdirectory( deps/mosquitto )
endif()

add_subdirectory( deps/libnetlink )

if ( WITH_FFMPEG )
    if ( WITH_X264 )
        set( X264_PREFIX x264 )
        set( X264_PROJECT ${X264_PREFIX}_proj )
        set( X264_INSTALL_DIR ${CMAKE_BINARY_DIR}/x264 )
        add_subdirectory( deps/x264 )
    endif()
    set( FFMPEG_PREFIX ffmpeg )
    set( FFMPEG_PROJECT ${FFMPEG_PREFIX}_proj )
    set( FFMPEG_INSTALL_DIR ${CMAKE_BINARY_DIR}/${FFMPEG_PREFIX} )

    if( "${ARCH}" STREQUAL "amd64" AND NOT ${HUB_TYPE} STREQUAL "vhub" )
        set( FFMPEG_INCLUDES ${CMAKE_BINARY_DIR}/include )
    else()
        set( FFMPEG_INCLUDES ${FFMPEG_INSTALL_DIR}/include )
    endif()

    add_subdirectory( deps/ffmpeg )
endif()

if ( WITH_JSON_C )
    add_subdirectory( deps/json-c )
    set(JSON_C_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/deps/json-c/include/json-c")
endif()

if(WITH_ZW_NVM_CONVERTER)
    add_subdirectory( deps/zipgateway )
    set(ZIPGATEWAY_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/deps/zipgateway/include")
endif()

if (NOT ${HUB_TYPE} MATCHES "g150" AND NOT ${HUB_TYPE} MATCHES "vhub")
    add_subdirectory( deps/ubus )
endif()

message( STATUS "${COLOR_NOTICE}*** ${COLOR_RESET}" )
message( STATUS "${COLOR_NOTICE}*** ${COLOR_ATTENTION}Adding ${COLOR_VAR_VAL}firmware${COLOR_ATTENTION} projects${COLOR_DOTS}...${COLOR_RESET}" )
message( STATUS "${COLOR_NOTICE}*** ${COLOR_RESET}" )
add_subdirectory( firmware )

if( IS_NEED_COVERAGE )
    add_custom_target(
            run_coverage
            COMMAND lcov --base-directory `pwd` --directory `pwd` --zerocounters -q
            COMMAND make run_tests
            COMMAND lcov --base-directory `pwd` --directory `pwd` -c -o coverage.info
            COMMAND lcov --remove coverage.info '/usr*' -o coverage.info
            COMMAND lcov --remove coverage.info 'tests/googletest-*' -o coverage.info
            COMMAND rm -rf ./test_coverage
            COMMAND genhtml -o ./test_coverage -t "coverage" --num-spaces 4 coverage.info
    )
endif()
