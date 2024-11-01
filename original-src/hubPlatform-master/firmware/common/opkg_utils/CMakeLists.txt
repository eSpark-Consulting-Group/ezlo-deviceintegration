cmake_minimum_required(VERSION 3.2)

get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )

include_directories(
    ${FIRMWARE_COMMON_DIR}
)

file ( GLOB SRC_FILES
    *.cpp
    feeds/*.cpp
    keys/*.cpp
    packages/*.cpp
    )

add_library( ${PROJECT_NAME} STATIC ${SRC_FILES} )
target_link_libraries( ${PROJECT_NAME}
    system_utils_static
    )

# Tests
CONFIGURE_ARCH_DEPENDENT_FILES( CURRENT_GENERATED_DIR tests/resources/opkg_test_resources.cpp )

set(TEST_LIBS_SET
    system_utils
    test_utils
    log_utils
    json_utils
    bus
    )

HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES("${PROJECT_NAME}" "${TEST_LIBS_SET}" "${CURRENT_GENERATED_DIR}/.."
    "${CURRENT_GENERATED_DIR}/tests/resources/opkg_test_resources.cpp"
)
