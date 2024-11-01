cmake_minimum_required( VERSION 3.2 )

set( CMAKE_CXX_STANDARD 11 )

message( STATUS "-- CMAKE is working in \"${CMAKE_CURRENT_SOURCE_DIR}\" " )

message( STATUS "The original value of the CMAKE_SYSTEM_NAME variable was set to \"${CMAKE_SYSTEM_NAME}\"" )
if( UNIX )
	set( CMAKE_SYSTEM_NAME "Linux" )
endif()
message( STATUS "The CMAKE_SYSTEM_NAME variable was programmatically set to \"${CMAKE_SYSTEM_NAME}\"" )

execute_process( COMMAND uname OUTPUT_VARIABLE UNIX_SYSTEM_NAME )
string( STRIP "${UNIX_SYSTEM_NAME}" UNIX_SYSTEM_NAME )
message( STATUS "The UNIX_SYSTEM_NAME variable was programmatically set to \"${UNIX_SYSTEM_NAME}\"" )

set( CMAKE_CXX_COMPILER_WORKS TRUE )
set( CMAKE_C_COMPILER_WORKS TRUE )

if( NOT CMAKE_BUILD_TYPE )
	set( CMAKE_BUILD_TYPE "Release" )
	message( INFO " CMAKE_BUILD_TYPE was not specified in command line parameters. Sorry, but we set it for you to the ${CMAKE_BUILD_TYPE} value." )
endif()

project( tiny )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Wno-deprecated-declarations -Wno-unused-result" )

#if( CMAKE_BUILD_TYPE MATCHES Release )
#	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")
#	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
#endif()

if( CMAKE_BUILD_TYPE MATCHES Debug )
	add_definitions( -D_DEBUG=1 )
endif()
if( CMAKE_BUILD_TYPE MATCHES Release )
	add_definitions( -DNDEBUG=1 )
endif()

option( DEBUG_LOGGING "Enable debug messages" FALSE )
if( DEBUG_LOGGING )
	add_definitions( -D__DEBUG_LOGGING__=1 )
endif()

option( NETWORK_TRAFFIC_LOGGING "Enable network traffic messages" FALSE )
if( NETWORK_TRAFFIC_LOGGING )
	add_definitions( -D__NETWORK_TRAFFIC_LOGGING__=1 )
endif()

option( ERROR_LOGGING "Enable error messages" FALSE )
if( ERROR_LOGGING OR CMAKE_BUILD_TYPE MATCHES Debug )
	add_definitions( -D__ERROR_LOGGING__=1 )
endif()
#
# *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***
#
if( ARCH )
	message( STATUS "-- ARCH is specified as ${ARCH}" )
else()
	set( ARCH "amd64" )
	message( STATUS "-- ARCH is not specified, defaulting to ${ARCH}" )
endif()

get_filename_component( SOURCES_ROOT "../../deps" ABSOLUTE )

if( TOOLCHAIN_DEPS_LIBS_FOLDER )
	message( STATUS "-- TOOLCHAIN_DEPS_LIBS_FOLDER is specified as ${TOOLCHAIN_DEPS_LIBS_FOLDER}" )
else()
	message( ERROR "-- TOOLCHAIN_DEPS_LIBS_FOLDER is not specified!" )
endif()

message( STATUS "UNIX_SYSTEM_NAME..............Building on host............... ${UNIX_SYSTEM_NAME}" )
message( STATUS "ARCH..........................Architecture................... ${ARCH}" )
message( STATUS "PROJECT_NAME................................................. ${PROJECT_NAME}" )
message( STATUS "CMAKE_BUILD_TYPE............................................. ${CMAKE_BUILD_TYPE}" )
message( STATUS "CMAKE_CURRENT_SOURCE_DIR..................................... ${CMAKE_CURRENT_SOURCE_DIR}" )
message( STATUS "INSTALL_ROOT..................Install target directory....... ${INSTALL_ROOT}" )
message( STATUS "SOURCES_ROOT..................Libraries source directory..... ${SOURCES_ROOT}" )
message( STATUS "INCLUDES_ROOT................................................ ${INCLUDES_ROOT}" )
#
# *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***
#

file (GLOB SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/include/tiny/http_parser/http_parser.h	${CMAKE_CURRENT_SOURCE_DIR}/src/http_parser/http_parser.c
	${CMAKE_CURRENT_SOURCE_DIR}/include/tiny/tiny_atomic_shared_ptr.h	${CMAKE_CURRENT_SOURCE_DIR}/src/tiny_atomic_shared_ptr.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tiny/tiny_multifunction.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/tiny/tiny_async_work.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/tiny/tiny_command_line_parser.h	${CMAKE_CURRENT_SOURCE_DIR}/src/tiny_command_line_parser.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tiny/tiny_cxx_replacements.h	${CMAKE_CURRENT_SOURCE_DIR}/src/tiny_cxx_replacements.cpp
#	${CMAKE_CURRENT_SOURCE_DIR}/include/tiny/tiny_filesystem.h			${CMAKE_CURRENT_SOURCE_DIR}/src/tiny_filesystem.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tiny/tiny_http.h				${CMAKE_CURRENT_SOURCE_DIR}/src/tiny_http.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tiny/tiny_dispatch.h			${CMAKE_CURRENT_SOURCE_DIR}/src/tiny_dispatch.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tiny/tiny_mail.h				${CMAKE_CURRENT_SOURCE_DIR}/src/tiny_mail.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tiny/tiny_netio.h				${CMAKE_CURRENT_SOURCE_DIR}/src/tiny_netio.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tiny/tiny_network.h				${CMAKE_CURRENT_SOURCE_DIR}/src/tiny_network.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tiny/tiny_path.h				${CMAKE_CURRENT_SOURCE_DIR}/src/tiny_path.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tiny/tiny_process.h				${CMAKE_CURRENT_SOURCE_DIR}/src/tiny_process.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tiny/tiny_multithreading.h		${CMAKE_CURRENT_SOURCE_DIR}/src/tiny_multithreading.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tiny/tiny_stats.h				${CMAKE_CURRENT_SOURCE_DIR}/src/tiny_stats.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tiny/tiny_thread_pool.h			${CMAKE_CURRENT_SOURCE_DIR}/src/tiny_thread_pool.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tiny/tiny_ws.h					${CMAKE_CURRENT_SOURCE_DIR}/src/tiny_ws.cpp
	)

include_directories(
		${COMPILER_INCLUDES}
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/include
		${CMAKE_CURRENT_SOURCE_DIR}/include/asio_standalone
)

#list( REMOVE_ITEM SRC_FILES src/tiny_consolecolors.cpp )
#list( REMOVE_ITEM SRC_FILES src/tiny_utils.cpp )
#list( REMOVE_ITEM SRC_FILES src/tiny_url.cpp )

add_library( tiny_consolecolors SHARED
	src/tiny_consolecolors.cpp
	src/tiny_consolecolors_filters.cpp
	src/tiny_utils.cpp
)
target_link_libraries( tiny_consolecolors tiny_url )
add_library( tiny_consolecolors_lib STATIC
	src/tiny_consolecolors.cpp
	src/tiny_consolecolors_filters.cpp
	src/tiny_utils.cpp
)
target_link_libraries( tiny_consolecolors_lib tiny_url_lib )
add_library( tiny_url SHARED
	src/tiny_url.cpp
)
add_library( tiny_url_lib STATIC
	src/tiny_url.cpp
)


add_library( ${PROJECT_NAME} SHARED
	${SRC_FILES}
)
target_link_libraries( ${PROJECT_NAME}
		${sslLIB}
		${cryptoLIB}
		${lib_name_atomic}
		${websocketsLIB}
		${uvLIB}
		${zLIB}
		${dlLIB}
		tiny_consolecolors
		tiny_url
)

#install( TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_BINARY_DIR}/local
#		 PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
#					 GROUP_EXECUTE GROUP_READ
#					 WORLD_EXECUTE WORLD_READ )

# Tests of fixes
set(allTinyTestLibraries tiny $<${LIBC_HAS_NSL}:${nslLIB}> ${lib_name_atomic})
HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES("${PROJECT_NAME}" "${allTinyTestLibraries}" "${CMAKE_CURRENT_SOURCE_DIR}" "")
