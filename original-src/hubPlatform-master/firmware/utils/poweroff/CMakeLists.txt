get_filename_component( PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME )
project ( ${PROJECT_ID} )

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

add_executable( ${PROJECT_ID} ${SRC_FILES} )
target_link_libraries( ${PROJECT_ID} ${PROJECT_LIBRARIES} )

set( PACKAGE_NAME firmware-utils-${PROJECT_ID} )
install( TARGETS ${PROJECT_ID} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
install( PROGRAMS ${SOURCES_ROOT}/system/h2_secure.1/opt/firmware/bin/poweroff.sh DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
