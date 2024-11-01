cmake_minimum_required(VERSION 3.2)

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/addons/bin )

set( HUB_ADDONS_PROJECT_NAME_PREFIX "ha-" )
set( HUB_ADDONS_PROJECT_NAME_SUFFIX "d" )

add_subdirectory( luad )
add_subdirectory( cameras )

add_subdirectory( luup )
add_subdirectory( zwave )
add_subdirectory( bluetooth )
add_subdirectory( zigbee )

if( NOT ${HUB_TYPE} MATCHES "g150" )
    add_subdirectory( mqtt_gateway )
endif()

if( ${WITH_2GIG} )
    add_subdirectory( 2gig )
endif()

if( ${HUB_TYPE} MATCHES "vhub" OR ${WITH_TESTS} )
    add_subdirectory( vidoo4hub )
endif()
