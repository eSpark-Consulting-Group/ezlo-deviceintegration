project(cored_model)

include_directories(
    ${FIRMWARE_CORE_DIR}
    ${FIRMWARE_COMMON_DIR}
    ${FIRMWARE_COMMON_GENERATED_DIR}
)

link_directories(${LIBRARIES_DIR})

file( GLOB SRC_FILES
    configuration/*.cpp
    device_setting/*.cpp
    gateway_setting/*.cpp
    setting/*.cpp
    resources/*.cpp
    values/*.cpp
    visitors/*.cpp
    *.cpp
    )

add_library( ${PROJECT_NAME} SHARED ${SRC_FILES} )
target_link_libraries( ${PROJECT_NAME}
    service_interfaces
    data_utils
    tiny
    hub_settings
    )

file( GLOB RESOURCES_SRC_FILES
        resources/*.cpp
        )
add_library( ${PROJECT_NAME}_resources_lib STATIC ${RESOURCES_SRC_FILES} )

set( PACKAGE_NAME firmware-core-${PROJECT_NAME})
install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
