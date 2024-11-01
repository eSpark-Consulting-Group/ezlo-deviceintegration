get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )

if( NOT DEFINED HUB_DEPLOY_ZIGBEE )
    message( FATAL_ERROR "CMake variable HUB_DEPLOY_ZIGBEE should be defined" )
endif()
add_definitions( -D__ZIGBEE_TEST_SUPPORTED__=$<BOOL:${HUB_DEPLOY_ZIGBEE}> )

if( NOT DEFINED HUB_WITH_LTE )
    message( FATAL_ERROR "CMake variable HUB_WITH_LTE should be defined" )
endif()
add_definitions( -D__LTE_TEST_SUPPORTED__=$<BOOL:${HUB_WITH_LTE}> )

file( GLOB SRC_FILES
        *.cpp
        hardware_tests/*.cpp
        utils/*.cpp
        )

link_directories( ${LIBRARIES_DIR} )

HUB_FIND_SHARED_LIBRARY_IN_TOOLCHAIN( libDBUS dbus-1 )
HUB_FIND_SHARED_LIBRARY_IN_TOOLCHAIN( libGLIB glib-2.0 )

HUB_FIND_LIBRARY_IN_TOOLCHAIN( libGDBUS gdbus )

include_directories(
        ${FIRMWARE_COMMON_DIR}
        ${FIRMWARE_UTILS_DIR}
        ${FIRMWARE_CORE_DIR}
        ${EMBER_ZNET_INCLUDES}
        ${GENERATED_DIR}/deps
        ${TOOLCHAIN_INCLUDES_FOLDER}/dbus-1.0
        ${TOOLCHAIN_INCLUDES_FOLDER}/glib-2.0
        ${TOOLCHAIN_LIB_FOLDER}/dbus-1.0/include
        ${TOOLCHAIN_LIB_FOLDER}/glib-2.0/include
        )

include_directories(
        SYSTEM ${TOOLCHAIN_INCLUDES_FOLDER}/dbus-1.0
        SYSTEM ${TOOLCHAIN_INCLUDES_FOLDER}/glib-2.0
        SYSTEM ${TOOLCHAIN_LIB_FOLDER}/dbus-1.0/include
        SYSTEM ${TOOLCHAIN_LIB_FOLDER}/glib-2.0/include
)

set( PROJECT_LIBRARIES
        ${lib_name_atomic}
        base_utils
        system_utils
        json_utils
        log_utils
        git_version
        hardware_manager
        "$<$<BOOL:${HUB_DEPLOY_ZIGBEE}>:ember_znet>"
        ${libGDBUS}
        ${libDBUS}
        ${libGLIB}
        )

add_executable( ${PROJECT_ID} ${SRC_FILES} )
target_link_libraries( ${PROJECT_ID} ${PROJECT_LIBRARIES} )

if(${HUB_TYPE} MATCHES "h2_secure.1")
    target_link_libraries( ${PROJECT_ID} libbq27441g1 )
endif()

set( PACKAGE_NAME firmware-utils-${PROJECT_ID})
install( TARGETS ${PROJECT_ID} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
