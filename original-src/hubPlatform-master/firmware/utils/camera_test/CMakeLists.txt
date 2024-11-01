get_filename_component( PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME )
project ( ${PROJECT_ID} )

file( GLOB SRC_FILES *.cpp )

include_directories(
    ${FIRMWARE_COMMON_DIR}
    ${FIRMWARE_UTILS_DIR}

    ${FFMPEG_INCLUDES}
)

link_directories( ${FFMPEG_INSTALL_DIR}/lib )
set( PROJECT_LIBRARIES
    base_utils log_utils
    avutil avformat avcodec swscale
    dbclient
)

add_executable( ${PROJECT_ID} ${SRC_FILES} )
target_link_libraries( ${PROJECT_ID} ${PROJECT_LIBRARIES} )

if ( WITH_X264 )
    target_link_options( ${PROJECT_NAME} PUBLIC -Wl,-rpath-link=${X264_INSTALL_DIR}/lib )
endif()
add_dependencies( ${PROJECT_ID} ${FFMPEG_PROJECT} )

set( PACKAGE_NAME firmware-dev-${PROJECT_ID})
install( TARGETS ${PROJECT_ID} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
