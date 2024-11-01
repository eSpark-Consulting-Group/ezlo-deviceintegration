cmake_minimum_required(VERSION 3.2)

get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_ID})


file(GLOB SRC_FILES
        *.cpp
        resources/*.cpp
        )
list(REMOVE_ITEM SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

include_directories(
        ${FIRMWARE_COMMON_DIR}
        ${FIRMWARE_CORE_DIR}
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
target_link_libraries(${PROJECT_NAME}
        ${lib_name_atomic}
        $<${LIBC_HAS_NSL}:${nslLIB}>
        service_interfaces
        common_utils
        icc_interface
)

add_executable(ha-${PROJECT_NAME} main.cpp)
target_link_libraries(ha-${PROJECT_NAME}
    ${PROJECT_NAME}
    ${lib_name_rt}
)

# remote interface
file( GLOB SRC_FILES
        remote_interface/*.cpp
        resources/*.cpp
        )
add_library( ${PROJECT_NAME}_interface SHARED ${SRC_FILES} )
target_link_libraries( ${PROJECT_NAME}_interface
        service_interfaces
        common_utils
        icc_interface
)

set( PACKAGE_NAME firmware-core-${PROJECT_NAME})
install( TARGETS ha-${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )

set( PACKAGE_NAME firmware-core-${PROJECT_NAME}_interface)
install( TARGETS ${PROJECT_NAME}_interface DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
install( PROGRAMS ${SOURCES_ROOT}/system/common/service_files/ha-10_external_controllers DESTINATION ${PACKAGE_NAME}/etc/init.d )
