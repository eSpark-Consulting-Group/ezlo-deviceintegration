cmake_minimum_required(VERSION 3.2)

project( "data_utils" )

include_directories( ${FIRMWARE_COMMON_DIR} )

file( GLOB SRC_FILES *.cpp )

add_library( ${PROJECT_NAME} ${SRC_FILES} )
link_directories(${LIBRARIES_DIR})

target_link_libraries( ${PROJECT_NAME}
    crypto_utils
    json_utils
    base_utils
    )

HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES(${PROJECT_NAME} "test_utils" ${FIRMWARE_COMMON_DIR} "")

set( PACKAGE_NAME firmware-common-${PROJECT_NAME})
install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
