cmake_minimum_required(VERSION 3.2)

# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

set( ARCH "mips-musl" )

set( TOOLCHAIN_NAME gcc7.4.0_musl-mipsel-v1.1 )
set( TOOLCHAIN_TARGET_NAME mipsel-openwrt-linux-musl )
set( TOOLCHAIN_COMPILER_PREFIX "mipsel-openwrt-linux-musl-" )
set( TOOLCHAIN_TOOLS_PREFIX "${TOOLCHAIN_COMPILER_PREFIX}" )

set( TOOLCHAIN_FOLDER /usr/local/toolchains/${TOOLCHAIN_NAME} )
set( TOOLCHAIN_BIN_FOLDER ${TOOLCHAIN_FOLDER}/bin )
set( TOOLCHAIN_COMPILER_BIN_FOLDER ${TOOLCHAIN_FOLDER}/bin )

get_filename_component( SOURCES_ROOT "." ABSOLUTE )

if(NOT EXISTS ${TOOLCHAIN_FOLDER})
    execute_process(COMMAND "${SOURCES_ROOT}/misc/scripts/prepare_toolchaine.sh" ARCH=mips-musl RESULT_VARIABLE CommandResult)
    if(NOT "${CommandResult}" STREQUAL "0")
        message(FATAL_ERROR "Cannot setup toolchaine for 'mips-musl' architecture.")
    endif()
endif()

# specify the cross compiler
set( CMAKE_C_COMPILER   ${TOOLCHAIN_BIN_FOLDER}/${TOOLCHAIN_COMPILER_PREFIX}gcc )
set( CMAKE_CXX_COMPILER ${TOOLCHAIN_BIN_FOLDER}/${TOOLCHAIN_COMPILER_PREFIX}g++ )

set( CMAKE_FIND_ROOT_PATH
        ${TOOLCHAIN_FOLDER}
        ${TOOLCHAIN_FOLDER}/${TOOLCHAIN_TARGET_NAME}
        ${TOOLCHAIN_FOLDER}/usr )
