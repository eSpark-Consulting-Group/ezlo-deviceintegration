get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )

include_directories(
		${FIRMWARE_COMMON_DIR}
		${FIRMWARE_COMMON_GENERATED_DIR}
)

file( GLOB SRC_FILES
    *.cpp
    )

add_library( ${PROJECT_NAME} ${SRC_FILES} )
target_link_libraries(${PROJECT_NAME}
    service_interfaces
    tiny
    http
	data_utils
    )

# Tests
#HUB_ADD_TESTS("${PROJECT_NAME}")
HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES(${PROJECT_NAME} "" ${FIRMWARE_COMMON_DIR} "")

set( PACKAGE_NAME firmware-common-${PROJECT_NAME})
install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
