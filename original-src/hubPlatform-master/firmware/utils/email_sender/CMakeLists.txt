get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )

file( GLOB SRC_FILES
    *.cpp
)

list( REMOVE_ITEM SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp )

link_directories( ${LIBRARIES_DIR} )

include_directories(
        ${FIRMWARE_COMMON_DIR}
        ${FIRMWARE_UTILS_DIR}
)

set( PROJECT_LIBRARIES
    log_utils
    pthread
    ${curlLIB}
)

add_executable( ${PROJECT_ID} main.cpp )
target_link_libraries( ${PROJECT_ID}
    smtp_client
)

set( PACKAGE_NAME firmware-dev-${PROJECT_ID})
install( TARGETS ${PROJECT_ID} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )

