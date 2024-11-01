project( common_binding_interface )

include_directories(
    ${FIRMWARE_COMMON_DIR}
)

link_directories( ${LIBRARIES_DIR} )

file( GLOB SRC_FILES *.cpp *.h )

add_library( ${PROJECT_NAME} SHARED ${SRC_FILES} )
target_link_libraries( ${PROJECT_NAME}
    service_interfaces
    plugin_events_subscriber
    log_utils
)

set( PACKAGE_NAME firmware-common-${PROJECT_NAME})
install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
