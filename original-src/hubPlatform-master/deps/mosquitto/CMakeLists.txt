cmake_minimum_required( VERSION 3.2 )
include(ExternalProject)

get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_ID})

set(PACKAGE_NAME firmware-common-mosquitto)

set(MOSQUITTO_PROJECT mosquitto)
set(MOSQUITTO_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR})

list(APPEND CMAKE_OPTS
     -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
     -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
     -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
     -DCMAKE_INSTALL_PREFIX=${MOSQUITTO_INSTALL_DIR}
     )

list(APPEND MOSQUITTO_OPTS
     -DWITH_BUNDLED_DEPS=yes
     -DWITH_CJSON=no
     -DWITH_WEBSOCKETS=no
     -DWITH_TLS=no
     -DWITH_CLIENTS=OFF
     -DWITH_PLUGINS=OFF
     -DDOCUMENTATION=OFF
     )

ExternalProject_Add(
    ${MOSQUITTO_PROJECT}
    CMAKE_ARGS ${CMAKE_OPTS} ${MOSQUITTO_OPTS}
    URL ${CMAKE_CURRENT_SOURCE_DIR}/mosquitto-2.0.11.zip
    LOG_DOWNLOAD 1
    )

ExternalProject_Add_Step(
    ${MOSQUITTO_PROJECT} copy2bin
    COMMAND ${CMAKE_COMMAND} -E copy ${MOSQUITTO_INSTALL_DIR}/sbin/mosquitto ${CMAKE_BINARY_DIR}/bin
    ALWAYS 1
    DEPENDEES install
)

install( PROGRAMS ${MOSQUITTO_INSTALL_DIR}/sbin/mosquitto DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
install( FILES ${SOURCES_ROOT}/system/common/opt/firmware/etc/mosquitto.conf DESTINATION ${PACKAGE_NAME}/opt/firmware/etc )

