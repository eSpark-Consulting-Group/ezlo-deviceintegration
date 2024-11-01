get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )

include_directories(
    ${FIRMWARE_COMMON_DIR}
    ${FIRMWARE_UTILS_DIR}
)

EXTRACT_PACKAGE_VERSION( FIRMWARE_VERSION hub-platform-firmware )
CONFIGURE_ARCH_DEPENDENT_FILES(CURRENT_GENERATED_DIR firmware_package_version.cpp)

file ( GLOB SRC_FILES
    *.cpp
    addons/*.cpp
    backup/*.cpp
    ${CURRENT_GENERATED_DIR}/firmware_package_version.cpp
    )

add_library( ${PROJECT_NAME} ${SRC_FILES} )
target_link_libraries(${PROJECT_NAME}
        ${curlLIB}
        log_utils
        config_reader
        data_utils
        crypto_utils
        system_utils
        plugin_utils
        )

if( (${HUB_TYPE} MATCHES "g150") OR
    (${HUB_TYPE} MATCHES "g450") OR
    (${HUB_TYPE} MATCHES "g550")
  )
    target_link_libraries(${PROJECT_NAME} vera_utils)
endif()

# Tests
HUB_ADD_TESTS("${PROJECT_NAME}")

set( PACKAGE_NAME firmware-common-${PROJECT_NAME})
install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
