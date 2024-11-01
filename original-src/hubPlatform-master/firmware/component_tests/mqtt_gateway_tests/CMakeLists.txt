get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( mqtt_gateway_component_tests )

include_directories(
    ${FIRMWARE_COMMON_DIR}
    ${FIRMWARE_COMMON_GENERATED_DIR}
    ${FIRMWARE_COMPONENT_TESTS_DIR}
)

link_directories(${LIBRARIES_DIR})

file(GLOB SRC_FILES
    *.cpp
    tests/*.cpp
    tests/resources/*.cpp
    tests/fixtures/*.cpp
)

set( GTEST_DEPS_LIBS
        gmock
        gtest )

add_executable(${PROJECT_NAME} ${SRC_FILES})
target_link_libraries(${PROJECT_NAME}
    ${lib_name_rt}
    ${GTEST_DEPS_LIBS}
    service_interfaces
    system_utils
    mqtt_client
    test_utils
    config_reader
    jsonrpc
    tiny
    fake_db
    common_test_resources
)

target_include_directories( ${PROJECT_NAME} PRIVATE ${PROJECT_REQUIRED_INCLUDES} )
set_target_properties( ${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${TESTS_DIR}"
        LINK_FLAGS "--enable-new-dtags -Wl,-rpath,${TESTS_DIR}" )

add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} )
set_tests_properties ( ${PROJECT_NAME} PROPERTIES COST 10 )
