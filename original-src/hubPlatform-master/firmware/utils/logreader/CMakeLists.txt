get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(LIBRARY_NAME ${PROJECT_NAME}_lib)
project ( ${PROJECT_NAME} )

file( GLOB SRC_FILES
    *.cpp
    ./log_utils/*.h
    ./log_reader/*.cpp
    ./log_writer/*.cpp
)

list( REMOVE_ITEM SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp )

link_directories( ${LIBRARIES_DIR} )

include_directories(
    ${FIRMWARE_COMMON_DIR}
    ${FIRMWARE_UTILS_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/log_writer
    ${FIRMWARE_COMMON_DIR}/config_reader/tests
    )

if (NOT ${HUB_TYPE} MATCHES "g150" AND NOT ${HUB_TYPE} MATCHES "vhub")
include_directories(${CMAKE_BINARY_DIR}/deps/ubus/include )
endif()

if (NOT ${HUB_TYPE} MATCHES "g150" AND NOT ${HUB_TYPE} MATCHES "vhub")
set(UBUS_LIBS    
    ubus
    ubox
    json-c
    blobmsg_json
    ubus_interface
)
else()
set(UBUS_LIBS "")
endif()

set( PROJECT_LIBRARIES
    ${UBUS_LIBS}
    tiny
    log_utils
    config_reader
)

add_library(${LIBRARY_NAME} STATIC ${SRC_FILES})
target_link_libraries( ${LIBRARY_NAME} ${PROJECT_LIBRARIES} )
add_executable( ${PROJECT_NAME} main.cpp )

if (NOT ${HUB_TYPE} MATCHES "g150" AND NOT ${HUB_TYPE} MATCHES "vhub")
    add_dependencies( ${PROJECT_NAME} ubus_ext_project libubox )
endif()

target_link_libraries( ${PROJECT_NAME} ${LIBRARY_NAME} )
set(TEST_LIBS_SET ${LIBRARY_NAME} )

file( GLOB EXTRA_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp )
HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES( ${PROJECT_NAME} "${TEST_LIBS_SET}" ${FIRMWARE_COMMON_DIR} ${EXTRA_TEST_SOURCES} )

set( PACKAGE_NAME firmware-utils-${PROJECT_NAME})
install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
install( FILES ${SOURCES_ROOT}/system/common/opt/firmware/etc/logreader.cfg DESTINATION ${PACKAGE_NAME}/opt/firmware/etc )