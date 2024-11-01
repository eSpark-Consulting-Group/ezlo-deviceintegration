#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "scene_tests/application.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "common_test_resources/resources/cored_resources.h"

#include "scenes_manager/test_remote_interface/test_scenes_remote_interface.h"
#include "scenes_manager/remote_interface/scenes_remote_interface.h"

#include "database_client/fake_db/fake_db_client.h"

#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/model/cored_item.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/values/value_type.h"
#include "cored/model/values/scalable_value.h"
#include "cored/model/resources/cored_resources.h"

#include "hardware_manager/battery/battery_info.h"

#include "jsonrpc/jsonrpc.h"
#include "database_client/dbtools.h"
#include "database_client/dbqueryparser.h"

#include "base_utils/gregorian_calendar.h"
#include "base_utils/timeutils.h"
#include "test_utils/component_tests/http_server/http_server.h"

#include "system_utils/process/shared_mutex.h"
#include "system_utils/find_free_local_port.h"

#include <gtest/gtest.h>
#include <iostream>
#include <ctime>
#include <functional>
#include <curl/curl.h>
#include "tiny/tiny_consolecolors.h"

#include "cored/model/values/values.h"
#include "cored/house_modes/house_mode_utils.h"

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN

DONE    0. Bug fix tests
DONE        0.0 createSceneEmptyWhenBlock
DONE        0.1 check sunset scene creation ( EFWR-1517 )
DONE        0.2 fix crash of scenes manager after "hub.scenes.edit" request ( EFWR-1561 )
DONE        0.3 scene with empty name should not be created ( EFWR-1580 )
DONE        0.4 actions with same item values and different delays ( EFWR-1585 )
DONE        0.5 check error info when attempt to delete a scene that does not exist ( EFWR-1581 )
DONE    1. Scene Manager
DONE        1.0 Create and delete scene
DONE        1.1 Interval scene
DONE        1.2 Daily scene ( isDate condition )
DONE            1.2.1 event switching to TRUE
DONE            1.2.2 switch to FALSE is recorded to states registry
DONE        1.3 Weekly scene ( isDate condition )
DONE        1.4 Monthly scene
DONE            1.4.1 isDate weeks of month
DONE            1.4.2 isDate yearWeeks: weeks of a year
DONE        1.5 Sunset scene
DONE            1.5.1 daily sunset intime event
DONE            1.5.2 daily some time after sunset event
DONE            1.5.3 daily some time before sunset event
DONE            1.5.4 weekly sunset intime event
DONE            1.5.5 weekly some time after sunset event
DONE            1.5.6 weekly some time before sunset event
DONE            1.5.7 monthly sunset intime event
DONE            1.5.8 monthly some time after sunset event
DONE            1.5.9 monthly some time before sunset event
DONE            1.5.10 range 'after' (from sunset till midnight of current day)
DONE            1.5.11 range 'before' (from midnight of current day till defined sunset)
DONE            1.5.12 scene not triggered on creation if the time range already begun
DONE        1.6 Sunrise scene
DONE            1.6.1 daily sunrise intime event
DONE            1.6.2 daily some time after sunrise event
DONE            1.6.3 daily some time before sunrise event
DONE            1.6.4 weekly sunrise intime event
DONE            1.6.5 weekly some time after sunrise event
DONE            1.6.6 weekly some time before sunrise event
DONE            1.6.7 monthly sunrise intime event
DONE            1.6.8 monthly some time after sunrise event
DONE            1.6.9 monthly some time before sunrise event
DONE            1.6.10 range 'after' (from sunrise till midnight of current day)
DONE            1.6.11 range 'before' (from midnight of current day till defined sunrise)
DONE            1.6.12 scene not triggered on creation if the time range already begun
DONE        1.7 isItemState scene
DONE            1.7.1 single item
DONE            1.7.2 multiple items in one device
DONE            1.7.3 items without armed state
DONE        1.8 isOnce scene
DONE        1.9 setItemValue action
DONE            1.9.1 single action
DONE            1.9.2 multicast action
DONE        1.10 setDeviceArmed action
TODO        1.11 increaseDimmer action
TODO        1.12 decreaseDimmer action
DONE        1.13 hub.scenes.get request
DONE        1.14 hub.scenes.list request
DONE        1.15 hub.scenes.edit request
DONE            1.15.1 hub scene edit request successfull
DONE            1.15.2 hub edit scene with setSceneState
DONE        1.16 hub.scenes.enabled.set request
DONE            1.16.1 check weekly scene enabling/disabling
DONE            1.16.2 check interval scene enabling/disabling
DONE            1.16.3 check isItemState scene enabling/disabling
DONE            1.16.4 check isButtonState scene enabling/disabling
DONE        1.17 hub.scenes.notification.add request
DONE        1.18 hub.scenes.notification.remove request
DONE        1.19 hub.scenes.run request
DONE            1.19.1 Scene hasn't "else" actions
DONE            1.19.2 Scene has "else" actions
DONE            1.19.3 Ambiguous request, when "block" field is required but not specified
DONE        1.20 Room assignment
DONE            1.20.1 hub.scenes.room.set request
DONE            1.20.2 delete room and check scene
DONE        1.21 House modes condition
DONE        1.22 Get Time Scenes List
DONE        1.23 Action Delay
TODO        1.24
DONE        1.25 userNotificationFalseOnStartScene
DONE        1.26 userNotificationTrueOnOtherSceneStatuses
DONE        1.27 hub.scenes.blocks.list
DONE            1.27.1 hub.scenes.blocks.list
DONE            1.27.2 align options
DONE                1.27.2.1 Value type
DONE                    1.27.2.1.1 Token
DONE                    1.27.2.1.2 ButtonState
DONE            1.27.3 option items
DONE            1.27.4 filtration by category and subcategory of device
DONE        1.28 when block compareNumbers
DONE            1.28.1 check errors during scene creation
DONE            1.28.2 check scene execution
DONE        1.29 hub.scenes.trigger.devices.list request
DONE        1.30 when block isButtonState
DONE            1.30.1 check errors during scene creation
DONE            1.30.2 check scene execution
DONE        1.31 requests are used by server
DONE        1.32 AND operator
DONE            1.32.1 AND operator errors testing
DONE            1.32.2 simple and nested AND operators
DONE            1.32.3 AND operators allowed time based blocks combinations
DONE        1.33 NOT operator
DONE            1.33.1 NOT operator errors testing
DONE            1.33.2 NOT operator testing
DONE        1.34 OR, XOR operator
DONE            1.34.1 OR operator errors testing
DONE            1.34.2 OR operator testing
DONE            1.34.3 XOR operator errors testing
DONE            1.34.4 XOR operator testing
DONE        1.35 sendCloudAbstractCommand action
DONE        1.36 when block isUserLockOperation
DONE            1.36.1 check errors during scene creation
DONE            1.36.2 check scene execution
DONE            1.36.3 check scene execution on user_code_action item
DONE        1.37 when block hasAtLeastOneDictionaryValue
DONE        1.38 then block switchHouseMode
DONE        1.39 modify scene
DONE            1.39.1 simple scene
DONE            1.39.2 nested scene
DONE        1.40 hub.scenes.icon.set request
DONE        1.41 hub.scenes.action.block.test request
DONE        1.42 hub.scenes.name.set request
DONE        1.43 hub.scenes.house_modes.set request
DONE        1.44 check runCustomScript action block
DONE        1.46 when block isHouseModeChangedTo
DONE            1.46.1 Success case
DONE            1.46.2 Errors
DONE        1.47 when block isHouseModeChangedFrom
DONE            1.47.1 Success case
DONE            1.47.2 Errors
DONE        1.48 when block isNetworkState
DONE            1.48.1 main interface
DONE            1.48.2 trigger at any main interface
DONE            1.48.3 connection failed
DONE            1.48.4 main interface failed
DONE            1.48.5 errors
DONE        1.49 when block isSceneState
DONE            1.49.1 errors
DONE            1.49.2 finished scene result
DONE            1.49.3 partially_finished scene result
DONE            1.49.4 any scene result
DONE            1.49.5 scene enabled
DONE            1.49.6 scene disabled
DONE        1.50 block id adding
DONE        1.51 when block isCloudState
DONE            1.51.1 connected state
DONE            1.51.2 disconnected state
DONE        1.52 enable/disable when block
DONE            1.52.1 single when block
DONE            1.52.2 operator when block
DONE            1.52.3 nested operators
DONE        1.53 When block condition trigger (then/else actions)
DONE        1.54 DictionaryChanged error test
DONE        1.55 DictionaryChanged added happypath test
DONE            1.55.1 DictionaryChanged updated happypath test
DONE            1.55.2 DictionaryChanged removed happypath test
DONE        1.56 Days of period event
DONE            1.56.1 Days of week
DONE            1.56.1 Days of month

*/

class SceneManagerTests : public SceneManagerTestsFixture {
public:
    static void SetUpTestCase();
    void SetUp() override;
    void TearDown() override;
};

void SceneManagerTests::TearDown() {
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

void SceneManagerTests::SetUpTestCase() {
    curl_global_init( CURL_GLOBAL_ALL );
    CoreDHouseModes::setAlarmPhase( nma().getDBClient(), "idle", "" );
}

void SceneManagerTests::SetUp() {
    SceneManagerTestsFixture::SetUp();
    callingFile( __FILE__ );
}

class SunsetSunriseTests : public SceneManagerTests {
public:
    void SetUp() override {
        SceneManagerTests::SetUp();
        setGeoCoordinates();
        callingFile( __FILE__ );
    }

    static const std::chrono::seconds clockDelayBeforeSceneStart;
};

const std::chrono::seconds SunsetSunriseTests::clockDelayBeforeSceneStart( -60 );

TEST_F( SceneManagerTests, createSceneEmptyWhenBlock ) {  // 0.0
    SceneID sceneId = createScene( createEmptyWhenBlockSceneJson );
    auto answer = nma().sendtoHubAndGetAnswer( getScenesList );
    JsonArray scenesList = answer[ "result" ][ "scenes" ];
    ASSERT_EQ( scenesList.size(), 1 );
    auto it = scenesList.begin();
    EXPECT_EQ( json_get_str( *it, "_id" ), sceneId );
    EXPECT_EQ( ( *it )[ "enabled" ].dump(), createEmptyWhenBlockSceneJson[ "params" ][ "enabled" ].dump() );
    EXPECT_EQ( ( *it )[ "group_id" ].dump(), createEmptyWhenBlockSceneJson[ "params" ][ "group_id" ].dump() );
    EXPECT_EQ( ( *it )[ "is_group" ].dump(), createEmptyWhenBlockSceneJson[ "params" ][ "is_group" ].dump() );
    EXPECT_EQ( ( *it )[ "name" ].dump(), createEmptyWhenBlockSceneJson[ "params" ][ "name" ].dump() );
    EXPECT_EQ( ( *it )[ "parent_id" ].dump(), createEmptyWhenBlockSceneJson[ "params" ][ "parent_id" ].dump() );
    EXPECT_EQ( ( *it )[ "then" ].dump(), createEmptyWhenBlockSceneJson[ "params" ][ "then" ].dump() );
    EXPECT_EQ( ( *it )[ "when" ].dump(), createEmptyWhenBlockSceneJson[ "params" ][ "when" ].dump() );
}

TEST_F( SceneManagerTests, createSunsetScene ) {  // 0.1
    SceneID sceneId = createScene( createSunsetScene );
    auto answer = nma().sendtoHubAndGetAnswer( getScenesList );
    JsonArray scenesList = answer[ "result" ][ "scenes" ];
    ASSERT_EQ( scenesList.size(), 1 );
    auto it = scenesList.begin();
    removeBlockIds( ( *it )[ "when" ] );
    EXPECT_EQ( json_get_str( *it, "_id" ), sceneId );
    EXPECT_EQ( ( *it )[ "enabled" ].dump(), createSunsetScene[ "params" ][ "enabled" ].dump() );
    EXPECT_EQ( ( *it )[ "group_id" ].dump(), createSunsetScene[ "params" ][ "group_id" ].dump() );
    EXPECT_EQ( ( *it )[ "is_group" ].dump(), createSunsetScene[ "params" ][ "is_group" ].dump() );
    EXPECT_EQ( ( *it )[ "name" ].dump(), createSunsetScene[ "params" ][ "name" ].dump() );
    EXPECT_EQ( ( *it )[ "parent_id" ].dump(), createSunsetScene[ "params" ][ "parent_id" ].dump() );
    EXPECT_EQ( ( *it )[ "then" ].dump(), createSunsetScene[ "params" ][ "then" ].dump() );
    EXPECT_EQ( ( *it )[ "when" ].dump(), createSunsetScene[ "params" ][ "when" ].dump() );
}

TEST_F( SceneManagerTests, fixCrashAfterEditRequest ) {  // 0.2
    SceneID sceneId = createScene( EFWR_1561_createScene );
    std::string dump = HubPlatform::Tests::EFWR_1561_editScene.dump();
    std::string toReplace = "__SCENE_ID__";
    dump.replace( dump.find( toReplace ), toReplace.size(), sceneId );
    JsonObject editScene = JsonObject::parse( dump );
    auto answer = nma().sendtoHubAndGetAnswer( editScene );
    const auto& error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    checkChangedBroadcastEditRequest( sceneId, editScene, __LINE__ );
}

TEST_F( SceneManagerTests, emptySceneName ) {  // 0.3
    auto checkError = []( const JsonObject& error, int codeLine ) {
        std::string codeLineStr = "code line: " + std::to_string( codeLine );
        EXPECT_EQ( json_get_int( error, "code" ), -32600 ) << codeLineStr;
        EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.empty.name" ) << codeLineStr;
        EXPECT_EQ( json_get_str( error, "message" ), "Bad request, name is empty" ) << codeLineStr;
    };

    auto answer = nma().sendtoHubAndGetAnswer( replaceTokens( EFWR_1580_createScene, { { "__NAME__", "" } } ) );
    checkError( answer[ "error" ], __LINE__ );

    answer = nma().sendtoHubAndGetAnswer( replaceTokens( EFWR_1580_createScene, { { "__NAME__", " " } } ) );
    checkError( answer[ "error" ], __LINE__ );

    answer = nma().sendtoHubAndGetAnswer( replaceTokens( EFWR_1580_createScene, { { "__NAME__", "      " } } ) );
    checkError( answer[ "error" ], __LINE__ );

    SceneID sceneId = createScene( replaceTokens( EFWR_1580_createScene, { { "__NAME__", "EFWR_1580" } } ) );
    answer = nma().sendtoHubAndGetAnswer(
        replaceTokens( EFWR_1580_editScene, { { "__NAME__", "" }, { "__SCENE_ID__", sceneId } } ) );
    checkError( answer[ "error" ], __LINE__ );

    answer = nma().sendtoHubAndGetAnswer(
        replaceTokens( EFWR_1580_editScene, { { "__NAME__", " " }, { "__SCENE_ID__", sceneId } } ) );
    checkError( answer[ "error" ], __LINE__ );

    answer = nma().sendtoHubAndGetAnswer(
        replaceTokens( EFWR_1580_editScene, { { "__NAME__", "   " }, { "__SCENE_ID__", sceneId } } ) );
    checkError( answer[ "error" ], __LINE__ );
}

TEST_F( SceneManagerTests, sameItemValuesAndDifferentDelays ) {  // 0.4
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );
    auto itemId =
        addItem( coreRemoteInterface, deviceId, "testItem", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    coreRemoteInterface.setItemValue( JSON( CoreD::Model::Resources::ItemFields::IdFieldName
                                            << itemId << CoreD::Model::Resources::ItemFields::ValueFieldName << 1 ) );
    checkItemUpdateBroadcast( itemId, 1, __LINE__ );

    auto createSceneRequest = replaceTokens( createSameActionsDifferentDelays, { { "__ITEM_ID__", itemId } } );
    SceneID sceneId = createScene( createSceneRequest );
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( createSceneRequest, sceneId, "started", __LINE__ );
    checkItemUpdateBroadcast( itemId, 5, __LINE__ );
    checkItemUpdateBroadcast( itemId, 4, __LINE__ );
    auto checkLastBroadcast = [&]( CoreDRemoteInterface::CoreDInterface::ItemID itemId, int itemValue, int codeLine ) {
        JsonObject broadcast =
            waitForAnyOfBroadcasts( { "hub.scene.run.progress", "hub.item.updated" }, getCodeLineString( codeLine ) );
        if( json_get_str( broadcast, "msg_subclass" ) == "hub.scene.run.progress" ) {
            testRunProgressBroadcast( broadcast, createSceneRequest, sceneId, "finished", codeLine );
        }
        else {
            checkItemUpdateBroadcast( broadcast, itemId, itemValue, codeLine );
        }
    };
    checkLastBroadcast( itemId, 3, __LINE__ );
    checkLastBroadcast( itemId, 2, __LINE__ );
    checkLastBroadcast( itemId, 2, __LINE__ );

    JsonObject broadcast;
    coreRemoteInterface.removeDevice( deviceId );
    broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
}

TEST_F( SceneManagerTests, sceneDeletingError ) {  // 0.5
    auto deleteRequest = replaceTokens( HubPlatform::Tests::deleteScene, { { "__SCENE_ID__", "" } } );
    auto answer = nma().sendtoHubAndGetAnswer( deleteRequest );
    auto error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32600 );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.empty._id" );
    EXPECT_EQ( json_get_str( error, "message" ), "Bad request, _id is empty" );

    answer = nma().sendtoHubAndGetAnswer( HubPlatform::Tests::deleteScene );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.not.exist" );
    EXPECT_EQ( json_get_str( error, "message" ), "The scene with this id does not exist" );
}

/// @brief
///  create scene ( hub.scenes.create )
///  check hub.scene.added broadcast
///  delete scene
///  check hub.scene.deleted broadcast
TEST_F( SceneManagerTests, createDeleteScene ) {  // 1.0
    auto request = replaceTokens( createSceneRequest, { { "__THEN_BLOCKS__", "" }, { "__WHEN_BLOCKS__", "" } } );
    auto wrongRequest = request;
    wrongRequest[ "params" ].erase( "enabled" );
    auto answer = nma().sendtoHubAndGetAnswer( wrongRequest );
    checkErrorFieldNotFound( answer, "enabled", __LINE__ );

    SceneID sceneId = createScene( createSceneJson );
    answer = nma().sendtoHubAndGetAnswer( getScenesList );
    JsonArray scenesList = answer[ "result" ][ "scenes" ];
    ASSERT_EQ( scenesList.size(), 1 );
    auto it = scenesList.begin();
    removeBlockIds( ( *it )[ "when" ] );
    EXPECT_EQ( json_get_str( *it, "_id" ), sceneId );
    EXPECT_EQ( ( *it )[ "enabled" ].dump(), createSceneJson[ "params" ][ "enabled" ].dump() );
    EXPECT_EQ( ( *it )[ "group_id" ].dump(), createSceneJson[ "params" ][ "group_id" ].dump() );
    EXPECT_EQ( ( *it )[ "is_group" ].dump(), createSceneJson[ "params" ][ "is_group" ].dump() );
    EXPECT_EQ( ( *it )[ "name" ].dump(), createSceneJson[ "params" ][ "name" ].dump() );
    EXPECT_EQ( ( *it )[ "parent_id" ].dump(), createSceneJson[ "params" ][ "parent_id" ].dump() );
    EXPECT_EQ( ( *it )[ "then" ].dump(), createSceneJson[ "params" ][ "then" ].dump() );
    EXPECT_EQ( ( *it )[ "when" ].dump(), createSceneJson[ "params" ][ "when" ].dump() );
}

/// @brief
/// create scene with 2 second period
/// check hub.run.progress broadcast after 1st second
/// check hub.run.progress broadcast after 2nd second
/// delete scene and check hub.scene.deleted broadcast
TEST_F( SceneManagerTests, intervalScene ) {  // 1.1
    SceneID sceneId = createScene( createPeriodicalScene );
    checkNoBroadcastReceived( "Waiting for interval", __LINE__, std::chrono::seconds( 1 ) );
    testRunProgressBroadcast( createPeriodicalScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createPeriodicalScene, sceneId, "finished", __LINE__ );
    checkNoBroadcastReceived( "Waiting for interval", __LINE__, std::chrono::seconds( 1 ) );
    testRunProgressBroadcast( createPeriodicalScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createPeriodicalScene, sceneId, "finished", __LINE__ );
    checkNoBroadcastReceived( "Waiting for interval", __LINE__, std::chrono::seconds( 1 ) );
    testRunProgressBroadcast( createPeriodicalScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createPeriodicalScene, sceneId, "finished", __LINE__ );
}

/// @brief
/// create daily scene with two events within each day
/// check both events at first day
/// check both events at second day
/// delete scene and check hub.scene.deleted broadcast
TEST_F( SceneManagerTests, dailyScene ) {  // 1.2.1
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    auto response = scenesManagerInterface.setTestClock( std::chrono::seconds( 1562675471LL ) );
    ASSERT_TRUE( response[ "error" ].empty() );
    SceneID sceneId = createScene( createDailyScene );
    response = scenesManagerInterface.setTestClock( std::chrono::seconds( 1562676298LL ) );
    testRunProgressBroadcast( createDailyScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createDailyScene, sceneId, "finished", __LINE__ );

    scenesManagerInterface.setTestClock( std::chrono::seconds( 1562714218LL ) );
    testRunProgressBroadcast( createDailyScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createDailyScene, sceneId, "finished", __LINE__ );

    scenesManagerInterface.setTestClock( std::chrono::seconds( 1562762698LL ) );
    testRunProgressBroadcast( createDailyScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createDailyScene, sceneId, "finished", __LINE__ );

    scenesManagerInterface.setTestClock( std::chrono::seconds( 1562800618LL ) );
    testRunProgressBroadcast( createDailyScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createDailyScene, sceneId, "finished", __LINE__ );
}

TEST_F( SceneManagerTests, dailySceneSwitchToFalseRecordedToStatesRegistry ) {  // 1.2.2
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    CoreDRemoteInterface::CoreDInterface& coredRemote = makeRemote( nma() );

    auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    auto thenBlock = replaceTokensInString(
        setItemValueThenBlock, { { "__ITEM_ID__", device.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "1" } } );
    auto scene = createDailyScene;
    scene[ "params" ][ "then" ] = JSON_ARRAY( Json::parse( thenBlock ) );
    scenesManagerInterface.setTestClock( std::chrono::seconds( 1562675471LL ) );
    const SceneID sceneId = createScene( scene );

    scenesManagerInterface.setTestClock( std::chrono::seconds( 1562676298LL ) );
    testRunProgressBroadcast( scene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( scene, sceneId, "finished", __LINE__ );
    waitForBroadcast( "hub.item.updated", "code line: " + std::to_string( __LINE__ ) );

    const Json sceneStatus = getSceneStatus( sceneId, __LINE__ );

    {
        std::string state;
        std::vector< Json > statePath{ "result", "status", "when", 0, "state" };
        ASSERT_TRUE( json_get( sceneStatus, statePath, state ) );
        EXPECT_EQ( state, "false" );
    }
    {
        std::string lastChange;
        std::vector< Json > lastChangePath{ "result", "status", "when", 0, "lastChange" };
        ASSERT_TRUE( json_get( sceneStatus, lastChangePath, lastChange ) );
        EXPECT_NE( Json( lastChange ), Json() );
    }
}

/// @brief
/// create weekly scene with two events within two weekdays
/// check both events at first day
/// check both events at second day
/// delete scene and check hub.scene.deleted broadcast
TEST_F( SceneManagerTests, weeklyScene ) {  // 1.3
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    auto response = scenesManagerInterface.setTestClock( std::chrono::seconds( 1562505419LL ) );
    ASSERT_TRUE( response[ "error" ].empty() );
    SceneID sceneId = createScene( createWeeklyScene );

    scenesManagerInterface.setTestClock( std::chrono::seconds( 1562667598LL ) );
    testRunProgressBroadcast( createWeeklyScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createWeeklyScene, sceneId, "finished", __LINE__ );

    scenesManagerInterface.setTestClock( std::chrono::seconds( 1562675998LL ) );
    testRunProgressBroadcast( createWeeklyScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createWeeklyScene, sceneId, "finished", __LINE__ );

    scenesManagerInterface.setTestClock( std::chrono::seconds( 1562926798LL ) );
    testRunProgressBroadcast( createWeeklyScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createWeeklyScene, sceneId, "finished", __LINE__ );

    scenesManagerInterface.setTestClock( std::chrono::seconds( 1562935198LL ) );
    testRunProgressBroadcast( createWeeklyScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createWeeklyScene, sceneId, "finished", __LINE__ );
}

/// @brief
/// create monthly scene with two events within two days of one month
/// check both events at first day
/// check both events at second day
/// delete scene and check hub.scene.deleted broadcast
TEST_F( SceneManagerTests, monthlyScene ) {  // 1.4
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    auto response = scenesManagerInterface.setTestClock( std::chrono::seconds( 1562505419LL ) );
    ASSERT_TRUE( response[ "error" ].empty() );
    SceneID sceneId = createScene( createMonthlyScene );

    scenesManagerInterface.setTestClock( std::chrono::seconds( 1562321998LL ) );
    testRunProgressBroadcast( createMonthlyScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createMonthlyScene, sceneId, "finished", __LINE__ );

    scenesManagerInterface.setTestClock( std::chrono::seconds( 1562330398LL ) );
    testRunProgressBroadcast( createMonthlyScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createMonthlyScene, sceneId, "finished", __LINE__ );

    scenesManagerInterface.setTestClock( std::chrono::seconds( 1562926798LL ) );
    testRunProgressBroadcast( createMonthlyScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createMonthlyScene, sceneId, "finished", __LINE__ );

    scenesManagerInterface.setTestClock( std::chrono::seconds( 1562935198LL ) );
    testRunProgressBroadcast( createMonthlyScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createMonthlyScene, sceneId, "finished", __LINE__ );
}

TEST_F( SunsetSunriseTests, dailySunsetIntime ) {  // 1.5.1
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    JsonObject createSunsetScene = getSunStateRequest( "daily", "sunset", "intime" );
    scenesManagerInterface.setTestClock( std::chrono::seconds( 1564075678LL ) + clockDelayBeforeSceneStart );
    SceneID sceneId = createScene( createSunsetScene );
    scenesManagerInterface.setTestClock( std::chrono::seconds( 1564075678LL ) );
    testRunProgressBroadcast( createSunsetScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createSunsetScene, sceneId, "finished", __LINE__ );
}

TEST_F( SunsetSunriseTests, dailySunsetAfter ) {  // 1.5.2
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    JsonObject createSunsetScene = getSunStateRequest( "daily", "sunset", "after" );
    scenesManagerInterface.setTestClock( std::chrono::seconds( 1564081272LL ) + clockDelayBeforeSceneStart );
    SceneID sceneId = createScene( createSunsetScene );
    scenesManagerInterface.setTestClock( std::chrono::seconds( 1564081272LL ) );
    testRunProgressBroadcast( createSunsetScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createSunsetScene, sceneId, "finished", __LINE__ );
}

TEST_F( SunsetSunriseTests, dailySunsetBefore ) {  // 1.5.4
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    JsonObject createSunsetScene = getSunStateRequest( "daily", "sunset", "before" );
    scenesManagerInterface.setTestClock( std::chrono::seconds( 1564070084LL ) + clockDelayBeforeSceneStart );
    SceneID sceneId = createScene( createSunsetScene );
    scenesManagerInterface.setTestClock( std::chrono::seconds( 1564070084LL ) );
    testRunProgressBroadcast( createSunsetScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createSunsetScene, sceneId, "finished", __LINE__ );
}

TEST_F( SunsetSunriseTests, weeklySunsetIntime ) {  // 1.5.4
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    JsonObject createSunsetScene = getSunStateRequest( "weekly", "sunset", "intime" );
    scenesManagerInterface.setTestClock( std::chrono::seconds( 1564075678LL ) + clockDelayBeforeSceneStart );
    SceneID sceneId = createScene( createSunsetScene );
    scenesManagerInterface.setTestClock( std::chrono::seconds( 1564075678LL ) );
    testRunProgressBroadcast( createSunsetScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createSunsetScene, sceneId, "finished", __LINE__ );
}

TEST_F( SunsetSunriseTests, weeklySunsetAfter ) {  // 1.5.5
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    JsonObject createSunsetScene = getSunStateRequest( "weekly", "sunset", "after" );
    scenesManagerInterface.setTestClock( std::chrono::seconds( 1564081272LL ) + clockDelayBeforeSceneStart );
    SceneID sceneId = createScene( createSunsetScene );
    scenesManagerInterface.setTestClock( std::chrono::seconds( 1564081272LL ) );
    testRunProgressBroadcast( createSunsetScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createSunsetScene, sceneId, "finished", __LINE__ );
}

TEST_F( SunsetSunriseTests, weeklySunsetBefore ) {  // 1.5.6
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    JsonObject createSunsetScene = getSunStateRequest( "weekly", "sunset", "before" );
    scenesManagerInterface.setTestClock( std::chrono::seconds( 1564070084LL ) + clockDelayBeforeSceneStart );
    SceneID sceneId = createScene( createSunsetScene );
    scenesManagerInterface.setTestClock( std::chrono::seconds( 1564070084LL ) );
    testRunProgressBroadcast( createSunsetScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createSunsetScene, sceneId, "finished", __LINE__ );
}

TEST_F( SunsetSunriseTests, monthlySunsetIntime ) {  // 1.5.7
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    JsonObject createSunsetScene = getSunStateRequest( "monthly", "sunset", "intime" );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-04T17:45:58+0000" )
                                             .addSeconds( clockDelayBeforeSceneStart.count() )
                                             .toSeconds() );
    SceneID sceneId = createScene( createSunsetScene );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-04T17:45:58+0000" ).toSeconds() );
    testRunProgressBroadcast( createSunsetScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createSunsetScene, sceneId, "finished", __LINE__ );
}

TEST_F( SunsetSunriseTests, monthlySunsetAfter ) {  // 1.5.8
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    JsonObject createSunsetScene = getSunStateRequest( "monthly", "sunset", "after" );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-04T19:19:12+0000" )
                                             .addSeconds( clockDelayBeforeSceneStart.count() )
                                             .toSeconds() );
    SceneID sceneId = createScene( createSunsetScene );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-04T19:19:12+0000" ).toSeconds() );
    testRunProgressBroadcast( createSunsetScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createSunsetScene, sceneId, "finished", __LINE__ );
}

TEST_F( SunsetSunriseTests, monthlySunsetBefore ) {  // 1.5.9
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    JsonObject createSunsetScene = getSunStateRequest( "monthly", "sunset", "before" );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-04T16:12:44+0000" )
                                             .addSeconds( clockDelayBeforeSceneStart.count() )
                                             .toSeconds() );
    SceneID sceneId = createScene( createSunsetScene );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-04T16:12:44+0000" ).toSeconds() );
    testRunProgressBroadcast( createSunsetScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createSunsetScene, sceneId, "finished", __LINE__ );
}

TEST_F( SunsetSunriseTests, SunsetRangeAfter ) {  // 1.5.10
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-04T00:01:00+0000" ).toSeconds() );
    JsonObject createSunsetScene = getSunStateRequest( "daily", "sunset", "intime", /*range*/ "after" );
    SceneID sceneId = createScene( createSunsetScene );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-04T18:00:00+0000" ).toSeconds() );
    testRunProgressBroadcast( createSunsetScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createSunsetScene, sceneId, "finished", __LINE__ );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-05T00:01:00+0000" ).toSeconds() );
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-05T18:00:00+0000" ).toSeconds() );
    testRunProgressBroadcast( createSunsetScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createSunsetScene, sceneId, "finished", __LINE__ );
}

TEST_F( SunsetSunriseTests, SunsetRangeBefore ) {  // 1.5.11
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-04T23:00:00+0000" ).toSeconds() );
    JsonObject createSunsetScene = getSunStateRequest( "daily", "sunset", "intime", /*range*/ "before" );
    SceneID sceneId = createScene( createSunsetScene );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-04T17:00:00+0000" ).toSeconds() );
    testRunProgressBroadcast( createSunsetScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createSunsetScene, sceneId, "finished", __LINE__ );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-04T18:00:00+0000" ).toSeconds() );
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-05T00:01:00+0000" ).toSeconds() );
    testRunProgressBroadcast( createSunsetScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createSunsetScene, sceneId, "finished", __LINE__ );
}

TEST_F( SunsetSunriseTests, SunsetNotTriggerSceneOnCreation ) {  // 1.5.12
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-04T17:00:00+0000" ).toSeconds() );
    JsonObject createSunsetScene = getSunStateRequest( "daily", "sunset", "intime", /*range*/ "before" );
    SceneID sceneId = createScene( createSunsetScene );
    checkNoBroadcastReceived( "Scene should not be executed on creation", __LINE__, std::chrono::seconds( 2 ) );
}

TEST_F( SunsetSunriseTests, dailySunriseIntime ) {  // 1.6.1
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    JsonObject createSunsetScene = getSunStateRequest( "daily", "sunrise", "intime" );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-25T01:53:58+0000" )
                                             .addSeconds( clockDelayBeforeSceneStart.count() )
                                             .toSeconds() );
    SceneID sceneId = createScene( createSunsetScene );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-25T01:53:58+0000" ).toSeconds() );
    testRunProgressBroadcast( createSunsetScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createSunsetScene, sceneId, "finished", __LINE__ );
}

TEST_F( SunsetSunriseTests, dailySunriseAfter ) {  // 1.6.2
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    JsonObject createSunsetScene = getSunStateRequest( "daily", "sunrise", "after" );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-25T03:27:12+0000" )
                                             .addSeconds( clockDelayBeforeSceneStart.count() )
                                             .toSeconds() );
    SceneID sceneId = createScene( createSunsetScene );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-25T03:27:12+0000" ).toSeconds() );
    testRunProgressBroadcast( createSunsetScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createSunsetScene, sceneId, "finished", __LINE__ );
}

TEST_F( SunsetSunriseTests, dailySunriseBefore ) {  // 1.6.3
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    JsonObject createSunsetScene = getSunStateRequest( "daily", "sunrise", "before" );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-25T00:20:44+0000" )
                                             .addSeconds( clockDelayBeforeSceneStart.count() )
                                             .toSeconds() );
    SceneID sceneId = createScene( createSunsetScene );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-25T00:20:44+0000" ).toSeconds() );
    testRunProgressBroadcast( createSunsetScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createSunsetScene, sceneId, "finished", __LINE__ );
}

TEST_F( SunsetSunriseTests, weeklySunriseIntime ) {  // 1.6.4
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    JsonObject createSunsetScene = getSunStateRequest( "weekly", "sunrise", "intime" );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-25T01:53:58+0000" )
                                             .addSeconds( clockDelayBeforeSceneStart.count() )
                                             .toSeconds() );
    SceneID sceneId = createScene( createSunsetScene );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-25T01:53:58+0000" ).toSeconds() );
    testRunProgressBroadcast( createSunsetScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createSunsetScene, sceneId, "finished", __LINE__ );
}

TEST_F( SunsetSunriseTests, weeklySunriseAfter ) {  // 1.6.5
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    JsonObject createSunsetScene = getSunStateRequest( "weekly", "sunrise", "after" );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-25T03:27:12+0000" )
                                             .addSeconds( clockDelayBeforeSceneStart.count() )
                                             .toSeconds() );
    SceneID sceneId = createScene( createSunsetScene );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-25T03:27:12+0000" ).toSeconds() );
    testRunProgressBroadcast( createSunsetScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createSunsetScene, sceneId, "finished", __LINE__ );
}

TEST_F( SunsetSunriseTests, weeklySunriseBefore ) {  // 1.6.6
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    JsonObject createSunsetScene = getSunStateRequest( "weekly", "sunrise", "before" );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-25T00:20:44+0000" )
                                             .addSeconds( clockDelayBeforeSceneStart.count() )
                                             .toSeconds() );
    SceneID sceneId = createScene( createSunsetScene );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-25T00:20:44+0000" ).toSeconds() );
    testRunProgressBroadcast( createSunsetScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createSunsetScene, sceneId, "finished", __LINE__ );
}

TEST_F( SunsetSunriseTests, monthlySunriseIntime ) {  // 1.6.7
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    JsonObject createSunsetScene = getSunStateRequest( "monthly", "sunrise", "intime" );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-03T01:30:58+0000" )
                                             .addSeconds( clockDelayBeforeSceneStart.count() )
                                             .toSeconds() );
    SceneID sceneId = createScene( createSunsetScene );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-03T01:30:58+0000" ).toSeconds() );
    testRunProgressBroadcast( createSunsetScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createSunsetScene, sceneId, "finished", __LINE__ );
}

TEST_F( SunsetSunriseTests, monthlySunriseAfter ) {  // 1.6.8
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    JsonObject createSunsetScene = getSunStateRequest( "monthly", "sunrise", "after" );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-03T03:04:12+0000" )
                                             .addSeconds( clockDelayBeforeSceneStart.count() )
                                             .toSeconds() );
    SceneID sceneId = createScene( createSunsetScene );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-03T03:04:12+0000" ).toSeconds() );
    testRunProgressBroadcast( createSunsetScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createSunsetScene, sceneId, "finished", __LINE__ );
}

TEST_F( SunsetSunriseTests, monthlySunriseBefore ) {  // 1.6.9
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    JsonObject createSunsetScene = getSunStateRequest( "monthly", "sunrise", "before" );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-02T23:57:44+0000" )
                                             .addSeconds( clockDelayBeforeSceneStart.count() )
                                             .toSeconds() );
    SceneID sceneId = createScene( createSunsetScene );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-02T23:57:44+0000" ).toSeconds() );
    testRunProgressBroadcast( createSunsetScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createSunsetScene, sceneId, "finished", __LINE__ );
}

TEST_F( SunsetSunriseTests, SunriseRangeAfter ) {  // 1.6.10
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-25T01:00:00+0000" ).toSeconds() );
    JsonObject createSunsetScene = getSunStateRequest( "daily", "sunrise", "intime", /*range*/ "after" );
    SceneID sceneId = createScene( createSunsetScene );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-25T02:00:00+0000" ).toSeconds() );
    testRunProgressBroadcast( createSunsetScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createSunsetScene, sceneId, "finished", __LINE__ );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-26T01:00:00+0000" ).toSeconds() );
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-26T02:00:00+0000" ).toSeconds() );
    testRunProgressBroadcast( createSunsetScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createSunsetScene, sceneId, "finished", __LINE__ );
}

TEST_F( SunsetSunriseTests, SunriseRangeBefore ) {  // 1.6.11
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-26T02:00:00+0000" ).toSeconds() );
    JsonObject createSunsetScene = getSunStateRequest( "daily", "sunrise", "intime", /*range*/ "before" );
    SceneID sceneId = createScene( createSunsetScene );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-26T01:00:00+0000" ).toSeconds() );
    testRunProgressBroadcast( createSunsetScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createSunsetScene, sceneId, "finished", __LINE__ );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-25T02:00:00+0000" ).toSeconds() );
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-26T01:00:00+0000" ).toSeconds() );
    testRunProgressBroadcast( createSunsetScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createSunsetScene, sceneId, "finished", __LINE__ );
}

TEST_F( SunsetSunriseTests, SunriseNotTriggerSceneOnCreation ) {  // 1.6.12
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-07-26T01:00:00+0000" ).toSeconds() );
    JsonObject createSunsetScene = getSunStateRequest( "daily", "sunrise", "intime", /*range*/ "before" );
    SceneID sceneId = createScene( createSunsetScene );
    checkNoBroadcastReceived( "Scene should not be executed on creation", __LINE__, std::chrono::seconds( 2 ) );
}

TEST_F( SceneManagerTests, isItemStateSingleItem ) {  // 1.7.1
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );
    auto itemId =
        addItem( coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    auto securityItemId = addItem(
        coreRemoteInterface, deviceId, "security_threat", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, true } }, __LINE__ );

    coreRemoteInterface.setItemValue( JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << 1 ) );
    checkItemUpdateBroadcast( itemId, 1, __LINE__ );

    auto request = replaceTokens( createIsItemState, { { "__ITEM_ID__", itemId } } );
    SceneID sceneId = createScene( request );

    coreRemoteInterface.setItemValue( JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << 2 ) );
    checkItemUpdateBroadcast( itemId, 2, __LINE__ );
    coreRemoteInterface.setItemValue( JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << 10 ) );
    checkItemUpdateBroadcast( itemId, 10, __LINE__ );

    testRunProgressBroadcast( createIsItemState, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createIsItemState, sceneId, "finished", __LINE__ );

    coreRemoteInterface.setDeviceArmed( deviceId, false );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, false } }, __LINE__ );
    coreRemoteInterface.setDeviceArmed( deviceId, true );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, true } }, __LINE__ );

    testRunProgressBroadcast( createIsItemState, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createIsItemState, sceneId, "finished", __LINE__ );

    coreRemoteInterface.setDeviceArmed( deviceId, false );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, false } }, __LINE__ );

    coreRemoteInterface.setItemValue( JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << 2 ) );
    checkItemUpdateBroadcast( itemId, 2, __LINE__ );
    coreRemoteInterface.setItemValue( JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << 10 ) );
    checkItemUpdateBroadcast( itemId, 10, __LINE__ );
    coreRemoteInterface.setItemValue( JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << 3 ) );
    checkItemUpdateBroadcast( itemId, 3, __LINE__ );

    coreRemoteInterface.setDeviceArmed( deviceId, true );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, true } }, __LINE__ );

    deleteScenes();

    SceneID sceneId1 = createScene( request );
    SceneID sceneId2 = createScene( request );
    deleteScene( sceneId2 );

    coreRemoteInterface.setItemValue( JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << 10 ) );
    checkItemUpdateBroadcast( itemId, 10, __LINE__ );

    testRunProgressBroadcast( createIsItemState, sceneId1, "started", __LINE__ );
    testRunProgressBroadcast( createIsItemState, sceneId1, "finished", __LINE__ );

    deleteScenes();

    request[ "params" ][ "when" ][ 0 ][ "fields" ][ 1 ][ "value" ] = 10;
    request[ "params" ][ "when" ][ 0 ][ "fields" ].erase( 2 );
    sceneId = createScene( request );

    coreRemoteInterface.setItemValue( JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << 2 ) );
    checkItemUpdateBroadcast( itemId, 2, __LINE__ );
    coreRemoteInterface.setItemValue( JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << 10 ) );
    checkItemUpdateBroadcast( itemId, 10, __LINE__ );

    testRunProgressBroadcast( createIsItemState, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createIsItemState, sceneId, "finished", __LINE__ );

    coreRemoteInterface.setDeviceArmed( deviceId, false );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, false } }, __LINE__ );
    coreRemoteInterface.setDeviceArmed( deviceId, true );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, true } }, __LINE__ );

    coreRemoteInterface.setItemValue( JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << 2 ) );
    checkItemUpdateBroadcast( itemId, 2, __LINE__ );
    coreRemoteInterface.setItemValue( JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << 10 ) );
    checkItemUpdateBroadcast( itemId, 10, __LINE__ );

    testRunProgressBroadcast( createIsItemState, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createIsItemState, sceneId, "finished", __LINE__ );

    coreRemoteInterface.setDeviceArmed( deviceId, false );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, false } }, __LINE__ );

    coreRemoteInterface.setItemValue( JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << 2 ) );
    checkItemUpdateBroadcast( itemId, 2, __LINE__ );
    coreRemoteInterface.setItemValue( JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << 10 ) );
    checkItemUpdateBroadcast( itemId, 10, __LINE__ );

    testRunProgressBroadcast( createIsItemState, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createIsItemState, sceneId, "finished", __LINE__ );

    coreRemoteInterface.setDeviceArmed( deviceId, true );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, true } }, __LINE__ );

    // remove devices to prevent unnecessary update broadcast
    unsigned long itemsNumber = coreRemoteInterface.getItemsByDeviceId( deviceId ).size();
    removeDevice( coreRemoteInterface, deviceId, itemsNumber, 0, __LINE__ );
}

TEST_F( SceneManagerTests, isItemStateMultipleItems ) {  // 1.7.2
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );
    auto securityItemId = addItem(
        coreRemoteInterface, deviceId, "security_threat", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, true } }, __LINE__ );

    auto itemId1 =
        addItem( coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    coreRemoteInterface.setItemValue( JSON( ItemFields::IdFieldName << itemId1 << ItemFields::ValueFieldName << 10 ) );
    checkItemUpdateBroadcast( itemId1, 10, __LINE__ );

    auto itemId2 =
        addItem( coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    coreRemoteInterface.setItemValue( JSON( ItemFields::IdFieldName << itemId2 << ItemFields::ValueFieldName << 10 ) );
    checkItemUpdateBroadcast( itemId2, 10, __LINE__ );

    auto itemId3 =
        addItem( coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    coreRemoteInterface.setItemValue( JSON( ItemFields::IdFieldName << itemId3 << ItemFields::ValueFieldName << 10 ) );
    checkItemUpdateBroadcast( itemId3, 10, __LINE__ );

    coreRemoteInterface.setDeviceArmed( deviceId, false );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, false } }, __LINE__ );

    SceneID sceneId = createScene(
        replaceTokens( createIsItemStateMultipleItems,
                       { { "__ITEM_ID1__", itemId1 }, { "__ITEM_ID2__", itemId2 }, { "__ITEM_ID3__", itemId3 } } ) );

    coreRemoteInterface.setDeviceArmed( deviceId, true );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, true } }, __LINE__ );

    testRunProgressBroadcast( createIsItemStateMultipleItems, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createIsItemStateMultipleItems, sceneId, "finished", __LINE__ );

    deleteScenes();

    coreRemoteInterface.setDeviceArmed( deviceId, false );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, false } }, __LINE__ );

    SceneID sceneId1 = createScene( replaceTokens( createIsItemState, { { "__ITEM_ID__", itemId1 } } ) );
    SceneID sceneId2 = createScene( replaceTokens( createIsItemState, { { "__ITEM_ID__", itemId2 } } ) );
    SceneID sceneId3 = createScene( replaceTokens( createIsItemState, { { "__ITEM_ID__", itemId3 } } ) );

    coreRemoteInterface.setDeviceArmed( deviceId, true );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, true } }, __LINE__ );

    auto testRunProgressBroadcast = [this]( const JsonObject& createJson,
                                            const std::set< std::string >& possibleSceneIds,
                                            const std::string& status,
                                            int codeLine ) {
        std::string codeLineStr = "code line: " + std::to_string( codeLine );
        JsonObject broadcast = waitForBroadcast( "hub.scene.run.progress", codeLineStr );
        JsonObject result = JSON_EMPTY_OBJ;
        EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << codeLineStr;
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.run.progress" ) << codeLineStr;
        EXPECT_EQ( result[ "scene_name" ].dump(), createJson[ "params" ][ "name" ].dump() ) << codeLineStr;
        EXPECT_EQ( json_get_str( result, "status" ), status ) << codeLineStr;
        EXPECT_EQ( json_get_bool( result, "userNotification" ), status != "started" ) << codeLineStr;
        auto sceneId = json_get_str( result, "scene_id" );
        EXPECT_TRUE( possibleSceneIds.find( sceneId ) != possibleSceneIds.end() ) << codeLineStr;
        ;
    };

    testRunProgressBroadcast( createIsItemState, { sceneId1, sceneId2, sceneId3 }, "started", __LINE__ );
    testRunProgressBroadcast( createIsItemState, { sceneId1, sceneId2, sceneId3 }, "finished", __LINE__ );

    testRunProgressBroadcast( createIsItemState, { sceneId1, sceneId2, sceneId3 }, "started", __LINE__ );
    testRunProgressBroadcast( createIsItemState, { sceneId1, sceneId2, sceneId3 }, "finished", __LINE__ );

    testRunProgressBroadcast( createIsItemState, { sceneId1, sceneId2, sceneId3 }, "started", __LINE__ );
    testRunProgressBroadcast( createIsItemState, { sceneId1, sceneId2, sceneId3 }, "finished", __LINE__ );

    deleteScenes();

    coreRemoteInterface.setDeviceArmed( deviceId, false );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, false } }, __LINE__ );

    coreRemoteInterface.setDeviceArmed( deviceId, true );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, true } }, __LINE__ );

    // remove devices to prevent unnecessary update broadcast
    unsigned long itemsNumber = coreRemoteInterface.getItemsByDeviceId( deviceId ).size();
    removeDevice( coreRemoteInterface, deviceId, itemsNumber, 0, __LINE__ );
}

TEST_F( SceneManagerTests, isItemStateNotArmedItems ) {  // 1.7.3
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId1 = addDevice( coreRemoteInterface,
                                "Smart Switch",    /*name*/
                                "switch.outlet",   /*type*/
                                "134_3_24",        /*deviceTypeId*/
                                "switch",          /*category*/
                                "interior_plugin", /*subcategory*/
                                false /*batteryPowered*/,
                                __LINE__ );
    auto deviceId2 = addDevice( coreRemoteInterface,
                                "Touchscreen Deadbolt", /*name*/
                                "doorlock",             /*type*/
                                "297_32770_1536",       /*deviceTypeId*/
                                "door_lock",            /*category*/
                                "",                     /*subcategory*/
                                false /*batteryPowered*/,
                                __LINE__ );
    auto itemId1 =
        addItem( coreRemoteInterface, deviceId1, "switch", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId2 = addItem(
        coreRemoteInterface, deviceId2, "user_codes", CoreD::Model::ValueType::Enum::Token, true, true, __LINE__ );

    SceneID sceneId = createScene( replaceTokens( createIsItemStateNotArmedDevices,
                                                  { { "__ITEM_ID1__", itemId1 }, { "__ITEM_ID2__", itemId2 } } ) );

    coreRemoteInterface.setItemValue(
        JSON( ItemFields::IdFieldName << itemId1 << ItemFields::ValueFieldName << true ) );
    checkItemUpdateBroadcast( itemId1, true, __LINE__ );

    testRunProgressBroadcast( createIsItemStateNotArmedDevices, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createIsItemStateNotArmedDevices, sceneId, "finished", __LINE__ );

    coreRemoteInterface.setItemValue(
        JSON( ItemFields::IdFieldName << itemId2 << ItemFields::ValueFieldName << "single_user_code_deleted" ) );
    checkItemUpdateBroadcast( itemId2, "single_user_code_deleted", __LINE__ );

    testRunProgressBroadcast( createIsItemStateNotArmedDevices, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createIsItemStateNotArmedDevices, sceneId, "finished", __LINE__ );

    // remove devices to prevent unnecessary update broadcast
    JsonObject broadcast;
    coreRemoteInterface.removeDevice( deviceId1 );
    broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
    coreRemoteInterface.removeDevice( deviceId2 );
    broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
}

TEST_F( SceneManagerTests, isOnceScene ) {  // 1.8
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-01-05T10:19:58" ).toSeconds() );
    SceneID sceneId = createScene( createIsOnceScene );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-12-05T10:19:58" ).toSeconds() );
    testRunProgressBroadcast( createIsOnceScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createIsOnceScene, sceneId, "finished", __LINE__ );
}

/// @brief
/// add new device
/// add new item
/// create scene with setItemValue action
/// check scene execution
/// check item value
/// several action with same value
TEST_F( SceneManagerTests, setItemValueSingleAction ) {  // 1.9.1
    // add new device
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );
    // add new item
    auto itemId =
        addItem( coreRemoteInterface, deviceId, "testItem", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    coreRemoteInterface.setItemValue( JSON( CoreD::Model::Resources::ItemFields::IdFieldName
                                            << itemId << CoreD::Model::Resources::ItemFields::ValueFieldName << 1 ) );
    checkItemUpdateBroadcast( itemId, 1, __LINE__ );

    // create scene with setItemValue action
    auto createSetItemValueAction =
        replaceTokens( HubPlatform::Tests::createSetItemValueAction,
                       { { "__ITEM_ID__", itemId }, { "12345", "10" }, { "__ITEM_VALUE_TYPE__", "int" } } );

    SceneID sceneId = createScene( createSetItemValueAction );

    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( createSetItemValueAction, sceneId, "started", __LINE__ );

    using namespace std::placeholders;
    using Sig1 = void ( SceneManagerTests::* )(
        const JsonObject&, const JsonObject&, const std::string&, const std::string&, int, const JsonArray& );
    JsonArray arr = JSON_EMPTY_ARR;
    auto testRunProgressBroadcastBind =
        std::bind( static_cast< Sig1 >( &SceneManagerTests::testRunProgressBroadcast ), this, _1, _2, _3, _4, _5, arr );

    using Sig2 =
        void ( SceneManagerTests::* )( const JsonObject&, CoreDRemoteInterface::CoreDInterface::ItemID, int, int );
    auto checkItemUpdateBroadcastBind =
        std::bind( static_cast< Sig2 >( &SceneManagerTests::checkItemUpdateBroadcast ), this, _1, _2, _3, _4 );

    checkBroadcasts(
        { { "hub.scene.run.progress",
            std::bind( testRunProgressBroadcastBind, _1, createSetItemValueAction, sceneId, "finished", __LINE__ ) },
          { "hub.item.updated", std::bind( checkItemUpdateBroadcastBind, _1, itemId, 10, __LINE__ ) } },
        __LINE__ );

    auto itemValue = coreRemoteInterface.getItem( itemId );
    EXPECT_EQ( json_get_int( itemValue, "value" ), 10 );

    // remove devices to prevent unnecessary update broadcast
    JsonObject broadcast;
    coreRemoteInterface.removeDevice( deviceId );
    broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );

    deleteScene( sceneId, __LINE__ );
}

TEST_F( SceneManagerTests, setItemValueMulticast ) {  // 1.9.2
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );
    auto itemId1 =
        addItem( coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    coreRemoteInterface.setItemValue( JSON( CoreD::Model::Resources::ItemFields::IdFieldName
                                            << itemId1 << CoreD::Model::Resources::ItemFields::ValueFieldName << 1 ) );
    checkItemUpdateBroadcast( itemId1, 1, __LINE__ );

    auto itemId2 =
        addItem( coreRemoteInterface, deviceId, "testItem2", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    coreRemoteInterface.setItemValue( JSON( CoreD::Model::Resources::ItemFields::IdFieldName
                                            << itemId2 << CoreD::Model::Resources::ItemFields::ValueFieldName << 1 ) );
    checkItemUpdateBroadcast( itemId2, 1, __LINE__ );

    auto itemId3 = addItem(
        coreRemoteInterface, deviceId, "testItem3", CoreD::Model::ValueType::Enum::Power, true, true, __LINE__ );
    coreRemoteInterface.setItemValue( JSON( CoreD::Model::Resources::ItemFields::IdFieldName
                                            << itemId3 << CoreD::Model::Resources::ItemFields::ValueFieldName << 1
                                            << "scale"
                                            << "watt" ) );
    checkItemUpdateBroadcast( itemId3, 1, __LINE__ );

    auto itemId4 = addItem(
        coreRemoteInterface, deviceId, "testItem4", CoreD::Model::ValueType::Enum::Power, true, true, __LINE__ );
    coreRemoteInterface.setItemValue( JSON( CoreD::Model::Resources::ItemFields::IdFieldName
                                            << itemId4 << CoreD::Model::Resources::ItemFields::ValueFieldName << 1
                                            << "scale"
                                            << "watt" ) );
    checkItemUpdateBroadcast( itemId4, 1, __LINE__ );

    auto setItemValueBlock1 = replaceTokensInString(
        setItemValueThenBlock, { { "__ITEM_ID__", itemId1 }, { "__TYPE__", "int" }, { "__VALUE__", "10" } } );

    auto setItemValueBlock2 = replaceTokensInString(
        setItemValueThenBlock, { { "__ITEM_ID__", itemId2 }, { "__TYPE__", "int" }, { "__VALUE__", "10" } } );

    auto setItemValueBlock3 = replaceTokensInString(
        setItemValueThenBlockWithScale,
        { { "__ITEM_ID__", itemId3 }, { "__TYPE__", "power" }, { "__SCALE__", "watt" }, { "__VALUE__", "10" } } );

    auto setItemValueBlock4 = replaceTokensInString(
        setItemValueThenBlockWithScale,
        { { "__ITEM_ID__", itemId4 }, { "__TYPE__", "power" }, { "__SCALE__", "watt" }, { "__VALUE__", "20" } } );

    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "SetItemValue" },
          { "__EXEC_POLICY__", "check_result" },
          { "__WHEN_BLOCKS__", "" },
          { "__THEN_BLOCKS__",
            setItemValueBlock1 + "," + setItemValueBlock2 + "," + setItemValueBlock3 + "," + setItemValueBlock4 } } );
    SceneID sceneId = createScene( request, __LINE__ );
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( createSetItemValueAction, sceneId, "started", __LINE__ );

    checkBroadcasts(
        { [this, sceneId]( const Json& broadcast ) {
             return isRunProgressBroadcast( broadcast, createSetItemValueAction, sceneId, "finished" );
         },
          [this, itemId1]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId1, 10 ); },
          [this, itemId2]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId2, 10 ); },
          [this, itemId3]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId3, 10 ); },
          [this, itemId4]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId4, 20 ); } },
        __LINE__ );

    // remove devices to prevent unnecessary update broadcast
    JsonObject broadcast;
    coreRemoteInterface.removeDevice( deviceId );
    broadcast = waitForBroadcast( "hub.item.removed", NMA_TEST_MAKE_MESSAGE( "hub.item.removed expected" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.item.removed", NMA_TEST_MAKE_MESSAGE( "hub.item.removed expected" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.item.removed", NMA_TEST_MAKE_MESSAGE( "hub.item.removed expected" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.item.removed", NMA_TEST_MAKE_MESSAGE( "hub.item.removed expected" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", NMA_TEST_MAKE_MESSAGE( "hub.device.removed expected" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );

    deleteScene( sceneId, __LINE__ );
}

/// @brief
/// add new device
/// create scene
/// check scene execution
/// check device armed flag
/// delete scene and check hub.scene.deleted broadcast
TEST_F( SceneManagerTests, setDeviceArmed ) {  // 1.10
    using namespace CoreD::Model::Resources;

    // add new device
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    CoreD::Model::Device device(
        "5d287f77f587be6495c2d178", "testDevice", "smoke", "134_3_24", "security_sensor", false /*batteryPowered*/ );
    device.setSubcategory( "smoke" );
    auto deviceAddingResult = coreRemoteInterface.addDevice( device );
    ASSERT_TRUE( deviceAddingResult.second );
    auto deviceId = deviceAddingResult.first;
    JsonObject broadcast = waitForBroadcast( "hub.device.added", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( broadcast[ "msg_subclass" ].dump(), "\"hub.device.added\"" );

    auto securityItemId = addItem(
        coreRemoteInterface, deviceId, "security_threat", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, true } }, __LINE__ );

    auto deviceInfo = coreRemoteInterface.getSingleDevice( deviceId );
    bool isArmed;
    EXPECT_TRUE( json_get( deviceInfo, "armed", isArmed ) );
    EXPECT_TRUE( isArmed );

    // create scene
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    auto response = scenesManagerInterface.setTestClock( std::chrono::seconds( 1562505419LL ) );
    ASSERT_TRUE( response[ "error" ].empty() );
    std::string dump = HubPlatform::Tests::createSetArmedDeviceAction.dump();
    std::string toReplace = "5d287f77f587be6495c2d178";
    dump.replace( dump.find( toReplace ), toReplace.size(), deviceId );
    JsonObject createSetArmedDeviceAction = JsonObject::parse( dump );
    SceneID sceneId = createScene( createSetArmedDeviceAction );

    // check scene execution
    response = scenesManagerInterface.setTestClock( std::chrono::seconds( 1562676298LL ) );
    testRunProgressBroadcast( createSetArmedDeviceAction, sceneId, "started", __LINE__ );
    broadcast = waitForBroadcast( "hub.device.updated", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );
    testRunProgressBroadcast( createSetArmedDeviceAction, sceneId, "finished", __LINE__ );

    // check device armed flag
    deviceInfo = coreRemoteInterface.getSingleDevice( deviceId );
    EXPECT_TRUE( json_get( deviceInfo, DeviceFields::ArmedFieldName, isArmed ) );
    EXPECT_FALSE( isArmed );

    // remove devices to prevent unnecessary update broadcast
    unsigned long itemsNumber = coreRemoteInterface.getItemsByDeviceId( deviceId ).size();
    removeDevice( coreRemoteInterface, deviceId, itemsNumber, 0, __LINE__ );
}

TEST_F( SceneManagerTests, getScene ) {  // 1.13
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-01-05T15:19:58" ).toSeconds() );
    SceneID sceneId = createScene( createEmptyScene );

    std::string dump = HubPlatform::Tests::getScene.dump();
    std::string toReplace = "__SCENE_ID__";
    dump.replace( dump.find( toReplace ), toReplace.size(), sceneId );
    JsonObject getScene = JsonObject::parse( dump );

    auto answer = nma().sendtoHubAndGetAnswer( getScene );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    JsonObject scene;
    ASSERT_TRUE( json_get_obj( answer, "result", scene ) );
    EXPECT_EQ( json_get_str( scene, "_id" ), sceneId );
    EXPECT_EQ( scene[ "enabled" ].dump(), createEmptyScene[ "params" ][ "enabled" ].dump() );
    EXPECT_EQ( scene[ "group_id" ].dump(), createEmptyScene[ "params" ][ "group_id" ].dump() );
    EXPECT_EQ( scene[ "is_group" ].dump(), createEmptyScene[ "params" ][ "is_group" ].dump() );
    EXPECT_EQ( scene[ "name" ].dump(), createEmptyScene[ "params" ][ "name" ].dump() );
    EXPECT_EQ( scene[ "parent_id" ].dump(), createEmptyScene[ "params" ][ "parent_id" ].dump() );
    EXPECT_EQ( scene[ "then" ].dump(), createEmptyScene[ "params" ][ "then" ].dump() );
    EXPECT_EQ( scene[ "when" ].dump(), createEmptyScene[ "params" ][ "when" ].dump() );
}

TEST_F( SceneManagerTests, getScenesList ) {  // 1.14
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-01-05T15:19:58" ).toSeconds() );
    std::map< SceneID, JsonObject > sceneIds;
    sceneIds.insert( { createScene( createSceneJson ), createSceneJson } );
    sceneIds.insert( { createScene( createDailyScene ), createDailyScene } );
    sceneIds.insert( { createScene( createWeeklyScene ), createWeeklyScene } );
    auto absentParentIdRequest = createIsOnceScene;
    auto sceneId = createScene( absentParentIdRequest );
    absentParentIdRequest[ "params" ][ "parent_id" ] = "";
    sceneIds.insert( { std::move( sceneId ), std::move( absentParentIdRequest ) } );

    auto answer = nma().sendtoHubAndGetAnswer( getScenesList );
    JsonArray scenesList = answer[ "result" ][ "scenes" ];
    EXPECT_EQ( scenesList.size(), sceneIds.size() );

    for( auto it = scenesList.begin(); it != scenesList.end(); ++it ) {
        removeBlockIds( ( *it )[ "when" ] );
        SceneID sceneId = json_get_str( *it, "_id" );
        ASSERT_FALSE( sceneId.empty() );
        auto shouldBe = sceneIds.find( sceneId );
        ASSERT_TRUE( shouldBe != sceneIds.end() );
        JsonObject sceneShouldBe = shouldBe->second;
        EXPECT_EQ( ( *it )[ "enabled" ].dump(), sceneShouldBe[ "params" ][ "enabled" ].dump() );
        EXPECT_EQ( ( *it )[ "group_id" ].dump(), sceneShouldBe[ "params" ][ "group_id" ].dump() );
        EXPECT_EQ( ( *it )[ "is_group" ].dump(), sceneShouldBe[ "params" ][ "is_group" ].dump() );
        EXPECT_EQ( ( *it )[ "name" ].dump(), sceneShouldBe[ "params" ][ "name" ].dump() );
        EXPECT_EQ( ( *it )[ "parent_id" ].dump(), sceneShouldBe[ "params" ][ "parent_id" ].dump() );
        EXPECT_EQ( ( *it )[ "then" ].dump(), sceneShouldBe[ "params" ][ "then" ].dump() );
        EXPECT_EQ( ( *it )[ "when" ].dump(), sceneShouldBe[ "params" ][ "when" ].dump() );
    }
}

TEST_F( SceneManagerTests, editScene ) {  // 1.15.1
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-01-05T15:19:58" ).toSeconds() );
    SceneID sceneId = createScene( createSceneJson );

    std::string dump = HubPlatform::Tests::editScene.dump();
    std::string toReplace = "__SCENE_ID__";
    dump.replace( dump.find( toReplace ), toReplace.size(), sceneId );
    toReplace = "\"__NEW_SCENE__\"";
    dump.replace( dump.find( toReplace ), toReplace.size(), createIsOnceScene[ "params" ].dump() );
    JsonObject editScene = JsonObject::parse( dump );

    auto answer = nma().sendtoHubAndGetAnswer( editScene );
    const auto& error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    checkChangedBroadcastEditRequest( sceneId, editScene, __LINE__ );
}

TEST_F( SceneManagerTests, editSceneWithSetSceneState ) {  // 1.15.2

    auto scene1Request = createEmptyScene;
    SceneID scene1Id = createScene( scene1Request );

    auto scene2Request = createEmptyScene;
    scene2Request[ "params" ][ "then" ].push_back(
        replaceTokens( setSceneStateThenBlock, { { "__SCENE_ID__", scene1Id }, { "__ENABLED__", "false" } } ) );

    SceneID scene2Id = createScene( scene2Request );

    ScenesManager::RemoteInterface sceneManagerRemoteInterface( nma() );
    auto scene = sceneManagerRemoteInterface.getScene( scene2Id );

    scene[ "then" ] = JSON_ARRAY(
        replaceTokens( setSceneStateThenBlock, { { "__SCENE_ID__", scene2Id }, { "__ENABLED__", "false" } } ) );

    std::string dump = HubPlatform::Tests::editScene.dump();
    std::string toReplace = "__SCENE_ID__";
    dump.replace( dump.find( toReplace ), toReplace.size(), scene2Id );
    toReplace = "\"__NEW_SCENE__\"";
    dump.replace( dump.find( toReplace ), toReplace.size(), scene.dump() );
    JsonObject editScene = JsonObject::parse( dump );

    auto answer = nma().sendtoHubAndGetAnswer( editScene );
    const auto& error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    checkChangedBroadcastEditRequest( scene2Id, editScene, __LINE__ );

    runSceneManually( scene2Id, __LINE__ );

    checkBroadcasts(
        { [&]( const Json& broadcast ) {
             return isRunProgressBroadcast( broadcast, scene2Request, scene2Id, "started" );
         },
          [&]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, scene2Request, scene2Id, "finished" );
          },
          [&]( const Json& broadcast ) { return isEnableChangedBroadcast( broadcast, scene2Id, false ); } },
        __LINE__ );

    deleteScenes();
}

TEST_F( SceneManagerTests, setWeeklySceneEnabled ) {  // 1.16.1
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    auto response = scenesManagerInterface.setTestClock( std::chrono::seconds( 1562505419LL ) );
    ASSERT_TRUE( response[ "error" ].empty() ) << response[ "error" ];
    SceneID sceneId = createScene( createWeeklyScene );

    JsonObject enableScene = replaceTokens( HubPlatform::Tests::enableScene, { { "__SCENE_ID__", sceneId } } );
    JsonObject disableScene = replaceTokens( HubPlatform::Tests::disableScene, { { "__SCENE_ID__", sceneId } } );

    sendRequest( disableScene, __LINE__ );
    checkChangedBroadcastEnableRequest( sceneId,
                                        createWeeklyScene,
                                        /*enable*/ false,
                                        __LINE__ );
    scenesManagerInterface.setTestClock( std::chrono::seconds( 1562667598LL ) );
    checkNoBroadcastReceived( "disabled scene: " + sceneId, __LINE__ );

    sendRequest( enableScene, __LINE__ );
    checkChangedBroadcastEnableRequest( sceneId, createWeeklyScene, /*enable*/ true, __LINE__ );
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
    scenesManagerInterface.setTestClock( std::chrono::seconds( 1562675999LL ) );
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
    testRunProgressBroadcast( createWeeklyScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createWeeklyScene, sceneId, "finished", __LINE__ );

    sendRequest( disableScene, __LINE__ );
    checkChangedBroadcastEnableRequest( sceneId, createWeeklyScene, /*enable*/ false, __LINE__ );
    scenesManagerInterface.setTestClock( std::chrono::seconds( 1562926798LL ) );
    checkNoBroadcastReceived( "disabled scene: " + sceneId, __LINE__ );

    sendRequest( enableScene, __LINE__ );
    checkChangedBroadcastEnableRequest( sceneId, createWeeklyScene, /*enable*/ true, __LINE__ );
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
    scenesManagerInterface.setTestClock( std::chrono::seconds( 1562935199LL ) );
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
    testRunProgressBroadcast( createWeeklyScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createWeeklyScene, sceneId, "finished", __LINE__ );
}

TEST_F( SceneManagerTests, setIntervalSceneEnabled ) {  // 1.16.2
    SceneID sceneId = createScene( createPeriodicalScene );
    JsonObject enableScene = replaceTokens( HubPlatform::Tests::enableScene, { { "__SCENE_ID__", sceneId } } );
    JsonObject disableScene = replaceTokens( HubPlatform::Tests::disableScene, { { "__SCENE_ID__", sceneId } } );

    testRunProgressBroadcast( createPeriodicalScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createPeriodicalScene, sceneId, "finished", __LINE__ );

    sendRequest( disableScene, __LINE__ );
    checkChangedBroadcastEnableRequest( sceneId, createPeriodicalScene, /*enable*/ false, __LINE__ );
    checkNoBroadcastReceived( "disabled scene: " + sceneId, __LINE__, std::chrono::seconds( 7 ) );

    sendRequest( enableScene, __LINE__ );
    checkChangedBroadcastEnableRequest( sceneId, createPeriodicalScene, /*enable*/ true, __LINE__ );
    testRunProgressBroadcast( createPeriodicalScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createPeriodicalScene, sceneId, "finished", __LINE__ );

    sendRequest( disableScene, __LINE__ );
    checkChangedBroadcastEnableRequest( sceneId, createPeriodicalScene, /*enable*/ false, __LINE__ );
    checkNoBroadcastReceived( "disabled scene: " + sceneId, __LINE__, std::chrono::seconds( 7 ) );
}

TEST_F( SceneManagerTests, isItemStateEnabling ) {  // 1.16.3
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 = addItem(
        coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );

    auto isItemStateBlock = replaceTokensInString(
        isItemStateWhenBlock, { { "__ITEM_ID__", itemId1 }, { "__TYPE__", "bool" }, { "__VALUE__", "true" } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isItemStateEnabling" }, { "__WHEN_BLOCKS__", isItemStateBlock }, { "__THEN_BLOCKS__", "" } } );
    SceneID sceneId = createScene( request );
    JsonObject enableScene = replaceTokens( HubPlatform::Tests::enableScene, { { "__SCENE_ID__", sceneId } } );
    JsonObject disableScene = replaceTokens( HubPlatform::Tests::disableScene, { { "__SCENE_ID__", sceneId } } );

    setItemValue( coreRemoteInterface, itemId1, true, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    setItemValue( coreRemoteInterface, itemId1, false, __LINE__ );

    sendRequest( disableScene, __LINE__ );
    checkChangedBroadcastEnableRequest( sceneId, request, /*enable*/ false, __LINE__ );

    setItemValue( coreRemoteInterface, itemId1, true, __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, false, __LINE__ );

    sendRequest( enableScene, __LINE__ );
    checkChangedBroadcastEnableRequest( sceneId, request, /*enable*/ true, __LINE__ );

    setItemValue( coreRemoteInterface, itemId1, true, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    // remove devices to prevent unnecessary update broadcast
    JsonObject broadcast;
    coreRemoteInterface.removeDevice( deviceId );
    broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
}

TEST_F( SceneManagerTests, isButtonStateEnabling ) {  // 1.16.4
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "Remote Control", /*name*/
                               "device",         /*type*/
                               "520_512_11",     /*deviceTypeId*/
                               "remote_control", /*category*/
                               "",               /*subcategory*/
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 = addItem( coreRemoteInterface,
                            deviceId,
                            "button_state",
                            CoreD::Model::ValueType::Enum::ButtonState,
                            true,
                            true,
                            __LINE__ );

    auto setButtonValue = [&coreRemoteInterface, this](
                              const std::string& itemId, const std::string& buttonState, int buttonId, int codeLine ) {
        JsonObject value = JSON( ButtonStateValueFields::ButtonStateFieldName
                                 << buttonState << ButtonStateValueFields::ButtonIdFieldName << buttonId );
        coreRemoteInterface.setItemValue(
            JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << value ) );
        checkItemUpdateBroadcast( itemId, value, codeLine );
    };

    setButtonValue( itemId1, "released", 1, __LINE__ );

    auto isButtonState = replaceTokensInString( isButtonStateWhenBlock,
                                                { { "__ITEM_ID__", itemId1 }, { "__VALUE__", "press_1_time" } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isButtonStateEnabling" }, { "__WHEN_BLOCKS__", isButtonState }, { "__THEN_BLOCKS__", "" } } );
    SceneID sceneId = createScene( request );
    JsonObject enableScene = replaceTokens( HubPlatform::Tests::enableScene, { { "__SCENE_ID__", sceneId } } );
    JsonObject disableScene = replaceTokens( HubPlatform::Tests::disableScene, { { "__SCENE_ID__", sceneId } } );

    setButtonValue( itemId1, "press_1_time", 1, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    setButtonValue( itemId1, "released", 1, __LINE__ );

    sendRequest( disableScene, __LINE__ );
    checkChangedBroadcastEnableRequest( sceneId, request, /*enable*/ false, __LINE__ );
    setButtonValue( itemId1, "press_1_time", 1, __LINE__ );
    setButtonValue( itemId1, "released", 1, __LINE__ );

    sendRequest( enableScene, __LINE__ );
    checkChangedBroadcastEnableRequest( sceneId, request, /*enable*/ true, __LINE__ );
    setButtonValue( itemId1, "press_1_time", 1, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    // remove devices to prevent unnecessary update broadcast
    JsonObject broadcast;
    coreRemoteInterface.removeDevice( deviceId );
    broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
}

/// brief
/// check notifications within hub.scenes.run.progress broadcast
/// add new userId for notification
/// check hub.scene.changed broadcast
/// check updated notifications within hub.scenes.run.progress broadcast
TEST_F( SceneManagerTests, notificationAdd ) {  // 1.17
    // check notifications within hub.scenes.run.progress broadcast
    SceneID sceneId = createScene( createNotificationScene );
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( createNotificationScene, sceneId, "started", __LINE__, JsonArray( { "1111", "2222" } ) );
    testRunProgressBroadcast( createNotificationScene, sceneId, "finished", __LINE__, JsonArray( { "1111", "2222" } ) );

    // add new userId for notification
    std::string dump = HubPlatform::Tests::addSceneNotification.dump();
    std::string toReplace = "__SCENE_ID__";
    dump.replace( dump.find( toReplace ), toReplace.size(), sceneId );
    JsonObject addSceneNotification = JsonObject::parse( dump );
    auto answer = nma().sendtoHubAndGetAnswer( addSceneNotification );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    // check hub.scene.changed broadcast
    JsonObject broadcast = waitForBroadcast(
        "hub.scene.changed", "hub.scene.changed scene: " + sceneId + " code line: " + std::to_string( __LINE__ ) );
    JsonObject result;
    JsonObject resultNotifications;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.changed" );
    EXPECT_TRUE( json_get_arr( result, "user_notifications", resultNotifications ) );
    EXPECT_EQ( resultNotifications.dump(), JsonArray( { "1111", "2222", "3333" } ).dump() );

    // check updated notifications within hub.scenes.run.progress broadcast
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast(
        createNotificationScene, sceneId, "started", __LINE__, JsonArray( { "1111", "2222", "3333" } ) );
    testRunProgressBroadcast(
        createNotificationScene, sceneId, "finished", __LINE__, JsonArray( { "1111", "2222", "3333" } ) );
}

/// @brief
/// check notifications within hub.scenes.run.progress broadcast
/// remove new userId for notification
/// check hub.scene.changed broadcast
/// check updated notifications within hub.scenes.run.progress broadcast
TEST_F( SceneManagerTests, notificationRemove ) {  // 1.18
    // check notifications within hub.scenes.run.progress broadcast
    SceneID sceneId = createScene( createNotificationScene );
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( createNotificationScene, sceneId, "started", __LINE__, JsonArray( { "1111", "2222" } ) );
    testRunProgressBroadcast( createNotificationScene, sceneId, "finished", __LINE__, JsonArray( { "1111", "2222" } ) );

    // remove one userId for notification
    std::string dump = HubPlatform::Tests::removeSceneNotification.dump();
    std::string toReplace = "__SCENE_ID__";
    dump.replace( dump.find( toReplace ), toReplace.size(), sceneId );
    JsonObject removeSceneNotification = JsonObject::parse( dump );
    auto answer = nma().sendtoHubAndGetAnswer( removeSceneNotification );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    // check hub.scene.changed broadcast
    JsonObject broadcast = waitForBroadcast(
        "hub.scene.changed", "hub.scene.changed scene: " + sceneId + " code line: " + std::to_string( __LINE__ ) );
    JsonObject result;
    JsonObject resultNotifications;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.changed" );
    EXPECT_TRUE( json_get_arr( result, "user_notifications", resultNotifications ) );
    EXPECT_EQ( resultNotifications.dump(), JsonArray( { "2222" } ).dump() );

    // check updated notifications within hub.scenes.run.progress broadcast
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( createNotificationScene, sceneId, "started", __LINE__, JsonArray( { "2222" } ) );
    testRunProgressBroadcast( createNotificationScene, sceneId, "finished", __LINE__, JsonArray( { "2222" } ) );
}

/// @brief
/// Ensure an appropriate action block is executed when a scene has only "then" actions
/// The logic depends on the value of "block" field of scenes.run request
/// In case if there are no "else" actions, "then" actions are executed unless the opposite is specified explicitly
TEST_F( SceneManagerTests, runScene_withoutElseActions ) {  // 1.19.1
    auto sceneRequest = createEmptyScene;
    SceneID sceneId = createScene( sceneRequest );

    // Run scene without the "block" field
    runSceneManually( sceneId, __LINE__ );
    // Expect started/finished progress broadcasts sent by the empty "then" block
    testRunProgressBroadcast( sceneRequest, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( sceneRequest, sceneId, "finished", __LINE__ );

    // "block" field is "then", same expectation
    runSceneManually( sceneId, "then", __LINE__ );
    testRunProgressBroadcast( sceneRequest, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( sceneRequest, sceneId, "finished", __LINE__ );

    // "block" field is "else"
    runSceneManually( sceneId, "else", __LINE__ );
    // no broadcasts expected
}

/// @brief
/// Ensure an appropriate action block is executed when a scene contains "then" actions
/// The logic depends on the value of "block" field of scenes.run request
/// In case if there are no "then" actions, but there're "else" actions, by default "else" actions are executed
/// Otherwise a corresponding action block must be executed
TEST_F( SceneManagerTests, runScene_withElseActions ) {  // 1.19.2
    auto itemData = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Bool, __LINE__ );
    auto sceneRequest = createEmptyScene;
    sceneRequest[ "params" ][ "else" ].push_back(
        replaceTokens( setItemValueThenBlock,
                       { { "__ITEM_ID__", itemData.m_itemId }, { "__TYPE__", "bool" }, { "__VALUE__", "true" } } ) );

    SceneID sceneId = createScene( sceneRequest );

    // Run scene without the "block" field
    runSceneManually( sceneId, __LINE__ );
    // Expect "else" block actions to be executed, since there are no "then" actions
    checkItemUpdateBroadcast( itemData.m_itemId, true, __LINE__ );

    // "block" field is "then"
    runSceneManually( sceneId, "then", __LINE__ );
    // Expect started/finished progress broadcasts sent by the empty "then" block
    testRunProgressBroadcast( sceneRequest, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( sceneRequest, sceneId, "finished", __LINE__ );

    // Toggle item value to get item update made by the "else" block action
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    namespace ItemFields = CoreD::Model::Resources::ItemFields;
    coreRemoteInterface.setItemValue(
        JSON( ItemFields::IdFieldName << itemData.m_itemId << ItemFields::ValueFieldName << false ) );
    checkItemUpdateBroadcast( itemData.m_itemId, false, __LINE__ );
    // "block" field is "else"
    runSceneManually( sceneId, "else", __LINE__ );
    // Expect item update made by the "else" block action
    checkItemUpdateBroadcast( itemData.m_itemId, true, __LINE__ );
}

/// @brief
/// If a scene has both "then" and "else" actions, "then" actions are executed by default
TEST_F( SceneManagerTests, runScene_defaultWithBothActionBlocks ) {  // 1.19.3
    auto item1Data = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Bool, __LINE__ );
    auto item2Data = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Bool, __LINE__ );
    auto sceneRequest = createEmptyScene;
    sceneRequest[ "params" ][ "then" ].push_back(
        replaceTokens( setItemValueThenBlock,
                       { { "__ITEM_ID__", item1Data.m_itemId }, { "__TYPE__", "bool" }, { "__VALUE__", "true" } } ) );
    sceneRequest[ "params" ][ "else" ].push_back(
        replaceTokens( setItemValueThenBlock,
                       { { "__ITEM_ID__", item2Data.m_itemId }, { "__TYPE__", "bool" }, { "__VALUE__", "true" } } ) );

    SceneID sceneId = createScene( sceneRequest );

    // Run scene without the "block" field
    runSceneManually( sceneId, __LINE__ );
    // Expect that "then" block is executed
    checkBroadcasts( { [this, &sceneId, &sceneRequest]( const Json& broadcast ) {
                          return isRunProgressBroadcast( broadcast, sceneRequest, sceneId, "started" );
                      },
                       [this, &sceneId, &sceneRequest]( const Json& broadcast ) {
                           return isRunProgressBroadcast( broadcast, sceneRequest, sceneId, "finished" );
                       },
                       [this, &item1Data]( const Json& broadcast ) {
                           return isItemUpdateBroadcast( broadcast, item1Data.m_itemId, true );
                       } },
                     __LINE__ );

    // Other possible values of "block" field are already covered by other runScene_* test cases
}

/// @brief
/// create scene
/// check invalid room answer (send request hub.scenes.room.set)
/// add new room to core
/// link scene with room (send request hub.scenes.room.set)
/// check hub.scene.changed broadcast
/// delete scene and check hub.scene.deleted broadcast
/// delete room and and remove it from scenes
TEST_F( SceneManagerTests, setRoom ) {  // 1.20.1
    // create scene
    SceneID sceneId = createScene( createEmptyScene );

    // check invalid room answer
    auto setSceneRoom = HubPlatform::Tests::setSceneRoom;
    setSceneRoom[ "params" ][ "sceneId" ] = sceneId;
    setSceneRoom[ "params" ][ "roomId" ] = "randomid";
    auto answer = nma().sendtoHubAndGetAnswer( setSceneRoom );
    auto error = answer[ "error" ];
    ASSERT_FALSE( error.empty() ) << error.dump( 4 );

    // add new room to core
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject room = coreRemoteInterface.createRoom( "newRoom" );
    std::string roomId = json_get_str( room, "_id" );
    EXPECT_FALSE( roomId.empty() );
    waitForBroadcast( "hub.room.created", "hub.room.created: " + roomId + " code line: " + std::to_string( __LINE__ ) );

    // link scene with room
    setSceneRoom[ "params" ][ "roomId" ] = roomId;
    answer = nma().sendtoHubAndGetAnswer( setSceneRoom );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    // check hub.scene.changed broadcast
    JsonObject broadcast = waitForBroadcast(
        "hub.scene.changed", "hub.scene.changed scene: " + sceneId + " code line: " + std::to_string( __LINE__ ) );
    JsonObject result;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.changed" );
    EXPECT_EQ( json_get_str( result, "parent_id" ), roomId );
}

TEST_F( SceneManagerTests, deleteRoomAndCheckScene ) {  // 1.20.2
    SceneID sceneId = createScene( createEmptyScene );

    // add new room to core
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject room = coreRemoteInterface.createRoom( "newRoom" );
    std::string roomId = json_get_str( room, "_id" );
    EXPECT_FALSE( roomId.empty() );
    waitForBroadcast( "hub.room.created", "hub.room.created: " + roomId + " code line: " + std::to_string( __LINE__ ) );

    // link scene with room
    auto setSceneRoom = HubPlatform::Tests::setSceneRoom;
    setSceneRoom[ "params" ][ "sceneId" ] = sceneId;
    setSceneRoom[ "params" ][ "roomId" ] = roomId;
    JsonObject answer = nma().sendtoHubAndGetAnswer( setSceneRoom );
    JsonObject error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    // check hub.scene.changed broadcast
    JsonObject broadcast = waitForBroadcast(
        "hub.scene.changed", "hub.scene.changed scene: " + sceneId + " code line: " + std::to_string( __LINE__ ) );
    JsonObject result;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.changed" );
    EXPECT_EQ( json_get_str( result, "parent_id" ), roomId );

    // unlink scene from room with command hub.scenes.room.set with params ( roomid "" or "0" )
    {
        std::string roomId = "0";
        auto setSceneRoom = HubPlatform::Tests::setSceneRoom;
        setSceneRoom[ "params" ][ "sceneId" ] = sceneId;
        setSceneRoom[ "params" ][ "roomId" ] = roomId;
        JsonObject answer = nma().sendtoHubAndGetAnswer( setSceneRoom );
        JsonObject error = answer[ "error" ];
        ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    }

    // check hub.scene.changed broadcast
    {
        JsonObject broadcast = waitForBroadcast(
            "hub.scene.changed", "hub.scene.changed scene: " + sceneId + " code line: " + std::to_string( __LINE__ ) );
        JsonObject result;
        EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.changed" );
        EXPECT_EQ( json_get_str( result, "parent_id" ), "" );
    }

    // link scene with room again
    {
        auto setSceneRoom = HubPlatform::Tests::setSceneRoom;
        setSceneRoom[ "params" ][ "sceneId" ] = sceneId;
        setSceneRoom[ "params" ][ "roomId" ] = roomId;
        JsonObject answer = nma().sendtoHubAndGetAnswer( setSceneRoom );
        JsonObject error = answer[ "error" ];
        ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    }

    // check hub.scene.changed broadcast
    {
        JsonObject broadcast = waitForBroadcast(
            "hub.scene.changed", "hub.scene.changed scene: " + sceneId + " code line: " + std::to_string( __LINE__ ) );
        JsonObject result;
        EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.changed" );
        EXPECT_EQ( json_get_str( result, "parent_id" ), roomId );
    }

    // delete room
    {
        JsonObject result = coreRemoteInterface.deleteRoom( roomId );
        EXPECT_FALSE( hasError( result ) );
        JsonObject broadcast = waitForBroadcast(
            "hub.room.deleted", "hub.room.deleted: " + roomId + " code line: " + std::to_string( __LINE__ ) );
    }

    // check hub.scene.changed broadcast
    {
        JsonObject broadcast = waitForBroadcast(
            "hub.scene.changed", "hub.scene.changed scene: " + sceneId + " code line: " + std::to_string( __LINE__ ) );
        JsonObject result;
        EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.changed" );
        EXPECT_EQ( json_get_str( result, "parent_id" ), std::string() );
    }
}

/// @brief
/// set switch delay of house modes to 0
/// set house mode is not allowed by scene
/// set house mode is allowed by scene
/// set house mode is not allowed by scene
/// set house mode is allowed by scene
TEST_F( SceneManagerTests, houseModeCondition ) {  // 1.21
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );

    // set switch delay of house modes to 0
    coreRemoteInterface.setSwitchDelay( 0 );

    // set house mode is not allowed by scene
    auto houseModeId = coreRemoteInterface.getCurrentHouseModeId();
    if( houseModeId != "1" ) {
        coreRemoteInterface.setHouseMode( "1" );
        testHouseModesSwitchedBroadcast( "1", "begin", __LINE__ );
        testHouseModesSwitchedBroadcast( "1", "done", __LINE__ );
    }

    EXPECT_EQ( coreRemoteInterface.getCurrentHouseModeId(), "1" );
    scenesManagerInterface.setTestClock( std::chrono::seconds( 1562505419LL ) );
    SceneID sceneId = createScene( createHouseModeScene );
    runSceneManually( sceneId, __LINE__ );
    checkNoBroadcastReceived( "scene: " + sceneId, __LINE__ );

    // set house mode is allowed by scene
    coreRemoteInterface.setHouseMode( "2" );
    testHouseModesSwitchedBroadcast( "2", "begin", __LINE__ );
    testHouseModesSwitchedBroadcast( "2", "done", __LINE__ );
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( createHouseModeScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createHouseModeScene, sceneId, "finished", __LINE__ );

    // set house mode is not allowed by scene
    coreRemoteInterface.setHouseMode( "1" );
    testHouseModesSwitchedBroadcast( "1", "begin", __LINE__ );
    testHouseModesSwitchedBroadcast( "1", "done", __LINE__ );
    runSceneManually( sceneId, __LINE__ );
    checkNoBroadcastReceived( "scene: " + sceneId, __LINE__ );

    // set house mode is allowed by scene
    coreRemoteInterface.setHouseMode( "4" );
    testHouseModesSwitchedBroadcast( "4", "begin", __LINE__ );
    testHouseModesSwitchedBroadcast( "4", "done", __LINE__ );
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( createHouseModeScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createHouseModeScene, sceneId, "finished", __LINE__ );
}

TEST_F( SceneManagerTests, actionDelay ) {  // 1.23
    SceneID sceneId = createScene( createActionDelay );
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( createActionDelay, sceneId, "started", __LINE__ );
    checkNoBroadcastReceived( "check delay, scene: " + sceneId, __LINE__, std::chrono::seconds( 2 ) );
    testRunProgressBroadcast( createActionDelay, sceneId, "failed", __LINE__ );
}

TEST_F( SceneManagerTests, userNotificationFalseOnStartScene ) {  // 1.25
    SceneID sceneId = createScene( createSceneJson );
    runSceneManually( sceneId, __LINE__ );
    std::string codeLineStr = "code line: " + std::to_string( __LINE__ );
    std::string status = "started";
    JsonObject broadcast =
        waitForBroadcast( "hub.scene.run.progress",
                          "testRunProgressBroadcast scene: " + sceneId + ", status: " + status + ", " + codeLineStr,
                          progressBroadcastTimeout );
    JsonObject result = JSON_EMPTY_OBJ;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << codeLineStr;
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.run.progress" ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "scene_id" ), sceneId ) << codeLineStr;
    EXPECT_EQ( result[ "scene_name" ].dump(), createSceneJson[ "params" ][ "name" ].dump() ) << codeLineStr;

    EXPECT_EQ( json_get_bool( result, "userNotification" ), false ) << codeLineStr;

    testRunProgressBroadcast( createSceneJson, sceneId, "finished", __LINE__ );
}

TEST_F( SceneManagerTests, userNotificationTrueOnOtherSceneStatuses ) {  // 1.26
    SceneID sceneId = createScene( createSceneJson );
    runSceneManually( sceneId, __LINE__ );
    std::string codeLineStr = "code line: " + std::to_string( __LINE__ );
    testRunProgressBroadcast( createSceneJson, sceneId, "started", __LINE__ );

    JsonObject broadcast = waitForBroadcast( "hub.scene.run.progress",
                                             "testRunProgressBroadcast scene: " + sceneId + ", " + codeLineStr,
                                             progressBroadcastTimeout );
    JsonObject result = JSON_EMPTY_OBJ;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << codeLineStr;

    std::string status = json_get_str( result, "status" );

    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.run.progress" ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "scene_id" ), sceneId ) << codeLineStr;
    EXPECT_EQ( result[ "scene_name" ].dump(), createSceneJson[ "params" ][ "name" ].dump() ) << codeLineStr;

    EXPECT_EQ( json_get_bool( result, "userNotification" ), true ) << codeLineStr << "\nscene status is: " << status;
}

TEST_F( SceneManagerTests, blockList ) {  // 1.27.1
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId1 = addDevice( coreRemoteInterface,
                                "testDevice1",
                                "smoke",
                                "134_3_24",
                                "security_sensor",
                                "smoke",
                                false /*batteryPowered*/,
                                __LINE__ );
    auto deviceId2 = addDevice( coreRemoteInterface,
                                "testDevice1",
                                "smoke",
                                "134_3_24",
                                "security_sensor",
                                "smoke",
                                false /*batteryPowered*/,
                                __LINE__ );
    auto itemId1 = addItem(
        coreRemoteInterface, deviceId1, "testItem1", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    auto itemId2 = addItem(
        coreRemoteInterface, deviceId1, "testItem2", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    auto itemId3 = addItem(
        coreRemoteInterface, deviceId2, "testItem1", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    auto itemId4 = addItem(
        coreRemoteInterface, deviceId2, "testItem2", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    using ResultContainer = std::multiset< std::string >;
    JsonArray whenBlocks;
    ResultContainer itemIds;
    JsonObject answer, error;
    auto getResult = []( const JsonObject& whenBlocks, int codeLine ) -> ResultContainer {
        std::string codeLineStr = "code line: " + std::to_string( codeLine );
        ResultContainer itemIds;
        for( auto& block : whenBlocks ) {
            std::string id;
            EXPECT_TRUE( json_get( block, std::vector< Json >( { "fields", 0, "value" } ), id ) ) << codeLineStr;
            itemIds.emplace( std::move( id ) );
        }
        return itemIds;
    };

    auto request = replaceTokens( getScenesBlocksList, { { "__DEVICE_ID__", "" } } );
    request[ "params" ].erase( "devices" );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32600 );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.notfound.devices" );
    EXPECT_EQ( json_get_str( error, "message" ), "Bad request, devices does not exist" );

    request = replaceTokens( getScenesBlocksList, { { "__DEVICE_ID__", "" } } );
    request[ "params" ].erase( "blockType" );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32600 );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.notfound.blockType" );
    EXPECT_EQ( json_get_str( error, "message" ), "Bad request, blockType does not exist" );

    answer = nma().sendtoHubAndGetAnswer( replaceTokens( getScenesBlocksList, { { "__DEVICE_ID__", deviceId1 } } ) );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    EXPECT_TRUE( json_get( answer, { "result", "when" }, whenBlocks ) );
    itemIds = getResult( whenBlocks, __LINE__ );
    EXPECT_EQ( itemIds, ResultContainer( { itemId1, itemId2 } ) );

    answer = nma().sendtoHubAndGetAnswer( replaceTokens( getScenesBlocksList, { { "__DEVICE_ID__", deviceId2 } } ) );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    EXPECT_TRUE( json_get( answer, { "result", "when" }, whenBlocks ) );
    itemIds = getResult( whenBlocks, __LINE__ );
    EXPECT_EQ( itemIds, ResultContainer( { itemId3, itemId4 } ) );

    answer = nma().sendtoHubAndGetAnswer( replaceTokens( getScenesBlocksList, { { "__DEVICE_ID__", "invalidId" } } ) );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    whenBlocks = answer[ "result" ][ "when" ];
    EXPECT_TRUE( whenBlocks.empty() );

    // remove devices to prevent unnecessary update broadcast
    JsonObject broadcast;
    coreRemoteInterface.removeDevice( deviceId1 );
    broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
    coreRemoteInterface.removeDevice( deviceId2 );
    broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
}

TEST_F( SceneManagerTests, blockListAlignOptionsValueTypeToken ) {  // 1.27.2.1.1
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId1 = addDevice( coreRemoteInterface,
                                "testDevice1",
                                "smoke",
                                "134_3_24",
                                "security_sensor",
                                "smoke",
                                false /*batteryPowered*/,
                                __LINE__ );
    CoreD::Model::Item::Enum enum1 = { "manual_lock_operation", "rf_unlock_operation", "keypad_lock_operation" };
    CoreD::Model::Item::Enum enum2 = {
        "manual_unlock_operation", "rf_lock_operation", "manual_not_fully_locked_operation"
    };
    addItem( coreRemoteInterface,
             deviceId1,
             "testItem1",
             CoreD::Model::ValueType::Enum::Token,
             true,
             true,
             enum1,
             __LINE__ );
    addItem( coreRemoteInterface,
             deviceId1,
             "testItem2",
             CoreD::Model::ValueType::Enum::Token,
             true,
             true,
             enum2,
             __LINE__ );

    JsonArray whenBlocks;
    auto answer =
        nma().sendtoHubAndGetAnswer( replaceTokens( getScenesBlocksList, { { "__DEVICE_ID__", deviceId1 } } ) );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    EXPECT_TRUE( json_get( answer, { "result", "when" }, whenBlocks ) );

    EXPECT_EQ( JsonArray( enum1 ).dump(), getOptionValues( whenBlocks[ 0 ] ).dump() );
    EXPECT_EQ( JsonArray( enum2 ).dump(), getOptionValues( whenBlocks[ 1 ] ).dump() );

    removeDevice( coreRemoteInterface, deviceId1, /*itemNumbers*/ 2, 0, __LINE__ );
}

TEST_F( SceneManagerTests, blockListAlignOptionsValueTypeButtonState ) {  // 1.27.2.1.2
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId1 = addDevice( coreRemoteInterface,
                                "testDevice1",
                                "dimmer.outlet",
                                "0_0_0",
                                "dimmable_light",
                                "dimmable_plugged",
                                false /*batteryPowered*/,
                                __LINE__ );
    const CoreD::Model::Item::Enum enum1 = { "released", "press_1_time" };

    addItem( coreRemoteInterface,
             deviceId1,
             "button_state",
             CoreD::Model::ValueType::Enum::ButtonState,
             true,
             true,
             enum1,
             __LINE__ );

    JsonArray whenBlocks;
    auto answer =
        nma().sendtoHubAndGetAnswer( replaceTokens( getScenesBlocksList, { { "__DEVICE_ID__", deviceId1 } } ) );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    EXPECT_TRUE( json_get( answer, { "result", "when" }, whenBlocks ) );

    EXPECT_EQ( JsonArray( enum1 ).dump(), getOptionValues( whenBlocks[ 0 ] ).dump() );

    removeDevice( coreRemoteInterface, deviceId1, /*itemNumbers*/ 1, 0, __LINE__ );
}

TEST_F( SceneManagerTests, blockListOptionItems ) {  // 1.27.3
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId1 = addDevice( coreRemoteInterface,
                                "testDevice1",
                                "smoke",
                                "134_3_24",
                                "security_sensor",
                                "smoke",
                                false /*batteryPowered*/,
                                __LINE__ );
    auto deviceId2 = addDevice( coreRemoteInterface,
                                "testDevice1",
                                "smoke",
                                "134_3_24",
                                "security_sensor",
                                "smoke",
                                false /*batteryPowered*/,
                                __LINE__ );
    auto deviceId3 = addDevice( coreRemoteInterface,
                                "testDevice3",
                                "smoke",
                                "134_3_24",
                                "security_sensor",
                                "smoke",
                                false /*batteryPowered*/,
                                __LINE__ );
    auto userCodesId1 = addItem(
        coreRemoteInterface, deviceId1, "user_codes", CoreD::Model::ValueType::Enum::Token, true, true, __LINE__ );
    auto userLockOperationId1 = addItem( coreRemoteInterface,
                                         deviceId1,
                                         "user_lock_operation",
                                         CoreD::Model::ValueType::Enum::Token,
                                         true,
                                         true,
                                         __LINE__ );
    auto userCodesId2 = addItem(
        coreRemoteInterface, deviceId2, "user_codes", CoreD::Model::ValueType::Enum::Token, true, true, __LINE__ );
    auto userLockOperationId2 = addItem( coreRemoteInterface,
                                         deviceId2,
                                         "user_lock_operation",
                                         CoreD::Model::ValueType::Enum::Token,
                                         true,
                                         true,
                                         __LINE__ );
    auto userCodesId3 = addItem(
        coreRemoteInterface, deviceId3, "user_codes", CoreD::Model::ValueType::Enum::Token, true, true, __LINE__ );
    auto userCodeActionId3 = addItem( coreRemoteInterface,
                                      deviceId3,
                                      "user_code_action",
                                      CoreD::Model::ValueType::Enum::Token,
                                      true,
                                      true,
                                      __LINE__ );
    auto testOptionItem = [this]( const std::string& userLockOperationId,
                                  const std::string& userCodesId,
                                  const std::string& deviceId,
                                  int codeLine ) {
        std::string codeLineStr = getCodeLineString( codeLine );
        JsonArray whenBlocks;
        auto answer =
            nma().sendtoHubAndGetAnswer( replaceTokens( getScenesBlocksList, { { "__DEVICE_ID__", deviceId } } ) );
        auto error = answer[ "error" ];
        ASSERT_TRUE( error.empty() ) << error.dump( 4 ) << codeLineStr;
        ASSERT_TRUE( json_get( answer, { "result", "when" }, whenBlocks ) ) << codeLineStr;
        ASSERT_EQ( whenBlocks.size(), 1 ) << codeLineStr;
        auto& whenBlock = whenBlocks[ 0 ];
        std::string methodName;
        std::string itemId;
        JsonObject optionItem;
        EXPECT_TRUE( json_get( whenBlock, { "blockOptions", "method", "name" }, methodName ) ) << codeLineStr;
        EXPECT_TRUE( json_get( whenBlock, { "fields", 0, "value" }, itemId ) ) << codeLineStr;
        EXPECT_EQ( itemId, userLockOperationId ) << codeLineStr;
        ASSERT_TRUE( json_get( whenBlock, { "fields", 2, "optionItemValue" }, optionItem ) ) << codeLineStr;
        EXPECT_EQ( "itemId", optionItem.begin().key() ) << codeLineStr;
        EXPECT_EQ( userCodesId, optionItem.begin()->get< std::string >() ) << codeLineStr;
    };
    testOptionItem( userLockOperationId1, userCodesId1, deviceId1, __LINE__ );
    testOptionItem( userLockOperationId2, userCodesId2, deviceId2, __LINE__ );
    testOptionItem( userCodeActionId3, userCodesId3, deviceId3, __LINE__ );

    removeItem( coreRemoteInterface, userLockOperationId1, __LINE__ );
    removeItem( coreRemoteInterface, userCodesId1, __LINE__ );

    userLockOperationId1 = addItem( coreRemoteInterface,
                                    deviceId1,
                                    "user_lock_operation",
                                    CoreD::Model::ValueType::Enum::Token,
                                    true,
                                    true,
                                    __LINE__ );
    userCodesId1 = addItem(
        coreRemoteInterface, deviceId1, "user_codes", CoreD::Model::ValueType::Enum::Token, true, true, __LINE__ );

    testOptionItem( userLockOperationId1, userCodesId1, deviceId1, __LINE__ );
    testOptionItem( userLockOperationId2, userCodesId2, deviceId2, __LINE__ );

    removeDevice( coreRemoteInterface, deviceId1, 2, 0, __LINE__ );
    removeDevice( coreRemoteInterface, deviceId2, 2, 0, __LINE__ );
    removeDevice( coreRemoteInterface, deviceId3, 2, 0, __LINE__ );
}

TEST_F( SceneManagerTests, blockListDeviceCategoryAndSubcategory ) {  // 1.27.4
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId1 = addDevice( coreRemoteInterface,
                                "water_valve",
                                "shutter.valve",
                                "134_3_24",
                                "switch",
                                "valve",
                                false /*batteryPowered*/,
                                __LINE__ );
    auto deviceId2 = addDevice( coreRemoteInterface,
                                "plug1",
                                "switch.outlet",
                                "134_3_24",
                                "switch",
                                "interior_plugin",
                                false /*batteryPowered*/,
                                __LINE__ );
    auto deviceId3 = addDevice(
        coreRemoteInterface, "plug2", "switch.outlet", "134_3_24", "switch", "", false /*batteryPowered*/, __LINE__ );
    auto itemId1 =
        addItem( coreRemoteInterface, deviceId1, "switch", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId2 =
        addItem( coreRemoteInterface, deviceId2, "switch", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId3 =
        addItem( coreRemoteInterface, deviceId3, "switch", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );

    auto testResult = [this]( const std::string& switchId,
                              const std::string& deviceId,
                              const std::string& expectedMethodName,
                              int codeLine ) {
        std::string codeLineStr = getCodeLineString( codeLine );
        JsonArray whenBlocks;
        auto answer =
            nma().sendtoHubAndGetAnswer( replaceTokens( getScenesBlocksList, { { "__DEVICE_ID__", deviceId } } ) );
        auto error = answer[ "error" ];
        ASSERT_TRUE( error.empty() ) << error.dump( 4 ) << codeLineStr;
        ASSERT_TRUE( json_get( answer, { "result", "when" }, whenBlocks ) ) << codeLineStr;
        ASSERT_EQ( whenBlocks.size(), 1 ) << codeLineStr;
        auto& whenBlock = whenBlocks[ 0 ];
        std::string methodName;
        std::string itemId;
        EXPECT_TRUE( json_get( whenBlock, { "blockOptions", "method", "name" }, methodName ) ) << codeLineStr;
        EXPECT_EQ( methodName, expectedMethodName ) << codeLineStr;
        EXPECT_TRUE( json_get( whenBlock, { "fields", 0, "value" }, itemId ) ) << codeLineStr;
        EXPECT_EQ( itemId, switchId ) << codeLineStr;
        JsonObject options;
        EXPECT_TRUE( json_get( whenBlock, { "fields", 1, "options" }, options ) ) << codeLineStr;
        if( methodName == "isItemStateOnlySwitchValve" ) {
            JsonArray expectedOptions = R"(
            [
                {
                    "value":true,
                    "label":{
                        "lang_tag":"ui7_mode_open",
                        "text":"Open"
                    }
                },
                {
                    "value":false,
                    "label":{
                        "lang_tag":"ui7_mode_closed",
                        "text":"Closed"
                    }
                }
            ]
            )"_json;
            EXPECT_EQ( options.dump(), expectedOptions.dump() );
        }
        else if( methodName == "isItemStateNotSwitchValve" ) {
            JsonArray expectedOptions = R"(
            [
                {
                    "value":true,
                    "label":{
                        "lang_tag":"ui7_on",
                        "text":"On"
                    }
                },
                {
                    "value":false,
                    "label":{
                        "lang_tag":"ui7_mode_off",
                        "text":"Off"
                    }
                }
            ]
            )"_json;
            EXPECT_EQ( options.dump(), expectedOptions.dump() );
        }
    };

    testResult( itemId1, deviceId1, "isItemStateOnlySwitchValve", __LINE__ );
    testResult( itemId2, deviceId2, "isItemStateNotSwitchValve", __LINE__ );
    testResult( itemId3, deviceId3, "isItemStateNotSwitchValve", __LINE__ );

    removeDevice( coreRemoteInterface, deviceId1, 1, 0, __LINE__ );
    removeDevice( coreRemoteInterface, deviceId2, 1, 0, __LINE__ );
    removeDevice( coreRemoteInterface, deviceId3, 1, 0, __LINE__ );
}

TEST_F( SceneManagerTests, compareNumbersCreationErrors ) {  // 1.28.1
    auto checkError = [this]( const JsonObject& request, int codeLine ) {
        std::string codeLineStr = getCodeLineString( codeLine );
        auto answer = nma().sendtoHubAndGetAnswer( request );
        auto error = answer[ "error" ];
        ASSERT_FALSE( error.empty() ) << codeLineStr;
        EXPECT_EQ( json_get_int( error, "code" ), -32500 ) << codeLineStr;
        EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.block.when.wrong" ) << codeLineStr;
        EXPECT_EQ( json_get_str( error, "message" ), "Scene is ill-formed. Can't parse when block" ) << codeLineStr;
    };
    const auto referenceRequest =
        replaceTokens( createCompareNumbersScene, { { "__ITEM_ID1__", "itemId1" }, { "__ITEM_ID2__", "itemId2" } } );
    JsonObject request = referenceRequest;
    request[ "params" ][ "when" ][ 0 ][ "fields" ].erase( 0 );
    checkError( request, __LINE__ );

    request = referenceRequest;
    request[ "params" ][ "when" ][ 0 ][ "fields" ].erase( 1 );
    checkError( request, __LINE__ );

    request = referenceRequest;
    request[ "params" ][ "when" ][ 0 ][ "fields" ].erase( 2 );
    checkError( request, __LINE__ );

    request = referenceRequest;
    request[ "params" ][ "when" ][ 0 ].erase( "fields" );
    checkError( request, __LINE__ );
}

TEST_F( SceneManagerTests, compareNumbersExecution ) {  // 1.28.2
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 =
        addItem( coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    auto itemId2 = addItem(
        coreRemoteInterface, deviceId, "testItem2", CoreD::Model::ValueType::Enum::Float, true, true, __LINE__ );

    coreRemoteInterface.setItemValue( JSON( ItemFields::IdFieldName << itemId1 << ItemFields::ValueFieldName << 2 ) );
    checkItemUpdateBroadcast( itemId1, 2, __LINE__ );
    coreRemoteInterface.setItemValue(
        JSON( ItemFields::IdFieldName << itemId2 << ItemFields::ValueFieldName << 2.2f ) );
    checkItemUpdateBroadcast( itemId2, 2.2f, __LINE__ );

    auto request =
        replaceTokens( createCompareNumbersScene, { { "__ITEM_ID1__", itemId1 }, { "__ITEM_ID2__", itemId2 } } );
    SceneID sceneId = createScene( request );

    coreRemoteInterface.setItemValue( JSON( ItemFields::IdFieldName << itemId1 << ItemFields::ValueFieldName << 51 ) );
    checkItemUpdateBroadcast( itemId1, 51, __LINE__ );
    coreRemoteInterface.setItemValue(
        JSON( ItemFields::IdFieldName << itemId2 << ItemFields::ValueFieldName << 35.55f ) );
    checkItemUpdateBroadcast( itemId2, 35.55f, __LINE__ );

    coreRemoteInterface.setItemValue( JSON( ItemFields::IdFieldName << itemId1 << ItemFields::ValueFieldName << 52 ) );
    checkItemUpdateBroadcast( itemId1, 52, __LINE__ );
    coreRemoteInterface.setItemValue(
        JSON( ItemFields::IdFieldName << itemId2 << ItemFields::ValueFieldName << 35.6f ) );
    auto checkLastBroadcast = [&](
                                  CoreDRemoteInterface::CoreDInterface::ItemID itemId, float itemValue, int codeLine ) {
        auto codeLineStr = getCodeLineString( codeLine );
        JsonObject broadcast =
            waitForAnyOfBroadcasts( { "hub.scene.run.progress", "hub.item.updated" }, getCodeLineString( codeLine ) );
        if( json_get_str( broadcast, "msg_subclass" ) == "hub.scene.run.progress" ) {
            JsonObject result = JSON_EMPTY_OBJ;
            EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << codeLineStr;
            if( json_get_str( result, "status" ) == "started" ) {
                testRunProgressBroadcast( broadcast, createCompareNumbersScene, sceneId, "started", codeLine );
            }
            else {
                testRunProgressBroadcast( broadcast, createCompareNumbersScene, sceneId, "finished", codeLine );
            }
        }
        else {
            checkItemUpdateBroadcast( broadcast, itemId, itemValue, codeLine );
        }
    };
    checkLastBroadcast( itemId2, 35.6f, __LINE__ );
    checkLastBroadcast( itemId2, 35.6f, __LINE__ );
    checkLastBroadcast( itemId2, 35.6f, __LINE__ );
    testRunProgressBroadcast( createCompareNumbersScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createCompareNumbersScene, sceneId, "finished", __LINE__ );

    coreRemoteInterface.setItemValue( JSON( ItemFields::IdFieldName << itemId1 << ItemFields::ValueFieldName << 60 ) );
    checkItemUpdateBroadcast( itemId1, 60, __LINE__ );
    coreRemoteInterface.setItemValue(
        JSON( ItemFields::IdFieldName << itemId2 << ItemFields::ValueFieldName << 60.0f ) );
    checkItemUpdateBroadcast( itemId2, 60.0f, __LINE__ );

    coreRemoteInterface.setItemValue( JSON( ItemFields::IdFieldName << itemId1 << ItemFields::ValueFieldName << 70 ) );
    checkItemUpdateBroadcast( itemId1, 70, __LINE__ );
    coreRemoteInterface.setItemValue(
        JSON( ItemFields::IdFieldName << itemId2 << ItemFields::ValueFieldName << 70.0f ) );
    checkItemUpdateBroadcast( itemId2, 70.0f, __LINE__ );

    coreRemoteInterface.setItemValue( JSON( ItemFields::IdFieldName << itemId1 << ItemFields::ValueFieldName << 50 ) );
    checkItemUpdateBroadcast( itemId1, 50, __LINE__ );
    coreRemoteInterface.setItemValue(
        JSON( ItemFields::IdFieldName << itemId2 << ItemFields::ValueFieldName << 30.0f ) );
    checkItemUpdateBroadcast( itemId2, 30.0f, __LINE__ );

    coreRemoteInterface.setItemValue( JSON( ItemFields::IdFieldName << itemId1 << ItemFields::ValueFieldName << 55 ) );
    checkItemUpdateBroadcast( itemId1, 55, __LINE__ );
    coreRemoteInterface.setItemValue(
        JSON( ItemFields::IdFieldName << itemId2 << ItemFields::ValueFieldName << 55.0f ) );
    checkLastBroadcast( itemId2, 55.0f, __LINE__ );
    checkLastBroadcast( itemId2, 55.0f, __LINE__ );
    checkLastBroadcast( itemId2, 55.0f, __LINE__ );
    testRunProgressBroadcast( createCompareNumbersScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createCompareNumbersScene, sceneId, "finished", __LINE__ );

    deleteScenes();

    coreRemoteInterface.setItemValue( JSON( ItemFields::IdFieldName << itemId1 << ItemFields::ValueFieldName << 50 ) );
    checkItemUpdateBroadcast( itemId1, 50, __LINE__ );

    coreRemoteInterface.setItemValue( JSON( ItemFields::IdFieldName << itemId1 << ItemFields::ValueFieldName << 52 ) );
    checkItemUpdateBroadcast( itemId1, 52, __LINE__ );

    checkNoBroadcastReceived( "no hub.scene.run.progress broadcasts should be", __LINE__, std::chrono::seconds( 1 ) );

    // remove devices to prevent unnecessary update broadcast
    JsonObject broadcast;
    coreRemoteInterface.removeDevice( deviceId );
    broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
}

TEST_F( SceneManagerTests, triggerDevicesListRequest ) {  // 1.29
    clearExistingDevices( __LINE__ );
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId1 = addDevice( coreRemoteInterface,
                                "testDevice1",
                                "smoke",
                                "134_3_24",
                                "security_sensor",
                                "smoke",
                                false /*batteryPowered*/,
                                __LINE__ );
    auto deviceId2 = addDevice( coreRemoteInterface,
                                "testDevice1",
                                "smoke",
                                "134_3_24",
                                "security_sensor",
                                "smoke",
                                false /*batteryPowered*/,
                                __LINE__ );

    auto testScenesTriggerDevicesList =
        [this]( std::vector< CoreDRemoteInterface::CoreDInterface::DeviceID > expectedDeviceIds, int codeLine ) {
            std::string codeLineStr = getCodeLineString( codeLine );
            auto answer = nma().sendtoHubAndGetAnswer( getScenesTriggerDevicesList );
            auto& error = answer[ "error" ];
            EXPECT_TRUE( error.empty() ) << codeLineStr << ", " << error.dump( 4 );

            JsonArray deviceIdsJson = JSON_EMPTY_ARR;
            EXPECT_TRUE( json_get( answer, { "result", "devices" }, deviceIdsJson ) ) << codeLineStr;
            std::vector< CoreDRemoteInterface::CoreDInterface::DeviceID > deviceIds;
            for( const auto& id : deviceIdsJson ) {
                deviceIds.emplace_back( id.get< std::string >() );
            }
            std::size_t size =
                expectedDeviceIds.size() > deviceIds.size() ? expectedDeviceIds.size() : deviceIds.size();
            std::sort( expectedDeviceIds.begin(), expectedDeviceIds.end() );
            std::sort( deviceIds.begin(), deviceIds.end() );
            std::vector< CoreDRemoteInterface::CoreDInterface::DeviceID > result( size );
            auto it = std::set_symmetric_difference( expectedDeviceIds.begin(),
                                                     expectedDeviceIds.end(),
                                                     deviceIds.begin(),
                                                     deviceIds.end(),
                                                     result.begin() );
            auto print = []( const std::vector< CoreDRemoteInterface::CoreDInterface::DeviceID >& vector ) {
                std::string str;
                for( const auto& value : vector ) {
                    str += value + " ";
                }
                return str;
            };
            EXPECT_TRUE( it == result.begin() )
                << codeLineStr
                << ", item ids: " << print( deviceIds ) + ", expected ids: " + print( expectedDeviceIds );
        };

    testScenesTriggerDevicesList( {}, __LINE__ );

    addItem( coreRemoteInterface, deviceId1, "testItem1", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    addItem( coreRemoteInterface, deviceId1, "testItem2", CoreD::Model::ValueType::Enum::Float, true, true, __LINE__ );
    addItem( coreRemoteInterface, deviceId2, "testItem3", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    addItem( coreRemoteInterface, deviceId2, "testItem4", CoreD::Model::ValueType::Enum::Float, true, true, __LINE__ );

    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    testScenesTriggerDevicesList( { deviceId1 }, __LINE__ );

    addItem( coreRemoteInterface, deviceId2, "testItem2", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    testScenesTriggerDevicesList( { deviceId1, deviceId2 }, __LINE__ );

    // remove devices to prevent unnecessary update broadcast
    JsonObject broadcast;
    coreRemoteInterface.removeDevice( deviceId1 );
    broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );

    testScenesTriggerDevicesList( { deviceId2 }, __LINE__ );

    coreRemoteInterface.removeDevice( deviceId2 );
    broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );

    testScenesTriggerDevicesList( {}, __LINE__ );
}

TEST_F( SceneManagerTests, isButtonStateCreationErrors ) {  // 1.30.1
    auto checkError = [this]( const JsonObject& request, int codeLine ) {
        std::string codeLineStr = getCodeLineString( codeLine );
        auto answer = nma().sendtoHubAndGetAnswer( request );
        auto error = answer[ "error" ];
        ASSERT_FALSE( error.empty() ) << codeLineStr;
        EXPECT_EQ( json_get_int( error, "code" ), -32500 ) << codeLineStr;
        EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.block.when.wrong" ) << codeLineStr;
        EXPECT_EQ( json_get_str( error, "message" ), "Scene is ill-formed. Can't parse when block" ) << codeLineStr;
    };
    const auto referenceRequest =
        replaceTokens( createIsButtonStateScene, { { "__ITEM_ID1__", "itemId1" }, { "__ITEM_ID2__", "itemId2" } } );
    JsonObject request = referenceRequest;
    request[ "params" ][ "when" ][ 0 ][ "fields" ].erase( 0 );
    checkError( request, __LINE__ );

    request = referenceRequest;
    request[ "params" ][ "when" ][ 0 ][ "fields" ].erase( 1 );
    checkError( request, __LINE__ );

    request = referenceRequest;
    request[ "params" ][ "when" ][ 0 ].erase( "fields" );
    checkError( request, __LINE__ );
}

TEST_F( SceneManagerTests, isButtonStateExecution ) {  // 1.30.2
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "Remote Control", /*name*/
                               "device",         /*type*/
                               "520_512_11",     /*deviceTypeId*/
                               "remote_control", /*category*/
                               "",               /*subcategory*/
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 = addItem( coreRemoteInterface,
                            deviceId,
                            "button_state",
                            CoreD::Model::ValueType::Enum::ButtonState,
                            true,
                            true,
                            __LINE__ );
    auto itemId2 = addItem( coreRemoteInterface,
                            deviceId,
                            "button_state",
                            CoreD::Model::ValueType::Enum::ButtonState,
                            true,
                            true,
                            __LINE__ );

    JsonObject value = JSON( ButtonStateValueFields::ButtonStateFieldName
                             << "released" << ButtonStateValueFields::ButtonIdFieldName << 1 );
    coreRemoteInterface.setItemValue(
        JSON( ItemFields::IdFieldName << itemId1 << ItemFields::ValueFieldName << value ) );
    checkItemUpdateBroadcast( itemId1, value, __LINE__ );
    coreRemoteInterface.setItemValue(
        JSON( ItemFields::IdFieldName << itemId2 << ItemFields::ValueFieldName << value ) );
    checkItemUpdateBroadcast( itemId2, value, __LINE__ );

    auto request =
        replaceTokens( createIsButtonStateScene, { { "__ITEM_ID1__", itemId1 }, { "__ITEM_ID2__", itemId2 } } );
    SceneID sceneId = createScene( request );

    value[ ButtonStateValueFields::ButtonStateFieldName ] = "press_1_time";
    coreRemoteInterface.setItemValue(
        JSON( ItemFields::IdFieldName << itemId1 << ItemFields::ValueFieldName << value ) );
    checkItemUpdateBroadcast( itemId1, value, __LINE__ );
    value[ ButtonStateValueFields::ButtonStateFieldName ] = "released";
    coreRemoteInterface.setItemValue(
        JSON( ItemFields::IdFieldName << itemId1 << ItemFields::ValueFieldName << value ) );
    checkItemUpdateBroadcast( itemId1, value, __LINE__ );

    testRunProgressBroadcast( createIsButtonStateScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createIsButtonStateScene, sceneId, "finished", __LINE__ );

    value[ ButtonStateValueFields::ButtonStateFieldName ] = "press_1_time";
    coreRemoteInterface.setItemValue(
        JSON( ItemFields::IdFieldName << itemId2 << ItemFields::ValueFieldName << value ) );
    checkItemUpdateBroadcast( itemId2, value, __LINE__ );

    testRunProgressBroadcast( createIsButtonStateScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createIsButtonStateScene, sceneId, "finished", __LINE__ );

    value[ ButtonStateValueFields::ButtonStateFieldName ] = "released";
    coreRemoteInterface.setItemValue(
        JSON( ItemFields::IdFieldName << itemId2 << ItemFields::ValueFieldName << value ) );
    checkItemUpdateBroadcast( itemId2, value, __LINE__ );

    // remove devices to prevent unnecessary update broadcast
    JsonObject broadcast;
    coreRemoteInterface.removeDevice( deviceId );
    broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
}

TEST_F( SceneManagerTests, serverRequests ) {  // 1.31
    SceneID sceneId = createScene( createEmptyWhenBlockSceneJson );

    auto answer = nma().sendtoHubAndGetAnswer( getScenesList, ComponentTestApplication::ConnectionType::NMA );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    auto request = replaceTokens( HubPlatform::Tests::runSceneManually, { { "__ID__", sceneId } } );
    answer = nma().sendtoHubAndGetAnswer( request, ComponentTestApplication::ConnectionType::NMA );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    testRunProgressBroadcast( createEmptyWhenBlockSceneJson, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createEmptyWhenBlockSceneJson, sceneId, "finished", __LINE__ );
}

TEST_F( SceneManagerTests, AndOperatorErrors ) {  // 1.32.1
    auto andOperator = replaceTokensInString( andOperatorWhenBlock, { { "__BLOCKS__", "" } } );
    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "andOperatorError" },
                                    { "__WHEN_BLOCKS__", std::move( andOperator ) },
                                    { "__THEN_BLOCKS__", "" } } );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.block.when.wrong" );
    EXPECT_EQ( json_get_str( error, "message" ), "Scene is ill-formed. Can't parse when block" );

    andOperator = replaceTokensInString( andOperatorWhenBlock, { { "__BLOCKS__", "{}" } } );
    request = replaceTokens( createSceneRequest,
                             { { "__NAME__", "andOperatorError" },
                               { "__WHEN_BLOCKS__", std::move( andOperator ) },
                               { "__THEN_BLOCKS__", "" } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.block.when.wrong" );
    EXPECT_EQ( json_get_str( error, "message" ), "Scene is ill-formed. Can't parse when block" );

    auto deviceTriggerBlock = replaceTokensInString( isItemStateWhenBlock, { { "__VALUE__", "true" } } );
    andOperator = replaceTokensInString( andOperatorWhenBlock,
                                         { { "__BLOCKS__", deviceTriggerBlock + "," + deviceTriggerBlock } } );
    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "AndOperatorError" }, { "__WHEN_BLOCKS__", andOperator }, { "__THEN_BLOCKS__", "" } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "scenes.when.same_item_in_and" );
    EXPECT_EQ( json_get_str( error, "message" ),
               "Scene contain conditions for same functionality inside of AND condition" );

    deviceTriggerBlock = isButtonStateWhenBlock;
    andOperator = replaceTokensInString( andOperatorWhenBlock,
                                         { { "__BLOCKS__", deviceTriggerBlock + "," + deviceTriggerBlock } } );
    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "AndOperatorError" }, { "__WHEN_BLOCKS__", andOperator }, { "__THEN_BLOCKS__", "" } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "scenes.when.same_button_in_and" );
    EXPECT_EQ( json_get_str( error, "message" ),
               "Scene contain conditions for same functionality inside of AND condition" );

    auto createCompareNumbersBlock = [&]( const std::string& comparator, const std::string& value ) {
        return replaceTokensInString( compareNumbersWhenBlock,
                                      { { "__COMPARATOR__", comparator },
                                        { "__VALUE__", value },
                                        { "__SOURCE_NAME__", "item" },
                                        { "__SOURCE_TYPE__", "item" },
                                        { "__SOURCE_VALUE__", "\"item\"" } } );
    };

    auto compareNumberBlock1 = createCompareNumbersBlock( "<", "10" );
    auto compareNumberBlock2 = createCompareNumbersBlock( ">", "30" );
    auto compareNumberBlock3 = createCompareNumbersBlock( "==", "20" );
    auto compareNumberBlock4 = createCompareNumbersBlock( "==", "25" );

    andOperator = replaceTokensInString( andOperatorWhenBlock,
                                         { { "__BLOCKS__",
                                             compareNumberBlock1 + "," + compareNumberBlock2 + "," +
                                                 compareNumberBlock3 + "," + compareNumberBlock4 } } );
    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "AndOperatorError" }, { "__WHEN_BLOCKS__", andOperator }, { "__THEN_BLOCKS__", "" } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "scenes.when.not_intersect_numbers" );
    EXPECT_EQ( json_get_str( error, "message" ),
               "Scene contain conditions for not intersect numbers values inside of AND condition" );
}

TEST_F( SceneManagerTests, AndOperator ) {  // 1.32.2
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 = addItem(
        coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId2 = addItem(
        coreRemoteInterface, deviceId, "testItem2", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );

    auto isItemStateBlock1 = replaceTokensInString(
        isItemStateWhenBlock, { { "__ITEM_ID__", itemId1 }, { "__TYPE__", "bool" }, { "__VALUE__", "true" } } );
    auto whenBlock2 = replaceTokensInString(
        isItemStateWhenBlock, { { "__ITEM_ID__", itemId2 }, { "__TYPE__", "bool" }, { "__VALUE__", "true" } } );
    isItemStateBlock1 += ",";
    isItemStateBlock1.append( std::move( whenBlock2 ) );
    auto andOperator = replaceTokensInString( andOperatorWhenBlock, { { "__BLOCKS__", isItemStateBlock1 } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "AndOperator" }, { "__WHEN_BLOCKS__", andOperator }, { "__THEN_BLOCKS__", "" } } );
    SceneID sceneId = createScene( request );
    setItemValue( coreRemoteInterface, itemId1, true, __LINE__ );
    setItemValue( coreRemoteInterface, itemId2, true, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, false, __LINE__ );
    setItemValue( coreRemoteInterface, itemId2, false, __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, true, __LINE__ );
    setItemValue( coreRemoteInterface, itemId2, true, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, false, __LINE__ );
    setItemValue( coreRemoteInterface, itemId2, false, __LINE__ );

    deleteScene( sceneId );

    setItemValue( coreRemoteInterface, itemId1, true, __LINE__ );
    setItemValue( coreRemoteInterface, itemId2, true, __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, false, __LINE__ );
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    auto resetDate = GregorianCalendar::Date( "2020-03-02T00:20:44+0000" ).toSeconds();
    auto runDate = GregorianCalendar::Date( "2020-03-18T15:54:58+0000" );
    scenesManagerInterface.setTestClock( resetDate );
    auto runDailyScene = [&scenesManagerInterface, &runDate]() {
        runDate.incrementDay();
        scenesManagerInterface.setTestClock( runDate.toSeconds() );
    };
    auto isDateDailyBlock = replaceTokensInString( isDateDailyWhenBlock,
                                                   {
                                                       { "__TIME_ARRAY__", "\"15:55\"" },
                                                   } );
    request[ "params" ][ "when" ].push_back( JsonObject::parse( isDateDailyBlock ) );
    sceneId = createScene( request );
    scenesManagerInterface.setTestClock( runDate.toSeconds() );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    runDailyScene();
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId );

    andOperator =
        replaceTokensInString( andOperatorWhenBlock, { { "__BLOCKS__", isItemStateBlock1 + "," + isDateDailyBlock } } );
    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "AndOperator" }, { "__WHEN_BLOCKS__", andOperator }, { "__THEN_BLOCKS__", "" } } );
    sceneId = createScene( request );
    runDailyScene();
    setItemValue( coreRemoteInterface, itemId1, true, __LINE__ );
    runDailyScene();
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId );

    setItemValue( coreRemoteInterface, itemId1, false, __LINE__ );
    setItemValue( coreRemoteInterface, itemId2, false, __LINE__ );
    andOperator = replaceTokensInString( andOperatorWhenBlock, { { "__BLOCKS__", isItemStateBlock1 } } );
    auto topAndOperator =
        replaceTokensInString( andOperatorWhenBlock, { { "__BLOCKS__", andOperator + "," + isDateDailyBlock } } );
    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "AndOperator" }, { "__WHEN_BLOCKS__", topAndOperator }, { "__THEN_BLOCKS__", "" } } );
    sceneId = createScene( request );
    setItemValue( coreRemoteInterface, itemId1, true, __LINE__ );
    setItemValue( coreRemoteInterface, itemId2, true, __LINE__ );
    runDailyScene();
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, false, __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, true, __LINE__ );

    removeDevice( coreRemoteInterface, deviceId, 2, 0, __LINE__ );
}

TEST_F( SceneManagerTests, AndOperatorAllowedTimeBasedBlocksCombinations ) {  // 1.32.3
    auto testTimeBasedScenes = [this]( const std::string& scene1, const std::string& scene2, int codeLine ) {
        auto codeLineStr = getCodeLineString( codeLine );
        auto andOperator = replaceTokensInString( andOperatorWhenBlock, { { "__BLOCKS__", scene1 + "," + scene2 } } );
        auto sceneName = "datesIdAnd_" + std::to_string( codeLine );
        auto request = replaceTokens(
            createSceneRequest,
            { { "__NAME__", sceneName }, { "__WHEN_BLOCKS__", andOperator }, { "__THEN_BLOCKS__", "" } } );
        createScene( request, codeLine );
    };

    const auto isDateBlock = replaceTokensInString( isDateDailyWhenBlock, { { "__TIME_ARRAY__", "\"15:55\"" } } );
    const auto isOnceBlock = replaceTokensInString(
        isOnceSceneWhenBlock,
        { { "__TIME__", "15:55" }, { "__DAY__", "1" }, { "__MONTH__", "1" }, { "__YEAR__", "2020" } } );
    const auto isSunStateBlock =
        replaceTokensInString( isSunStateWhenBlock, { { "__TYPE__", "before" }, { "__TIME__", "10:30" } } );
    const auto isIntervalBlock = replaceTokensInString( isIntervalWhenBlock, { { "__INTERVAL__", "30s" } } );

    testTimeBasedScenes( isDateBlock, isDateBlock, __LINE__ );
    testTimeBasedScenes( isDateBlock, isOnceBlock, __LINE__ );
    testTimeBasedScenes( isDateBlock, isSunStateBlock, __LINE__ );

    testTimeBasedScenes( isOnceBlock, isDateBlock, __LINE__ );
    testTimeBasedScenes( isOnceBlock, isOnceBlock, __LINE__ );
    testTimeBasedScenes( isOnceBlock, isSunStateBlock, __LINE__ );

    testTimeBasedScenes( isSunStateBlock, isDateBlock, __LINE__ );
    testTimeBasedScenes( isSunStateBlock, isOnceBlock, __LINE__ );
    testTimeBasedScenes( isSunStateBlock, isSunStateBlock, __LINE__ );

    testTimeBasedScenes( isIntervalBlock, isDateBlock, __LINE__ );
    testTimeBasedScenes( isIntervalBlock, isOnceBlock, __LINE__ );
    testTimeBasedScenes( isIntervalBlock, isSunStateBlock, __LINE__ );
    testTimeBasedScenes( isIntervalBlock, isIntervalBlock, __LINE__ );
}

TEST_F( SceneManagerTests, NotOperatorErrors ) {  // 1.33.1
    auto notOperator = replaceTokensInString( notOperatorWhenBlock, { { "__BLOCK__", "null" } } );
    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "notOperatorError" },
                                    { "__WHEN_BLOCKS__", std::move( notOperator ) },
                                    { "__THEN_BLOCKS__", "" } } );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.block.when.wrong" );
    EXPECT_EQ( json_get_str( error, "message" ), "Scene is ill-formed. Can't parse when block" );

    notOperator = replaceTokensInString( notOperatorWhenBlock, { { "__BLOCK__", "{}" } } );
    request = replaceTokens( createSceneRequest,
                             { { "__NAME__", "NotOperatorError" },
                               { "__WHEN_BLOCKS__", std::move( notOperator ) },
                               { "__THEN_BLOCKS__", "" } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.block.when.wrong" );
    EXPECT_EQ( json_get_str( error, "message" ), "Scene is ill-formed. Can't parse when block" );

    auto isItemStateBlock = replaceTokensInString(
        isItemStateWhenBlock, { { "__ITEM_ID__", "itemId" }, { "__TYPE__", "bool" }, { "__VALUE__", "true" } } );
    notOperator = replaceTokensInString( notOperatorWhenBlock,
                                         { { "__BLOCK__", "[" + isItemStateBlock + "," + isItemStateBlock + "]" } } );
    request = replaceTokens( createSceneRequest,
                             { { "__NAME__", "NotOperatorError" },
                               { "__WHEN_BLOCKS__", std::move( notOperator ) },
                               { "__THEN_BLOCKS__", "" } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.block.when.wrong" );
    EXPECT_EQ( json_get_str( error, "message" ), "Scene is ill-formed. Can't parse when block" );
}

TEST_F( SceneManagerTests, NotOperator ) {  // 1.33.2
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId = addItem(
        coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    setItemValue( coreRemoteInterface, itemId, true, __LINE__ );
    auto isItemStateBlock = replaceTokensInString(
        isItemStateWhenBlock, { { "__ITEM_ID__", itemId }, { "__TYPE__", "bool" }, { "__VALUE__", "true" } } );
    auto notOperator =
        replaceTokensInString( notOperatorWhenBlock, { { "__BLOCK__", std::move( isItemStateBlock ) } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "NotOperator" }, { "__WHEN_BLOCKS__", notOperator }, { "__THEN_BLOCKS__", "" } } );
    SceneID sceneId = createScene( request );
    setItemValue( coreRemoteInterface, itemId, false, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    setItemValue( coreRemoteInterface, itemId, true, __LINE__ );
    setItemValue( coreRemoteInterface, itemId, false, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    setItemValue( coreRemoteInterface, itemId, true, __LINE__ );

    deleteScene( sceneId );

    setItemValue( coreRemoteInterface, itemId, false, __LINE__ );
    notOperator = replaceTokensInString( notOperatorWhenBlock, { { "__BLOCK__", std::move( notOperator ) } } );
    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "NotOperator" }, { "__WHEN_BLOCKS__", std::move( notOperator ) }, { "__THEN_BLOCKS__", "" } } );
    sceneId = createScene( request );
    setItemValue( coreRemoteInterface, itemId, true, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    setItemValue( coreRemoteInterface, itemId, false, __LINE__ );

    deleteScene( sceneId );

    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    auto resetDate = GregorianCalendar::Date( "2020-03-02T00:20:44+0000" ).toSeconds();
    auto runDate = GregorianCalendar::Date( "2020-03-18T15:54:58+0000" );
    scenesManagerInterface.setTestClock( resetDate );
    auto isDateDailyBlock = replaceTokensInString( isDateDailyWhenBlock,
                                                   {
                                                       { "__TIME_ARRAY__", "\"15:55\"" },
                                                   } );
    notOperator = replaceTokensInString( notOperatorWhenBlock, { { "__BLOCK__", std::move( isDateDailyBlock ) } } );
    notOperator = replaceTokensInString( notOperatorWhenBlock, { { "__BLOCK__", std::move( notOperator ) } } );
    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "NotOperator" }, { "__WHEN_BLOCKS__", std::move( notOperator ) }, { "__THEN_BLOCKS__", "" } } );
    sceneId = createScene( request );
    scenesManagerInterface.setTestClock( runDate.toSeconds() );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    removeDevice( coreRemoteInterface, deviceId, 1, 0, __LINE__ );
}

TEST_F( SceneManagerTests, OrOperatorErrors ) {  // 1.34.1
    auto orOperator = replaceTokensInString( orOperatorWhenBlock, { { "__BLOCKS__", "" } } );
    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "orOperatorError" },
                                    { "__WHEN_BLOCKS__", std::move( orOperator ) },
                                    { "__THEN_BLOCKS__", "" } } );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.block.when.wrong" );
    EXPECT_EQ( json_get_str( error, "message" ), "Scene is ill-formed. Can't parse when block" );

    orOperator = replaceTokensInString( orOperatorWhenBlock, { { "__BLOCKS__", "{}" } } );
    request = replaceTokens( createSceneRequest,
                             { { "__NAME__", "orOperatorError" },
                               { "__WHEN_BLOCKS__", std::move( orOperator ) },
                               { "__THEN_BLOCKS__", "" } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.block.when.wrong" );
    EXPECT_EQ( json_get_str( error, "message" ), "Scene is ill-formed. Can't parse when block" );
}

TEST_F( SceneManagerTests, OrOperator ) {  // 1.34.2
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 = addItem(
        coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId2 = addItem(
        coreRemoteInterface, deviceId, "testItem2", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId3 = addItem(
        coreRemoteInterface, deviceId, "testItem3", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId4 = addItem(
        coreRemoteInterface, deviceId, "testItem4", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );

    auto whenBlock1 = replaceTokensInString(
        isItemStateWhenBlock, { { "__ITEM_ID__", itemId1 }, { "__TYPE__", "bool" }, { "__VALUE__", "true" } } );
    whenBlock1.append( "," ).append( replaceTokensInString(
        isItemStateWhenBlock, { { "__ITEM_ID__", itemId2 }, { "__TYPE__", "bool" }, { "__VALUE__", "true" } } ) );
    auto whenBlock2 = replaceTokensInString(
        isItemStateWhenBlock, { { "__ITEM_ID__", itemId3 }, { "__TYPE__", "bool" }, { "__VALUE__", "true" } } );
    whenBlock2.append( "," ).append( replaceTokensInString(
        isItemStateWhenBlock, { { "__ITEM_ID__", itemId4 }, { "__TYPE__", "bool" }, { "__VALUE__", "true" } } ) );

    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    auto resetDate = GregorianCalendar::Date( "2020-03-02T00:20:44+0000" ).toSeconds();
    auto runDate = GregorianCalendar::Date( "2020-03-18T15:54:58+0000" );
    scenesManagerInterface.setTestClock( resetDate );
    whenBlock1.append( "," ).append( replaceTokensInString( isDateDailyWhenBlock,
                                                            {
                                                                { "__TIME_ARRAY__", "\"15:55\"" },
                                                            } ) );
    auto orOperator = replaceTokensInString( orOperatorWhenBlock, { { "__BLOCKS__", std::move( whenBlock2 ) } } );
    orOperator = replaceTokensInString(
        orOperatorWhenBlock,
        { { "__BLOCKS__", std::move( orOperator.append( "," ).append( std::move( whenBlock1 ) ) ) } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "OrOperator" }, { "__WHEN_BLOCKS__", std::move( orOperator ) }, { "__THEN_BLOCKS__", "" } } );
    SceneID sceneId = createScene( request );
    setItemValue( coreRemoteInterface, itemId1, true, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    setItemValue( coreRemoteInterface, itemId2, true, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    scenesManagerInterface.setTestClock( runDate.toSeconds() );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, false, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    setItemValue( coreRemoteInterface, itemId2, false, __LINE__ );
    scenesManagerInterface.setTestClock( runDate.incrementDay().toSeconds() );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    setItemValue( coreRemoteInterface, itemId2, true, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, true, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, false, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    setItemValue( coreRemoteInterface, itemId2, false, __LINE__ );
    setItemValue( coreRemoteInterface, itemId3, true, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    setItemValue( coreRemoteInterface, itemId4, true, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    setItemValue( coreRemoteInterface, itemId3, false, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    setItemValue( coreRemoteInterface, itemId4, false, __LINE__ );

    removeDevice( coreRemoteInterface, deviceId, 4, 0, __LINE__ );
}

TEST_F( SceneManagerTests, XorOperatorErrors ) {  // 1.34.3
    auto xorOperator = replaceTokensInString( xorOperatorWhenBlock, { { "__BLOCKS__", "" } } );
    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "xorOperatorError" },
                                    { "__WHEN_BLOCKS__", std::move( xorOperator ) },
                                    { "__THEN_BLOCKS__", "" } } );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.block.when.wrong" );
    EXPECT_EQ( json_get_str( error, "message" ), "Scene is ill-formed. Can't parse when block" );

    xorOperator = replaceTokensInString( xorOperatorWhenBlock, { { "__BLOCKS__", "{}" } } );
    request = replaceTokens( createSceneRequest,
                             { { "__NAME__", "xorOperatorError" },
                               { "__WHEN_BLOCKS__", std::move( xorOperator ) },
                               { "__THEN_BLOCKS__", "" } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.block.when.wrong" );
    EXPECT_EQ( json_get_str( error, "message" ), "Scene is ill-formed. Can't parse when block" );
}

TEST_F( SceneManagerTests, XorOperator ) {  // 1.34.4
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 = addItem(
        coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId2 = addItem(
        coreRemoteInterface, deviceId, "testItem2", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId3 = addItem(
        coreRemoteInterface, deviceId, "testItem3", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId4 = addItem(
        coreRemoteInterface, deviceId, "testItem4", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );

    auto whenBlock1 = replaceTokensInString(
        isItemStateWhenBlock, { { "__ITEM_ID__", itemId1 }, { "__TYPE__", "bool" }, { "__VALUE__", "true" } } );
    whenBlock1.append( "," ).append( replaceTokensInString(
        isItemStateWhenBlock, { { "__ITEM_ID__", itemId2 }, { "__TYPE__", "bool" }, { "__VALUE__", "true" } } ) );
    whenBlock1.append( ", " ).append( replaceTokensInString(
        isItemStateWhenBlock, { { "__ITEM_ID__", itemId3 }, { "__TYPE__", "bool" }, { "__VALUE__", "true" } } ) );
    whenBlock1.append( "," ).append( replaceTokensInString(
        isItemStateWhenBlock, { { "__ITEM_ID__", itemId4 }, { "__TYPE__", "bool" }, { "__VALUE__", "true" } } ) );

    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    auto xorOperator = replaceTokensInString( xorOperatorWhenBlock, { { "__BLOCKS__", std::move( whenBlock1 ) } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "XorOperator" }, { "__WHEN_BLOCKS__", std::move( xorOperator ) }, { "__THEN_BLOCKS__", "" } } );
    SceneID sceneId = createScene( request );
    const auto shortWait = std::chrono::seconds( 2 );

    // xor(1 0 0 0) -> 1
    setItemValue( coreRemoteInterface, itemId1, true, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    // xor ( 1 1 0 0 ) -> 0
    setItemValue( coreRemoteInterface, itemId2, true, __LINE__ );
    checkNoBroadcastReceived( NMA_TEST_MAKE_MESSAGE( "Xor Operator should be false" ), __LINE__, shortWait );

    // xor ( 1 1 1 0 ) -> 1
    setItemValue( coreRemoteInterface, itemId3, true, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    // xor ( 1 1 1 1 ) -> 0
    setItemValue( coreRemoteInterface, itemId4, true, __LINE__ );
    checkNoBroadcastReceived( NMA_TEST_MAKE_MESSAGE( "Xor(1,1,1,1) Operator should be false" ), __LINE__, shortWait );

    // xor ( 1 1 0 1 ) -> 1
    setItemValue( coreRemoteInterface, itemId3, false, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    // xor(0 0 0 0) -> 0
    setItemValue( coreRemoteInterface, itemId1, false, __LINE__ );
    checkNoBroadcastReceived( "scene: " + sceneId, __LINE__, std::chrono::seconds( 1 ) );
    setItemValue( coreRemoteInterface, itemId2, false, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    setItemValue( coreRemoteInterface, itemId4, false, __LINE__ );
    checkNoBroadcastReceived( NMA_TEST_MAKE_MESSAGE( "Xor(0,0,0,0) Operator should be false" ), __LINE__, shortWait );

    removeDevice( coreRemoteInterface, deviceId, 4, 0, __LINE__ );
}

TEST_F( SceneManagerTests, sendCloudAbstractCommand ) {  // 1.35

    auto goodCloudResponse = []( rpc::Message& rpcMessage ) {
        JsonObject& joAnswer = rpcMessage.getAnswer();
        JsonObject& joMessage = rpcMessage.getMessage();
        joAnswer[ "method" ] = json_get_str( joMessage, "method" );
        joAnswer[ "result" ] = JSON_EMPTY_OBJ;
        joAnswer[ "error" ] = nullptr;
    };

    auto badCloudResponse = []( rpc::Message& rpcMessage ) {
        JsonObject& joAnswer = rpcMessage.getAnswer();
        JsonObject& joMessage = rpcMessage.getMessage();
        joAnswer[ "method" ] = json_get_str( joMessage, "method" );
        joAnswer[ "result" ] = JSON_EMPTY_OBJ;
        joAnswer[ "error" ] = JSON( "code" << -32500 );
    };

    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "SendCloudAbstractCommand" },
                                    { "__WHEN_BLOCKS__", "" },
                                    { "__THEN_BLOCKS__", sendCloudAbstractCommandThenBlock } } );
    SceneID sceneId = createScene( request );

    nma().setCloudMethod( "cloud.controller_abstracts_set", goodCloudResponse );
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    nma().setCloudMethod( "cloud.controller_abstracts_set", badCloudResponse );
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "failed", __LINE__ );

    deleteScenes();

    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 = addItem(
        coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );

    auto setItemValueBlock = replaceTokensInString(
        setItemValueThenBlock, { { "__ITEM_ID__", itemId1 }, { "__TYPE__", "bool" }, { "__VALUE__", "true" } } );
    request = replaceTokens( createSceneRequest,
                             { { "__NAME__", "SendCloudAbstractCommand" },
                               { "__EXEC_POLICY__", "check_result" },
                               { "__WHEN_BLOCKS__", "" },
                               { "__THEN_BLOCKS__", sendCloudAbstractCommandThenBlock + "," + setItemValueBlock } } );
    sceneId = createScene( request );
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "failed", __LINE__ );

    auto checkLastBroadcast = [&sceneId, &request, this]( CoreDRemoteInterface::CoreDInterface::ItemID itemId,
                                                          bool itemValue,
                                                          const std::string& sceneStatus,
                                                          int codeLine ) {
        auto broadcast = waitForAnyOfBroadcasts( { "hub.scene.run.progress", "hub.item.updated" },
                                                 "line: " + std::to_string( __LINE__ ) );
        if( json_get_str( broadcast, "msg_subclass" ) == "hub.scene.run.progress" ) {
            testRunProgressBroadcast( broadcast, request, sceneId, sceneStatus, codeLine );
        }
        else {
            checkItemUpdateBroadcast( broadcast, itemId, itemValue, codeLine );
        }
    };

    nma().setCloudMethod( "cloud.controller_abstracts_set", goodCloudResponse );
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    checkLastBroadcast( itemId1, true, "finished", __LINE__ );
    checkLastBroadcast( itemId1, true, "finished", __LINE__ );

    setItemValue( coreRemoteInterface, itemId1, false, __LINE__ );

    nma().setCloudMethod( "cloud.controller_abstracts_set", badCloudResponse );
    auto sendCloudAbstractCommand =
        replaceTokensInString( sendCloudAbstractCommandThenBlock, { { "__EXEC_POLICY__", "check_result" } } );
    request = replaceTokens( createSceneRequest,
                             { { "__NAME__", "SendCloudAbstractCommand" },
                               { "__WHEN_BLOCKS__", "" },
                               { "__THEN_BLOCKS__", sendCloudAbstractCommand + "," + setItemValueBlock } } );
    sceneId = createScene( request );
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "failed", __LINE__ );

    request = replaceTokens( createSceneRequest,
                             { { "__NAME__", "SendCloudAbstractCommand" },
                               { "__WHEN_BLOCKS__", "" },
                               { "__THEN_BLOCKS__", sendCloudAbstractCommandThenBlock + "," + setItemValueBlock } } );
    sceneId = createScene( request );
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    checkLastBroadcast( itemId1, true, "partially_finished", __LINE__ );
    checkLastBroadcast( itemId1, true, "partially_finished", __LINE__ );

    setItemValue( coreRemoteInterface, itemId1, false, __LINE__ );

    request = replaceTokens( createSceneRequest,
                             { { "__NAME__", "SendCloudAbstractCommand" },
                               { "__EXEC_POLICY__", "ignore_result" },
                               { "__WHEN_BLOCKS__", "" },
                               { "__THEN_BLOCKS__", sendCloudAbstractCommandThenBlock + "," + setItemValueBlock } } );
    sceneId = createScene( request );
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    checkLastBroadcast( itemId1, true, "partially_finished", __LINE__ );
    checkLastBroadcast( itemId1, true, "partially_finished", __LINE__ );

    setItemValue( coreRemoteInterface, itemId1, false, __LINE__ );

    request = replaceTokens( createSceneRequest,
                             { { "__NAME__", "SendCloudAbstractCommand" },
                               { "__EXEC_POLICY__", "ignore_result" },
                               { "__WHEN_BLOCKS__", "" },
                               { "__THEN_BLOCKS__", setItemValueBlock + "," + sendCloudAbstractCommandThenBlock } } );
    sceneId = createScene( request );
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    checkLastBroadcast( itemId1, true, "partially_finished", __LINE__ );
    checkLastBroadcast( itemId1, true, "partially_finished", __LINE__ );

    setItemValue( coreRemoteInterface, itemId1, false, __LINE__ );

    request = replaceTokens( createSceneRequest,
                             { { "__NAME__", "SendCloudAbstractCommand" },
                               { "__EXEC_POLICY__", "ignore_result" },
                               { "__WHEN_BLOCKS__", "" },
                               { "__THEN_BLOCKS__", setItemValueBlock + "," + sendCloudAbstractCommand } } );
    sceneId = createScene( request );
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    checkLastBroadcast( itemId1, true, "failed", __LINE__ );
    checkLastBroadcast( itemId1, true, "failed", __LINE__ );

    nma().resetCloudMethod( "cloud.controller_abstracts_set" );
    removeDevice( coreRemoteInterface, deviceId, /*itemNumber*/ 1, 0, __LINE__ );
}

TEST_F( SceneManagerTests, isUserLockOperationCreationErrors ) {  // 1.36.1
    auto checkError = [this]( const JsonObject& request, int codeLine ) {
        std::string codeLineStr = getCodeLineString( codeLine );
        auto answer = nma().sendtoHubAndGetAnswer( request );
        auto error = answer[ "error" ];
        ASSERT_FALSE( error.empty() ) << codeLineStr;
        EXPECT_EQ( json_get_int( error, "code" ), -32500 ) << codeLineStr;
        EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.block.when.wrong" ) << codeLineStr;
        EXPECT_EQ( json_get_str( error, "message" ), "Scene is ill-formed. Can't parse when block" ) << codeLineStr;
    };
    auto isUserLockOperation = replaceTokensInString( isUserLockOperationWhenBlock,
                                                      { { "__ITEM_ID__", "itemId1" }, { "__USER_ID__", "Code 1" } } );
    const auto referenceRequest = replaceTokens( createSceneRequest,
                                                 { { "__NAME__", "IsUserLockOperationError" },
                                                   { "__WHEN_BLOCKS__", std::move( isUserLockOperation ) },
                                                   { "__THEN_BLOCKS__", "" } } );

    JsonObject request = referenceRequest;
    request[ "params" ][ "when" ][ 0 ][ "fields" ].erase( 0 );
    checkError( request, __LINE__ );

    request = referenceRequest;
    request[ "params" ][ "when" ][ 0 ][ "fields" ].erase( 1 );
    checkError( request, __LINE__ );

    request = referenceRequest;
    request[ "params" ][ "when" ][ 0 ].erase( "fields" );
    checkError( request, __LINE__ );
}

TEST_F( SceneManagerTests, isUserLockOperationExecution ) {  // 1.36.2
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "Touchscreen Deadbolt",
                               "doorlock",
                               "297_32770_1536",
                               "door_lock",
                               "",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 = addItem( coreRemoteInterface,
                            deviceId,
                            "user_lock_operation",
                            CoreD::Model::ValueType::Enum::UserLockOperation,
                            true,
                            true,
                            __LINE__ );

    auto isUserLockOperation =
        replaceTokensInString( isUserLockOperationWhenBlock, { { "__ITEM_ID__", itemId1 }, { "__USER_ID__", "2" } } );
    const auto request1 = replaceTokens( createSceneRequest,
                                         { { "__NAME__", "IsUserLockOperation1" },
                                           { "__WHEN_BLOCKS__", std::move( isUserLockOperation ) },
                                           { "__THEN_BLOCKS__", "" } } );
    isUserLockOperation =
        replaceTokensInString( isUserLockOperationWhenBlock, { { "__ITEM_ID__", itemId1 }, { "__USER_ID__", "3" } } );
    const auto request2 = replaceTokens( createSceneRequest,
                                         { { "__NAME__", "IsUserLockOperation2" },
                                           { "__WHEN_BLOCKS__", std::move( isUserLockOperation ) },
                                           { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId1 = createScene( request1 );
    SceneID sceneId2 = createScene( request2 );

    BroadcastCheckers broadcastCheckers;
    std::string checkCode;
    SceneID sceneId;
    JsonObject request;
    broadcastCheckers[ "hub.user.notification" ] = [this, &checkCode]( const JsonObject& broadcast, int codeLine ) {
        checkUserNotificationBroadcast( broadcast, "Touchscreen Deadbolt", "", checkCode, codeLine );
    };
    broadcastCheckers[ "hub.scene.run.progress" ] = [this, &request, &sceneId]( const JsonObject& broadcast,
                                                                                int codeLine ) {
        testRunProgressBroadcast( broadcast, request, sceneId, "started", codeLine );
    };

    JsonObject value = JSON( UserLockOperation::Fields::Action << UserLockOperation::Actions::Lock
                                                               << UserLockOperation::Fields::UserId << 1 );
    setItemValue( coreRemoteInterface, itemId1, value, __LINE__ );
    checkUserNotificationBroadcast( "Touchscreen Deadbolt", "", "Code 1", __LINE__ );

    value[ UserLockOperation::Fields::UserId ] = 2;
    setItemValue( coreRemoteInterface, itemId1, value, __LINE__ );
    checkCode = "Code 2";
    sceneId = sceneId1;
    request = request1;
    checkBroadcasts( broadcastCheckers, __LINE__ );
    testRunProgressBroadcast( request1, sceneId1, "finished", __LINE__ );

    value[ UserLockOperation::Fields::UserId ] = 3;
    setItemValue( coreRemoteInterface, itemId1, value, __LINE__ );
    checkCode = "Code 3";
    sceneId = sceneId2;
    request = request2;
    checkBroadcasts( broadcastCheckers, __LINE__ );
    testRunProgressBroadcast( request2, sceneId2, "finished", __LINE__ );

    value[ UserLockOperation::Fields::UserId ] = 4;
    setItemValue( coreRemoteInterface, itemId1, value, __LINE__ );
    checkUserNotificationBroadcast( "Touchscreen Deadbolt", "", "Code 4", __LINE__ );

    deleteScenes();

    value[ UserLockOperation::Fields::UserId ] = 2;
    setItemValue( coreRemoteInterface, itemId1, value, __LINE__ );
    checkUserNotificationBroadcast( "Touchscreen Deadbolt", "", "Code 2", __LINE__ );
    value[ UserLockOperation::Fields::UserId ] = 3;
    setItemValue( coreRemoteInterface, itemId1, value, __LINE__ );
    checkUserNotificationBroadcast( "Touchscreen Deadbolt", "", "Code 3", __LINE__ );

    removeDevice( coreRemoteInterface, deviceId, 1, 0, __LINE__ );
}

TEST_F( SceneManagerTests, isUserLockOperationExecutionOnUserCodeActionItem ) {  // 1.36.3
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "Touchscreen Deadbolt",
                               "doorlock",
                               "297_32770_1536",
                               "door_lock",
                               "",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 = addItem( coreRemoteInterface,
                            deviceId,
                            "user_code_action",
                            CoreD::Model::ValueType::Enum::UserCodeAction,
                            true,
                            true,
                            __LINE__ );

    auto isUserLockOperation = replaceTokensInString( isUserLockOperationWhenBlockWithUserCodeAction,
                                                      { { "__ITEM_ID__", itemId1 }, { "__USER_ID__", "2" } } );
    const auto request1 = replaceTokens( createSceneRequest,
                                         { { "__NAME__", "IsUserLockOperation1" },
                                           { "__WHEN_BLOCKS__", std::move( isUserLockOperation ) },
                                           { "__THEN_BLOCKS__", "" } } );
    isUserLockOperation = replaceTokensInString( isUserLockOperationWhenBlockWithUserCodeAction,
                                                 { { "__ITEM_ID__", itemId1 }, { "__USER_ID__", "3" } } );
    const auto request2 = replaceTokens( createSceneRequest,
                                         { { "__NAME__", "IsUserLockOperation2" },
                                           { "__WHEN_BLOCKS__", std::move( isUserLockOperation ) },
                                           { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId1 = createScene( request1 );
    SceneID sceneId2 = createScene( request2 );

    JsonObject value = JSON( UserCodeAction::Fields::Action << UserCodeAction::Actions::UserCodeEntered
                                                            << UserCodeAction::Fields::UserId << 1 );
    setItemValue( coreRemoteInterface, itemId1, value, __LINE__ );
    checkUserNotificationBroadcast( "Touchscreen Deadbolt", "", "Code 1", __LINE__ );

    value[ UserCodeAction::Fields::UserId ] = 2;
    setItemValue( coreRemoteInterface, itemId1, value, __LINE__ );
    checkBroadcasts( { [this, &sceneId1]( const Json& broadcast ) {
                          return isRunProgressBroadcast( broadcast, sceneId1, "started" );
                      },
                       [this, &sceneId1]( const Json& broadcast ) {
                           return isRunProgressBroadcast( broadcast, sceneId1, "finished" );
                       },
                       [this]( const Json& broadcast ) {
                           return isUserNotificationBroadcast( broadcast, "Touchscreen Deadbolt", "", "Code 2" );
                       } },
                     __LINE__ );

    value[ UserCodeAction::Fields::UserId ] = 3;
    setItemValue( coreRemoteInterface, itemId1, value, __LINE__ );
    checkBroadcasts( { [this, &sceneId2]( const Json& broadcast ) {
                          return isRunProgressBroadcast( broadcast, sceneId2, "started" );
                      },
                       [this, &sceneId2]( const Json& broadcast ) {
                           return isRunProgressBroadcast( broadcast, sceneId2, "finished" );
                       },
                       [this]( const Json& broadcast ) {
                           return isUserNotificationBroadcast( broadcast, "Touchscreen Deadbolt", "", "Code 3" );
                       } },
                     __LINE__ );

    value[ UserLockOperation::Fields::UserId ] = 4;
    setItemValue( coreRemoteInterface, itemId1, value, __LINE__ );
    checkUserNotificationBroadcast( "Touchscreen Deadbolt", "", "Code 4", __LINE__ );

    deleteScenes();

    value[ UserLockOperation::Fields::UserId ] = 2;
    setItemValue( coreRemoteInterface, itemId1, value, __LINE__ );
    checkUserNotificationBroadcast( "Touchscreen Deadbolt", "", "Code 2", __LINE__ );
    value[ UserLockOperation::Fields::UserId ] = 3;
    setItemValue( coreRemoteInterface, itemId1, value, __LINE__ );
    checkUserNotificationBroadcast( "Touchscreen Deadbolt", "", "Code 3", __LINE__ );

    removeDevice( coreRemoteInterface, deviceId, 1, 0, __LINE__ );
}

TEST_F( SceneManagerTests, hasAtLeastOneDictionaryValue ) {  // 1.37
    using namespace CoreD::Model;
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "Garage Door Opener",
                               "shutter.garage",
                               "297_32770_1536",
                               "garage_door",
                               "",
                               false /*batteryPowered*/,
                               __LINE__ );
    auto itemId1 = addItem( coreRemoteInterface,
                            deviceId,
                            "barrier_problem_sensors",
                            ValueType::makeDictionaryType( ValueType::Enum::Token ),
                            true,
                            true,
                            __LINE__ );
    // add dictionary element
    auto addDictionaryValue = replaceTokens(
        hubItemDictionaryValueAdd,
        { { "__ITEM_ID__", itemId1 }, { "__TYPE__", "token" }, { "__VALUE__", "\"low_battery\"" } } )[ "params" ];
    auto setDictionaryValue = replaceTokens( hubItemDictionaryValueSet,
                                             { { "__ITEM_ID__", itemId1 },
                                               { "__KEY__", "1" },
                                               { "__TYPE__", "token" },
                                               { "__VALUE__", "\"low_battery\"" } } )[ "params" ];
    KeyJsonList fields = {
        { "operation", "added" },
        { "element",
          JSON( "value"
                << "" ) },
        { "_id", itemId1 },
        { "deviceId", deviceId },
        { "deviceName", "Garage Door Opener" },
        { "deviceCategory", "garage_door" },
        { "deviceSubcategory", "" },
        { "roomName", "" },
        { "name", "barrier_problem_sensors" },
    };
    auto addItemDictionaryValue =
        [&coreRemoteInterface, &fields, &addDictionaryValue, this](
            const std::string& itemId, const std::string& key, const std::string& value, int codeLine ) {
            std::string codeLineStr = getCodeLineString( codeLine );
            addDictionaryValue[ "element" ][ "value" ] = value;
            EXPECT_NO_THROW( coreRemoteInterface.addItemDictionaryValue( addDictionaryValue ) ) << codeLineStr;
            fields[ 0 ].second = "added";
            fields[ 1 ].second = JSON( key << JSON( "value" << value ) );
            checkItemDictionaryUpdatedBroadcast( itemId, fields, codeLine );
        };
    auto setItemDictionaryValue =
        [&coreRemoteInterface, &fields, &setDictionaryValue, this](
            const std::string& itemId, const std::string& key, const std::string& value, int codeLine ) {
            std::string codeLineStr = getCodeLineString( codeLine );
            setDictionaryValue[ "key" ] = key;
            setDictionaryValue[ "element" ][ "value" ] = value;
            EXPECT_NO_THROW( coreRemoteInterface.setItemDictionaryValue( setDictionaryValue ) ) << codeLineStr;
            fields[ 0 ].second = "updated";
            fields[ 1 ].second = JSON( key << JSON( "value" << value ) );
            checkItemDictionaryUpdatedBroadcast( itemId, fields, codeLine );
        };

    auto hasAtLeastOneDictionaryValue = replaceTokensInString(
        hasAtLeastOneDictionaryValueWhenBlock, { { "__ITEM_ID__", itemId1 }, { "__TOKEN__", "low_battery" } } );
    const auto request1 = replaceTokens( createSceneRequest,
                                         { { "__NAME__", "hasAtLeastOneDictionaryValue" },
                                           { "__WHEN_BLOCKS__", std::move( hasAtLeastOneDictionaryValue ) },
                                           { "__THEN_BLOCKS__", "" } } );
    SceneID sceneId1 = createScene( request1 );
    addItemDictionaryValue( itemId1, "1", "low_battery", __LINE__ );
    testRunProgressBroadcast( request1, sceneId1, "started", __LINE__ );
    testRunProgressBroadcast( request1, sceneId1, "finished", __LINE__ );

    addItemDictionaryValue( itemId1, "2", "not_detected", __LINE__ );
    addItemDictionaryValue( itemId1, "3", "low_battery", __LINE__ );
    testRunProgressBroadcast( request1, sceneId1, "started", __LINE__ );
    testRunProgressBroadcast( request1, sceneId1, "finished", __LINE__ );

    addItemDictionaryValue( itemId1, "4", "not_detected", __LINE__ );
    setItemDictionaryValue( itemId1, "2", "low_battery", __LINE__ );
    testRunProgressBroadcast( request1, sceneId1, "started", __LINE__ );
    testRunProgressBroadcast( request1, sceneId1, "finished", __LINE__ );

    setItemDictionaryValue( itemId1, "4", "low_battery", __LINE__ );
    testRunProgressBroadcast( request1, sceneId1, "started", __LINE__ );
    testRunProgressBroadcast( request1, sceneId1, "finished", __LINE__ );

    removeDevice( coreRemoteInterface, deviceId, 1, 0, __LINE__ );  // hard-coded number of items
}

TEST_F( SceneManagerTests, switchHouseMode ) {  // 1.38
    auto switchHouseMode =
        replaceTokensInString( switchHouseModeThenBlock, { { "__HOUSE_MODE_ID__", "houseModeId" } } );
    auto request1 = replaceTokens( createSceneRequest,
                                   { { "__NAME__", "switchHouseMode" },
                                     { "__WHEN_BLOCKS__", "" },
                                     { "__THEN_BLOCKS__", std::move( switchHouseMode ) } } );
    auto answer = nma().sendtoHubAndGetAnswer( request1 );
    auto error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32602 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.house_mode.not.exist" );
    EXPECT_EQ( json_get_str( error, "message" ), "The house mode with this id does not exist" );

    using namespace CoreD::Model;
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    coreRemoteInterface.setSwitchDelay( 0 );

    switchHouseMode = replaceTokensInString( switchHouseModeThenBlock, { { "__HOUSE_MODE_ID__", "1" } } );
    request1 = replaceTokens( createSceneRequest,
                              { { "__NAME__", "switchHouseMode" },
                                { "__WHEN_BLOCKS__", "" },
                                { "__THEN_BLOCKS__", std::move( switchHouseMode ) } } );
    switchHouseMode = replaceTokensInString( switchHouseModeThenBlock, { { "__HOUSE_MODE_ID__", "2" } } );
    auto request2 = replaceTokens( createSceneRequest,
                                   { { "__NAME__", "switchHouseMode" },
                                     { "__WHEN_BLOCKS__", "" },
                                     { "__THEN_BLOCKS__", std::move( switchHouseMode ) } } );
    SceneID sceneId1 = createScene( request1 );
    SceneID sceneId2 = createScene( request2 );

    runSceneManually( sceneId2, __LINE__ );

    checkBroadcasts(
        { [this]( const Json& broadcast ) { return isHouseModeSwitchedBroadcast( broadcast, "2", "begin" ); },
          [this]( const Json& broadcast ) { return isHouseModeSwitchedBroadcast( broadcast, "2", "done" ); },
          [this, &sceneId2, &request2]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, request2, sceneId2, "started" );
          },
          [this, &sceneId2, &request2]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, request2, sceneId2, "finished" );
          } },
        __LINE__ );

    runSceneManually( sceneId1, __LINE__ );

    checkBroadcasts(
        { [this]( const Json& broadcast ) { return isHouseModeSwitchedBroadcast( broadcast, "1", "begin" ); },
          [this]( const Json& broadcast ) { return isHouseModeSwitchedBroadcast( broadcast, "1", "done" ); },
          [this, &sceneId1, &request1]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, request1, sceneId1, "started" );
          },
          [this, &sceneId1, &request1]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, request1, sceneId1, "finished" );
          } },
        __LINE__ );
}

TEST_F( SceneManagerTests, modifyScenesSimpleScene ) {  // 1.39.1
    std::string options = R"(
    [
        {
            "value":"no_changes",
            "label":{
                "lang_tag":"stop_detecting",
                "text":"Stop detecting"
            }
        },
        {
            "value":"rapid_temperature_rise",
            "label":{
                "lang_tag":"rapid_temperature_rise",
                "text":"Rapid temperature rise"
            }
        },
        {
            "value":"rapid_temperature_fail",
            "label":{
                "lang_tag":"rapid_temperature_fail",
                "text":"Rapid temperature fail"
            }
        }
    ]
    )";
    std::string expectedOptions = R"(
    [
        {
            "value":"no_changes",
            "label":{
                "lang_tag":"stop_detecting",
                "text":"Stop detecting"
            }
        },
        {
            "value":"rapid_temperature_rise",
            "label":{
                "lang_tag":"rapid_temperature_rise",
                "text":"Rapid temperature rise"
            }
        },
        {
            "value":"rapid_temperature_fall",
            "label":{
                "lang_tag":"rapid_temperature_fail",
                "text":"Rapid temperature fall"
            }
        }
    ]
    )";
    auto isItemStateOptions = replaceTokensInString(
        isItemStateOptionsWhenBlock,
        { { "__ITEM_ID__", "2" }, { "__OPTIONS__", options }, { "__VALUE__", "\"rapid_temperature_fail\"" } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isItemStateOptions" },
          { "__WHEN_BLOCKS__", isItemStateOptions + "," + isItemStateOptions + "," + isItemStateOptions },
          { "__THEN_BLOCKS__", "" } } );
    request[ "params" ][ "when" ][ 0 ][ "blockId" ] = "123";
    request[ "params" ][ "when" ][ 2 ][ "blockId" ] = "123";
    SceneID sceneId = createScene( request );

    JsonArray commands = R"(
    [
        {
            "command": "replace_token",
            "to_replace": "rapid_temperature_fail",
            "replace_by": "rapid_temperature_fall"
        },
        {
            "command": "replace_option_label",
            "language_tag": "rapid_temperature_fail",
            "replace_by_text": "Rapid temperature fall"
        }
    ]
    )"_json;

    ScenesManager::RemoteInterface sceneManagerRemoteInterface( nma() );
    sceneManagerRemoteInterface.modifyScenes( commands, "123" );

    auto expectedWhenBlock = replaceTokens( isItemStateOptionsWhenBlock,
                                            { { "__BLOCK_ID__", "123" },
                                              { "__ITEM_ID__", "2" },
                                              { "__OPTIONS__", expectedOptions },
                                              { "__VALUE__", "\"rapid_temperature_fall\"" } } );
    auto scene = sceneManagerRemoteInterface.getScene( sceneId );
    removeBlockIds( scene[ "when" ] );
    EXPECT_EQ( scene[ "when" ][ 0 ].dump(), expectedWhenBlock.dump() );
    EXPECT_EQ( scene[ "when" ][ 1 ].dump(), Json::parse( isItemStateOptions ).dump() );
    EXPECT_EQ( scene[ "when" ][ 2 ].dump(), expectedWhenBlock.dump() );
}

TEST_F( SceneManagerTests, modifyScenesNestedScene ) {  // 1.39.2
    std::string options1 = R"(
    [
        {
            "value":"not_sterted",
            "label":{
                "lang_tag":"not_started",
                "text":"Not started"
            }
        },
        {
            "value":"performing_progress",
            "label":{
                "lang_tag":"in_progress",
                "text":"In progress"
            }
        },
        {
            "value":"process_failed",
            "label":{
                "lang_tag":"failed",
                "text":"Failed"
            }
        }
    ]
    )";
    std::string options2 = R"(
    [
        {
            "value":"made_enabled",
            "label":{
                "lang_tag":"ui7_enabled",
                "text":"Disabled"
            }
        },
        {
            "value":"made_disabled",
            "label":{
                "lang_tag":"ui7_disabled",
                "text":"Enabled"
            }
        }
    ]
    )";
    std::string expectedOptions1 = R"(
    [
        {
            "value":"not_started",
            "label":{
                "lang_tag":"not_started",
                "text":"Not started"
            }
        },
        {
            "value":"performing_process",
            "label":{
                "lang_tag":"in_progress",
                "text":"In progress"
            }
        },
        {
            "value":"process_completed",
            "label":{
                "lang_tag":"failed",
                "text":"Completed"
            }
        }
    ]
    )";
    std::string expectedOptions2 = R"(
    [
        {
            "value":"mode_enabled",
            "label":{
                "lang_tag":"ui7_enabled",
                "text":"Enabled"
            }
        },
        {
            "value":"mode_disabled",
            "label":{
                "lang_tag":"ui7_disabled",
                "text":"Disabled"
            }
        }
    ]
    )";
    auto isItemStateOptions1 = replaceTokensInString( isItemStateOptionsWhenBlock,
                                                      { { "__BLOCK_ID__", "123" },
                                                        { "__ITEM_ID__", "2" },
                                                        { "__OPTIONS__", options1 },
                                                        { "__VALUE__", "\"not_sterted\"" } } );
    auto isItemStateOptions2 = replaceTokensInString( isItemStateOptionsWhenBlock,
                                                      { { "__BLOCK_ID__", "123" },
                                                        { "__ITEM_ID__", "3" },
                                                        { "__OPTIONS__", options2 },
                                                        { "__VALUE__", "\"made_enabled\"" } } );
    auto isItemStateOptions3 = replaceTokensInString(
        isItemStateOptionsWhenBlock,
        { { "__ITEM_ID__", "4" }, { "__OPTIONS__", options2 }, { "__VALUE__", "\"made_enabled\"" } } );
    auto notOperator1 = replaceTokensInString( notOperatorWhenBlock, { { "__BLOCK__", isItemStateOptions1 } } );
    auto notOperator2 = replaceTokensInString( notOperatorWhenBlock, { { "__BLOCK__", isItemStateOptions2 } } );
    auto andOperator = replaceTokensInString( andOperatorWhenBlock,
                                              { { "__BLOCKS__",
                                                  notOperator1 + "," + notOperator2 + "," + isItemStateOptions1 + "," +
                                                      isItemStateOptions2 + "," + isItemStateOptions3 } } );
    auto orOperator = replaceTokensInString( orOperatorWhenBlock,
                                             { { "__BLOCKS__",
                                                 notOperator1 + "," + notOperator2 + "," + isItemStateOptions1 + "," +
                                                     isItemStateOptions2 + "," + isItemStateOptions3 } } );
    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "isItemStateOptionsNested" },
                                    { "__WHEN_BLOCKS__", andOperator + "," + orOperator },
                                    { "__THEN_BLOCKS__", "" } } );
    SceneID sceneId = createScene( request );

    JsonArray commands = R"(
    [
        {
            "command": "replace_token",
            "to_replace": "not_sterted",
            "replace_by": "not_started"
        },
        {
            "command": "replace_token",
            "to_replace": "performing_progress",
            "replace_by": "performing_process"
        },
        {
            "command": "replace_token",
            "to_replace": "process_failed",
            "replace_by": "process_completed"
        },
        {
            "command": "replace_token",
            "to_replace": "made_enabled",
            "replace_by": "mode_enabled"
        },
        {
            "command": "replace_token",
            "to_replace": "made_disabled",
            "replace_by": "mode_disabled"
        },
        {
            "command": "replace_option_label",
            "language_tag": "not_sterted",
            "replace_by_text": "Not started"
        },
        {
            "command": "replace_option_label",
            "language_tag": "failed",
            "replace_by_text": "Completed"
        },
        {
            "command": "replace_option_label",
            "language_tag": "ui7_enabled",
            "replace_by_text": "Enabled"
        },
        {
            "command": "replace_option_label",
            "language_tag": "ui7_disabled",
            "replace_by_text": "Disabled"
        }
    ]
    )"_json;

    ScenesManager::RemoteInterface sceneManagerRemoteInterface( nma() );
    sceneManagerRemoteInterface.modifyScenes( commands, JSON_ARRAY_( "123" ) );

    auto expectedWhenBlock1 = replaceTokens( isItemStateOptionsWhenBlock,
                                             { { "__BLOCK_ID__", "123" },
                                               { "__ITEM_ID__", "2" },
                                               { "__OPTIONS__", expectedOptions1 },
                                               { "__VALUE__", "\"not_started\"" } } );
    auto expectedWhenBlock2 = replaceTokens( isItemStateOptionsWhenBlock,
                                             { { "__BLOCK_ID__", "123" },
                                               { "__ITEM_ID__", "3" },
                                               { "__OPTIONS__", expectedOptions2 },
                                               { "__VALUE__", "\"mode_enabled\"" } } );
    auto expectedWhenBlock3 = Json::parse( std::move( isItemStateOptions3 ) );
    auto scene = sceneManagerRemoteInterface.getScene( sceneId );
    removeBlockIds( scene[ "when" ] );
    EXPECT_EQ( scene[ "when" ][ 0 ][ "fields" ][ 0 ][ "value" ][ 0 ][ "fields" ][ 0 ][ "value" ].dump(),
               expectedWhenBlock1.dump() );
    EXPECT_EQ( scene[ "when" ][ 0 ][ "fields" ][ 0 ][ "value" ][ 1 ][ "fields" ][ 0 ][ "value" ].dump(),
               expectedWhenBlock2.dump() );
    EXPECT_EQ( scene[ "when" ][ 0 ][ "fields" ][ 0 ][ "value" ][ 2 ].dump(), expectedWhenBlock1.dump() );
    EXPECT_EQ( scene[ "when" ][ 0 ][ "fields" ][ 0 ][ "value" ][ 3 ].dump(), expectedWhenBlock2.dump() );
    EXPECT_EQ( scene[ "when" ][ 0 ][ "fields" ][ 0 ][ "value" ][ 4 ].dump(), expectedWhenBlock3.dump() );
    EXPECT_EQ( scene[ "when" ][ 1 ][ "fields" ][ 0 ][ "value" ][ 0 ][ "fields" ][ 0 ][ "value" ].dump(),
               expectedWhenBlock1.dump() );
    EXPECT_EQ( scene[ "when" ][ 1 ][ "fields" ][ 0 ][ "value" ][ 1 ][ "fields" ][ 0 ][ "value" ].dump(),
               expectedWhenBlock2.dump() );
    EXPECT_EQ( scene[ "when" ][ 1 ][ "fields" ][ 0 ][ "value" ][ 2 ].dump(), expectedWhenBlock1.dump() );
    EXPECT_EQ( scene[ "when" ][ 1 ][ "fields" ][ 0 ][ "value" ][ 3 ].dump(), expectedWhenBlock2.dump() );
    EXPECT_EQ( scene[ "when" ][ 1 ][ "fields" ][ 0 ][ "value" ][ 4 ].dump(), expectedWhenBlock3.dump() );
}

TEST_F( SceneManagerTests, syncNotificationFlag ) {
    using HubPlatform::CoreD::Model::Resources::System::SyncNotification;

    const auto defaultScene = createEmptyScene;
    const std::string sceneName = "SceneName";
    std::string sceneId;

    auto sendScene = [this]( const Json& scene, int codeLine ) {
        auto answer = nma().sendtoHubAndGetAnswer( scene );
        const auto& error = answer[ "error" ];
        ASSERT_TRUE( error.empty() ) << error.dump( 4 ) << getCodeLineString( codeLine );
    };

    auto validateSceneId = [&sceneId]( const Json& scene ) -> bool {
        std::string sceneBroadcastId = json_get_str( scene, "_id" );
        return ( !sceneBroadcastId.empty() && sceneBroadcastId == sceneId );
    };

    {  // create scene
        auto createSceneJson = defaultScene;
        createSceneJson[ "params" ][ "name" ] = sceneName;
        sendScene( createSceneJson, __LINE__ );
        JsonObject createSceneBroadcast = waitForBroadcast( "hub.scene.added", "createScene" );
        const JsonObject& createSceneResult = createSceneBroadcast[ "result" ];
        EXPECT_TRUE( json_get_bool( createSceneResult, SyncNotification, false ) );

        sceneId = json_get_str( createSceneResult, "_id" );
    }

    {  // change scene with same name
        auto duplicateSceneJson = Tests::editScene;
        duplicateSceneJson[ "params" ][ "_id" ] = sceneId;
        duplicateSceneJson[ "params" ][ "eo" ] = defaultScene[ "params" ];
        duplicateSceneJson[ "params" ][ "eo" ][ "name" ] = sceneName;
        sendScene( duplicateSceneJson, __LINE__ );
        JsonObject duplicateSceneBroadcast = waitForBroadcast( "hub.scene.changed", "editScene" );
        const JsonObject& duplicateSceneResult = duplicateSceneBroadcast[ "result" ];
        EXPECT_TRUE( validateSceneId( duplicateSceneResult ) );
        EXPECT_FALSE( json_get_bool( duplicateSceneResult, SyncNotification, false ) );
    }

    {  // change scene with new name
        auto newNameSceneJson = Tests::editScene;
        newNameSceneJson[ "params" ][ "_id" ] = sceneId;
        newNameSceneJson[ "params" ][ "eo" ] = defaultScene[ "params" ];
        newNameSceneJson[ "params" ][ "eo" ][ "name" ] = sceneName;
        sendScene( newNameSceneJson, __LINE__ );
        JsonObject newNameSceneBroadcast = waitForBroadcast( "hub.scene.changed", std::string( "editScene" ) );
        const JsonObject& newNameSceneResult = newNameSceneBroadcast[ "result" ];
        EXPECT_TRUE( validateSceneId( newNameSceneResult ) );
        EXPECT_FALSE( json_get_bool( newNameSceneResult, SyncNotification, true ) );
    }

    {  // delete scene
        auto deleteSceneJson = HubPlatform::Tests::deleteScene;
        deleteSceneJson[ "params" ][ "_id" ] = sceneId;
        deleteSceneJson[ "params" ][ "name" ] = sceneName;
        sendScene( deleteSceneJson, __LINE__ );
        JsonObject deleteSceneBroadcast = waitForBroadcast( "hub.scene.deleted", "deleteScene: " + sceneId );
        const JsonObject& deleteSceneResult = deleteSceneBroadcast[ "result" ];
        EXPECT_TRUE( validateSceneId( deleteSceneResult ) );
        EXPECT_TRUE( json_get_bool( deleteSceneResult, SyncNotification, false ) );
    }
}

TEST_F( SceneManagerTests, iconSetRequest ) {  // 1.40
    std::string overMaxId;
    std::string overMaxIcon;

    const int SceneIdLengthLimit = 64;
    const int SceneIconLengthLimit = 128;

    for( int i = 0; i < SceneIdLengthLimit + 1; ++i ) {
        overMaxId.push_back( 'a' );
    }

    for( int i = 0; i < SceneIconLengthLimit + 1; ++i ) {
        overMaxIcon.push_back( 'a' );
    }
    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "SceneWithIcon" },
                                    { "__ICON__", overMaxIcon },
                                    { "__WHEN_BLOCKS__", "" },
                                    { "__THEN_BLOCKS__", "" } } );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32602 );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.range.invalid.icon" );
    EXPECT_EQ( json_get_str( error, "message" ), "Value is not in valid range" );

    request = replaceTokens( createSceneRequest,
                             { { "__NAME__", "SceneWithIcon" },
                               { "__ICON__", "icon1" },
                               { "__WHEN_BLOCKS__", "" },
                               { "__THEN_BLOCKS__", "" } } );
    auto scene = request[ "params" ];
    SceneID sceneId = createScene( request );
    request = replaceTokens( getScene, { { "__SCENE_ID__", sceneId } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_EQ( answer[ "result" ][ "icon" ], "icon1" );

    request = replaceTokens( setIconRequest, { { "__SCENE_ID__", "sceneId123" }, { "__ICON__", "icon2" } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.not.exist" );
    EXPECT_EQ( json_get_str( error, "message" ), "The scene with this id does not exist" );

    auto wrongRequest = request;
    wrongRequest[ "params" ].erase( "_id" );
    answer = nma().sendtoHubAndGetAnswer( wrongRequest );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32600 );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.notfound._id" );
    EXPECT_EQ( json_get_str( error, "message" ), "Bad request, _id does not exist" );

    wrongRequest = request;
    wrongRequest[ "params" ].erase( "icon" );
    answer = nma().sendtoHubAndGetAnswer( wrongRequest );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32600 );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.notfound.icon" );
    EXPECT_EQ( json_get_str( error, "message" ), "Bad request, icon does not exist" );

    request = replaceTokens( setIconRequest, { { "__SCENE_ID__", sceneId }, { "__ICON__", "icon2" } } );
    wrongRequest = request;
    wrongRequest[ "params" ][ "_id" ] = std::move( overMaxId );
    answer = nma().sendtoHubAndGetAnswer( wrongRequest );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32602 );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.range.invalid._id" );
    EXPECT_EQ( json_get_str( error, "message" ), "Value is not in valid range" );

    wrongRequest = request;
    wrongRequest[ "params" ][ "icon" ] = std::move( overMaxIcon );
    answer = nma().sendtoHubAndGetAnswer( wrongRequest );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32602 );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.range.invalid.icon" );
    EXPECT_EQ( json_get_str( error, "message" ), "Value is not in valid range" );

    request = replaceTokens( getScene, { { "__SCENE_ID__", sceneId } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_EQ( answer[ "result" ][ "icon" ], "icon1" );

    request = replaceTokens( setIconRequest, { { "__SCENE_ID__", sceneId }, { "__ICON__", "icon2" } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() );

    request = replaceTokens( getScene, { { "__SCENE_ID__", sceneId } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_EQ( answer[ "result" ][ "icon" ], "icon2" );

    scene[ "icon" ] = "icon2";
    checkChangedBroadcast( sceneId, scene, __LINE__ );
}

TEST_F( SceneManagerTests, actionBlockTest ) {  // 1.41
    Tests::HttpServer server;
    uint16_t freeEmptyPort = 0;

    {
        Process::SharedMutex processMutex( "uniquePortMutex" );
        std::unique_lock< Process::SharedMutex > lock( processMutex );

        getPort() = system_utils::getUniqueFreeLocalPort();
        server.setPort( getPort() );
        server.setSecurePort( system_utils::getUniqueFreeLocalPort() );
        server.start();

        freeEmptyPort = system_utils::getUniqueFreeLocalPort();

        lock.unlock();
    }
    auto actionBlock = replaceTokensInString( sendHttpRequestThenBlock,
                                              { { "__URL__", makeUrl( "/reflect" ) },
                                                { "__USER__", "" },
                                                { "__PASSWORD__", "" },
                                                { "__REQUEST_TYPE__", "GET" },
                                                { "__CONTENT_TYPE__", "text/plain" },
                                                { "__CONTENT__", "" },
                                                { "__HEADERS__", "" },
                                                { "__SKIP_SECURITY__", "true" } } );
    auto request = replaceTokens( testActionBlock, { { "__BLOCK__", actionBlock } } );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() );
    auto result = answer[ "result" ];
    EXPECT_EQ( json_get_int( result, "httpAnswerCode" ), 200 );

    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "HttpRequest" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", std::move( actionBlock ) } } );
    SceneID sceneId = createScene( request );
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    // Make a http request to free empty port of the current host, emulate http status "503 Service Unavailable" instead
    // of DNS error.
    std::string hostNameToFail = std::string{ "localhost:" }.append( std::to_string( freeEmptyPort ) );
    LOGGER_INFO( "Query not handled by the HTTP server localhost:port with sendHttpRequest ... " );
    actionBlock = replaceTokensInString( sendHttpRequestThenBlock,
                                         { { "__URL__", makeUrl( "/reflect", hostNameToFail.c_str(), "http" ) },
                                           { "__USER__", "" },
                                           { "__PASSWORD__", "" },
                                           { "__REQUEST_TYPE__", "GET" },
                                           { "__CONTENT_TYPE__", "text/plain" },
                                           { "__CONTENT__", "" },
                                           { "__HEADERS__", "" },
                                           { "__SKIP_SECURITY__", "true" } } );
    request = replaceTokens( testActionBlock, { { "__BLOCK__", actionBlock } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() );
    result = answer[ "result" ];
    const int ServiceUnavailableError = 503;
    EXPECT_EQ( json_get_int( result, "httpAnswerCode" ), ServiceUnavailableError );

    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "HttpRequest" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", std::move( actionBlock ) } } );
    sceneId = createScene( request );
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "failed", __LINE__ );
}

TEST_F( SceneManagerTests, nameSetRequest ) {  // 1.42
    std::string overMaxId;
    std::string overMaxName;

    const int SceneIdLengthLimit = 64;
    const int SceneNameLengthLimit = 128;

    for( int i = 0; i < SceneIdLengthLimit + 1; ++i ) {
        overMaxId.push_back( 'a' );
    }
    for( int i = 0; i < SceneNameLengthLimit + 1; ++i ) {
        overMaxName.push_back( 'a' );
    }
    auto request = replaceTokens(
        createSceneRequest, { { "__NAME__", overMaxName }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", "" } } );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32602 );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.range.invalid.name" );
    EXPECT_EQ( json_get_str( error, "message" ), "Value is not in valid range" );

    request = replaceTokens(
        createSceneRequest, { { "__NAME__", "SceneWithName" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", "" } } );
    auto scene = request[ "params" ];
    SceneID sceneId = createScene( request );

    request = replaceTokens( setNameRequest, { { "__SCENE_ID__", "sceneId123" }, { "__NAME__", "New Name" } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.not.exist" );
    EXPECT_EQ( json_get_str( error, "message" ), "The scene with this id does not exist" );

    auto wrongRequest = request;
    wrongRequest[ "params" ].erase( "_id" );
    answer = nma().sendtoHubAndGetAnswer( wrongRequest );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32600 );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.notfound._id" );
    EXPECT_EQ( json_get_str( error, "message" ), "Bad request, _id does not exist" );

    wrongRequest = request;
    wrongRequest[ "params" ].erase( "name" );
    answer = nma().sendtoHubAndGetAnswer( wrongRequest );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32600 );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.notfound.name" );
    EXPECT_EQ( json_get_str( error, "message" ), "Bad request, name does not exist" );

    request = replaceTokens( setNameRequest, { { "__SCENE_ID__", sceneId }, { "__NAME__", "New Name" } } );
    wrongRequest = request;
    wrongRequest[ "params" ][ "_id" ] = std::move( overMaxId );
    answer = nma().sendtoHubAndGetAnswer( wrongRequest );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32602 );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.range.invalid._id" );
    EXPECT_EQ( json_get_str( error, "message" ), "Value is not in valid range" );

    wrongRequest = request;
    wrongRequest[ "params" ][ "name" ] = std::move( overMaxName );
    answer = nma().sendtoHubAndGetAnswer( wrongRequest );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32602 );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.range.invalid.name" );
    EXPECT_EQ( json_get_str( error, "message" ), "Value is not in valid range" );

    request = replaceTokens( getScene, { { "__SCENE_ID__", sceneId } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_EQ( answer[ "result" ][ "name" ], "SceneWithName" );

    request = replaceTokens( setNameRequest, { { "__SCENE_ID__", sceneId }, { "__NAME__", "New Name" } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() );

    request = replaceTokens( getScene, { { "__SCENE_ID__", sceneId } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_EQ( answer[ "result" ][ "name" ], "New Name" );

    scene[ "name" ] = "New Name";
    checkChangedBroadcast( sceneId, scene, __LINE__ );
}

TEST_F( SceneManagerTests, houseModesSetRequest ) {  // 1.43
    std::string overMaxId;
    for( int i = 0; i < 65; ++i ) {
        overMaxId.push_back( 'a' );
    }
    auto request = replaceTokens(
        createSceneRequest, { { "__NAME__", "SetHouseModes" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", "" } } );
    auto scene = request[ "params" ];
    SceneID sceneId = createScene( request );

    request = replaceTokens( setHouseModesRequest,
                             { { "__SCENE_ID__", "sceneId123" }, { "__HOUSE_MODES__", "[ \"1\", \"4\" ]" } } );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.not.exist" );
    EXPECT_EQ( json_get_str( error, "message" ), "The scene with this id does not exist" );

    auto wrongRequest = request;
    wrongRequest[ "params" ].erase( "_id" );
    answer = nma().sendtoHubAndGetAnswer( wrongRequest );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32600 );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.notfound._id" );
    EXPECT_EQ( json_get_str( error, "message" ), "Bad request, _id does not exist" );

    wrongRequest = request;
    wrongRequest[ "params" ].erase( "houseModes" );
    answer = nma().sendtoHubAndGetAnswer( wrongRequest );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32600 );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.notfound.houseModes" );
    EXPECT_EQ( json_get_str( error, "message" ), "Bad request, houseModes does not exist" );

    wrongRequest = request;
    wrongRequest[ "params" ][ "_id" ] = std::move( overMaxId );
    answer = nma().sendtoHubAndGetAnswer( wrongRequest );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32602 );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.range.invalid._id" );
    EXPECT_EQ( json_get_str( error, "message" ), "Value is not in valid range" );

    request = replaceTokens( getScene, { { "__SCENE_ID__", sceneId } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_EQ( answer[ "result" ][ "house_modes" ], nullptr );

    request = replaceTokens( setHouseModesRequest,
                             { { "__SCENE_ID__", sceneId }, { "__HOUSE_MODES__", "[ \"1\", \"4\" ]" } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() );

    request = replaceTokens( getScene, { { "__SCENE_ID__", sceneId } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_EQ( answer[ "result" ][ "house_modes" ], JSON_ARRAY_( "1", "4" ) );

    scene[ "house_modes" ] = JSON_ARRAY_( "1", "4" );
    checkChangedBroadcast( sceneId, scene, __LINE__ );
}

TEST_F( SceneManagerTests, runCustomScriptActionTest ) {  // 1.44
                                                          // add a custom script
    auto request = replaceTokens( addCustomScriptRequest,
                                  { { "__NAME__", "test script" },
                                    { "__CODE__",
                                      R"_( st = require \"storage\"; st.set_number( \"best_key\", 100500 ) )_" } } );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() );

    auto result = answer[ "result" ];
    const auto scriptId = json_get_str( result, "_id" );

    // add a scene associated with a custom script
    auto actionBlock = replaceTokensInString( runCustomScriptThenBlock, { { "__SCRIPT_ID__", scriptId } } );
    auto sceneCreateRequest = replaceTokens( createSceneRequest,
                                             { { "__NAME__", "Run custom script" },
                                               { "__WHEN_BLOCKS__", "" },
                                               { "__THEN_BLOCKS__", std::move( actionBlock ) } } );
    SceneID sceneId = createScene( sceneCreateRequest );

    // try to delete a custom script associated with a scene
    request = replaceTokens( deleteCustomScriptRequest, { { "__SCRIPT_ID__", scriptId } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "anonymous.plugin.acquired" );
    EXPECT_EQ( json_get_str( error, "message" ), "Anonymous plugin acquired!" );

    // check a runCustomScrip action block
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( sceneCreateRequest, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( sceneCreateRequest, sceneId, "finished", __LINE__ );

    const auto& dbClient = nma().getDBClient();
    const auto json = dbClient.findOne( getPluginDBName( scriptId ), JSON( db::ID_FIELD_NAME << "best_key" ), nullptr );
    ASSERT_FALSE( json.empty() );

    EXPECT_EQ( json_get_int( json, "value" ), 100500 );

    // check a scene changing operation doesn't lead to a plugin destroying
    actionBlock = replaceTokensInString( runCustomScriptThenBlock, { { "__SCRIPT_ID__", scriptId } } );
    request = replaceTokens( EFWR_2195_editSceneRequest,
                             { { "__SCENE_ID__", sceneId },
                               { "__NAME__", "Just another name" },
                               { "__THEN_BLOCKS__", std::move( actionBlock ) } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() );
    waitForBroadcast( "hub.scene.changed", getCodeLineString( __LINE__ ) );

    EXPECT_TRUE( dbClient.exist( getPluginDBName( scriptId ) ) );

    // add another scene associated with the same custom script
    actionBlock = replaceTokensInString( runCustomScriptThenBlock, { { "__SCRIPT_ID__", scriptId } } );
    sceneCreateRequest = replaceTokens( createSceneRequest,
                                        { { "__NAME__", "Run custom script 2" },
                                          { "__WHEN_BLOCKS__", "" },
                                          { "__THEN_BLOCKS__", std::move( actionBlock ) } } );
    SceneID anotherSceneId = createScene( sceneCreateRequest );

    // check plugin/custom script sharing between scenes
    deleteScene( sceneId );
    EXPECT_TRUE( dbClient.exist( getPluginDBName( scriptId ) ) );

    request = replaceTokens( deleteCustomScriptRequest, { { "__SCRIPT_ID__", scriptId } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "anonymous.plugin.acquired" );
    EXPECT_EQ( json_get_str( error, "message" ), "Anonymous plugin acquired!" );

    // check plugin/custom script removing after all scenes owning it were destroyed
    deleteScene( anotherSceneId );

    /* Anonymous plugins: requirement to maintain the lifetime of a plugin only for the time it has links to it is
     * removed. The script should exist and can be deleted only manually!
     *
     * The test only worked for one scene holding the script.
     */
    // In this case all scenes which acquired plugin were deleted and plugin should be removed successfully:
    request = replaceTokens( deleteCustomScriptRequest, { { "__SCRIPT_ID__", scriptId } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() );
}

TEST_F( SceneManagerTests, isHouseModeChangedTo ) {  // 1.46.1
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    coreRemoteInterface.setSwitchDelay( 0 );
    auto houseMode = coreRemoteInterface.getCurrentHouseModeId();
    if( houseMode != "1" ) {
        coreRemoteInterface.setHouseMode( "1" );
        testHouseModesSwitchedBroadcast( "1", "begin", __LINE__ );
        testHouseModesSwitchedBroadcast( "1", "done", __LINE__ );
    }

    auto isHouseModeChangedTo =
        replaceTokensInString( isHouseModeChangedToWhenBlock, { { "__HOUSE_MODES__", R"( "1", "2" )" } } );
    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "isHouseModeChangedTo" },
                                    { "__WHEN_BLOCKS__", isHouseModeChangedTo },
                                    { "__THEN_BLOCKS__", "" } } );
    auto sceneId = createScene( request );

    coreRemoteInterface.setHouseMode( "3" );
    testHouseModesSwitchedBroadcast( "3", "begin", __LINE__ );
    testHouseModesSwitchedBroadcast( "3", "done", __LINE__ );

    coreRemoteInterface.setHouseMode( "4" );
    testHouseModesSwitchedBroadcast( "4", "begin", __LINE__ );
    testHouseModesSwitchedBroadcast( "4", "done", __LINE__ );

    coreRemoteInterface.setHouseMode( "1" );
    checkBroadcasts(
        { [this]( const Json& broadcast ) { return isHouseModeSwitchedBroadcast( broadcast, "1", "begin" ); },
          [this]( const Json& broadcast ) { return isHouseModeSwitchedBroadcast( broadcast, "1", "done" ); },
          [this, &sceneId, &request]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, request, sceneId, "started" );
          },
          [this, &sceneId, &request]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, request, sceneId, "finished" );
          } },
        __LINE__ );

    coreRemoteInterface.setHouseMode( "2" );
    checkBroadcasts(
        { [this]( const Json& broadcast ) { return isHouseModeSwitchedBroadcast( broadcast, "2", "begin" ); },
          [this]( const Json& broadcast ) { return isHouseModeSwitchedBroadcast( broadcast, "2", "done" ); },
          [this, &sceneId, &request]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, request, sceneId, "started" );
          },
          [this, &sceneId, &request]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, request, sceneId, "finished" );
          } },
        __LINE__ );
}

TEST_F( SceneManagerTests, isHouseModeChangedToErrors ) {  // 1.46.2
    auto isHouseModeChangedTo =
        replaceTokensInString( isHouseModeChangedToWhenBlock, { { "__HOUSE_MODES__", "1, 2" } } );
    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "isHouseModeChangedTo" },
                                    { "__WHEN_BLOCKS__", isHouseModeChangedTo },
                                    { "__THEN_BLOCKS__", "" } } );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    EXPECT_FALSE( error.empty() ) << error.dump( 4 );
    EXPECT_EQ( json_get_int( error, "code" ), -32602 );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.wrong_field" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong field of object" );
    EXPECT_EQ( json_get_str( error, "reason" ), "mode is not string" );

    request[ "params" ][ "when" ][ 0 ][ "fields" ][ 0 ][ "value" ] = 1;
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    EXPECT_FALSE( error.empty() ) << error.dump( 4 );
    EXPECT_EQ( json_get_int( error, "code" ), -32602 );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.wrong_field" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong field of object" );
    EXPECT_EQ( json_get_str( error, "reason" ), "houseMode field is not array" );
}

TEST_F( SceneManagerTests, isHouseModeChangedFrom ) {  // 1.47.1
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    coreRemoteInterface.setSwitchDelay( 0 );
    auto houseMode = coreRemoteInterface.getCurrentHouseModeId();
    if( houseMode != "1" ) {
        coreRemoteInterface.setHouseMode( "1" );
        testHouseModesSwitchedBroadcast( "1", "begin", __LINE__ );
        testHouseModesSwitchedBroadcast( "1", "done", __LINE__ );
    }

    auto isHouseModeChangedFrom =
        replaceTokensInString( isHouseModeChangedFromWhenBlock, { { "__HOUSE_MODES__", R"( "3", "4" )" } } );
    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "isHouseModeChangedFrom" },
                                    { "__WHEN_BLOCKS__", isHouseModeChangedFrom },
                                    { "__THEN_BLOCKS__", "" } } );
    auto sceneId = createScene( request );

    coreRemoteInterface.setHouseMode( "2" );
    testHouseModesSwitchedBroadcast( "2", "begin", __LINE__ );
    testHouseModesSwitchedBroadcast( "2", "done", __LINE__ );

    coreRemoteInterface.setHouseMode( "3" );
    testHouseModesSwitchedBroadcast( "3", "begin", __LINE__ );
    testHouseModesSwitchedBroadcast( "3", "done", __LINE__ );

    coreRemoteInterface.setHouseMode( "4" );
    checkBroadcasts(
        { [this]( const Json& broadcast ) { return isHouseModeSwitchedBroadcast( broadcast, "4", "begin" ); },
          [this]( const Json& broadcast ) { return isHouseModeSwitchedBroadcast( broadcast, "4", "done" ); },
          [this, &sceneId, &request]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, request, sceneId, "started" );
          },
          [this, &sceneId, &request]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, request, sceneId, "finished" );
          } },
        __LINE__ );

    coreRemoteInterface.setHouseMode( "2" );
    checkBroadcasts(
        { [this]( const Json& broadcast ) { return isHouseModeSwitchedBroadcast( broadcast, "2", "begin" ); },
          [this]( const Json& broadcast ) { return isHouseModeSwitchedBroadcast( broadcast, "2", "done" ); },
          [this, &sceneId, &request]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, request, sceneId, "started" );
          },
          [this, &sceneId, &request]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, request, sceneId, "finished" );
          } },
        __LINE__ );
}

TEST_F( SceneManagerTests, isHouseModeChangedFromErrors ) {  // 1.47.2
    auto isHouseModeChangedFrom =
        replaceTokensInString( isHouseModeChangedFromWhenBlock, { { "__HOUSE_MODES__", "1, 2" } } );
    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "isHouseModeChangedFrom" },
                                    { "__WHEN_BLOCKS__", isHouseModeChangedFrom },
                                    { "__THEN_BLOCKS__", "" } } );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    EXPECT_FALSE( error.empty() ) << error.dump( 4 );
    EXPECT_EQ( json_get_int( error, "code" ), -32602 );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.wrong_field" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong field of object" );
    EXPECT_EQ( json_get_str( error, "reason" ), "mode is not string" );

    request[ "params" ][ "when" ][ 0 ][ "fields" ][ 0 ][ "value" ] = 1;
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    EXPECT_FALSE( error.empty() ) << error.dump( 4 );
    EXPECT_EQ( json_get_int( error, "code" ), -32602 );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.wrong_field" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong field of object" );
    EXPECT_EQ( json_get_str( error, "reason" ), "houseMode field is not array" );
}

TEST_F( SceneManagerTests, isNetworkMainState ) {  // 1.48.1
    auto isNetworkState = replaceTokensInString( isNetworkStateWhenBlock,
                                                 { { "__NETWORK_ID__", "interface1" }, { "__STATE__", "main" } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isNetworkState" }, { "__WHEN_BLOCKS__", isNetworkState }, { "__THEN_BLOCKS__", "" } } );
    auto sceneId = createScene( request );

    nma().sendBroadcast( NETWORK_BROADCAST_IP_ADDRESS_CHANGED,
                         JSON( "interfaceId"
                               << "interface1" ) );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    nma().sendBroadcast( NETWORK_BROADCAST_IP_ADDRESS_CHANGED,
                         JSON( "interfaceId"
                               << "interface2" ) );
    nma().sendBroadcast( NETWORK_BROADCAST_IP_ADDRESS_CHANGED,
                         JSON( "interfaceId"
                               << "interface1" ) );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
}

TEST_F( SceneManagerTests, isNetworkStateAnyMainInterface ) {  // 1.48.2
    auto isNetworkState =
        replaceTokensInString( isNetworkStateWhenBlock, { { "__NETWORK_ID__", "" }, { "__STATE__", "main" } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isNetworkState" }, { "__WHEN_BLOCKS__", isNetworkState }, { "__THEN_BLOCKS__", "" } } );
    auto sceneId = createScene( request );

    nma().sendBroadcast( NETWORK_BROADCAST_IP_ADDRESS_CHANGED,
                         JSON( "interfaceId"
                               << "interface1" ) );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    nma().sendBroadcast( NETWORK_BROADCAST_IP_ADDRESS_CHANGED,
                         JSON( "interfaceId"
                               << "interface2" ) );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
}

TEST_F( SceneManagerTests, isNetworkStateFailedConnection ) {  // 1.48.3
    auto isNetworkState = replaceTokensInString( isNetworkStateWhenBlock,
                                                 { { "__NETWORK_ID__", "interface1" }, { "__STATE__", "failed" } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isNetworkState" }, { "__WHEN_BLOCKS__", isNetworkState }, { "__THEN_BLOCKS__", "" } } );
    auto sceneId = createScene( request );

    sendNetworkChangedBroadcast( "interface1", true );
    sendNetworkChangedBroadcast( "interface1", false );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    sendNetworkChangedBroadcast( "interface1", true );
    sendNetworkChangedBroadcast( "interface1", false );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    sendNetworkChangedBroadcast( "interface1", true );
}

TEST_F( SceneManagerTests, isNetworkStateMainInterfaceFailed ) {  // 1.48.4
    nma().sendBroadcast( NETWORK_BROADCAST_IP_ADDRESS_CHANGED,
                         JSON( "interfaceId"
                               << "interface1" ) );

    auto isNetworkState =
        replaceTokensInString( isNetworkStateWhenBlock, { { "__NETWORK_ID__", "" }, { "__STATE__", "failed" } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isNetworkState" }, { "__WHEN_BLOCKS__", isNetworkState }, { "__THEN_BLOCKS__", "" } } );
    auto sceneId = createScene( request );

    sendNetworkChangedBroadcast( "interface2", true );
    sendNetworkChangedBroadcast( "interface2", false );

    sendNetworkChangedBroadcast( "interface1", true );
    sendNetworkChangedBroadcast( "interface1", false );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    sendNetworkChangedBroadcast( "interface1", true );
}

TEST_F( SceneManagerTests, isNetworkStateErrors ) {  // 1.48.5
    auto isNetworkState =
        replaceTokensInString( isNetworkStateWhenBlock, { { "__NETWORK_ID__", "" }, { "__STATE__", "" } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isNetworkState" }, { "__WHEN_BLOCKS__", isNetworkState }, { "__THEN_BLOCKS__", "" } } );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    EXPECT_FALSE( error.empty() ) << error.dump( 4 );
    EXPECT_EQ( json_get_int( error, "code" ), -32602 );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.wrong_field" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong field of object" );
    EXPECT_EQ( json_get_str( error, "reason" ), "wrong state field" );

    isNetworkState =
        replaceTokensInString( isNetworkStateWhenBlock, { { "__NETWORK_ID__", "" }, { "__STATE__", "wrong" } } );
    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isNetworkState" }, { "__WHEN_BLOCKS__", isNetworkState }, { "__THEN_BLOCKS__", "" } } );

    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    EXPECT_FALSE( error.empty() ) << error.dump( 4 );
    EXPECT_EQ( json_get_int( error, "code" ), -32602 );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.wrong_field" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong field of object" );
    EXPECT_EQ( json_get_str( error, "reason" ), "wrong 'state' field" );
}

TEST_F( SceneManagerTests, isSceneStateErrors ) {  // 1.49.1
    auto isSceneState =
        replaceTokensInString( isSceneStateWhenBlock, { { "__SCENE_ID__", "unknown" }, { "__STATE__", "finished" } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isSceneState" }, { "__WHEN_BLOCKS__", isSceneState }, { "__THEN_BLOCKS__", "" } } );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    EXPECT_FALSE( error.empty() ) << error.dump( 4 );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.not.exist" );
    EXPECT_EQ( json_get_str( error, "message" ), "The scene with this id does not exist" );
    EXPECT_EQ( json_get_str( error, "reason" ), "" );

    auto triggerScene = replaceTokens(
        createSceneRequest, { { "__NAME__", "TriggerScene" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", "" } } );
    auto sceneTriggerId = createScene( triggerScene, __LINE__ );

    isSceneState = replaceTokensInString( isSceneStateWhenBlock,
                                          { { "__SCENE_ID__", sceneTriggerId }, { "__STATE__", "unknown" } } );
    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isSceneState" }, { "__WHEN_BLOCKS__", isSceneState }, { "__THEN_BLOCKS__", "" } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    EXPECT_FALSE( error.empty() ) << error.dump( 4 );
    EXPECT_EQ( json_get_int( error, "code" ), -32602 );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.wrong_field" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong field of object" );
    EXPECT_EQ( json_get_str( error, "reason" ), "wrong 'state' field" );

    isSceneState = replaceTokensInString( isSceneStateWhenBlock,
                                          { { "__SCENE_ID__", sceneTriggerId }, { "__STATE__", "finished" } } );
    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isSceneState" }, { "__WHEN_BLOCKS__", isSceneState }, { "__THEN_BLOCKS__", "" } } );

    auto stateSceneId = createScene( request, __LINE__ );

    isSceneState = replaceTokensInString( isSceneStateWhenBlock,
                                          { { "__SCENE_ID__", stateSceneId }, { "__STATE__", "finished" } } );
    request =
        replaceTokens( editScenesRequest,
                       { { "__ID__", stateSceneId }, { "__THEN_BLOCKS__", "" }, { "__WHEN_BLOCKS__", isSceneState } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    EXPECT_FALSE( error.empty() ) << error.dump( 4 );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "scenes.when.recursive_execute" );
    EXPECT_EQ( json_get_str( error, "message" ), "Scene contain conditions with current scene id" );
    EXPECT_EQ( json_get_str( error, "reason" ), "" );
}

TEST_F( SceneManagerTests, isSceneStateFinished ) {  // 1.49.2
    testIsSceneStateResult( "finished" );
}

TEST_F( SceneManagerTests, isSceneStatePartiallyFinished ) {  // 1.49.3
    testIsSceneStateResult( "partially_finished" );
}

TEST_F( SceneManagerTests, isSceneStateFailed ) {  // 1.49.4
    testIsSceneStateResult( "failed" );
}

TEST_F( SceneManagerTests, isSceneStateAnyResult ) {  // 1.49.5
    testIsSceneStateResult( "any_result" );
}

TEST_F( SceneManagerTests, isSceneStateEnabled ) {  // 1.49.6
    auto triggerScene = replaceTokens(
        createSceneRequest, { { "__NAME__", "TriggerScene" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", "" } } );
    auto sceneId1 = createScene( triggerScene, __LINE__ );

    auto isSceneState = replaceTokensInString( isSceneStateWhenBlock,
                                               { { "__SCENE_ID__", sceneId1 }, { "__STATE__", "scene_enabled" } } );
    auto sceneStateScene = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isSceneState" }, { "__WHEN_BLOCKS__", isSceneState }, { "__THEN_BLOCKS__", "" } } );
    auto sceneId2 = createScene( sceneStateScene, __LINE__ );

    auto enableSceneRequest =
        replaceTokens( setEnableSceneRequest, { { "__ID__", sceneId1 }, { "__ENABLED__", "true" } } );
    auto disableSceneRequest =
        replaceTokens( setEnableSceneRequest, { { "__ID__", sceneId1 }, { "__ENABLED__", "false" } } );

    auto answer = nma().sendtoHubAndGetAnswer( disableSceneRequest );
    EXPECT_TRUE( answer[ "error" ].empty() ) << answer.dump( 4 );
    checkChangedBroadcastEnableRequest( sceneId1, triggerScene, false, __LINE__ );

    answer = nma().sendtoHubAndGetAnswer( enableSceneRequest );
    EXPECT_TRUE( answer[ "error" ].empty() ) << answer.dump( 4 );

    checkBroadcasts(
        { [this, &sceneId1]( const Json& broadcast ) { return isEnableChangedBroadcast( broadcast, sceneId1, true ); },
          [this, &sceneId2]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, sceneId2, "started" );
          },
          [this, &sceneId2]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, sceneId2, "finished" );
          } },
        __LINE__ );
}

TEST_F( SceneManagerTests, isSceneStateDisabled ) {  // 1.49.7
    auto triggerScene = replaceTokens(
        createSceneRequest, { { "__NAME__", "TriggerScene" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", "" } } );
    auto sceneId1 = createScene( triggerScene, __LINE__ );

    auto isSceneState = replaceTokensInString( isSceneStateWhenBlock,
                                               { { "__SCENE_ID__", sceneId1 }, { "__STATE__", "scene_disabled" } } );
    auto sceneStateScene = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isSceneState" }, { "__WHEN_BLOCKS__", isSceneState }, { "__THEN_BLOCKS__", "" } } );
    auto sceneId2 = createScene( sceneStateScene, __LINE__ );

    auto enableSceneRequest =
        replaceTokens( setEnableSceneRequest, { { "__ID__", sceneId1 }, { "__ENABLED__", "true" } } );
    auto disableSceneRequest =
        replaceTokens( setEnableSceneRequest, { { "__ID__", sceneId1 }, { "__ENABLED__", "false" } } );

    auto answer = nma().sendtoHubAndGetAnswer( disableSceneRequest );
    EXPECT_TRUE( answer[ "error" ].empty() ) << answer.dump( 4 );

    checkBroadcasts(
        { [this, &sceneId1]( const Json& broadcast ) { return isEnableChangedBroadcast( broadcast, sceneId1, false ); },
          [this, &sceneId2]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, sceneId2, "started" );
          },
          [this, &sceneId2]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, sceneId2, "finished" );
          } },
        __LINE__ );

    answer = nma().sendtoHubAndGetAnswer( enableSceneRequest );
    EXPECT_TRUE( answer[ "error" ].empty() ) << answer.dump( 4 );
    checkChangedBroadcastEnableRequest( sceneId1, triggerScene, true, __LINE__ );
}

TEST_F( SceneManagerTests, blockIdAdding ) {  // 1.50
    auto isItemState = replaceTokensInString( isItemStateWhenBlock, { { "__VALUE__", "true" } } );
    auto setItemValue = replaceTokensInString( setItemValueThenBlock, { { "__VALUE__", "true" } } );
    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "BlockIdAdding" },
                                    { "__WHEN_BLOCKS__", isItemState + "," + isItemState },
                                    { "__THEN_BLOCKS__", setItemValue + "," + setItemValue } } );

    sendRequestAndCheckAnswer( request, __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.scene.added", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.added" );
    JsonObject result;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    SceneID id = json_get_str( result, "_id" );
    EXPECT_TRUE( !id.empty() );
    m_createdScenes.insert( id );

    auto& thenBlocks = result[ "then" ];
    for( auto& block : thenBlocks ) {
        EXPECT_FALSE( json_get_str( block, "_id" ).empty() );
    }

    auto& whenBlocks = result[ "then" ];
    for( auto& block : whenBlocks ) {
        EXPECT_FALSE( json_get_str( block, "_id" ).empty() );
    }
}

TEST_F( SceneManagerTests, isCloudStateConnected ) {  // 1.51.1
    auto isCloudState = replaceTokensInString( isCloudStateWhenBlock, { { "__STATE__", "connected" } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isCloudState" }, { "__WHEN_BLOCKS__", isCloudState }, { "__THEN_BLOCKS__", "" } } );
    auto sceneId = createScene( request, __LINE__ );

    nma().sendBroadcast( CLOUD_UNREGISTERED, JSON_EMPTY_OBJ );
    nma().sendBroadcast( CLOUD_REGISTERED, JSON_EMPTY_OBJ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    nma().sendBroadcast( CLOUD_UNREGISTERED, JSON_EMPTY_OBJ );
    nma().sendBroadcast( CLOUD_REGISTERED, JSON_EMPTY_OBJ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    nma().sendBroadcast( CLOUD_UNREGISTERED, JSON_EMPTY_OBJ );
}

TEST_F( SceneManagerTests, isCloudStateDisconnected ) {  // 1.51.2
    auto isCloudState = replaceTokensInString( isCloudStateWhenBlock, { { "__STATE__", "disconnected" } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isCloudState" }, { "__WHEN_BLOCKS__", isCloudState }, { "__THEN_BLOCKS__", "" } } );
    auto sceneId = createScene( request, __LINE__ );

    nma().sendBroadcast( CLOUD_REGISTERED, JSON_EMPTY_OBJ );
    nma().sendBroadcast( CLOUD_UNREGISTERED, JSON_EMPTY_OBJ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    nma().sendBroadcast( CLOUD_REGISTERED, JSON_EMPTY_OBJ );
    nma().sendBroadcast( CLOUD_UNREGISTERED, JSON_EMPTY_OBJ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    nma().sendBroadcast( CLOUD_REGISTERED, JSON_EMPTY_OBJ );
}

TEST_F( SceneManagerTests, enableWhenBlock ) {  // 1.52.1
    nma().sendBroadcast( NETWORK_BROADCAST_IP_ADDRESS_CHANGED,
                         JSON( "interfaceId"
                               << "interface1" ) );

    auto isNetworkState =
        replaceTokens( isNetworkStateWhenBlock, { { "__NETWORK_ID__", "" }, { "__STATE__", "failed" } } );
    isNetworkState[ "_id" ] = "1";
    isNetworkState[ "enabled" ] = false;

    auto isCloudState = replaceTokens( isCloudStateWhenBlock, { { "__STATE__", "connected" } } );
    isCloudState[ "_id" ] = "2";
    isCloudState[ "enabled" ] = false;

    auto sceneRequest = replaceTokens( createSceneRequest,
                                       { { "__NAME__", "Enable block" },
                                         { "__WHEN_BLOCKS__", isCloudState.dump() + "," + isNetworkState.dump() },
                                         { "__THEN_BLOCKS__", "" } } );

    auto sceneId = createScene( sceneRequest, __LINE__ );

    nma().sendBroadcast( CLOUD_UNREGISTERED, JSON_EMPTY_OBJ );
    nma().sendBroadcast( CLOUD_REGISTERED, JSON_EMPTY_OBJ );

    sendNetworkChangedBroadcast( "interface1", true );
    sendNetworkChangedBroadcast( "interface1", false );

    enableSceneBlock( sceneId, "1", true, __LINE__ );
    enableSceneBlock( sceneId, "2", true, __LINE__ );

    nma().sendBroadcast( CLOUD_UNREGISTERED, JSON_EMPTY_OBJ );
    nma().sendBroadcast( CLOUD_REGISTERED, JSON_EMPTY_OBJ );

    testRunProgressBroadcast( sceneRequest, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( sceneRequest, sceneId, "finished", __LINE__ );

    sendNetworkChangedBroadcast( "interface1", true );
    sendNetworkChangedBroadcast( "interface1", false );

    testRunProgressBroadcast( sceneRequest, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( sceneRequest, sceneId, "finished", __LINE__ );
}

TEST_F( SceneManagerTests, enableOperatorWhenBlock ) {  // 1.52.2
    nma().sendBroadcast( NETWORK_BROADCAST_IP_ADDRESS_CHANGED,
                         JSON( "interfaceId"
                               << "interface1" ) );

    auto isNetworkState =
        replaceTokensInString( isNetworkStateWhenBlock, { { "__NETWORK_ID__", "" }, { "__STATE__", "failed" } } );

    auto isCloudState = replaceTokensInString( isCloudStateWhenBlock, { { "__STATE__", "connected" } } );

    auto andOperator = replaceTokens( andOperatorWhenBlock, { { "__BLOCKS__", isNetworkState + "," + isCloudState } } );
    andOperator[ "_id" ] = "1";
    andOperator[ "enabled" ] = false;

    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "Enable operator" }, { "__WHEN_BLOCKS__", andOperator.dump() }, { "__THEN_BLOCKS__", "" } } );

    auto sceneId = createScene( request, __LINE__ );

    nma().sendBroadcast( CLOUD_UNREGISTERED, JSON_EMPTY_OBJ );
    nma().sendBroadcast( CLOUD_REGISTERED, JSON_EMPTY_OBJ );

    sendNetworkChangedBroadcast( "interface1", true );
    sendNetworkChangedBroadcast( "interface1", false );

    nma().sendBroadcast( CLOUD_UNREGISTERED, JSON_EMPTY_OBJ );
    sendNetworkChangedBroadcast( "interface1", true );

    enableSceneBlock( sceneId, "1", true, __LINE__ );

    nma().sendBroadcast( CLOUD_REGISTERED, JSON_EMPTY_OBJ );

    sendNetworkChangedBroadcast( "interface1", false );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
}

TEST_F( SceneManagerTests, enableNestedOperators ) {  // 1.52.3
    nma().sendBroadcast( NETWORK_BROADCAST_IP_ADDRESS_CHANGED,
                         JSON( "interfaceId"
                               << "interface1" ) );

    auto isNetworkState =
        replaceTokensInString( isNetworkStateWhenBlock, { { "__NETWORK_ID__", "" }, { "__STATE__", "failed" } } );

    auto isCloudState = replaceTokensInString( isCloudStateWhenBlock, { { "__STATE__", "connected" } } );

    auto notOperator = replaceTokens( notOperatorWhenBlock, { { "__BLOCK__", isCloudState } } );
    notOperator[ "_id" ] = "1";
    notOperator[ "enabled" ] = true;

    auto orOperator =
        replaceTokens( orOperatorWhenBlock, { { "__BLOCKS__", notOperator.dump() + "," + isNetworkState } } );
    orOperator[ "_id" ] = "2";
    orOperator[ "enabled" ] = false;

    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "Enable operator" }, { "__WHEN_BLOCKS__", orOperator.dump() }, { "__THEN_BLOCKS__", "" } } );

    auto sceneId = createScene( request, __LINE__ );

    nma().sendBroadcast( CLOUD_REGISTERED, JSON_EMPTY_OBJ );
    nma().sendBroadcast( CLOUD_UNREGISTERED, JSON_EMPTY_OBJ );

    sendNetworkChangedBroadcast( "interface1", false );
    sendNetworkChangedBroadcast( "interface1", true );

    enableSceneBlock( sceneId, "1", false, __LINE__ );
    enableSceneBlock( sceneId, "2", true, __LINE__ );

    nma().sendBroadcast( CLOUD_REGISTERED, JSON_EMPTY_OBJ );
    nma().sendBroadcast( CLOUD_UNREGISTERED, JSON_EMPTY_OBJ );

    sendNetworkChangedBroadcast( "interface1", true );
    sendNetworkChangedBroadcast( "interface1", false );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
}

/// @brief
/// "else" block must be executed in case if the condition of "when" block is triggered to "false"
/// We create two devices with one Int item
/// A scene is triggered on item updates of the 1st device, the action blocks update item value of the 2nd device
TEST_F( SceneManagerTests, elseBlockConditionTrigger ) {  // 1.53
    auto item1Data = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    auto item2Data = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    auto request = createEmptyScene;
    request[ "params" ][ "when" ].push_back(
        replaceTokens( isItemStateWhenBlock,
                       { { "__ITEM_ID__", item1Data.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "20" } } ) );
    request[ "params" ][ "then" ].push_back(
        replaceTokens( setItemValueThenBlock,
                       { { "__ITEM_ID__", item2Data.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "30" } } ) );
    request[ "params" ][ "else" ].push_back(
        replaceTokens( setItemValueThenBlock,
                       { { "__ITEM_ID__", item2Data.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "40" } } ) );

    SceneID sceneId = createScene( request );

    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );

    // Set item1 value to satisfy the "when" condition
    // Expect execution of "when" block: scene progress broadcasts, corresponding item updates
    setItemValue( coreRemoteInterface, item1Data.m_itemId, 20, __LINE__ );
    checkBroadcasts( { [this, &sceneId, &request]( const Json& broadcast ) {
                          return isRunProgressBroadcast( broadcast, request, sceneId, "started" );
                      },
                       [this, &sceneId, &request]( const Json& broadcast ) {
                           return isRunProgressBroadcast( broadcast, request, sceneId, "finished" );
                       },
                       [this, &item2Data]( const Json& broadcast ) {
                           return isItemUpdateBroadcast( broadcast, item2Data.m_itemId, 30 );
                       } },
                     __LINE__ );

    // Set item1 value to trigger the "else" block
    // Scene progress broadcasts are NOT expected, expected corresponding item updates
    setItemValue( coreRemoteInterface, item1Data.m_itemId, 5, __LINE__ );
    checkItemUpdateBroadcast( item2Data.m_itemId, 40, __LINE__ );
}

TEST_F( SceneManagerTests, isDateWeeksOfMonth ) {  // 1.4.1
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-08-15T00:00:00+0000" ).toSeconds() );

    const SceneID sceneId1 = createScene( createEmptyWhenBlockSceneJson );

    auto whenBlock = replaceTokens( isDateWeeksWhenBlockTemplate,
                                    { { "__WEEKS_TYPE__", "\"weeks\"" }, { "__WEEKS_ARRAY__", R"([1, -1])" } } );
    auto elseBlock = replaceTokens( runSceneThenBlock, { { "__SCENE_ID__", sceneId1 } } );

    JsonObject sceneJson2 = createEmptyWhenBlockSceneJson;
    sceneJson2[ "params" ][ "name" ] = "isDateWeeksTest";
    sceneJson2[ "params" ][ "when" ] = JSON_ARRAY_( std::move( whenBlock ) );
    sceneJson2[ "params" ][ "else" ] = JSON_ARRAY_( std::move( elseBlock ) );
    const SceneID sceneId2 = createScene( sceneJson2 );

    // week -1
    GregorianCalendar::Date rangeStartDate( "2021-08-31T00:00:00+0000" );
    rangeStartDate.moveToWeekday( 1 );
    scenesManagerInterface.setTestClock( rangeStartDate.addMilliseconds( -200 ).toSeconds() );
    testRunProgressBroadcast( sceneJson2, sceneId2, "started", __LINE__ );
    testRunProgressBroadcast( sceneJson2, sceneId2, "finished", __LINE__ );

    // week 1
    GregorianCalendar::Date rangeEndDate( "2021-09-01T00:00:00+0000" );
    rangeEndDate.incrementWeek().moveToWeekday( 1 );
    scenesManagerInterface.setTestClock( rangeEndDate.addMilliseconds( -200 ).toSeconds() );
    testRunProgressBroadcast( createEmptyWhenBlockSceneJson, sceneId1, "started", __LINE__ );  // from "else" block
    testRunProgressBroadcast( createEmptyWhenBlockSceneJson, sceneId1, "finished", __LINE__ );
}

TEST_F( SceneManagerTests, isDateWeeksOfYear ) {  // 1.4.2
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2020-08-15T00:00:00+0000" ).toSeconds() );

    const SceneID sceneId1 = createScene( createEmptyWhenBlockSceneJson );

    auto whenBlock = replaceTokens( isDateWeeksWhenBlockTemplate,
                                    { { "__WEEKS_TYPE__", "\"yearWeeks\"" }, { "__WEEKS_ARRAY__", R"([1, -1])" } } );
    auto elseBlock = replaceTokens( runSceneThenBlock, { { "__SCENE_ID__", sceneId1 } } );

    JsonObject sceneJson2 = createEmptyWhenBlockSceneJson;
    sceneJson2[ "params" ][ "name" ] = "isDateWeeksTest";
    sceneJson2[ "params" ][ "when" ] = JSON_ARRAY_( std::move( whenBlock ) );
    sceneJson2[ "params" ][ "else" ] = JSON_ARRAY_( std::move( elseBlock ) );
    const SceneID sceneId2 = createScene( sceneJson2 );

    // week -1
    GregorianCalendar::Date rangeStartDate( "2020-12-31T00:00:00+0000" );
    rangeStartDate.moveToWeekday( 1 );
    scenesManagerInterface.setTestClock( rangeStartDate.addMilliseconds( -200 ).toSeconds() );
    testRunProgressBroadcast( sceneJson2, sceneId2, "started", __LINE__ );
    testRunProgressBroadcast( sceneJson2, sceneId2, "finished", __LINE__ );

    // week 1
    GregorianCalendar::Date rangeEndDate( "2021-01-01T00:00:00+0000" );
    rangeEndDate.incrementWeek().moveToWeekday( 1 );
    scenesManagerInterface.setTestClock( rangeEndDate.addMilliseconds( -200 ).toSeconds() );
    testRunProgressBroadcast( createEmptyWhenBlockSceneJson, sceneId1, "started", __LINE__ );  // from "else" block
    testRunProgressBroadcast( createEmptyWhenBlockSceneJson, sceneId1, "finished", __LINE__ );
}

TEST_F( SceneManagerTests, CloudAPI ) {
    Log::setLocalLogLevel( Log::TRACE );
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );

    auto request = replaceTokensInString( createSceneRequest,
                                          { { "__NAME__", "сloudAPIAction" },
                                            { "__WHEN_BLOCKS__", "" },
                                            { "__THEN_BLOCKS__", HubPlatform::Tests::cloudAPIAction } } );

    int cloudCallsCounter = 0;

    auto goodCloudResponse = [&cloudCallsCounter]( rpc::Message& rpcMessage ) {
        LOGGER_ERROR_RAW( "sendCloudMethod / lambda / message: " + cc::j( rpcMessage.getMessage() ) );
        JsonObject& joAnswer = rpcMessage.getAnswer();
        JsonObject& joMessage = rpcMessage.getMessage();
        joAnswer[ "method" ] = json_get_str( joMessage, "method" );
        joAnswer[ "result" ] = JSON_EMPTY_OBJ;
        joAnswer[ "error" ] = nullptr;

        EXPECT_EQ( json_get_str( joMessage, "method" ), "cloud.some_api_call" );
        EXPECT_EQ( json_get_int( joMessage, "api" ), 1 );
        JsonObject params;
        json_get_obj( joMessage, "params", params );
        EXPECT_EQ( params.dump(), R"({"a":"b"})" );

        cloudCallsCounter++;
    };

    nma().setCloudMethod( "cloud.some_api_call", goodCloudResponse );

    JsonObject CloudAPI = JsonObject::parse( request );
    SceneID sceneId = createScene( CloudAPI );
    runSceneManually( sceneId, __LINE__ );

    testRunProgressBroadcast( CloudAPI, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( CloudAPI, sceneId, "finished", __LINE__ );

    ASSERT_EQ( cloudCallsCounter, 1 ) << "One call of cloud message expected";
}

TEST_F( SceneManagerTests, isDictionaryChangedError ) {  // 1.54
    auto whenBlockTemplate = replaceTokensInString(
        isDictionaryChangedWhenBlockTemplate,
        { { "__ITEM_ID__", "618a6370120bdd4c74ab420a" }, { "__KEY__", "key_1" }, { "__OPERATION__", "added" } } );

    JsonObject request = replaceTokens( createSceneRequest,
                                        { { "__NAME__", "isDictionaryChanged" },
                                          { "__WHEN_BLOCKS__", whenBlockTemplate },
                                          { "__THEN_BLOCKS__", "" } } );

    auto sceneId = createScene( request, __LINE__ );

    EXPECT_FALSE( sceneId.empty() );
}

TEST_F( SceneManagerTests, isDictionaryChangedAdded ) {  // 1.55
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "door",           /*name*/
                               "doorlock",       /*type*/
                               "297_32770_1536", /*deviceTypeId*/
                               "door_lock",      /*category*/
                               "",               /*subcategory*/
                               false,
                               __LINE__ );

    CoreDRemoteInterface::CoreDInterface::ItemID itemId;
    itemId = addItem( coreRemoteInterface,
                      deviceId,
                      "test_device",
                      CoreD::Model::ValueType::makeDictionaryType( CoreD::Model::ValueType::Enum::Token ),
                      true,
                      true,
                      __LINE__ );

    auto addDictionaryValue = replaceTokens(
        hubItemDictionaryValueAdd,
        { { "__ITEM_ID__", itemId }, { "__TYPE__", "token" }, { "__VALUE__", "\"dictionary\"" } } )[ "params" ];

    KeyJsonList fields = {
        { "operation", "added" },
        { "element",
          JSON( "value"
                << "" ) },
        { "_id", itemId },
        { "deviceId", deviceId },
        { "deviceName", "door" },
        { "deviceCategory", "door_lock" },
        { "deviceSubcategory", "" },
        { "roomName", "" },
        { "name", "test_device" },
    };

    auto addItemDictionaryValue =
        [&coreRemoteInterface, &fields, &addDictionaryValue, this](
            const std::string& itemId, const std::string& key, const std::string& value, int codeLine ) {
            std::string codeLineStr = getCodeLineString( codeLine );
            addDictionaryValue[ "element" ][ "value" ] = value;
            EXPECT_NO_THROW( coreRemoteInterface.addItemDictionaryValue( addDictionaryValue ) ) << codeLineStr;
            fields[ 0 ].second = "added";
            fields[ 1 ].second = JSON( key << JSON( "value" << value ) );
            checkItemDictionaryUpdatedBroadcast( itemId, fields, codeLine );
        };

    auto isDictionaryChanged =
        replaceTokensInString( isDictionaryChangedWhenBlockTemplate,
                               { { "__ITEM_ID__", itemId }, { "__KEY__", "1" }, { "__OPERATION__", "added" } } );

    const auto request = replaceTokens( createSceneRequest,
                                        { { "__NAME__", "isDictionaryChanged" },
                                          { "__WHEN_BLOCKS__", std::move( isDictionaryChanged ) },
                                          { "__THEN_BLOCKS__", "" } } );

    auto sceneId = createScene( request, __LINE__ );

    addItemDictionaryValue( itemId, "1", "dictionary", __LINE__ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
}

TEST_F( SceneManagerTests, isDictionaryChangedUpdated ) {  // 1.55.1
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "door",           /*name*/
                               "doorlock",       /*type*/
                               "297_32770_1536", /*deviceTypeId*/
                               "door_lock",      /*category*/
                               "",               /*subcategory*/
                               false,
                               __LINE__ );

    CoreDRemoteInterface::CoreDInterface::ItemID itemId;
    itemId = addItem( coreRemoteInterface,
                      deviceId,
                      "test_device",
                      CoreD::Model::ValueType::makeDictionaryType( CoreD::Model::ValueType::Enum::Token ),
                      true,
                      true,
                      __LINE__ );

    auto addDictionaryValue = replaceTokens(
        hubItemDictionaryValueAdd,
        { { "__ITEM_ID__", itemId }, { "__TYPE__", "token" }, { "__VALUE__", "\"dictionary\"" } } )[ "params" ];

    auto setDictionaryValue = replaceTokens( hubItemDictionaryValueSet,
                                             { { "__ITEM_ID__", itemId },
                                               { "__KEY__", "1" },
                                               { "__TYPE__", "token" },
                                               { "__VALUE__", "\"dictionary\"" } } )[ "params" ];

    KeyJsonList fields = {
        { "operation", "added" },
        { "element",
          JSON( "value"
                << "" ) },
        { "_id", itemId },
        { "deviceId", deviceId },
        { "deviceName", "door" },
        { "deviceCategory", "door_lock" },
        { "deviceSubcategory", "" },
        { "roomName", "" },
        { "name", "test_device" },
    };

    auto setItemDictionaryValue =
        [&coreRemoteInterface, &fields, &setDictionaryValue, this](
            const std::string& itemId, const std::string& key, const std::string& value, int codeLine ) {
            std::string codeLineStr = getCodeLineString( codeLine );
            setDictionaryValue[ "element" ][ "value" ] = value;
            EXPECT_NO_THROW( coreRemoteInterface.setItemDictionaryValue( setDictionaryValue ) ) << codeLineStr;
            fields[ 0 ].second = "updated";
            fields[ 1 ].second = JSON( key << JSON( "value" << value ) );
            checkItemDictionaryUpdatedBroadcast( itemId, fields, codeLine );
        };

    auto addItemDictionaryValue =
        [&coreRemoteInterface, &fields, &addDictionaryValue, this](
            const std::string& itemId, const std::string& key, const std::string& value, int codeLine ) {
            std::string codeLineStr = getCodeLineString( codeLine );
            addDictionaryValue[ "element" ][ "value" ] = value;
            EXPECT_NO_THROW( coreRemoteInterface.addItemDictionaryValue( addDictionaryValue ) ) << codeLineStr;
            fields[ 0 ].second = "added";
            fields[ 1 ].second = JSON( key << JSON( "value" << value ) );
            checkItemDictionaryUpdatedBroadcast( itemId, fields, codeLine );
        };

    auto isDictionaryChanged =
        replaceTokensInString( isDictionaryChangedWhenBlockTemplate,
                               { { "__ITEM_ID__", itemId }, { "__KEY__", "1" }, { "__OPERATION__", "updated" } } );

    const auto request = replaceTokens( createSceneRequest,
                                        { { "__NAME__", "isDictionaryChanged" },
                                          { "__WHEN_BLOCKS__", std::move( isDictionaryChanged ) },
                                          { "__THEN_BLOCKS__", "" } } );

    auto sceneId = createScene( request, __LINE__ );

    addItemDictionaryValue( itemId, "1", "dictionary", __LINE__ );

    setItemDictionaryValue( itemId, "1", "dictionary1", __LINE__ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
}

TEST_F( SceneManagerTests, isDictionaryChangedRemoved ) {  // 1.55.2
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "door",           /*name*/
                               "doorlock",       /*type*/
                               "297_32770_1536", /*deviceTypeId*/
                               "door_lock",      /*category*/
                               "",               /*subcategory*/
                               false,
                               __LINE__ );

    CoreDRemoteInterface::CoreDInterface::ItemID itemId;
    itemId = addItem( coreRemoteInterface,
                      deviceId,
                      "test_device",
                      CoreD::Model::ValueType::makeDictionaryType( CoreD::Model::ValueType::Enum::Token ),
                      true,
                      true,
                      __LINE__ );

    auto addDictionaryValue = replaceTokens(
        hubItemDictionaryValueAdd,
        { { "__ITEM_ID__", itemId }, { "__TYPE__", "token" }, { "__VALUE__", "\"dictionary\"" } } )[ "params" ];
    auto removeDictionaryValue =
        replaceTokens( hubItemDictionaryValueRemove, { { "__ITEM_ID__", itemId }, { "__KEY__", "1" } } )[ "params" ];

    KeyJsonList fields = {
        { "operation", "added" },
        { "element",
          JSON( "value"
                << "" ) },
        { "_id", itemId },
        { "deviceId", deviceId },
        { "deviceName", "door" },
        { "deviceCategory", "door_lock" },
        { "deviceSubcategory", "" },
        { "roomName", "" },
        { "name", "test_device" },
    };

    auto addItemDictionaryValue =
        [&coreRemoteInterface, &fields, &addDictionaryValue, this](
            const std::string& itemId, const std::string& key, const std::string& value, int codeLine ) {
            std::string codeLineStr = getCodeLineString( codeLine );
            addDictionaryValue[ "element" ][ "value" ] = value;
            EXPECT_NO_THROW( coreRemoteInterface.addItemDictionaryValue( addDictionaryValue ) ) << codeLineStr;
            fields[ 0 ].second = "added";
            fields[ 1 ].second = JSON( key << JSON( "value" << value ) );
            checkItemDictionaryUpdatedBroadcast( itemId, fields, codeLine );
        };

    auto removeItemDictionaryValue =
        [&coreRemoteInterface, &fields, &removeDictionaryValue, this](
            const std::string& itemId, const std::string& key, const std::string& value, int codeLine ) {
            std::string codeLineStr = getCodeLineString( codeLine );
            EXPECT_NO_THROW( coreRemoteInterface.removeItemDictionaryValue( removeDictionaryValue ) ) << codeLineStr;
            fields[ 0 ].second = "removed";
            fields[ 1 ].second = JSON( key << JSON( "value" << value ) );
            checkItemDictionaryUpdatedBroadcast( itemId, fields, codeLine );
        };

    auto isDictionaryChanged =
        replaceTokensInString( isDictionaryChangedWhenBlockTemplate,
                               { { "__ITEM_ID__", itemId }, { "__KEY__", "1" }, { "__OPERATION__", "removed" } } );

    const auto request = replaceTokens( createSceneRequest,
                                        { { "__NAME__", "isDictionaryChanged" },
                                          { "__WHEN_BLOCKS__", std::move( isDictionaryChanged ) },
                                          { "__THEN_BLOCKS__", "" } } );

    auto sceneId = createScene( request, __LINE__ );
    addItemDictionaryValue( itemId, "1", "dictionary", __LINE__ );
    removeItemDictionaryValue( itemId, "1", "dictionary", __LINE__ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
}

TEST_F( SceneManagerTests, dayOfWeekEvent ) {  // 1.56.1
    const uint8_t weekday = 3;

    GregorianCalendar::Date date( "2021-08-15T00:00:00+0000" );
    date.moveToWeekday( 1 );

    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    scenesManagerInterface.setTestClock( date.toSeconds() );

    const SceneID sceneId1 = createScene( createEmptyWhenBlockSceneJson );

    auto whenBlock = replaceTokens( isDateWithTimeWhenBlock,
                                    { { "__TYPE__", "weekly" },
                                      { "__TIME_ARRAY__", "[]" },
                                      { "__DAYS_ARRAY_FIELD__", "weekdays" },
                                      { "__DAYS_ARRAY__", "[" + std::to_string( weekday ) + "]" } } );
    auto elseBlock = replaceTokens( runSceneThenBlock, { { "__SCENE_ID__", sceneId1 } } );

    JsonObject sceneJson2 = createEmptyWhenBlockSceneJson;
    sceneJson2[ "params" ][ "name" ] = "dayOfWeekTest";
    sceneJson2[ "params" ][ "when" ] = JSON_ARRAY_( std::move( whenBlock ) );
    sceneJson2[ "params" ][ "else" ] = JSON_ARRAY_( std::move( elseBlock ) );
    const SceneID sceneId2 = createScene( sceneJson2 );

    date.moveToWeekday( weekday );  // rangeStartDate
    scenesManagerInterface.setTestClock( date.addMilliseconds( -200 ).toSeconds() );
    testRunProgressBroadcast( sceneJson2, sceneId2, "started", __LINE__ );
    testRunProgressBroadcast( sceneJson2, sceneId2, "finished", __LINE__ );

    // week 1
    date.moveToWeekday( weekday + 1 );  // rangeEndDate
    scenesManagerInterface.setTestClock( date.addMilliseconds( -200 ).toSeconds() );
    testRunProgressBroadcast( createEmptyWhenBlockSceneJson, sceneId1, "started", __LINE__ );  // from "else" block
    testRunProgressBroadcast( createEmptyWhenBlockSceneJson, sceneId1, "finished", __LINE__ );
}

TEST_F( SceneManagerTests, dayOfMonthEvent ) {  // 1.56.2
    const uint8_t dayOfMonth = 13;

    GregorianCalendar::Date date( "2021-08-01T00:00:00+0000" );
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    scenesManagerInterface.setTestClock( date.toSeconds() );

    const SceneID sceneId1 = createScene( createEmptyWhenBlockSceneJson );

    auto whenBlock = replaceTokens( isDateWithTimeWhenBlock,
                                    { { "__TYPE__", "monthly" },
                                      { "__TIME_ARRAY__", "[]" },
                                      { "__DAYS_ARRAY_FIELD__", "days" },
                                      { "__DAYS_ARRAY__", "[" + std::to_string( dayOfMonth ) + "]" } } );
    auto elseBlock = replaceTokens( runSceneThenBlock, { { "__SCENE_ID__", sceneId1 } } );

    JsonObject sceneJson2 = createEmptyWhenBlockSceneJson;
    sceneJson2[ "params" ][ "name" ] = "dayOfMonthTest";
    sceneJson2[ "params" ][ "when" ] = JSON_ARRAY_( std::move( whenBlock ) );
    sceneJson2[ "params" ][ "else" ] = JSON_ARRAY_( std::move( elseBlock ) );
    const SceneID sceneId2 = createScene( sceneJson2 );

    date.setDay( dayOfMonth );  // rangeStartDate
    scenesManagerInterface.setTestClock( date.addMilliseconds( -200 ).toSeconds() );
    testRunProgressBroadcast( sceneJson2, sceneId2, "started", __LINE__ );
    testRunProgressBroadcast( sceneJson2, sceneId2, "finished", __LINE__ );

    // week 1
    date.setDay( dayOfMonth + 1 );  // rangeEndDate
    scenesManagerInterface.setTestClock( date.addMilliseconds( -200 ).toSeconds() );
    testRunProgressBroadcast( createEmptyWhenBlockSceneJson, sceneId1, "started", __LINE__ );  // from "else" block
    testRunProgressBroadcast( createEmptyWhenBlockSceneJson, sceneId1, "finished", __LINE__ );
}

TEST_F( SceneManagerTests, createSceneWithGeneratorData ) {  // 1.57.1

    auto sceneJson = createEmptyWhenBlockSceneJson;
    auto generatorData = JSON( "name"
                               << "tcds_name"
                               << "version"
                               << "1234"
                               << "role"
                               << "system" );
    sceneJson[ "params" ][ "generator" ] = generatorData;
    sendRequestAndCheckAnswer( sceneJson, __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.scene.added", "scene added broadcast" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.added" );
    JsonObject result;
    JsonObject params;
    JsonObject generator;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    EXPECT_TRUE( json_get_obj( sceneJson, "params", params ) );
    EXPECT_EQ( json_get_bool( result, "enabled" ), json_get_bool( params, "enabled" ) );
    EXPECT_EQ( json_get_str( result, "group_id" ), json_get_str( params, "group_id" ) );
    EXPECT_EQ( json_get_bool( result, "is_group" ), json_get_bool( params, "is_group" ) );
    EXPECT_EQ( json_get_str( result, "name" ), json_get_str( params, "name" ) );
    EXPECT_EQ( json_get_str( result, "parent_id" ), json_get_str( params, "parent_id" ) );
    EXPECT_EQ( json_get_str( result, "execPolicy" ), json_get_str( params, "execPolicy" ) );
    EXPECT_EQ( json_get_str( result, "icon" ), json_get_str( params, "icon" ) );
    EXPECT_TRUE( json_get_obj( result, "generator", generator ) );
    EXPECT_EQ( json_get_str( generator, "name" ), json_get_str( generatorData, "name" ) );
    EXPECT_EQ( json_get_str( generator, "version" ), json_get_str( generatorData, "version" ) );
    EXPECT_EQ( json_get_str( generator, "role" ), json_get_str( generatorData, "role" ) );
    EXPECT_TRUE( json_contains( generator, "created" ) );
    EXPECT_TRUE( json_contains( generator, "modified" ) );
    auto sceneId = json_get_str( result, "_id" );

    deleteScene( sceneId, __LINE__ );
}

TEST_F( SceneManagerTests, createSceneWithoutGeneratorData ) {  // 1.57.2

    auto sceneJson = createEmptyWhenBlockSceneJson;
    auto expectedGeneratorData = JSON( "name"
                                       << "unknown"
                                       << "version"
                                       << ""
                                       << "role"
                                       << "user" );
    sendRequestAndCheckAnswer( sceneJson, __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.scene.added", "scene added broadcast" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.added" );
    JsonObject result;
    JsonObject params;
    JsonObject generator;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    EXPECT_TRUE( json_get_obj( sceneJson, "params", params ) );
    EXPECT_EQ( json_get_bool( result, "enabled" ), json_get_bool( params, "enabled" ) );
    EXPECT_EQ( json_get_str( result, "group_id" ), json_get_str( params, "group_id" ) );
    EXPECT_EQ( json_get_bool( result, "is_group" ), json_get_bool( params, "is_group" ) );
    EXPECT_EQ( json_get_str( result, "name" ), json_get_str( params, "name" ) );
    EXPECT_EQ( json_get_str( result, "parent_id" ), json_get_str( params, "parent_id" ) );
    EXPECT_EQ( json_get_str( result, "execPolicy" ), json_get_str( params, "execPolicy" ) );
    EXPECT_EQ( json_get_str( result, "icon" ), json_get_str( params, "icon" ) );
    EXPECT_TRUE( json_get_obj( result, "generator", generator ) );
    EXPECT_EQ( json_get_str( generator, "name" ), json_get_str( expectedGeneratorData, "name" ) );
    EXPECT_EQ( json_get_str( generator, "version" ), json_get_str( expectedGeneratorData, "version" ) );
    EXPECT_EQ( json_get_str( generator, "role" ), json_get_str( expectedGeneratorData, "role" ) );
    EXPECT_TRUE( json_contains( generator, "created" ) );
    EXPECT_TRUE( json_contains( generator, "modified" ) );
    auto sceneId = json_get_str( result, "_id" );

    deleteScene( sceneId, __LINE__ );
}

TEST_F( SceneManagerTests, editSceneWithoutGeneratorField ) {  // 1.58
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-01-05T15:19:58" ).toSeconds() );
    auto sceneJson = createSceneJson;
    auto generator = JSON( "name"
                           << "tcds_name"
                           << "version"
                           << "1234"
                           << "role"
                           << "system" );
    sceneJson[ "params" ][ "generator" ] = generator;
    sendRequestAndCheckAnswer( sceneJson, __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.scene.added", "scene added" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.added" );
    JsonObject result, sceneCreatedGenerator;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    EXPECT_TRUE( json_get_obj( result, "generator", sceneCreatedGenerator ) );

    SceneID sceneId = json_get_str( result, "_id" );

    std::this_thread::sleep_for( std::chrono::seconds{ 1 } );
    std::string dump = HubPlatform::Tests::editScene.dump();
    std::string toReplace = "__SCENE_ID__";
    dump.replace( dump.find( toReplace ), toReplace.size(), sceneId );
    toReplace = "\"__NEW_SCENE__\"";
    dump.replace( dump.find( toReplace ), toReplace.size(), createIsOnceScene[ "params" ].dump() );
    JsonObject editScene = JsonObject::parse( dump );

    auto answer = nma().sendtoHubAndGetAnswer( editScene );
    auto& error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    broadcast = waitForBroadcast( "hub.scene.changed", "scene changes" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.changed" );
    JsonObject generatorData;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    EXPECT_EQ( json_get_str( result, "_id" ), sceneId );
    EXPECT_TRUE( json_get_obj( result, "generator", generatorData ) );

    EXPECT_EQ( json_get_str( generatorData, "name" ), sceneCreatedGenerator[ "name" ] );
    EXPECT_EQ( json_get_str( generatorData, "version" ), sceneCreatedGenerator[ "version" ] );
    EXPECT_EQ( json_get_int( generatorData, "created" ), json_get_int( sceneCreatedGenerator, "created" ) );
    EXPECT_NE( json_get_int( generatorData, "modified" ), json_get_int( sceneCreatedGenerator, "modified" ) );
    EXPECT_EQ( json_get_str( generatorData, "role" ), "user" );

    deleteScene( sceneId, __LINE__ );
}

TEST_F( SceneManagerTests, editSceneWithGeneratorField ) {  // 1.59
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-01-05T15:19:58" ).toSeconds() );
    auto sceneJson = createSceneJson;
    sceneJson[ "params" ][ "generator" ] = JSON( "name"
                                                 << "tcds_name"
                                                 << "version"
                                                 << "1234"
                                                 << "role"
                                                 << "system" );
    sendRequestAndCheckAnswer( sceneJson, __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.scene.added", "scene added" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.added" );
    JsonObject result, sceneCreatedGenerator;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    EXPECT_TRUE( json_get_obj( result, "generator", sceneCreatedGenerator ) );
    SceneID sceneId = json_get_str( result, "_id" );

    std::this_thread::sleep_for( std::chrono::seconds{ 1 } );
    std::string dump = HubPlatform::Tests::editScene.dump();
    std::string toReplace = "__SCENE_ID__";
    dump.replace( dump.find( toReplace ), toReplace.size(), sceneId );
    toReplace = "\"__NEW_SCENE__\"";
    auto updatedGenerator = JSON( "name"
                                  << "new_name"
                                  << "version"
                                  << "5555"
                                  << "created" << sceneCreatedGenerator[ "created" ] << "role"
                                  << "system" );
    auto updatedScene = createIsOnceScene[ "params" ];
    updatedScene[ "generator" ] = updatedGenerator;
    dump.replace( dump.find( toReplace ), toReplace.size(), updatedScene.dump() );
    JsonObject editScene = JsonObject::parse( dump );

    auto answer = nma().sendtoHubAndGetAnswer( editScene );
    auto& error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    broadcast = waitForBroadcast( "hub.scene.changed", "scene changes" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.changed" );
    Json generatorData;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    EXPECT_EQ( json_get_str( result, "_id" ), sceneId );
    EXPECT_TRUE( json_get_obj( result, "generator", generatorData ) );

    EXPECT_EQ( json_get_str( generatorData, "name" ), updatedGenerator[ "name" ] );
    EXPECT_EQ( json_get_str( generatorData, "version" ), updatedGenerator[ "version" ] );
    EXPECT_EQ( json_get_int( generatorData, "created" ), json_get_int( sceneCreatedGenerator, "created" ) );
    EXPECT_NE( json_get_int( generatorData, "modified" ), json_get_int( sceneCreatedGenerator, "modified" ) );
    EXPECT_EQ( json_get_str( generatorData, "role" ), json_get_str( updatedGenerator, "role" ) );

    deleteScene( sceneId, __LINE__ );
}

TEST_F( SceneManagerTests, sceneGetApiTestForGeneratorData ) {  // 1.60

    auto sceneJson = createEmptyWhenBlockSceneJson;
    auto expectedGeneratorData = JSON( "name"
                                       << "unknown"
                                       << "version"
                                       << ""
                                       << "role"
                                       << "user" );
    sendRequestAndCheckAnswer( sceneJson, __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.scene.added", NMA_TEST_MAKE_MESSAGE( "scene added broadcast" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.added" );
    JsonObject result;
    JsonObject generator;
    ASSERT_TRUE( json_get_obj( broadcast, "result", result ) );
    auto sceneId = json_get_str( result, "_id" );

    std::string dump = HubPlatform::Tests::getScene.dump();
    std::string toReplace = "__SCENE_ID__";
    dump.replace( dump.find( toReplace ), toReplace.size(), sceneId );
    JsonObject getScene = JsonObject::parse( dump );

    auto answer = nma().sendtoHubAndGetAnswer( getScene );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    JsonObject scene;
    ASSERT_TRUE( json_get_obj( answer, "result", scene ) );
    EXPECT_EQ( json_get_str( scene, "_id" ), sceneId );
    EXPECT_TRUE( json_get_obj( result, "generator", generator ) );
    EXPECT_EQ( json_get_str( generator, "name" ), json_get_str( expectedGeneratorData, "name" ) );
    EXPECT_EQ( json_get_str( generator, "version" ), json_get_str( expectedGeneratorData, "version" ) );
    EXPECT_EQ( json_get_str( generator, "role" ), json_get_str( expectedGeneratorData, "role" ) );
    EXPECT_TRUE( json_contains( generator, "created" ) );
    EXPECT_TRUE( json_contains( generator, "modified" ) );

    deleteScene( sceneId, __LINE__ );
}

TEST_F( SceneManagerTests, getScenesListParamsError ) {  // 1.61
    auto sceneListRequest = getScenesList;
    {
        sceneListRequest[ "params" ] = JSON( "filters" << JSON( "generator" << JSON( "role"
                                                                                     << "user" ) ) );
        auto answer = nma().sendtoHubAndGetAnswer( sceneListRequest );
        auto error = answer[ "error" ];
        ASSERT_FALSE( error.empty() ) << error.dump( 4 );
        EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.wrong_field" );
        EXPECT_EQ( json_get_str( error, "field" ), "include" );
        EXPECT_EQ( json_get_str( error, "message" ), "Wrong field of object" );
    }

    {
        sceneListRequest[ "params" ] = JSON( "generator" << JSON( "role"
                                                                  << "user" ) );
        auto answer = nma().sendtoHubAndGetAnswer( sceneListRequest );
        auto error = answer[ "error" ];
        ASSERT_FALSE( error.empty() ) << error.dump( 4 );
        EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.wrong_field" );
        EXPECT_EQ( json_get_str( error, "field" ), "filters" );
        EXPECT_EQ( json_get_str( error, "message" ), "Wrong field of object" );
    }

    {
        sceneListRequest[ "params" ] = JSON( "filters"
                                             << "user" );
        auto answer = nma().sendtoHubAndGetAnswer( sceneListRequest );
        auto error = answer[ "error" ];
        ASSERT_FALSE( error.empty() ) << error.dump( 4 );
        EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
        EXPECT_EQ( json_get_str( error, "field" ), "filters" );
        EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );
    }
}

}  // namespace Tests
}  // namespace HubPlatform
