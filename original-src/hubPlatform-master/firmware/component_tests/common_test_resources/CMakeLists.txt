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
    resources/*.cpp
    fixtures/*.cpp
)

add_library(${PROJECT_NAME} ${SRC_FILES})
target_link_libraries(${PROJECT_NAME}
    cored_model
    cored_interface
    test_utils
    jsonrpc
)

