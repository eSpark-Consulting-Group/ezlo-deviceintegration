cmake_minimum_required( VERSION 3.2 )
include(ExternalProject)

set(UBUS_EXT_PROJECT ubus_ext_project)
set(UBUS_INSTALL_DIR ${CMAKE_BINARY_DIR}/deps/ubus)

list(APPEND CMAKE_OPTS
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_AR=${CMAKE_AR}
    -DCMAKE_LINKER=${CMAKE_LINKER}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=${UBUS_INSTALL_DIR}
    -DCMAKE_PREFIX_PATH=${CMAKE_BINARY_DIR}/deps/json-c/lib
    -DCMAKE_C_STANDARD_INCLUDE_DIRECTORIES=${CMAKE_BINARY_DIR}/deps/ubus/include/

)

set (LIBUBOX_EXT_PROJECT libubox)
set (LIBUBOX_INSTALL_DIR ${CMAKE_BINARY_DIR}/ubus/libubox)
add_subdirectory(libubox)

list(APPEND UBUS_OPTS
    -DBUILD_LUA=OFF
    -DBUILD_EXAMPLES=OFF
)

ExternalProject_Add(
    ${UBUS_EXT_PROJECT}
    CMAKE_ARGS ${CMAKE_OPTS} ${UBUS_OPTS}
    URL ${CMAKE_CURRENT_SOURCE_DIR}/ubus-2018-10-06.tar.gz

    DEPENDS ${LIBUBOX_EXT_PROJECT}

    LOG_DOWNLOAD 1
    LOG_BUILD 1
    LOG_INSTALL 1
)

ExternalProject_Add_Step(
    ${UBUS_EXT_PROJECT} copy2bin
    COMMAND ${CMAKE_COMMAND} -E copy ${UBUS_INSTALL_DIR}/lib/libubox.so      ${CMAKE_BINARY_DIR}/lib/libubox.so
    COMMAND ${CMAKE_COMMAND} -E copy ${UBUS_INSTALL_DIR}/lib/libubus.so      ${CMAKE_BINARY_DIR}/lib/libubus.so
    COMMAND ${CMAKE_COMMAND} -E copy ${UBUS_INSTALL_DIR}/lib/libblobmsg_json.so ${CMAKE_BINARY_DIR}/lib/libblobmsg_json.so
    ALWAYS 1
    DEPENDEES install
)

set( PACKAGE_NAME firmware-common-libubus)
install( DIRECTORY ${UBUS_INSTALL_DIR}/lib DESTINATION ${PACKAGE_NAME}/opt/firmware )
