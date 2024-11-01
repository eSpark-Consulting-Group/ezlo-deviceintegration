#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "database_client/dbtools.h"
#include "cored/house_modes/house_mode_utils.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN

1.  DONE    SwitchTo phase test
2.  DONE    Main phase test
3.  DONE    Idle phase test
4.  DONE    EntryDelay phase test
5.  DONE    Check no triggered received if already in target phase
*/

class HouseModeAlarmRangePhaseEventTests : public SceneManagerTestsFixture {
public:
    static void SetUpTestCase();
    void SetUp() override;
    void TearDown() override;
};

void HouseModeAlarmRangePhaseEventTests::TearDown() {
    ignoreBroadcasts(
        []( const Json& broadcast ) {
            Log::error( cc::error( "# TearDown: Not handled broadcasts: " ) + cc::j( broadcast ) );
            return true;
        },
        __FILE__,
        __LINE__,
        std::chrono::milliseconds( 10 ) );

    SceneManagerTestsFixture::TearDown();
}

void HouseModeAlarmRangePhaseEventTests::SetUpTestCase() {}

void HouseModeAlarmRangePhaseEventTests::SetUp() {
    SceneManagerTestsFixture::SetUp();
    callingFile( __FILE__ );
    auto& dbClient = nma().getDBClient();
    CoreDHouseModes::setAlarmPhase( dbClient, "idle", "" );
    CoreDHouseModes::setSwitchToPhase( dbClient, "" );
}

TEST_F( HouseModeAlarmRangePhaseEventTests, switchToPhase ) {  // 1
    auto& remoteInterface = makeRemote( nma() );
    auto houseMode = remoteInterface.getCurrentHouseModeId();
    if( houseMode != "1" ) {
        remoteInterface.setHouseMode( "1" );
        testHouseModesSwitchedBroadcast( "1", "begin", __LINE__ );
        testHouseModesSwitchedBroadcast( "1", "done", __LINE__ );
    }

    auto& dbClient = nma().getDBClient();
    remoteInterface.setSwitchDelay( 1 );
    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "isHouseModeSwitchToRange" },
                                    { "__WHEN_BLOCKS__", isHouseModeSwitchToRangeWhenBlock },
                                    { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );
    remoteInterface.setHouseMode( "3" );
    testHouseModesSwitchedBroadcast( "3", "begin", __LINE__ );
    auto switchToStatus = CoreDHouseModes::getSwitchToPhase( dbClient );
    EXPECT_EQ( switchToStatus, "begin" );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    testHouseModesSwitchedBroadcast( "3", "done", __LINE__, Tests::BroadcastTimeout );
    switchToStatus = CoreDHouseModes::getSwitchToPhase( dbClient );
    EXPECT_EQ( switchToStatus, "" );
    deleteScene( sceneId, __LINE__ );
}

TEST_F( HouseModeAlarmRangePhaseEventTests, mainPhase ) {  // 2
    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory = "smoke";
    const std::string testItemName = "security_threat";

    auto& remoteInterface = makeRemote( nma() );
    remoteInterface.setSwitchDelay( 0 );
    remoteInterface.setAlarmDelay( 0 );
    auto houseMode = remoteInterface.getCurrentHouseModeId();
    if( houseMode != "1" ) {
        remoteInterface.setHouseMode( "1" );
        testHouseModesSwitchedBroadcast( "1", "begin", __LINE__ );
        testHouseModesSwitchedBroadcast( "1", "done", __LINE__ );
    }

    auto deviceId = addDevice( remoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId =
        addItem( remoteInterface, deviceId, testItemName, CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    checkDeviceUpdateBroadcast( deviceId, { { "armed", true } }, __LINE__ );

    auto isHouseModeAlarmRange =
        replaceTokensInString( isHouseModeAlarmedRangeWhenBlock, { { "__PHASE__", "alarmed.main" } } );
    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "isHouseModeAlarmRange" },
                                    { "__WHEN_BLOCKS__", isHouseModeAlarmRange },
                                    { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    setItemValue( remoteInterface, itemId, true, __LINE__ );
    auto broadcast = waitForBroadcast( "hub.modes.alarmed", NMA_TEST_MAKE_MESSAGE( "expected hub.modes.alarmed" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.alarmed" );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );
}

TEST_F( HouseModeAlarmRangePhaseEventTests, idlePhase ) {  // 3
    updatePredefineGroupsForTest( "normal" );
    auto& dbClient = nma().getDBClient();
    const auto AlarmDelay = 1;

    auto isHouseModeAlarmRange = replaceTokensInString( isHouseModeAlarmedRangeWhenBlock, { { "__PHASE__", "idle" } } );
    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "isHouseModeAlarmRange" },
                                    { "__WHEN_BLOCKS__", isHouseModeAlarmRange },
                                    { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );

    CoreDHouseModes::setAlarmDelay( dbClient, AlarmDelay );
    auto& remoteInterface = makeRemote( nma() );
    auto device = addDeviceWithSecurityThreatItem( __LINE__ );
    setItemValue( remoteInterface, device.m_itemId, true, __LINE__ );
    const auto CurrentModeId = "1";
    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, device.m_deviceId, "entryDelay", "begin", __LINE__ );
    auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.entry_delay.cancel" } } );
    EXPECT_TRUE( answer[ "result" ].empty() );
    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, "", "entryDelay", "canceled", __LINE__ );
    answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.get" }, { "api", "2.0" } } );
    EXPECT_TRUE( answer[ "result" ][ "alarmed" ].empty() );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );
}

TEST_F( HouseModeAlarmRangePhaseEventTests, entryDelayPhase ) {  // 4
    updatePredefineGroupsForTest( "normal" );
    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory = "smoke";
    const std::string testItemName = "security_threat";

    auto& remoteInterface = makeRemote( nma() );
    remoteInterface.setSwitchDelay( 0 );
    auto& dbClient = nma().getDBClient();
    CoreDHouseModes::setAlarmDelay( dbClient, 1 );
    auto houseMode = remoteInterface.getCurrentHouseModeId();
    if( houseMode != "1" ) {
        remoteInterface.setHouseMode( "1" );
        testHouseModesSwitchedBroadcast( "1", "begin", __LINE__ );
        testHouseModesSwitchedBroadcast( "1", "done", __LINE__ );
    }

    auto deviceId = addDevice( remoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId =
        addItem( remoteInterface, deviceId, testItemName, CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    checkDeviceUpdateBroadcast( deviceId, { { "armed", true } }, __LINE__ );

    auto isHouseModeAlarmRange =
        replaceTokensInString( isHouseModeAlarmedRangeWhenBlock, { { "__PHASE__", "alarmed.entryDelay" } } );
    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "isHouseModeAlarmRange" },
                                    { "__WHEN_BLOCKS__", isHouseModeAlarmRange },
                                    { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    setItemValue( remoteInterface, itemId, true, __LINE__ );
    auto broadcast = waitForBroadcast( "hub.modes.alarmed", NMA_TEST_MAKE_MESSAGE( "expected hub.modes.alarmed" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.alarmed" );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    broadcast = waitForBroadcast( "hub.modes.alarmed", std::to_string( __LINE__ ) );

    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.alarmed" );
    broadcast = waitForBroadcast( "hub.modes.alarmed", std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.alarmed" );

    deleteScene( sceneId, __LINE__ );
}

TEST_F( HouseModeAlarmRangePhaseEventTests, mainPhaseNoTriggeredIfAlreadyInMainPhase ) {  // 5
    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory = "smoke";
    const std::string testItemName = "security_threat";

    auto& remoteInterface = makeRemote( nma() );
    remoteInterface.setSwitchDelay( 0 );
    remoteInterface.setAlarmDelay( 0 );
    auto houseMode = remoteInterface.getCurrentHouseModeId();
    if( houseMode != "1" ) {
        remoteInterface.setHouseMode( "1" );
        testHouseModesSwitchedBroadcast( "1", "begin", __LINE__ );
        testHouseModesSwitchedBroadcast( "1", "done", __LINE__ );
    }

    auto deviceId = addDevice( remoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId =
        addItem( remoteInterface, deviceId, testItemName, CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    checkDeviceUpdateBroadcast( deviceId, { { "armed", true } }, __LINE__ );

    auto isHouseModeAlarmRange =
        replaceTokensInString( isHouseModeAlarmedRangeWhenBlock, { { "__PHASE__", "alarmed.main" } } );
    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "isHouseModeAlarmRange" },
                                    { "__WHEN_BLOCKS__", isHouseModeAlarmRange },
                                    { "__THEN_BLOCKS__", "" } } );

    auto& dbClient = nma().getDBClient();
    CoreDHouseModes::setAlarmPhase( dbClient, "alarmed.main", "begin" );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    setItemValue( remoteInterface, itemId, true, __LINE__ );
    auto broadcast = waitForBroadcast( "hub.modes.alarmed", NMA_TEST_MAKE_MESSAGE( "expected hub.modes.alarmed" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.alarmed" );

    checkNoBroadcastReceived( "", __LINE__, std::chrono::seconds( 5 ) );

    deleteScene( sceneId, __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
