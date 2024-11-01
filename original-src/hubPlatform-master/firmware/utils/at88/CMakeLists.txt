get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( ${PROJECT_ID} )

file( GLOB SRC_FILES
        at88sc.cpp
        at88ut.cpp
)

link_directories( ${LIBRARIES_DIR} )

include_directories(
        ${FIRMWARE_COMMON_DIR}
        ${FIRMWARE_UTILS_DIR}
)

set( AT88_LIBRARIES
        base_utils
        common_utils
        ${lib_name_atomic}
        pthread
        dl ${lib_name_rt}
        stdc++
        gcc_s
)

if( (${HUB_TYPE} MATCHES "g150") OR
    (${HUB_TYPE} MATCHES "g450") OR
    (${HUB_TYPE} MATCHES "g550")
  )
    list(APPEND AT88_LIBRARIES vera_utils)
endif()

add_executable( ${PROJECT_ID}util ${SRC_FILES} at88util.cpp at88_options.cpp )
target_link_libraries( ${PROJECT_ID}util ${AT88_LIBRARIES} )

add_executable( ${PROJECT_ID}serial ${SRC_FILES} at88serial.cpp )
target_link_libraries( ${PROJECT_ID}serial ${AT88_LIBRARIES} )

set( PACKAGE_NAME firmware-utils-${PROJECT_ID}util)
install( TARGETS ${PROJECT_ID}util DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
install( TARGETS ${PROJECT_ID}serial DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
