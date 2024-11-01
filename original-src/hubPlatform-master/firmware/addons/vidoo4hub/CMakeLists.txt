cmake_minimum_required(VERSION 3.2)

get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_ID})

set( PACKAGE_NAME firmware-addons-${PROJECT_NAME})
EXTRACT_PACKAGE_VERSION( FIRMWARE_ADDONS_VIDOO4HUB_VERSION ${PACKAGE_NAME} )
CONFIGURE_ARCH_DEPENDENT_FILES(CURRENT_GENERATED_DIR version/vidoo4hub_addon_version.cpp)

file( GLOB SRC_FILES
    *.cpp
    ./vidoo_device/*.cpp
    ./camera/*.cpp
    ./stream_manager/*.cpp
    ./camera_manager/*.cpp
    ./core_manager/*.cpp
    ./vidoo_manager/*.cpp
    ./vidoo_mediator/*.cpp
    ./vidoo_client/*.cpp
    ./dictionary_manager/*.cpp
    ./detectable_types/*.cpp
    ./resources/*.cpp
    ${CURRENT_GENERATED_DIR}/version/vidoo4hub_addon_version.cpp
    )

include_directories(
    ${FIRMWARE_ADDONS_DIR}
    ${FIRMWARE_COMMON_DIR}
    ${FIRMWARE_CORE_DIR}
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
target_link_libraries( ${PROJECT_NAME}
    addon_interfaces
    cored_interface
    lmdb
    common_utils
    log_utils
    tiny
    )

if( ${HUB_TYPE} MATCHES "vhub" OR ${WITH_TESTS} )
    target_link_libraries( ${PROJECT_NAME}  mqtt_client )
endif()

add_executable( ha-vidoo4hubd main.cpp )
target_link_libraries( ha-vidoo4hubd
    ${PROJECT_NAME}
    ${lib_name_rt}
)


file( GLOB EXTRA_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/utils/*.cpp )
HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES( ${PROJECT_NAME} "" "" ${EXTRA_TEST_SOURCES} )

if( ${HUB_TYPE} MATCHES "vhub" )
install( TARGETS ha-vidoo4hubd DESTINATION ${PACKAGE_NAME}/opt/firmware/bin/addons )
INSTALL_ADDON_SERVICE( ha-vidoo4hubd ${PACKAGE_NAME} )
endif()
