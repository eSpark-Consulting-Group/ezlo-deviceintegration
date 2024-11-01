cmake_minimum_required( VERSION 3.2 )
include(ExternalProject)

set(LIBUBOX_EXT_PROJECT libubox)
set(LIBUBOX_INSTALL_DIR ${CMAKE_BINARY_DIR}/deps/ubus)

list(APPEND CMAKE_OPTS
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_AR=${CMAKE_AR}
    -DCMAKE_LINKER=${CMAKE_LINKER}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=${LIBUBOX_INSTALL_DIR}
    -DCMAKE_C_STANDARD_INCLUDE_DIRECTORIES=${CMAKE_BINARY_DIR}/deps/json-c/include
    -DCMAKE_PREFIX_PATH=${CMAKE_BINARY_DIR}/deps/json-c/lib
)

set (JSON json-c-ext-project)
if (${HUB_TYPE} MATCHES "beewave.1")
  add_subdirectory(${DEPS_ROOT}/json-c ${CMAKE_BINARY_DIR}/deps/json-c)
endif()

list(APPEND LIBUBOX_OPTS
    -DBUILD_LUA=OFF
    -DBUILD_EXAMPLES=OFF
)

ExternalProject_Add(
    ${LIBUBOX_EXT_PROJECT}
    CMAKE_ARGS ${CMAKE_OPTS} ${LIBUBOX_OPTS}
    URL ${CMAKE_CURRENT_SOURCE_DIR}/libubox-2018-07-25.tar.gz
    PATCH_COMMAND sed -i "s@json/json.h@json-c/json.h@g" <SOURCE_DIR>/blobmsg_json.c
    COMMAND sed -i "s@json/json.h@json-c/json.h@g" <SOURCE_DIR>/jshn.c
    COMMAND patch -ruN -d <SOURCE_DIR> < ${CMAKE_CURRENT_SOURCE_DIR}/uloop_handle_signal.patch
    DEPENDS ${JSON}

    LOG_DOWNLOAD 1
    LOG_BUILD 1
    LOG_INSTALL 1
)
