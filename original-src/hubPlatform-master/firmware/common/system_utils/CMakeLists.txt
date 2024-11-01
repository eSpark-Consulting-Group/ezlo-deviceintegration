get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )

CONFIGURE_ARCH_DEPENDENT_FILES( CURRENT_GENERATED_DIR environment.cpp )

include_directories(
    ${FIRMWARE_COMMON_DIR}
)

file ( GLOB SRC_FILES
            *.cpp
            crontab/*.cpp
            timezone/*.cpp
            process/*.cpp
            "${CURRENT_GENERATED_DIR}/environment.cpp" )

add_library( ${PROJECT_NAME}_static STATIC ${SRC_FILES} )
add_library( ${PROJECT_NAME} SHARED ${SRC_FILES} )
target_link_libraries( ${PROJECT_NAME}
    tiny_consolecolors
    ${lib_name_rt}
)

set( PACKAGE_NAME firmware-common-${PROJECT_NAME})
install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )

HUB_ADD_TESTS(${PROJECT_NAME})
