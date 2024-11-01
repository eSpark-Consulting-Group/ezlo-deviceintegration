get_filename_component( PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME )
project( ${PROJECT_ID} )

file( GLOB SRC_FILES
        *.cpp
        )

link_directories( ${LIBRARIES_DIR} )

include_directories(
        ${FIRMWARE_COMMON_DIR}
        )

set( PROJECT_LIBRARIES
        system_utils
        )

add_executable( ${PROJECT_ID}d ${SRC_FILES} )
target_link_libraries( ${PROJECT_ID}d ${PROJECT_LIBRARIES} )

set( PACKAGE_NAME firmware-core-hardware_manager )
install( TARGETS ${PROJECT_ID}d DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
install( PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/failsafe_reboot DESTINATION ${PACKAGE_NAME}/etc/init.d/ )
