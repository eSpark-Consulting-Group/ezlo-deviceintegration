get_filename_component( PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME )
project ( ${PROJECT_ID} )

set( PACKAGE_NAME firmware-dev-${PROJECT_ID})
install( PROGRAMS two_way_audio_test.py DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )

