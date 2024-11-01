project(cored_policies)

include_directories(
        ${FIRMWARE_CORE_DIR}
        ${FIRMWARE_COMMON_DIR}
        ${FIRMWARE_COMMON_GENERATED_DIR}
)

link_directories(${LIBRARIES_DIR})

file( GLOB SRC_FILES
        *.cpp
        )

add_library( ${PROJECT_NAME} SHARED ${SRC_FILES} )
target_link_libraries( ${PROJECT_NAME}
        service_interfaces
        ${service_name}d_model
        tiny
        )

set( PACKAGE_NAME firmware-core-${PROJECT_NAME})
install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
