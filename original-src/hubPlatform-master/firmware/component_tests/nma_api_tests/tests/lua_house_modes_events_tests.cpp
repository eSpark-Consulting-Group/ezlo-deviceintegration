#include "common_test_resources/fixtures/core_test_fixture.h"
#include "common_test_resources/resources/cored_resources.h"
#include "test_utils/test_utils.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/house_modes/house_mode_utils.h"
#include "base_utils/utils.h"

#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. house_modes.subscribe
DONE        1.1 Exit time started and finished events
DONE        1.2 Exit time runs out event
DONE        1.3 Entry Delay event with done status
DONE        1.4 Entry Delay event with skipped status
DONE        1.5 Entry Delay event with canceled status
DONE        1.6 Local alarm off event

DONE    2. Lua house_modes protect button events
DONE        2.1 Add, update and remove device to protect buttons list, check lua events

*/

namespace HubPlatform {
namespace Tests {

namespace Resources {
static char SubscribeScript[] = "HUB:house_modes_test/scripts/subscribe_event";
static char UnSubscribeScript[] = "HUB:house_modes_test/scripts/unsubscribe_event";
}  // namespace Resources

class LuaHouseModesEventsTests : public CoreNmaTest {
public:
    void SetUp() override;
    void TearDown() override;

    JsonObject checkUiBroadcast( int line );
    void executeScript( const std::string& scriptPath, int line );
    void resetHouseModes( int line );

    void checkBroadcastParams( const std::string& event, const std::string& status, bool silent = false );

protected:
    JsonObject m_broadcast;

    void isProtectButtonWsEvent( const Json& broadcast,
                                 const std::string& operation,
                                 const std::string& deviceId,
                                 const std::string& serviceName,
                                 int codeLine );

    void isProtectButtonLuaEvent( const Json& broadcast,
                                  const std::string& operation,
                                  const std::string& deviceId,
                                  const std::string& serviceName,
                                  int codeLine );
};

void LuaHouseModesEventsTests::SetUp() {
    CoreNmaTest::SetUp();
    callingFile( __FILE__ );
}

void LuaHouseModesEventsTests::TearDown() {
    clearExistingDevices( __LINE__ );
    CoreNmaTest::TearDown();
}

JsonObject LuaHouseModesEventsTests::checkUiBroadcast( int line ) {
    JsonObject broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", getCodeLineString( line ) );
    EXPECT_EQ( broadcast.empty(), false ) << __FILE__ << ":" << getCodeLineString( line );
    JsonObject params;
    EXPECT_TRUE( json_get_obj( broadcast, "result", params ) ) << "line: " << getCodeLineString( line );
    return params;
}

void LuaHouseModesEventsTests::executeScript( const std::string& scriptPath, int line ) {
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", scriptPath } } );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    // TODO: EFWR-4409: make it reliable
    auto broadcast = checkUiBroadcast( line );

    checkExtensionsPluginRunProgressBroadcast( "started", line );

    checkExtensionsPluginRunProgressBroadcast( "finished", line );
}

void LuaHouseModesEventsTests::resetHouseModes( int line ) {
    CoreDRemoteInterface::CoreDInterface& coreRemoteInterface = makeRemote( nma() );

    coreRemoteInterface.setSwitchDelay( 0 );

    auto houseModeId = coreRemoteInterface.getCurrentHouseModeId();
    if( houseModeId != "1" ) {
        coreRemoteInterface.setHouseMode( "1" );
        testHouseModesSwitchedBroadcast( "1", "begin", line );
        testHouseModesSwitchedBroadcast( "1", "done", line );
    }
    EXPECT_EQ( coreRemoteInterface.getCurrentHouseModeId(), "1" );
}

void LuaHouseModesEventsTests::checkBroadcastParams( const std::string& event,
                                                     const std::string& status,
                                                     bool silent ) {
    EXPECT_EQ( json_get_str( m_broadcast, "event" ), event );
    EXPECT_EQ( json_get_str( m_broadcast, "plugin" ), "house_modes_test" );
    EXPECT_EQ( json_get_str( m_broadcast, "status" ), status );
    EXPECT_EQ( json_get_bool( m_broadcast, "silent" ), silent );
    if( event.find( "exit_time" ) == std::string::npos ) {
        EXPECT_TRUE( json_contains( m_broadcast, "sound_type" ) );
        EXPECT_TRUE( json_contains( m_broadcast, "chime" ) );
    }
}

void LuaHouseModesEventsTests::isProtectButtonWsEvent( const Json& broadcast,
                                                       const std::string& operation,
                                                       const std::string& deviceId,
                                                       const std::string& serviceName,
                                                       int codeLine ) {
    auto errorAt = getCodeLineString( codeLine );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.protect.buttons." + operation ) << errorAt;

    Json result;
    ASSERT_TRUE( json_get( broadcast, "result", result ) ) << errorAt;

    ASSERT_EQ( json_get_str( result, "deviceId" ), deviceId ) << errorAt;
    if( !serviceName.empty() ) {
        ASSERT_EQ( json_get_str( result, "service" ), serviceName ) << errorAt;
    }
}

void LuaHouseModesEventsTests::isProtectButtonLuaEvent( const Json& broadcast,
                                                        const std::string& operation,
                                                        const std::string& deviceId,
                                                        const std::string& serviceName,
                                                        int codeLine ) {
    auto errorAt = getCodeLineString( codeLine );

    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.extensions.plugin.ui_broadcast" ) << errorAt;

    Json result;
    ASSERT_TRUE( json_get( broadcast, "result", result ) ) << errorAt;

    const std::string expectedEvent = "protect_sos_button_" + operation;
    ASSERT_EQ( json_get_str( result, "event" ), expectedEvent ) << errorAt;
    ASSERT_EQ( json_get_str( result, "device_id" ), deviceId ) << errorAt;
    if( !serviceName.empty() ) {
        ASSERT_EQ( json_get_str( result, "service" ), serviceName ) << errorAt;
    }
}

TEST_F( LuaHouseModesEventsTests, exitTimeStartedFinishedEvent ) {  // 1.1
    Json broadcast;
    CoreDRemoteInterface::CoreDInterface& coreRemoteInterface = makeRemote( nma() );

    resetHouseModes( __LINE__ );
    coreRemoteInterface.setSwitchDelay( 2 );

    executeScript( Resources::SubscribeScript, __LINE__ );

    coreRemoteInterface.setHouseMode( "2" );
    testHouseModesSwitchedBroadcast( "2", "begin", __LINE__ );
    broadcast = checkUiBroadcast( __LINE__ );  // ignoring house_mode_switched lua event
    m_broadcast = checkUiBroadcast( __LINE__ );
    checkBroadcastParams( "exit_time_started", "" );

    testHouseModesSwitchedBroadcast( "2", "done", __LINE__ );
    broadcast = checkUiBroadcast( __LINE__ );  // ignoring house_mode_switched lua event
    m_broadcast = checkUiBroadcast( __LINE__ );
    checkBroadcastParams( "exit_time_finished", "" );

    executeScript( Resources::UnSubscribeScript, __LINE__ );
}

TEST_F( LuaHouseModesEventsTests, exitTimeRunsOutEvent ) {  // 1.2

    CoreDRemoteInterface::CoreDInterface& coreRemoteInterface = makeRemote( nma() );

    resetHouseModes( __LINE__ );
    coreRemoteInterface.setSwitchDelay( 15 );
    executeScript( Resources::SubscribeScript, __LINE__ );

    coreRemoteInterface.setHouseMode( "2" );
    testHouseModesSwitchedBroadcast( "2", "begin", __LINE__ );
    auto broadcast = checkUiBroadcast( __LINE__ );  // ignoring house_mode_switched lua event
    m_broadcast = checkUiBroadcast( __LINE__ );
    checkBroadcastParams( "exit_time_started", "" );

    m_broadcast = checkUiBroadcast( __LINE__ );
    checkBroadcastParams( "exit_time_runs_out", "" );

    testHouseModesSwitchedBroadcast( "2", "done", __LINE__, std::chrono::seconds( 15 ) );
    broadcast = checkUiBroadcast( __LINE__ );  // ignoring house_mode_switched lua event
    m_broadcast = checkUiBroadcast( __LINE__ );
    checkBroadcastParams( "exit_time_finished", "" );

    executeScript( Resources::UnSubscribeScript, __LINE__ );
    resetHouseModes( __LINE__ );
}

TEST_F( LuaHouseModesEventsTests, entryDelayEventWithDoneStatus ) {  // 1.3

    auto& dbClient = nma().getDBClient();
    const auto AlarmDelay = 1;
    CoreDHouseModes::setAlarmDelay( dbClient, AlarmDelay );
    CoreDRemoteInterface::CoreDInterface& remoteInterface = makeRemote( nma() );
    remoteInterface.setSwitchDelay( 0 );
    remoteInterface.setHouseMode( "2" );
    testHouseModesSwitchedBroadcast( "2", "begin", __LINE__ );
    testHouseModesSwitchedBroadcast( "2", "done", __LINE__ );
    auto device = addDeviceWithSecurityThreatItem( __LINE__ );

    executeScript( Resources::SubscribeScript, __LINE__ );

    setItemValue( remoteInterface, device.m_itemId, true, __LINE__ );
    const auto CurrentModeId = "2";
    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, device.m_deviceId, "entryDelay", "begin", __LINE__ );
    m_broadcast = checkUiBroadcast( __LINE__ );
    checkBroadcastParams( "entry_delay_started", "" );

    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, device.m_deviceId, "entryDelay", "done", __LINE__ );
    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, device.m_deviceId, "main", "begin", __LINE__ );
    m_broadcast = checkUiBroadcast( __LINE__ );
    checkBroadcastParams( "entry_delay_finished", "done" );

    m_broadcast = checkUiBroadcast( __LINE__ );  // ignoring house_mode_alarmed lua event
}

TEST_F( LuaHouseModesEventsTests, entryDelayEventWithSkippedStatus ) {  // 1.4

    auto& dbClient = nma().getDBClient();
    const auto AlarmDelay = 2;
    CoreDHouseModes::setAlarmDelay( dbClient, AlarmDelay );
    CoreDRemoteInterface::CoreDInterface& remoteInterface = makeRemote( nma() );
    auto device = addDeviceWithSecurityThreatItem( __LINE__ );

    setItemValue( remoteInterface, device.m_itemId, true, __LINE__ );
    const auto CurrentModeId = "2";
    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, device.m_deviceId, "entryDelay", "begin", __LINE__ );
    m_broadcast = checkUiBroadcast( __LINE__ );
    checkBroadcastParams( "entry_delay_started", "" );

    auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.entry_delay.skip" } } );
    EXPECT_TRUE( answer[ "error" ].empty() );
    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, "", "main", "begin", __LINE__ );
    m_broadcast = checkUiBroadcast( __LINE__ );
    checkBroadcastParams( "entry_delay_finished", "skipped" );
}

TEST_F( LuaHouseModesEventsTests, entryDelayEventWithCancelStatus ) {  // 1.5

    auto& dbClient = nma().getDBClient();
    const auto AlarmDelay = 2;
    CoreDHouseModes::setAlarmDelay( dbClient, AlarmDelay );
    CoreDRemoteInterface::CoreDInterface& remoteInterface = makeRemote( nma() );
    auto device = addDeviceWithSecurityThreatItem( __LINE__ );

    setItemValue( remoteInterface, device.m_itemId, true, __LINE__ );
    const auto CurrentModeId = "2";
    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, device.m_deviceId, "entryDelay", "begin", __LINE__ );
    m_broadcast = checkUiBroadcast( __LINE__ );
    checkBroadcastParams( "entry_delay_started", "" );

    auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.entry_delay.cancel" } } );
    EXPECT_TRUE( answer[ "error" ].empty() );
    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, "", "entryDelay", "canceled", __LINE__ );
    m_broadcast = checkUiBroadcast( __LINE__ );
    checkBroadcastParams( "entry_delay_finished", "canceled" );

    executeScript( Resources::UnSubscribeScript, __LINE__ );
    resetHouseModes( __LINE__ );
}

TEST_F( LuaHouseModesEventsTests, localAlarmOffEvent ) {  // 1.6

    executeScript( Resources::SubscribeScript, __LINE__ );
    auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.local.alarm.off" },
                                                 { "params",
                                                   JSON( "modeId"
                                                         << "1" ) } } );
    EXPECT_TRUE( answer[ "error" ].empty() );

    auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.local.alarmed.turned.off" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.local.alarmed.turned.off" );
    broadcast = checkUiBroadcast( __LINE__ );
    EXPECT_EQ( json_get_str( broadcast, "event" ), "house_mode_local_alarms_turned_off" );
    EXPECT_EQ( json_get_str( broadcast, "plugin" ), "house_modes_test" );

    executeScript( Resources::UnSubscribeScript, __LINE__ );
    resetHouseModes( __LINE__ );
}

TEST_F( LuaHouseModesEventsTests, houseModesEventsWithProtectAlarmItem ) {  // 1.7

    auto& dbClient = nma().getDBClient();
    CoreDRemoteInterface::CoreDInterface& remoteInterface = makeRemote( nma() );
    remoteInterface.setSwitchDelay( 0 );
    remoteInterface.setHouseMode( "2" );
    testHouseModesSwitchedBroadcast( "2", "begin", __LINE__ );
    testHouseModesSwitchedBroadcast( "2", "done", __LINE__ );
    auto device = addDeviceWithProtectAlarmItem( __LINE__ );

    executeScript( Resources::SubscribeScript, __LINE__ );

    {
        setItemValue( remoteInterface, device.m_itemId, "alarm_active", __LINE__ );
        const auto CurrentModeId = "2";

        testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, device.m_deviceId, "main", "begin", __LINE__ );
        m_broadcast = checkUiBroadcast( __LINE__ );
        checkBroadcastParams( "house_mode_alarmed", "begin" );
    }

    {
        setItemValue( remoteInterface, device.m_itemId, "alarm_active_silent", __LINE__ );
        const auto CurrentModeId = "2";
        testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, device.m_deviceId, "main", "begin", __LINE__, true );
        m_broadcast = checkUiBroadcast( __LINE__ );
        checkBroadcastParams( "house_mode_alarmed", "begin", true );
    }
    executeScript( Resources::UnSubscribeScript, __LINE__ );
    resetHouseModes( __LINE__ );
    auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.alarmed" );
}

TEST_F( LuaHouseModesEventsTests, protectDeviceAddUpdateRemoveEvents ) { /* 2.1 */

    // Create a button device
    auto device1 = addDeviceWithSecurityThreatItem( __LINE__ );

    // Subscribe to lua house_modes module events
    executeScript( Resources::SubscribeScript, __LINE__ );

    // On Exit: Unsubscribe from lua house_modes module events
    utils::AfterReturn onExit( [&]() { executeScript( Resources::UnSubscribeScript, __LINE__ ); } );

    // Add the device to house modes protect buttons collection with hub.modes.protect.buttons.set(  deviceId,
    // servicePolice )

    const std::string servicePolice = "police";
    const std::string serviceFire = "fire";

    auto formatProtectButtonSet = []( const std::string& deviceId, const std::string& serviceName ) -> JsonObject {
        JsonObject request = { { "method", "hub.modes.protect.buttons.set" },
                               { "params", JSON( "deviceId" << deviceId << "service" << serviceName ) } };

        if( serviceName.empty() ) {
            json_remove( request[ "params" ], "service" );
        }
        return request;
    };

    const std::string& deviceId = device1.m_deviceId;
    {
        auto protectButtonsSetCommand = formatProtectButtonSet( deviceId, servicePolice );

        Json answer = nma().sendtoHubAndGetAnswer( protectButtonsSetCommand );
        ASSERT_FALSE( answer.empty() ) << "should not be empty";
        ASSERT_TRUE( answer[ "error" ].is_null() );

        // wait for hub.modes.protect.buttons.added websocket broadcast
        // check lua event broadcasts: ui_broadcast, event = 'protect_sos_button_added'

        Json broadcast;
        auto checkers = CommonNmaTests::BroadcastCheckers{
            { "hub.modes.protect.buttons.added",
              [&]( const Json& broadcast, int line ) {
                  isProtectButtonWsEvent( broadcast, "added", deviceId, servicePolice, line );
              } },
            { "hub.extensions.plugin.ui_broadcast",
              [&]( const Json& broadcast, int line ) {
                  isProtectButtonLuaEvent( broadcast, "added", deviceId, servicePolice, line );
              } }
        };

        checkBroadcasts( checkers, __LINE__ );
    }

    // Update the protect button device with another service: emits "updated" events
    {
        auto protectButtonsSetCommand = formatProtectButtonSet( deviceId, serviceFire );

        Json answer = nma().sendtoHubAndGetAnswer( protectButtonsSetCommand );
        ASSERT_FALSE( answer.empty() ) << "should not be empty";
        ASSERT_TRUE( answer[ "error" ].is_null() );

        // wait for hub.modes.protect.buttons.added websocket broadcast
        // check lua event broadcasts: ui_broadcast, event = 'protect_sos_button_updated'

        Json broadcast;
        auto checkers = CommonNmaTests::BroadcastCheckers{
            { "hub.modes.protect.buttons.updated",
              [&]( const Json& broadcast, int line ) {
                  isProtectButtonWsEvent( broadcast, "updated", deviceId, serviceFire, line );
              } },
            { "hub.extensions.plugin.ui_broadcast",
              [&]( const Json& broadcast, int line ) {
                  isProtectButtonLuaEvent( broadcast, "updated", deviceId, serviceFire, line );
              } }
        };

        checkBroadcasts( checkers, __LINE__ );
    }

    // Remove the device from house modes protect buttons collection with hub.modes.protect.buttons.set( deviceId )
    // check lua event broadcasts: ui_broadcast, event = 'protect_sos_button_removed'

    {
        auto protectButtonsSetRemoveCommand = formatProtectButtonSet( deviceId, "" );

        Json answer = nma().sendtoHubAndGetAnswer( protectButtonsSetRemoveCommand );
        ASSERT_FALSE( answer.empty() ) << "should not be empty";
        ASSERT_TRUE( answer[ "error" ].is_null() );

        Json broadcast;
        auto checkers =
            CommonNmaTests::BroadcastCheckers{ { "hub.modes.protect.buttons.removed",
                                                 [&]( const Json& broadcast, int line ) {
                                                     isProtectButtonWsEvent( broadcast, "removed", deviceId, "", line );
                                                 } },
                                               { "hub.extensions.plugin.ui_broadcast",
                                                 [&]( const Json& broadcast, int line ) {
                                                     isProtectButtonLuaEvent(
                                                         broadcast, "removed", deviceId, "", line );
                                                 } } };

        checkBroadcasts( checkers, __LINE__ );
    }
}

}  // namespace Tests
}  // namespace HubPlatform
