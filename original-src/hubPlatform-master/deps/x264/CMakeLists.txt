cmake_minimum_required( VERSION 3.2 )
include( ExternalProject )

list( APPEND CMAKE_OPTS
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=${X264_INSTALL_DIR}
    )

set( CONFIG_OPTIONS
    --prefix=${X264_INSTALL_DIR}
    --enable-pic
    --enable-shared
    --disable-cli
    --disable-opencl
    --disable-avs
    --disable-gpac
    --disable-lsmash
    )

if( "${ARCH}" STREQUAL "arm" )
    list( APPEND CONFIG_OPTIONS
        --cross-prefix=${TOOLCHAIN_BIN_FOLDER}/${TOOLCHAIN_COMPILER_PREFIX}
        --host=armel-linux
        )
elseif( "${ARCH}" STREQUAL "arm-openwrt" )
    list( APPEND CONFIG_OPTIONS
        --cross-prefix=${TOOLCHAIN_BIN_FOLDER}/${TOOLCHAIN_COMPILER_PREFIX}
        --host=armel-linux
        )
elseif( "${ARCH}" STREQUAL "mips" OR "${ARCH}" STREQUAL "mips-musl" )
    list( APPEND CONFIG_OPTIONS
        --cross-prefix=${TOOLCHAIN_BIN_FOLDER}/${TOOLCHAIN_COMPILER_PREFIX}
        --host=mipsel-linux
        --disable-asm
        )
elseif( "${ARCH}" STREQUAL "amd64" )
    list( APPEND CONFIG_OPTIONS
        --cross-prefix=${TOOLCHAIN_COMPILER_BIN_FOLDER}/${TOOLCHAIN_TOOLS_PREFIX}
        --byte-order=little-endian
        )
    if( NOT ${HUB_TYPE} STREQUAL "vhub" )
        list( APPEND CONFIG_OPTIONS
            --prefix=${CMAKE_BINARY_DIR}
            )
    endif()
endif()

ExternalProject_Add(
    ${X264_PROJECT}
    CMAKE_ARGS ${CMAKE_OPTS}
    URL ${CMAKE_CURRENT_SOURCE_DIR}/x264-0.164.3075.tar.bz2

    UPDATE_COMMAND ""
    PATCH_COMMAND bash -c "patch -p1 < ${CMAKE_CURRENT_SOURCE_DIR}/x264-0.164.3075.patch.diff"
    CONFIGURE_COMMAND <SOURCE_DIR>/configure ${CONFIG_OPTIONS}

    BUILD_COMMAND make -j${PARALLEL_COUNT}
    INSTALL_COMMAND make -j${PARALLEL_COUNT} install

    LOG_DOWNLOAD 1
    LOG_CONFIGURE 1
    LOG_BUILD 1
    LOG_INSTALL 1
    )

set( PACKAGE_NAME firmware-common-ffmpeg )
install( DIRECTORY ${X264_INSTALL_DIR}/lib DESTINATION ${PACKAGE_NAME}/opt/firmware )
