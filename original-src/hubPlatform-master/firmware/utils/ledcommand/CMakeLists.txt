get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )


file( GLOB SRC_FILES *.cpp )

link_directories( ${LIBRARIES_DIR} )

include_directories(
        ${FIRMWARE_COMMON_DIR}
        ${FIRMWARE_UTILS_DIR}
        ${FIRMWARE_CORE_DIR}
)

set( PROJECT_LIBRARIES
        base_utils
        log_utils
        git_version
        pthread
        dl ${lib_name_rt}
        stdc++
        gcc_s
        hardware_manager
)

add_executable( ha-ledcmd ${SRC_FILES} )
target_link_libraries( ha-ledcmd ${PROJECT_LIBRARIES} )

set( PACKAGE_NAME firmware-utils-ledcmd)
install( TARGETS ha-ledcmd DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
