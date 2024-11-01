cmake_minimum_required(VERSION 3.2)

get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project (${PROJECT_ID})

include_directories(
    ${FIRMWARE_COMMON_DIR}
    ${FIRMWARE_CORE_DIR}
    ${FIRMWARE_UTILS_DIR}
)

file(GLOB SRC_FILES  *.cpp)
list(REMOVE_ITEM SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

add_library(${PROJECT_NAME}_lib STATIC ${SRC_FILES})
target_link_libraries(${PROJECT_ID}_lib
    system_utils_static
    opkg_utils
    platform_configuration
    # temporary, remove after debug printing removed
    log_utils
    )

add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_lib)

# Tests
CONFIGURE_ARCH_DEPENDENT_FILES( CURRENT_GENERATED_DIR tests/resources/update_test_resources.cpp )

set(TEST_LIBS_SET
    ${PROJECT_NAME}_lib
    )

HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES("${PROJECT_NAME}" "${TEST_LIBS_SET}" "${CURRENT_GENERATED_DIR}/.."
    "${CURRENT_GENERATED_DIR}/tests/resources/update_test_resources.cpp"
)

# Package configuration
set( PACKAGE_NAME firmware-utils-${PROJECT_ID})
install( TARGETS ${PROJECT_ID} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
if(${HUB_TYPE} MATCHES "g150" )
    install( PROGRAMS ${SOURCES_ROOT}/system/g150/opt/firmware/bin/update_recovery.sh DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
    install( PROGRAMS ${SOURCES_ROOT}/system/g150/opt/firmware/bin/update_boot_trap.sh DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
endif()
