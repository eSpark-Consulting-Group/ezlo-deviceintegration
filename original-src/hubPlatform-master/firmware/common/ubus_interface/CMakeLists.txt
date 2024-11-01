cmake_minimum_required(VERSION 3.5)
project(ubus_interface LANGUAGES C CXX)

link_directories(
    ${CMAKE_BINARY_DIR}/lib/
)

include_directories(
    ${FIRMWARE_COMMON_DIR}
    ${CMAKE_BINARY_DIR}/deps/ubus/include
    ${CMAKE_BINARY_DIR}/deps/json-c/include
)

file( GLOB SRC_FILES
    *.cpp
    *.h
)

add_library(ubus_interface ${SRC_FILES})

add_dependencies(ubus_interface ubus_ext_project)

set(LINKLIBS
    log_utils
    json-c
    ubus
    ubox
    blobmsg_json
)


target_link_libraries(ubus_interface ${LINKLIBS})
target_include_directories(ubus_interface PUBLIC ${CMAKE_BINARY_DIR}/deps/ubus/include)

set( PACKAGE_NAME firmware-common-${PROJECT_NAME})
install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
