get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_ID})

include_directories(
        ${FIRMWARE_COMMON_DIR}
        ${INCLUDES_ROOT}
        ${SOURCES_ROOT}
        ${DEPS_ROOT}
        ${FIRMWARE_CORE_DIR}
        ${CURRENT_GENERATED_DIR}
)

file( GLOB_RECURSE SRC_FILES *.cpp *.h )

add_library( ${PROJECT_NAME} ${SRC_FILES} )
target_link_libraries(${PROJECT_NAME}
        log_utils
        cored_model
        cored_interface
        )

set( PACKAGE_NAME firmware-core-scenes_engine)
install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
