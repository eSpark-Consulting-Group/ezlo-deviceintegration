cmake_minimum_required(VERSION 3.5)

set (PROJECT_NAME metrics)
set( BINARY_NAME ha-${PROJECT_NAME} )
set( CERTS_DIR metrics_draft_certs )
if ( ${CMAKE_BUILD_TYPE} MATCHES "Debug" )
    set ( protobuf_build protobufd )
else()
    set ( protobuf_build protobuf )
endif()
set( PROTOC_BIN_NAME protoc )
set( PROTOBUF_EXTRACT_DIR 	"/usr/local/toolchains" )
set( PROTOBUF_BUILD_DIR 	${PROTOBUF_EXTRACT_DIR}/protobuf-3.21.9 )
set( PROTOBUF_COMPILER 		${PROTOBUF_BUILD_DIR}/${PROTOC_BIN_NAME} )

set( METRICS_PROTO_FILE_NAME supervisor.proto )
set( WIRE_PROTO_FILE_NAME wire.proto )
set( PROTOBUF_FILE_PATH ${SOURCES_ROOT}/firmware/core/metrics/protocols )

set (METRICS_GENERATED_FOLDER ${GENERATED_DIR}/firmware/core/metrics)
file (MAKE_DIRECTORY ${METRICS_GENERATED_FOLDER})

if ( NOT EXISTS ${PROTOBUF_COMPILER} )
	MESSAGE(STATUS "Protobuf compiler doesn't exist under toolchains folder: ${PROTOBUF_COMPILER}. Build it.")

    execute_process(
    	COMMAND ${CMAKE_COMMAND} -E tar xvf ${DEPS_DIR}/protobuf/protobuf-cpp-3.21.9.tar.gz
    	WORKING_DIRECTORY ${PROTOBUF_EXTRACT_DIR}
    )
    execute_process(
    	COMMAND /bin/sh -c
    	"cmake -DCMAKE_CXX_COMPILER=/usr/bin/g++ -DCMAKE_C_COMPILER=/usr/bin/gcc -DBUILD_SHARED_LIBS=OFF -Dprotobuf_BUILD_TESTS=OFF . &&
    	make -j${PARALLEL_COUNT}"
    	WORKING_DIRECTORY ${PROTOBUF_BUILD_DIR}
    )
endif()

if ( EXISTS ${PROTOBUF_COMPILER} )
	MESSAGE(STATUS "Protobuf compiler found: ${PROTOBUF_COMPILER}")
    MESSAGE(STATUS "Compile protobuf definitions for ${PROJECT_NAME}")

    execute_process(
        COMMAND /bin/sh -c
            "${PROTOBUF_BUILD_DIR}/${PROTOC_BIN_NAME} -I=${PROTOBUF_FILE_PATH} --cpp_out=${METRICS_GENERATED_FOLDER} ${PROTOBUF_FILE_PATH}/${METRICS_PROTO_FILE_NAME} &&
    	    ${PROTOBUF_BUILD_DIR}/${PROTOC_BIN_NAME} -I=${PROTOBUF_FILE_PATH} --cpp_out=${METRICS_GENERATED_FOLDER} ${PROTOBUF_FILE_PATH}/${WIRE_PROTO_FILE_NAME}"
            WORKING_DIRECTORY ${PROTOBUF_BUILD_DIR}
            RESULT_VARIABLE COMPILATION_RESULT
    )
    if (NOT COMPILATION_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to compile protobuf proto files.")
    endif()
endif()

project(${PROJECT_NAME} LANGUAGES CXX)

# Files
file(GLOB SOURCES
    ${METRICS_GENERATED_FOLDER}/*.cc
    ${METRICS_GENERATED_FOLDER}/*.h
    internal_metrics/*.h
    internal_metrics/*.cpp
    ./*.h
    ./*.cpp
)

list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

link_directories(${CMAKE_BINARY_DIR}/deps/protobuf/lib/)

add_library(${PROJECT_NAME} STATIC ${SOURCES})
add_executable(${BINARY_NAME} main.cpp)
target_link_libraries(${BINARY_NAME} ${PROJECT_NAME})

# protobuf
add_dependencies(${BINARY_NAME} protobuf_ext)

# includes
include_directories(
    ${METRICS_GENERATED_FOLDER}
    ${FIRMWARE_COMMON_DIR}
    ${PROTOBUF_INCLUDE_DIRS}
    ${FIRMWARE_CORE_DIR}
)

# Link libraries
set(LINKLIBS
    ${sslLIB}
    ${cryptoLIB}
    common_utils
    ${protobuf_build}
    log_utils
    service_interfaces
    service_metrics_data
    ubus_interface
    cored_interface
)

target_link_libraries(${PROJECT_NAME} ${LINKLIBS} )

file( GLOB EXTRA_TEST_SOURCES tests/*.cpp tests/resources/*.cpp )
HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES( ${PROJECT_NAME} ${PROJECT_NAME} "" "${EXTRA_TEST_SOURCES}" )

set( PACKAGE_NAME firmware-core-${PROJECT_NAME})
install( TARGETS ${BINARY_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
install( DIRECTORY ${CERTS_DIR}/ DESTINATION ${PACKAGE_NAME}/opt/firmware/metrics/certs )
install( FILES ${SOURCES_ROOT}/system/common/opt/firmware/etc/metrics.cfg DESTINATION ${PACKAGE_NAME}/opt/firmware/etc )
install( PROGRAMS ${SOURCES_ROOT}/system/common/service_files/ha-metrics DESTINATION ${PACKAGE_NAME}/etc/init.d )
