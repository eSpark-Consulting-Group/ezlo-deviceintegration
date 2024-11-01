get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )

file( GLOB SRC_FILES *.cpp )
list( REMOVE_ITEM SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp )

link_directories( ${LIBRARIES_DIR} )

include_directories(
        ${FIRMWARE_COMMON_DIR}
        ${FIRMWARE_UTILS_DIR}
)

set( PROJECT_LIBRARIES
        base_utils
        system_utils
        data_utils
        json_utils
        mqtt_client
        service_interfaces
        log_utils
        tiny
)

add_library(${PROJECT_NAME}_lib STATIC ${SRC_FILES})
target_link_libraries( ${PROJECT_NAME}_lib ${PROJECT_LIBRARIES} )

add_executable( ${PROJECT_NAME} main.cpp )
target_link_libraries( ${PROJECT_NAME}
    ${PROJECT_NAME}_lib
)

set(TEST_LIBS_SET
    ${PROJECT_NAME}_lib
    )

if( ${HUB_TYPE} MATCHES "vhub" )
set( PACKAGE_NAME firmware-utils-${PROJECT_ID})
install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
endif()
