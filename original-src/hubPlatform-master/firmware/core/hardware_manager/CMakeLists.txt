project( hardware_manager )
message( STATUS "${COLOR_INFO}--->${COLOR_NOTICE} Generating project ${COLOR_PROJECT_NAME}${PROJECT_NAME}${COLOR_DOTS}...${COLOR_RESET}" )

include_directories(
    ${FIRMWARE_COMMON_DIR}
    ${FIRMWARE_CORE_DIR}
)

link_directories( ${LIBRARIES_DIR} )

if(${HUB_TYPE} MATCHES "h2_secure.1")
    file( GLOB BATTERY_CHIP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/battery/bq27441g1/*.cpp )
    add_library( libbq27441g1 STATIC ${BATTERY_CHIP_SRC})
    target_link_libraries( libbq27441g1
            dbclient
            log_utils
            system_utils
            service_interfaces
            )
            
    file( GLOB EMPIRICAL_BATTERY_CHIP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/battery/empirical_battery_manager/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/battery/battery_charger.cpp )
    add_library( libempirical_battery_manager STATIC ${EMPIRICAL_BATTERY_CHIP_SRC} )
    target_link_libraries( libempirical_battery_manager
            log_utils
            service_interfaces
            )
endif()

#include folders one by one, not by global mask here, its prevents tests.
file( GLOB HARDWARE_MANAGER_SOURCES
        *.h
        *.cpp
        battery/*.cpp
        button/*.cpp
        config/*.cpp
        effect/*.cpp
        led/*.cpp
        netlink_manager/*.cpp
        remote_interface/*.cpp
        resources/*.cpp
        utils/*.cpp
        hub_state/*.cpp
        effect/test_effects/*.cpp
        logs_handler/*.cpp
        )

if( (${HUB_TYPE} MATCHES "g150") OR
    (${HUB_TYPE} MATCHES "g450") OR
    (${HUB_TYPE} MATCHES "g550")
  )
    file( GLOB BUTTONS_IMPL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/button/vera/*.cpp )
else()
    file( GLOB BUTTONS_IMPL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/button/generic/*.cpp )
endif()

if(${HUB_WITH_SPEAKER})
    file( GLOB SPEAKER_MODEL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/speaker/model/*.cpp )
    file( GLOB SPEAKER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/speaker/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/speaker/remote/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/speaker/environment/*.cpp )
    file( GLOB SPEAKER_TESTS_SRC
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/mocks/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/speaker_fixture.cpp )
else()
    file( GLOB SPEAKER_MODEL_SOURCES )
    file( GLOB SPEAKER_SRC )
    file( GLOB SPEAKER_TESTS_SRC )
endif()

#for now this code is unnecessary
list( REMOVE_ITEM HARDWARE_MANAGER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/button/netlink_button_event_listener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/button/netlink_istream.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/button/netlink_istream_buf.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/button/polling_button_event_listener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/button/button_event.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/button/event.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/button/event_listener.cpp
    )

add_library( ${PROJECT_NAME} STATIC ${HARDWARE_MANAGER_SOURCES} ${BUTTONS_IMPL_SRC} ${SPEAKER_SRC})
target_link_libraries( ${PROJECT_NAME}
    dbclient
    log_utils
    base_utils
    system_utils
    bus
    service_interfaces
    events_loop
    extensions_manager_interface
    networkd_interface
    cored_interface
    hardware_model
)

file( GLOB HUB_STATE_MODEL_SOURCES
    hub_state/model/*.cpp
)

add_library( hardware_model SHARED
        ${HUB_STATE_MODEL_SOURCES}
        ${SPEAKER_MODEL_SOURCES}
        )

target_link_libraries( hardware_model
    log_utils
    dbclient
)

if(${HUB_WITH_SPEAKER})
    target_link_libraries( ${PROJECT_NAME} extensions_manager_interface )
endif()

if(${HUB_TYPE} MATCHES "h2_secure.1")
    target_link_libraries( ${PROJECT_NAME} libbq27441g1 )
    target_link_libraries( ${PROJECT_NAME} libempirical_battery_manager )
endif()

add_executable(ha-${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_link_libraries(ha-${PROJECT_NAME} ${PROJECT_NAME})

# remote interface
if(${HUB_WITH_SPEAKER})
    file( GLOB SRC_FILES
            remote_interface/*.cpp
            speaker/remote/*.cpp
            )
    add_library( ${PROJECT_NAME}_interface SHARED ${SRC_FILES} )
    target_link_libraries( ${PROJECT_NAME}_interface
            service_interfaces
            hardware_model
    )

    # speaker bindings
    add_library(lua_speaker SHARED
            speaker/bindings/speaker_lua_bindings.cpp
            )
    target_link_libraries(lua_speaker
            ${PROJECT_NAME}_interface
            extensions_manager_interface
            lua_binding_interface
    )
else()
    file( GLOB SRC_FILES
            remote_interface/*.cpp
            )
    add_library( ${PROJECT_NAME}_interface SHARED ${SRC_FILES} )
    target_link_libraries( ${PROJECT_NAME}_interface
            service_interfaces
            hardware_model
            )
endif()

# TODO: failsafe reboot functionality can be provided not only for h2_secure hubs
if(${HUB_TYPE} MATCHES "h2_secure.1")
    add_subdirectory( failsafe_reboot )
endif()

HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES( ${PROJECT_NAME} ${PROJECT_NAME}_interface "" "${SPEAKER_TESTS_SRC}" )

set( PACKAGE_NAME firmware-core-${PROJECT_NAME})
install( TARGETS ${PROJECT_NAME}_interface DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
install( TARGETS ha-${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/bin )
install( PROGRAMS ${SOURCES_ROOT}/system/common/service_files/ha-03_hardware_manager DESTINATION ${PACKAGE_NAME}/etc/init.d )

install( FILES ${SOURCES_ROOT}/system/common/opt/firmware/etc/hardware_manager.cfg DESTINATION ${PACKAGE_NAME}/opt/firmware/etc )

if(${HUB_TYPE} MATCHES "h2_secure.1")
    install( FILES ${SOURCES_ROOT}/system/common/opt/firmware/etc/battery.cfg DESTINATION ${PACKAGE_NAME}/opt/firmware/etc )
endif()
install(TARGETS hardware_model DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
if(${HUB_WITH_SPEAKER})
    install(TARGETS lua_speaker DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
endif()
