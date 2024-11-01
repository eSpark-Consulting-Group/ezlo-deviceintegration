cmake_minimum_required(VERSION 3.2)

get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_ID})

include_directories(
    ${FIRMWARE_COMMON_DIR}
    ${FIRMWARE_CORE_DIR}
)

link_directories(${LIBRARIES_DIR})

file(GLOB SRC_FILES *.cpp *.h
    backup/*.cpp
    resources/*.cpp
    script_events/*.cpp
    ${FIRMWARE_COMMON_DIR}/dblists.cpp
)

list(REMOVE_ITEM SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
target_link_libraries(${PROJECT_NAME}
    ${lib_name_rt}
    base_utils
    service_interfaces
    cloud_storage
    common_utils
    update_manager_interface
    cored_interface
)

add_executable(ha-${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_link_libraries(ha-${PROJECT_NAME} ${PROJECT_NAME})

# lua bindings
add_library( lua_backup SHARED bindings/backup_lua_bindings.cpp )
target_link_libraries( lua_backup
        ${PROJECT_NAME}
        lua_binding_interface
        )

# remote interface
file( GLOB SRC_FILES
        remote_interface/*.cpp
        resources/*.cpp
        )
add_library( ${PROJECT_NAME}_interface SHARED ${SRC_FILES} )
target_link_libraries( ${PROJECT_NAME}_interface
        service_interfaces
        )

set(TEST_FILES_PATH ${CMAKE_CURRENT_SOURCE_DIR}/tests/files)
CONFIGURE_ARCH_DEPENDENT_FILES( CURRENT_GENERATED_DIR tests/test_files.cpp )

HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES( ${PROJECT_NAME} ${PROJECT_NAME}_interface "" "${CURRENT_GENERATED_DIR}/tests/test_files.cpp" )
set( PACKAGE_NAME firmware-core-${PROJECT_NAME})
install( TARGETS ${PROJECT_NAME}_interface DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
install( TARGETS ha-${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
install( FILES ${SOURCES_ROOT}/system/common/opt/firmware/etc/backup.cfg DESTINATION ${PACKAGE_NAME}/opt/firmware/etc )
install( PROGRAMS ${SOURCES_ROOT}/system/common/service_files/ha-01_backupd DESTINATION ${PACKAGE_NAME}/etc/init.d )
