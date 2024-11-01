get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )


file( GLOB SRC_FILES *.cpp )

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

add_executable( ha-${PROJECT_ID} ${SRC_FILES} )
target_link_libraries( ha-${PROJECT_ID} ${PROJECT_LIBRARIES} )

set( PACKAGE_NAME firmware-utils-${PROJECT_ID})
install( TARGETS ha-${PROJECT_ID} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )