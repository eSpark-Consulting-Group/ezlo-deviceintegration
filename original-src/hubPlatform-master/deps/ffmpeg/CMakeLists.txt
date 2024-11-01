cmake_minimum_required( VERSION 3.2 )
include(ExternalProject)

set( PACKAGE_NAME firmware-common-ffmpeg )

list(APPEND CMAKE_OPTS
     -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
     -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
     -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
     -DCMAKE_INSTALL_PREFIX=${FFMPEG_INSTALL_DIR}
     )

if( ${ARCH} STREQUAL "amd64" AND NOT ${HUB_TYPE} STREQUAL "vhub" )
    set( X264_INCLUDE_FOLDER ${CMAKE_BINARY_DIR}/include )
    set( X264_LIB_FOLDER ${CMAKE_BINARY_DIR}/lib )
    set( RPATH_FOLDERS " -Wl,-rpath=${TOOLCHAIN_SYSTEM_LIB_FOLDER} -Wl,-rpath=${TOOLCHAIN_DEPS_LIBS_FOLDER} -Wl,-rpath=${CMAKE_LIBRARY_OUTPUT_DIRECTORY} -Wl,-rpath-link=${TOOLCHAIN_LIB_FOLDER}" )
else()
    set( X264_INCLUDE_FOLDER ${X264_INSTALL_DIR}/include )
    set( X264_LIB_FOLDER ${X264_INSTALL_DIR}/lib )
    set( RPATH_FOLDERS " -Wl,-rpath=/opt/firmware/lib -Wl,-rpath=/lib -Wl,-rpath=/usr/lib -Wl,-rpath=${X264_LIB_FOLDER} -Wl,-rpath-link=${TOOLCHAIN_LIB_FOLDER}" )
endif()

set(EXTRA_CFLAGS "-I${TOOLCHAIN_INCLUDES_FOLDER} -I${TOOLCHAIN_DEPS_INCLUDES_FOLDER} -I${X264_INCLUDE_FOLDER}")
set(EXTRA_LDFLAGS "-L$<${LIBC_HAS_NSL}:${nslLIB}> -L${TOOLCHAIN_LIB_FOLDER} -L${TOOLCHAIN_SYSTEM_LIB_FOLDER} -L${TOOLCHAIN_DEPS_LIBS_FOLDER} -L${X264_LIB_FOLDER} ${RPATH_FOLDERS}")

 set(CONFIG_OPTIONS
     --prefix=${FFMPEG_INSTALL_DIR}
     --enable-pic
     --enable-shared
     --disable-static
     --disable-avfilter
     --disable-avdevice
     --disable-programs
     --disable-debug
     --disable-everything
     --enable-swresample
     --enable-muxer=matroska,matroska_audio,g722,g723_1,g726,g726le,rtsp,mp4
     --enable-demuxer=rtsp
     --enable-parser=h264,hevc,aac,pcm_mulaw,mpeg4video,mpegvideo,mpegaudio
     --enable-decoder=h264,hevc,aac,pcm_mulaw,mpeg4
     --enable-encoder=mjpeg,aac
     --enable-protocol=file,tls
     --enable-network
     --enable-small
     --enable-openssl
     --enable-bsf=extract_extradata
     --cc=${CMAKE_C_COMPILER}
     --cxx=${CMAKE_CXX_COMPILER}
     --nm=${CMAKE_NM}
     --ar=${CMAKE_AR}
     --extra-cflags=${EXTRA_CFLAGS}
     --extra-ldflags=${EXTRA_LDFLAGS}
     )

if( ${WITH_X264} )
    list(APPEND CONFIG_OPTIONS
        --enable-encoder=libx264
        --enable-gpl
        --enable-nonfree
        --enable-libx264
        )
endif()

if("${ARCH}" STREQUAL "arm")
    list(APPEND CONFIG_OPTIONS
        --arch=armel
        --target-os=linux
        --enable-cross-compile
        --cross-prefix=${TOOLCHAIN_BIN_FOLDER}/${TOOLCHAIN_COMPILER_PREFIX}
        )
elseif("${ARCH}" STREQUAL "arm-openwrt")
    list(APPEND CONFIG_OPTIONS
        --arch=armel
        --target-os=linux
        --enable-cross-compile
        --cross-prefix=${TOOLCHAIN_BIN_FOLDER}/${TOOLCHAIN_COMPILER_PREFIX}
        )
elseif("${ARCH}" STREQUAL "mips" OR "${ARCH}" STREQUAL "mips-musl")
    list(APPEND CONFIG_OPTIONS
        --arch=mipsel
        --target-os=linux
        --enable-cross-compile
        --cross-prefix=${TOOLCHAIN_BIN_FOLDER}/${TOOLCHAIN_COMPILER_PREFIX}
        --cpu=24kec
        )
elseif("${ARCH}" STREQUAL "amd64")
    # Toolchaine's strip cannot strip ffmpeg libraries
    find_program(SYSTEM_STRIP strip
                    NO_CMAKE_PATH
                    NO_CMAKE_SYSTEM_PATH
                    NO_CMAKE_ENVIRONMENT_PATH
                    NO_CMAKE_FIND_ROOT_PATH)
    if(SYSTEM_STRIP)
        set( SYSTEM_STRIP --strip=${SYSTEM_STRIP} )
    else()
        set( SYSTEM_STRIP --disable-stripping )
    endif()

    list(APPEND CONFIG_OPTIONS
        --enable-rpath
        --arch=x86_64
        --disable-x86asm
        --enable-demuxer=mpeg4video,mpegvideo
        --target-os=linux
        --enable-cross-compile
        --cross-prefix=${TOOLCHAIN_COMPILER_BIN_FOLDER}/${TOOLCHAIN_TOOLS_PREFIX}
        ${SYSTEM_STRIP}
    )

    if(NOT ${HUB_TYPE} STREQUAL "vhub")
        list(APPEND CONFIG_OPTIONS
            --prefix=${CMAKE_BINARY_DIR}
        )
    endif()
endif()

ExternalProject_Add(
    ${FFMPEG_PROJECT}
    CMAKE_ARGS ${CMAKE_OPTS}
    URL ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg-4.3.1.tar.xz

    DEPENDS ${X264_PROJECT}

    UPDATE_COMMAND ""
    CONFIGURE_COMMAND <SOURCE_DIR>/configure ${CONFIG_OPTIONS}

    BUILD_COMMAND make -j${PARALLEL_COUNT}
    INSTALL_COMMAND make -j${PARALLEL_COUNT} install

    LOG_DOWNLOAD 1
    LOG_CONFIGURE 1
    LOG_BUILD 1
    LOG_INSTALL 1
    )

install( DIRECTORY ${FFMPEG_INSTALL_DIR}/lib DESTINATION ${PACKAGE_NAME}/opt/firmware )
