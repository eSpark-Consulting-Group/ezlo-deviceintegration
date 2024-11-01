cmake_minimum_required(VERSION 3.2)

get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )

CONFIGURE_ARCH_DEPENDENT_FILES( CURRNET_GENERATED_DIR addon_defs.cpp )

include_directories(
    ${FIRMWARE_COMMON_DIR}
    ${CURRNET_GENERATED_DIR}
)

file (GLOB SRC_FILES *.cpp)

link_directories(
    ${LIBRARIES_DIR}
)

add_library( ${PROJECT_NAME}
    ${SRC_FILES}
    ${CURRNET_GENERATED_DIR}/addon_defs.cpp
)
target_link_libraries( ${PROJECT_NAME}
    service_interfaces
)

# Tests
HUB_ADD_TESTS(${PROJECT_NAME})

set( PACKAGE_NAME firmware-common-${PROJECT_NAME})
install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )