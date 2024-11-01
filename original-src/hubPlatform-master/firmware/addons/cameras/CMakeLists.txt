cmake_minimum_required(VERSION 3.2)

get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_ID})

set( PACKAGE_NAME firmware-addons-${PROJECT_NAME})
EXTRACT_PACKAGE_VERSION( FIRMWARE_ADDONS_CAMERAS_VERSION ${PACKAGE_NAME} )
CONFIGURE_ARCH_DEPENDENT_FILES(CURRENT_GENERATED_DIR version/cameras_addon_version.cpp)

file( GLOB SRC_FILES
    *.cpp
    ./frame/*.cpp
    ./packet/*.cpp
    ./storages/*.cpp
    ./codec/*.cpp
    ./input_output/*.cpp
    ./cloud/*.cpp
    ./preview/*.cpp
    ./local_stream/*.cpp
    ./snapshot/*.cpp
    ./postponed/*.cpp
    ./rtsp_server/*.cpp
    ./rtsp_server/live555/*.cpp
    ./events/*.cpp
    ${CURRENT_GENERATED_DIR}/version/cameras_addon_version.cpp
    )

list(REMOVE_ITEM SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

include_directories(
    ${FIRMWARE_ADDONS_DIR}
    ${FIRMWARE_COMMON_DIR}
    ${FIRMWARE_CORE_DIR}

    ${FFMPEG_INCLUDES}
)

include_directories(
        SYSTEM ${DEPS_ROOT}/ffmpeg/src

        SYSTEM ${TOOLCHAIN_DEPS_INCLUDES_FOLDER}/liveMedia
        SYSTEM ${TOOLCHAIN_DEPS_INCLUDES_FOLDER}/groupsock
        SYSTEM ${TOOLCHAIN_DEPS_INCLUDES_FOLDER}/UsageEnvironment
        SYSTEM ${TOOLCHAIN_DEPS_INCLUDES_FOLDER}/BasicUsageEnvironment

        SYSTEM ${STAGING_DIR_INCLUDE_FOLDER}/liveMedia
        SYSTEM ${STAGING_DIR_INCLUDE_FOLDER}/groupsock
        SYSTEM ${STAGING_DIR_INCLUDE_FOLDER}/UsageEnvironment
        SYSTEM ${STAGING_DIR_INCLUDE_FOLDER}/BasicUsageEnvironment
)

link_directories( ${FFMPEG_INSTALL_DIR}/lib )

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
target_link_libraries( ${PROJECT_NAME}
    addon_interfaces
    liveMedia BasicUsageEnvironment UsageEnvironment groupsock
    avutil avformat avcodec swscale swresample
    lmdb
    common_utils
    cloud_storage
    http
    networkd_interface
    cored_interface
    )

add_library( lua_${PROJECT_NAME} SHARED
    bindings/cameras_lua_bindings.cpp
    bindings/cameras_client.cpp
    )
target_link_libraries( lua_${PROJECT_NAME}
    lua_binding_interface
    )

add_executable( ha-camerasd main.cpp )
target_link_libraries( ha-camerasd
    ${PROJECT_NAME}
    ${lib_name_rt}
)

if ( WITH_FFMPEG )
    if ( WITH_X264 )
        target_link_options(${PROJECT_NAME} PUBLIC -Wl,-rpath-link=${X264_INSTALL_DIR}/lib)
    endif()
    add_dependencies(${PROJECT_NAME} ${FFMPEG_PROJECT})
endif()

# Tests
set(VIDEO_SAMPLES_PATH ${CMAKE_CURRENT_SOURCE_DIR}/tests/samples)
CONFIGURE_ARCH_DEPENDENT_FILES( CURRENT_GENERATED_DIR tests/samples.cpp )

HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES("${PROJECT_NAME}" "tiny" "${CURRENT_GENERATED_DIR}" "${CURRENT_GENERATED_DIR}/tests/samples.cpp")

install( TARGETS ha-camerasd DESTINATION ${PACKAGE_NAME}/opt/firmware/bin/addons )
install( TARGETS lua_${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
install( FILES ${SOURCES_ROOT}/system/${HUB_TYPE}/opt/firmware/etc/cameras.cfg DESTINATION ${PACKAGE_NAME}/opt/firmware/etc )
install( PROGRAMS ${SOURCES_ROOT}/system/common/service_files/ha-camerasd DESTINATION ${PACKAGE_NAME}/etc/init.d )
INSTALL_ADDON_SERVICE( ha-camerasd ${PACKAGE_NAME} )
