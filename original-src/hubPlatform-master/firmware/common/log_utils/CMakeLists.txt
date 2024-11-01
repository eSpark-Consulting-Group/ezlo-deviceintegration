get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )

CONFIGURE_ARCH_DEPENDENT_FILES( CURRENT_GENERATED_DIR log_definitions.cpp )

include_directories(
    ${FIRMWARE_COMMON_DIR}
    ${CURRENT_GENERATED_DIR}
)

file ( GLOB SRC_FILES
    *.cpp
)

add_library( ${PROJECT_NAME}
    ${SRC_FILES}
    ${CURRENT_GENERATED_DIR}/log_definitions.cpp
)
target_link_libraries( ${PROJECT_NAME}
    config_reader
    tiny_consolecolors
)

add_library( ${PROJECT_NAME}_lib STATIC
    ${SRC_FILES}
    ${CURRENT_GENERATED_DIR}/log_definitions.cpp
)
target_link_libraries( ${PROJECT_NAME}_lib
    config_reader_lib
    tiny_consolecolors_lib
)

set( PACKAGE_NAME firmware-common-${PROJECT_NAME})
install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
