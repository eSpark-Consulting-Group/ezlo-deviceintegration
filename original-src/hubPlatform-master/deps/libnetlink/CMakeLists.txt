cmake_minimum_required( VERSION 3.2 )
include( ExternalProject )
project( libnetlink )

if ( "${ARCH}" STREQUAL "amd64" )
    set( CONFIG_OPTIONS
         ${CMAKE_AR}
         ${CMAKE_C_COMPILER}
    )
elseif ( "${ARCH}" STREQUAL "mips" )
    set( CONFIG_OPTIONS
         ${TOOLCHAIN_COMPILER_BIN_FOLDER}/${TOOLCHAIN_COMPILER_PREFIX}ar
         ${CMAKE_C_COMPILER}
    )
elseif ( "${ARCH}" STREQUAL "arm-openwrt" )
    set( CONFIG_OPTIONS
        ${TOOLCHAIN_COMPILER_BIN_FOLDER}/${TOOLCHAIN_COMPILER_PREFIX}ar
        ${CMAKE_C_COMPILER}
        )
elseif ( "${ARCH}" STREQUAL "arm" )
    set( CONFIG_OPTIONS
         ${CMAKE_AR}
         ${CMAKE_C_COMPILER}
    )
endif()

message( STATUS "-- CMAKE is working in \"${CMAKE_CURRENT_SOURCE_DIR}\" " )
message( STATUS "-- CMAKE CONFIG_OPTIONS \"${CONFIG_OPTIONS}\" " )
message( STATUS "-- CMAKE TOOLCHAIN_COMPILER_BIN_FOLDER/TOOLCHAIN_COMPILER_PREFIXgcc-ar \"${TOOLCHAIN_COMPILER_BIN_FOLDER}/${TOOLCHAIN_COMPILER_PREFIX}gcc-ar\" " )
message( STATUS "-- CMAKE CMAKE_C_COMPILER \"${CMAKE_C_COMPILER}\" " )
message( STATUS "-- CMAKE CMAKE_AR \"${CMAKE_AR}\" " )

ExternalProject_Add(
    iproute2
    #URL http://mirrors.edge.kernel.org/pub/linux/utils/net/iproute2/iproute2-4.4.0.tar.gz
    URL ${CMAKE_CURRENT_SOURCE_DIR}/iproute2-4.4.0.tar.gz
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/src

    CONFIGURE_COMMAND ${libnetlink_SOURCE_DIR}/configure ${CONFIG_OPTIONS}
    PATCH_COMMAND bash -c "cat ${CMAKE_CURRENT_SOURCE_DIR}/iproute2-4.4.0-shared.patch ${CMAKE_CURRENT_SOURCE_DIR}/iproute2-4.4.0-musl.patch | patch -N -p 1"
    BUILD_COMMAND make V=1
    INSTALL_COMMAND cp lib/libnetlink.so ${CMAKE_BINARY_DIR}/lib

    BUILD_IN_SOURCE 1
    DOWNLOAD_NO_PROGRESS 1
    LOG_CONFIGURE 1
    LOG_BUILD 1
    LOG_INSTALL 1
    LOG_PATCH 1
    )
