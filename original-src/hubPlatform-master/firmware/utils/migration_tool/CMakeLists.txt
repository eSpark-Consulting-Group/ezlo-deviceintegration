get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )

include_directories(
    ${FIRMWARE_COMMON_DIR}
    ${FIRMWARE_CORE_DIR}
    ${FIRMWARE_UTILS_DIR}
)

file( GLOB SRC_FILES
    *.cpp
    migration/*.cpp
    migration/**/*.cpp
    system_repair/*.cpp
    system_repair/**/*.cpp
    )

list(REMOVE_ITEM SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

add_library(${PROJECT_NAME}_lib STATIC ${SRC_FILES})
target_link_libraries( ${PROJECT_NAME}_lib
    base_utils
    log_utils
    dbclient
    data_utils
    common_utils
    uci_utils
    )

add_executable( ${PROJECT_ID} ${SRC_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp )
target_link_libraries( ${PROJECT_ID} ${PROJECT_NAME}_lib )

HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES( ${PROJECT_ID}
        ${PROJECT_NAME}_lib "" "${FIRMWARE_CORE_DIR}/scenes_manager/tests/resources/scene_scale_resources.cpp" )

set( PACKAGE_NAME firmware-utils-${PROJECT_ID})
install( TARGETS ${PROJECT_ID} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
