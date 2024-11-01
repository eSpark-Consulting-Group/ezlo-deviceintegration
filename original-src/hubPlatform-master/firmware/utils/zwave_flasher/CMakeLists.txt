get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )


file( GLOB SRC_FILES *.cpp )
list(REMOVE_ITEM SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

link_directories( ${LIBRARIES_DIR} )

include_directories(
        ${FIRMWARE_COMMON_DIR}
        ${FIRMWARE_UTILS_DIR}
)

set( PROJECT_LIBRARIES
        base_utils
        log_utils
        git_version
        ${lib_name_atomic}
        pthread
        dl ${lib_name_rt}
        stdc++
        gcc_s
)

add_library(${PROJECT_NAME}_lib STATIC ${SRC_FILES})
target_link_libraries(${PROJECT_ID}_lib
        ${PROJECT_LIBRARIES}
        )

add_executable(fb-zwflash ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_link_libraries(fb-zwflash ${PROJECT_NAME}_lib)

set( PACKAGE_NAME firmware-utils-zwflash)
install( TARGETS fb-zwflash DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
install( DIRECTORY ${SOURCES_ROOT}/system/common/opt/firmware/zw-flash/ DESTINATION ${PACKAGE_NAME}/opt/firmware/zw-flash FILES_MATCHING PATTERN "*.bin" )
install( FILES ${SOURCES_ROOT}/system/${HUB_TYPE}/opt/firmware/zw-flash/nvr.bin.factory DESTINATION ${PACKAGE_NAME}/opt/firmware/zw-flash OPTIONAL )
install( FILES ${SOURCES_ROOT}/system/common/opt/firmware/etc/zwave.cfg.${ZWAVE_REGION} DESTINATION ${PACKAGE_NAME}/opt/firmware/etc RENAME zwave.cfg )
