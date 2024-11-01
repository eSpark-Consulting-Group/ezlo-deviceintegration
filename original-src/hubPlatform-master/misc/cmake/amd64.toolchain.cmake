cmake_minimum_required(VERSION 3.2)

# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

set( ARCH "amd64" )

set( TOOLCHAIN_NAME gcc4.8.3-amd64-v1.6 )
set( TOOLCHAIN_TARGET_NAME x86_64-pc-linux-gnu )
set( TOOLCHAIN_COMPILER_PREFIX "${TOOLCHAIN_TARGET_NAME}-" )
set( TOOLCHAIN_TOOLS_PREFIX "" )

set( TOOLCHAIN_FOLDER /usr/local/toolchains/${TOOLCHAIN_NAME} )
set( TOOLCHAIN_BIN_FOLDER ${TOOLCHAIN_FOLDER}/bin )
set( TOOLCHAIN_COMPILER_BIN_FOLDER ${TOOLCHAIN_FOLDER}/${TOOLCHAIN_TARGET_NAME}/bin )

get_filename_component( SOURCES_ROOT "." ABSOLUTE )
if(NOT EXISTS ${TOOLCHAIN_FOLDER})
    execute_process(COMMAND "${SOURCES_ROOT}/misc/scripts/prepare_toolchaine.sh" ARCH=amd64 RESULT_VARIABLE CommandResult)
    if(NOT "${CommandResult}" STREQUAL "0")
        message(FATAL_ERROR "Cannot setup toolchaine for 'amd64' architecture.")
    endif()
endif()

set( HUB_TYPE local CACHE STRING "AMD64 local hub type" )

# specify the cross compiler
set( CMAKE_C_COMPILER   ${TOOLCHAIN_COMPILER_BIN_FOLDER}/${TOOLCHAIN_COMPILER_PREFIX}gcc )
set( CMAKE_CXX_COMPILER ${TOOLCHAIN_COMPILER_BIN_FOLDER}/${TOOLCHAIN_COMPILER_PREFIX}g++ )

set( CMAKE_FIND_ROOT_PATH
        ${TOOLCHAIN_BIN_FOLDER}
        ${TOOLCHAIN_FOLDER}/${TOOLCHAIN_TARGET_NAME}
        ${TOOLCHAIN_FOLDER}/usr )
