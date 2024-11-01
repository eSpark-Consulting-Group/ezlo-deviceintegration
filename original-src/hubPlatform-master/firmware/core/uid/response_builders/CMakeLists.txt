project(uid_response_builders)

include_directories(
    ${FIRMWARE_COMMON_DIR}
    ${FIRMWARE_CORE_DIR}
)

file ( GLOB SRC_FILES
    *.cpp
    getdevices/*.cpp
    getitems/*.cpp
    device_settings_get/*.cpp
    custom_nodes/*.cpp
    gateway_settings_get/*.cpp
    hub_modes_get/*.cpp
    )

add_library( ${PROJECT_NAME} SHARED ${SRC_FILES} )
target_link_libraries( ${PROJECT_NAME}
    base_utils
    json_utils
    cored_model
    service_interfaces
    cored_interface
    extensions_manager_interface
    )

set( PACKAGE_NAME firmware-core-${PROJECT_NAME})
install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
