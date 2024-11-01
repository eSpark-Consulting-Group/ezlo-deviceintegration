get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )


file( GLOB SRC_FILES *.cpp )

link_directories( ${LIBRARIES_DIR} )

include_directories(
        ${FIRMWARE_COMMON_DIR}
        ${FIRMWARE_UTILS_DIR}
)

set( PROJECT_LIBRARIES
        ${lib_name_atomic}
        base_utils
        log_utils
        dbclient
        git_version
)

add_executable( ${PROJECT_ID} ${SRC_FILES} )
target_link_libraries( ${PROJECT_ID} ${PROJECT_LIBRARIES} )

set( PACKAGE_NAME firmware-dev-${PROJECT_ID})
install( TARGETS ${PROJECT_ID} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
