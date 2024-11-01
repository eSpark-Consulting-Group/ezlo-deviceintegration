get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )

include_directories(
    ${FIRMWARE_CORE_DIR}
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
    ${GTEST_DEPS_LIBS}
    jsonrpc
    common_test_resources
    uid
    uid_interface
)

target_include_directories( ${PROJECT_NAME} PRIVATE ${PROJECT_REQUIRED_INCLUDES} )
set_target_properties( ${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${TESTS_DIR}"
        LINK_FLAGS "--enable-new-dtags -Wl,-rpath,${TESTS_DIR}" )

add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} )
set_tests_properties ( ${PROJECT_NAME} PROPERTIES COST 1000 )
