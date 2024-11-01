get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_ID})

set(PACKAGE_NAME firmware-addons-${PROJECT_NAME})
EXTRACT_PACKAGE_VERSION(FIRMWARE_ADDONS_2GIG_VERSION ${PACKAGE_NAME})

include_directories(
    ${FIRMWARE_ADDONS_DIR}
    ${FIRMWARE_CORE_DIR}
    ${FIRMWARE_COMMON_DIR}
    ${FIRMWARE_COMMON_GENERATED_DIR}
)

set(BINARY_NAME ${HUB_ADDONS_PROJECT_NAME_PREFIX}${PROJECT_NAME}${HUB_ADDONS_PROJECT_NAME_SUFFIX})
CONFIGURE_ARCH_DEPENDENT_FILES(CURRENT_GENERATED_DIR version/2gig_addon_version.cpp)

file(GLOB SRC_FILES
    *.cpp
    events/*.cpp
    resources/*.cpp
    ${CURRENT_GENERATED_DIR}/version/2gig_addon_version.cpp
)

list(REMOVE_ITEM SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

link_directories(
    ${LIBRARIES_DIR}
)

add_library(${PROJECT_NAME} STATIC
    ${SRC_FILES}
)
target_link_libraries(${PROJECT_NAME}
    ${lib_name_rt}
    addon_interfaces
    backupd_interface
    hardware_manager_interface
    common_utils
    system_utils
    jsonrpc
    json_utils
    2GIG
)

add_library(lua_${PROJECT_NAME} SHARED
    bindings/2gig_lua_bindings.cpp
)
target_link_libraries(lua_${PROJECT_NAME}
    ${PROJECT_NAME}
    lua_binding_interface
)

add_executable(${BINARY_NAME}
    main.cpp
)
target_link_libraries(${BINARY_NAME}
    ${PROJECT_NAME}
)

# Tests
HUB_ADD_TESTS(${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib)
install(TARGETS lua_${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib)
install(TARGETS ${BINARY_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin/addons)
install(FILES ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib2GIG.so DESTINATION ${PACKAGE_NAME}/opt/firmware/lib)
install(PROGRAMS ${SOURCES_ROOT}/system/common/service_files/ha-2gigd DESTINATION ${PACKAGE_NAME}/etc/init.d)
INSTALL_ADDON_SERVICE(${BINARY_NAME} ${PACKAGE_NAME})
