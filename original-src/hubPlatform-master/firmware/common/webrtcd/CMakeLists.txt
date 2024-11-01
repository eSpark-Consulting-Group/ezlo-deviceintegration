cmake_minimum_required( VERSION 3.2 )
include(ExternalProject)

get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )
set( PACKAGE_NAME firmware-common-${PROJECT_NAME} )

if ( NOT ${WITH_WEBRTC} )
    message(STATUS "Package ${PACKAGE_NAME} is not set for deploy. Skip" )
    return()
endif()

# Change a version here to download another version of webrtcd
set( WEBRTCD_VERSION "0.4.2" CACHE STRING "webrtcd version bound to firmware" FORCE )

message( STATUS "${COLOR_VAR_NAME}WEBRTCD_VERSION${COLOR_DOTS}..........................${COLOR_VAR_DESC}webrtcd version${COLOR_DOTS}...........................${COLOR_VAR_VAL}${WEBRTCD_VERSION}${COLOR_RESET}" )

set( BINARY_NAME ha-${PROJECT_NAME} )
set( WEBRTCD_RUNTIME_DIR  "${CMAKE_SERVICE_OUTPUT_DIRECTORY}/${PROJECT_NAME}" )

if( ${HUB_TYPE} MATCHES "h2.1" OR ${HUB_TYPE} MATCHES "h2_secure.1" )
    set( WEBRTCD_IMPL_URL  "http://dl.mios.com/linux_firmware/deps/firmware/packages/video/webrtcd/webrtcd-${WEBRTCD_VERSION}-arm.tar.gz" )
    message( STATUS "${COLOR_VAR_NAME}WEBRTCD_IMPL_URL${COLOR_DOTS}.........................${COLOR_VAR_DESC}WebRTC implementation URL to download${COLOR_DOTS}.....${COLOR_VAR_VAL}${WEBRTCD_IMPL_URL}${COLOR_RESET}" )
else()
    message( FATAL_ERROR "Dependencies for ${PACKAGE_NAME} is not defined for current hub type: ${HUB_TYPE}" )
endif()

# NOTE: by using SOURCE_DIR we omit copying the same files on each build ('make') command
# However, if services/webrtcd is deleted manually then project must be triggered/rebuild again
# Or one can implement copy_directory command in webrtcd_deploy target
ExternalProject_Add(webrtcd_impl
        URL "${WEBRTCD_IMPL_URL}"
        SOURCE_DIR "${WEBRTCD_RUNTIME_DIR}/files"
        LOG_DOWNLOAD 1
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        CONFIGURE_COMMAND ""
)

add_custom_target( ${PROJECT_ID} DEPENDS webrtcd_impl )
ExternalProject_Get_Property( webrtcd_impl SOURCE_DIR )
set( WEBRTCD_FILES_DIR ${SOURCE_DIR} )
set( WEBRTCD_BINARY "${WEBRTCD_FILES_DIR}/bin/ha-webrtcd" )
set( WEBRTCD_LOADER "${WEBRTCD_FILES_DIR}/lib/ld-linux-armhf.so.3" )

set( WEBRTCD_PACKAGE_PATH ${PACKAGE_NAME}/opt/firmware/services/${PROJECT_NAME} )

install( DIRECTORY ${WEBRTCD_FILES_DIR} DESTINATION ${WEBRTCD_PACKAGE_PATH} )
install( PROGRAMS ${WEBRTCD_BINARY} DESTINATION ${WEBRTCD_PACKAGE_PATH}/files/bin )
install( PROGRAMS ${WEBRTCD_LOADER} DESTINATION ${WEBRTCD_PACKAGE_PATH}/files/lib )
install( PROGRAMS ${SOURCES_ROOT}/system/common/service_files/ha-webrtcd DESTINATION ${PACKAGE_NAME}/etc/init.d )
