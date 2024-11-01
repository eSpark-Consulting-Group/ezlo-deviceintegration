get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )

file( GLOB SRC_FILES *.cpp )

list( REMOVE_ITEM SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp )

link_directories( ${LIBRARIES_DIR} )

include_directories(
    ${FIRMWARE_COMMON_DIR}
    ${FIRMWARE_UTILS_DIR}
)

set( PROJECT_LIBRARIES
        base_utils
        git_version
        ssl_utils
        gcc_s
)

add_library(${PROJECT_NAME}_lib STATIC ${SRC_FILES})
target_link_libraries(${PROJECT_ID}_lib
    ${PROJECT_LIBRARIES}
    )

add_executable(zb-flash ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_link_libraries(zb-flash ${PROJECT_NAME}_lib)

set( PACKAGE_NAME firmware-addons-zigbee )

# TODO temporary overcome to fix package deploy
#install( TARGETS zb-flash DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
install( PROGRAMS ${CMAKE_BINARY_DIR}/bin/zb-flash DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )

install( DIRECTORY ${SOURCES_ROOT}/system/common/opt/firmware/images/zigbee/ DESTINATION ${PACKAGE_NAME}/opt/firmware/images/zigbee FILES_MATCHING PATTERN "*.*" )
