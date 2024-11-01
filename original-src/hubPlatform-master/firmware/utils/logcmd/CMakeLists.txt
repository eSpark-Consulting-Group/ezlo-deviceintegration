get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )

file( GLOB SRC_FILES
    *.cpp
    ./log_utils/*.cpp
    ./filesystem/*.cpp
)

list( REMOVE_ITEM SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp )

link_directories( ${LIBRARIES_DIR} )

include_directories(
        ${FIRMWARE_COMMON_DIR}
        ${FIRMWARE_UTILS_DIR}
)

set( PROJECT_LIBRARIES
        base_utils
        system_utils
        json_utils
        dbclient
        cloud_storage
        http
        service_interfaces
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

file( GLOB EXTRA_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes/*.cpp )
HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES( ${PROJECT_NAME} "${TEST_LIBS_SET}" "" ${EXTRA_TEST_SOURCES} )

set( PACKAGE_NAME firmware-utils-${PROJECT_ID})
install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
install( PROGRAMS ${SOURCES_ROOT}/system/common/opt/firmware/scripts/firmware_status.sh DESTINATION ${PACKAGE_NAME}/opt/firmware/scripts )
