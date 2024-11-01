get_filename_component( PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME )
project ( ${PROJECT_ID} )

file( GLOB SRC_FILES
        *.cpp
        )

link_directories( ${LIBRARIES_DIR} )

include_directories(
        ${FIRMWARE_COMMON_DIR}
        ${FIRMWARE_CORE_DIR} # an em/plugin_model module/library should be moved to a common part
        )

set( PROJECT_LIBRARIES
        plugin_model_lib
        json_utils_lib
        cored_model_resources_lib
        )

add_executable( ${PROJECT_ID} ${SRC_FILES} )
target_link_libraries( ${PROJECT_ID} ${PROJECT_LIBRARIES} -static )
