get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )

include_directories(
    ${FIRMWARE_COMMON_DIR}
    ${FIRMWARE_COMMON_GENERATED_DIR}
)

file ( GLOB SRC_FILES *.cpp )

HUB_FIND_SHARED_LIBRARY_IN_TOOLCHAIN( spreadLIB tspread-core )

add_library( ${PROJECT_NAME} ${SRC_FILES} )
target_link_libraries( ${PROJECT_NAME}
        ${lib_name_rt}
        ${spreadLIB}
        $<${LIBC_HAS_NSL}:${nslLIB}>
        git_version
        system_utils
)

set( PACKAGE_NAME firmware-common-${PROJECT_NAME})
install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
