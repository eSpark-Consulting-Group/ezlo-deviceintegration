#include "jsonrpc/jsonrpc.h"
#include "common_test_resources/fixtures/core_test_fixture.h"
#include <database_client/dbqueryparser.h>
#include "cored/house_modes/house_mode_utils.h"
#include "base_utils/timeutils.h"
#include "common_test_resources/resources/cored_resources.h"

/*

TESTPLAN

DONE    1. Item updated broadcast and modes alarmed broadcasts during and after walk test
DONE    2. Hub modes get walk test progress info
DONE    3. Check walk test already started error
DONE    4. Check walk test stop error if not started
DONE    5. Check walk test stop error if pass incorrect operation id
DONE    6. Check walk test extend error
DONE    7. Check lua events
*/

namespace HubPlatform {
namespace Tests {

namespace Resources {
static char SubscribeScript[] = "HUB:house_modes_test/scripts/subscribe_event";
static char UnSubscribeScript[] = "HUB:house_modes_test/scripts/unsubscribe_event";
}  // namespace Resources

using namespace CoreD;
using namespace CoreDHouseModes;
using namespace Services;

class HubModesWalkTests : public CoreNmaTest {
public:
    void SetUp() override;
    void TearDown() override;
    void setItemUpdate( bool isProtectNotification, int codeLine );
    void switchToHouseMode( CoreDRemoteInterface::CoreDInterface& coredRemote,
                            const std::string& modeId,
                            int codeLine );
    void initHouseMode();
    void checkHubModesInfo( int codeLine );
    Json startWalkTest( bool expectError, bool waitBroadcast, int codeLine );
    Json stopWalkTest( bool expectError, int codeLine );
    Json stopWalkTest( const std::string& operationId, bool expectError, bool waitBroadcast, int codeLine );
    Json extendWalkTest( bool expectError, int codeLine );
    void executeScript( const std::string& scriptPath, int line );
    JsonObject checkUiBroadcast( int line );
};

void HubModesWalkTests::SetUp() {
    CoreNmaTest::SetUp();
    callingFile( __FILE__ );
}

void HubModesWalkTests::TearDown() {
    ignoreBroadcasts(
        []( const Json& broadcast ) {
            Log::error( cc::error( "# TearDown: Not handled broadcasts: " ) + cc::j( broadcast ) );
            return true;
        },
        __FILE__,
        __LINE__,
        std::chrono::milliseconds( 10 ) );
    CommonNmaTests::TearDown();
}

void HubModesWalkTests::switchToHouseMode( CoreDRemoteInterface::CoreDInterface& coredRemote,
                                           const std::string& modeId,
                                           int codeLine ) {
    coredRemote.setSwitchDelay( 0 );
    coredRemote.setHouseMode( modeId );
    testHouseModesSwitchedBroadcast( modeId, "begin", codeLine );
    testHouseModesSwitchedBroadcast( modeId, "done", codeLine );
}

void HubModesWalkTests::initHouseMode() {
    auto& dbClient = nma().getDBClient();
    const auto AlarmDelay = 1;
    CoreDRemoteInterface::CoreDInterface& remoteInterface = makeRemote( nma() );
    switchToHouseMode( remoteInterface, "3", __LINE__ );
    CoreDHouseModes::setAlarmDelay( dbClient, AlarmDelay );
    remoteInterface.setSwitchDelay( 0 );
    remoteInterface.setHouseMode( "2" );
    testHouseModesSwitchedBroadcast( "2", "begin", __LINE__ );
    testHouseModesSwitchedBroadcast( "2", "done", __LINE__ );
}

void HubModesWalkTests::setItemUpdate( bool isProtectNotification, int codeLine ) {
    CoreDRemoteInterface::CoreDInterface& remoteInterface = makeRemote( nma() );
    auto device = addDeviceWithMotionSensorItem( __LINE__ );
    setItemValueWithoutCheck( remoteInterface, device.m_itemId, true, __LINE__ );
    auto broadcastData = waitForBroadcast( "hub.item.updated", "line: " + std::to_string( codeLine ) );

    Json result;
    EXPECT_TRUE( json_get( broadcastData, "result", result ) );
    EXPECT_EQ( json_get_bool( result, "protectNotification" ), isProtectNotification ) << std::to_string( codeLine );
}

Json HubModesWalkTests::startWalkTest( bool expectError, bool waitBroadcast, int codeLine ) {
    auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.protect.system.test.start" },
                                                 { "api", "1.0" },
                                                 { "params",
                                                   JSON( "operationId"
                                                         << "operationId"
                                                         << "protectNotification" << true << "type"
                                                         << "walk" ) }

    } );
    JsonObject result;
    json_get_obj( answer, "result", result );
    if( expectError ) {
        EXPECT_FALSE( result.empty() );
    }
    else {
        EXPECT_TRUE( result.empty() );
        if( waitBroadcast ) {
            auto broadcast = waitForBroadcast( "hub.modes.system.test.progress", getCodeLineString( codeLine ) );
            result = broadcast[ "result" ];
            EXPECT_EQ( json_get_int( result, "completed" ), 0 );
            EXPECT_EQ( json_get_str( result, "operationId" ), "operationId" );
            EXPECT_EQ( json_get_str( result, "status" ), "started" );
            EXPECT_EQ( json_get_str( result, "type" ), "walk" );
            EXPECT_EQ( json_get_bool( result, "protectNotification" ), true );
        }
    }
    return result;
}

Json HubModesWalkTests::extendWalkTest( bool expectError, int codeLine ) {
    auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.protect.system.test.extend" },
                                                 { "api", "1.0" },
                                                 { "params",
                                                   JSON( "operationId"
                                                         << "operationId" ) }

    } );
    JsonObject result;
    json_get_obj( answer, "result", result );
    if( expectError ) {
        EXPECT_FALSE( result.empty() );
    }
    else {
        EXPECT_TRUE( result.empty() );
        auto broadcast = waitForBroadcast( "hub.modes.system.test.progress", getCodeLineString( codeLine ) );
        result = broadcast[ "result" ];
        EXPECT_EQ( json_get_int( result, "completed" ), 80 );
        EXPECT_EQ( json_get_str( result, "operationId" ), "operationId" );
        EXPECT_EQ( json_get_str( result, "status" ), "extended" );
        EXPECT_EQ( json_get_str( result, "type" ), "walk" );
        EXPECT_EQ( json_get_bool( result, "protectNotification" ), false );
    }
    return result;
}

Json HubModesWalkTests::stopWalkTest( const std::string& operationId,
                                      bool expectError,
                                      bool waitBroadcast,
                                      int codeLine ) {
    auto answer = nma().sendtoHubAndGetAnswer(
        { { "method", "hub.modes.protect.system.test.stop" },
          { "api", "1.0" },
          { "params", JSON( "operationId" << operationId << "protectNotification" << true ) }

        } );
    JsonObject result;
    json_get_obj( answer, "result", result );
    if( expectError ) {
        EXPECT_FALSE( result.empty() );
    }
    else {
        EXPECT_TRUE( result.empty() );
        if( waitBroadcast ) {
            auto broadcast = waitForBroadcast( "hub.modes.system.test.progress", getCodeLineString( codeLine ) );
            result = broadcast[ "result" ];
            EXPECT_EQ( json_get_int( result, "completed" ), 100 );
            EXPECT_EQ( json_get_str( result, "operationId" ), "operationId" );
            EXPECT_EQ( json_get_str( result, "status" ), "finished" );
            EXPECT_EQ( json_get_str( result, "type" ), "walk" );
            EXPECT_EQ( json_get_bool( result, "protectNotification" ), true );
        }
    }
    return result;
}

Json HubModesWalkTests::stopWalkTest( bool expectError, int codeLine ) {
    return stopWalkTest( "operationId", expectError, true, codeLine );
}

void HubModesWalkTests::checkHubModesInfo( int codeLine ) {
    Json result;
    auto startTestTime = TimeUtils::currentSystemTime();
    auto answer = nma().sendtoHubAndGetAnswer(
        { { "method", "hub.modes.get" }, { "api", "3.0" }, { "params", JSON_EMPTY_OBJ } } );

    EXPECT_TRUE( json_get( answer, "result", result ) );
    JsonObject systemTestInfo;
    EXPECT_TRUE( json_get( result, "systemTest", systemTestInfo ) );
    int duration = json_get_int( systemTestInfo, "duration" );
    auto startedAt = json_get_int( systemTestInfo, "startedAt" );
    EXPECT_TRUE( startTestTime.count() >= startedAt );
    auto expectedTimeLeft = duration - ( TimeUtils::currentSystemTime().count() - startedAt );
    EXPECT_EQ( json_get_int( systemTestInfo, "duration" ), 900 );
    EXPECT_EQ( expectedTimeLeft, 900 - ( startTestTime.count() - startedAt ) );
    EXPECT_EQ( json_get_int( systemTestInfo, "timesLeft" ), expectedTimeLeft );
    EXPECT_EQ( json_get_str( systemTestInfo, "operationId" ), "operationId" );
}

void HubModesWalkTests::executeScript( const std::string& scriptPath, int line ) {
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", scriptPath } } );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    auto broadcast = checkUiBroadcast( line );

    checkExtensionsPluginRunProgressBroadcast( "started", line );

    checkExtensionsPluginRunProgressBroadcast( "finished", line );
}

JsonObject HubModesWalkTests::checkUiBroadcast( int line ) {
    JsonObject broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", "line: " + std::to_string( line ) );
    EXPECT_EQ( broadcast.empty(), false ) << __FILE__ << ":" << getCodeLineString( line );
    JsonObject params;
    EXPECT_TRUE( json_get_obj( broadcast, "result", params ) ) << "line: " << getCodeLineString( line );
    return params;
}

TEST_F( HubModesWalkTests, testStartAutomaticWalkTest ) {  // 1

    auto& dbClient = nma().getDBClient();
    auto& coredRemote = makeRemote();
    initHouseMode();
    // start walk test
    startWalkTest( false, true, __LINE__ );
    bool expectProtectNotification = false;
    setItemUpdate( expectProtectNotification, __LINE__ );
    stopWalkTest( false, __LINE__ );

    // walk test finished:
    expectProtectNotification = true;
    setItemUpdate( expectProtectNotification, __LINE__ );
    // check house mode mashbot broadcasts:

    Json broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.alarmed" );
    Json result = broadcast[ "result" ];
    ASSERT_NE( result, nullptr );
    EXPECT_EQ( json_get_str( result, "modeId" ), "2" );
    EXPECT_EQ( json_get_str( result, "phase" ), "entryDelay" );
    EXPECT_EQ( json_get_str( result, "status" ), "begin" );

    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.alarmed" );
    result = broadcast[ "result" ];
    ASSERT_NE( result, nullptr );
    EXPECT_EQ( json_get_str( result, "modeId" ), "2" );
    EXPECT_EQ( json_get_str( result, "phase" ), "entryDelay" );
    EXPECT_EQ( json_get_str( result, "status" ), "done" );

    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.alarmed" );
    result = broadcast[ "result" ];
    ASSERT_NE( result, nullptr );
    EXPECT_EQ( json_get_str( result, "modeId" ), "2" );
    EXPECT_EQ( json_get_str( result, "phase" ), "main" );
    EXPECT_EQ( json_get_str( result, "status" ), "begin" );
}

TEST_F( HubModesWalkTests, checkHubModesGet ) {  // 2
    auto& dbClient = nma().getDBClient();
    auto& coredRemote = makeRemote();
    startWalkTest( false, true, __LINE__ );
    checkHubModesInfo( __LINE__ );
    stopWalkTest( false, __LINE__ );
}

TEST_F( HubModesWalkTests, checkAlreadyStartedError ) {  // 3
    auto& dbClient = nma().getDBClient();
    auto& coredRemote = makeRemote();
    startWalkTest( false, true, __LINE__ );
    Json result = startWalkTest( true, true, __LINE__ );
    Json error;
    json_get( result, "error", error );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "walk.test.already.started" );
    EXPECT_EQ( json_get_str( error, "message" ), "Walk test already started" );
    stopWalkTest( false, __LINE__ );
}

TEST_F( HubModesWalkTests, checkStopNotStartedTestError ) {  // 4
    auto& dbClient = nma().getDBClient();
    auto& coredRemote = makeRemote();
    Json result = stopWalkTest( true, __LINE__ );
    Json error;
    json_get( result, "error", error );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "walk.test.not.started" );
    EXPECT_EQ( json_get_str( error, "message" ), "Walk test not started" );
}

TEST_F( HubModesWalkTests, checkStopWithNoExistOperationIdError ) {  // 5
    auto& dbClient = nma().getDBClient();
    auto& coredRemote = makeRemote();
    startWalkTest( false, true, __LINE__ );
    Json result = stopWalkTest( "operationIdError", true, true, __LINE__ );
    Json error;
    json_get( result, "error", error );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "walk.test.incorrect.id" );
    EXPECT_EQ( json_get_str( error, "message" ), "Walk test incorrect operationId" );
    stopWalkTest( false, __LINE__ );
}

TEST_F( HubModesWalkTests, extendTest ) {  // 6
    auto& dbClient = nma().getDBClient();
    auto& coredRemote = makeRemote();
    startWalkTest( false, true, __LINE__ );
    Json result = extendWalkTest( true, __LINE__ );
    Json error;
    json_get( result, "error", error );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "walk.test.outside.extend.period" );
    EXPECT_EQ( json_get_str( error, "message" ),
               "Walk test extend is not permitted outside of the termination period" );
    stopWalkTest( false, __LINE__ );
}

TEST_F( HubModesWalkTests, testLuaEvents ) {  // 7
    executeScript( Resources::SubscribeScript, __LINE__ );
    auto& dbClient = nma().getDBClient();
    auto& coredRemote = makeRemote();
    startWalkTest( false, false, __LINE__ );

    Json broadcast;
    auto checkers =
        CommonNmaTests::BroadcastCheckers{ { "hub.modes.system.test.progress",
                                             [&]( const Json& broadcast, int line ) {
                                                 auto result = broadcast[ "result" ];
                                                 EXPECT_EQ( json_get_int( result, "completed" ), 0 );
                                                 EXPECT_EQ( json_get_str( result, "operationId" ), "operationId" );
                                                 EXPECT_EQ( json_get_str( result, "status" ), "started" );
                                                 EXPECT_EQ( json_get_str( result, "type" ), "walk" );
                                                 EXPECT_EQ( json_get_bool( result, "protectNotification" ), true );
                                             } },
                                           { "hub.extensions.plugin.ui_broadcast",
                                             [&]( const Json& broadcast, int line ) {
                                                 auto result = broadcast[ "result" ];
                                                 EXPECT_EQ( json_get_str( result, "plugin" ), "house_modes_test" );
                                                 EXPECT_EQ( json_get_str( result, "event" ), "system_test_started" );
                                                 EXPECT_EQ( json_get_str( result, "operation_id" ), "operationId" );
                                                 EXPECT_EQ( json_get_str( result, "type" ), "walk" );
                                                 EXPECT_EQ( json_get_int( result, "time_left" ), 900 );
                                             } } };

    checkBroadcasts( checkers, __LINE__ );
    stopWalkTest( "operationId", false, false, __LINE__ );
    checkers =
        CommonNmaTests::BroadcastCheckers{ { "hub.modes.system.test.progress",
                                             [&]( const Json& broadcast, int line ) {
                                                 auto result = broadcast[ "result" ];
                                                 EXPECT_EQ( json_get_int( result, "completed" ), 100 );
                                                 EXPECT_EQ( json_get_str( result, "operationId" ), "operationId" );
                                                 EXPECT_EQ( json_get_str( result, "status" ), "finished" );
                                                 EXPECT_EQ( json_get_str( result, "type" ), "walk" );
                                                 EXPECT_EQ( json_get_bool( result, "protectNotification" ), true );
                                             } },
                                           { "hub.extensions.plugin.ui_broadcast",
                                             [&]( const Json& broadcast, int line ) {
                                                 auto result = broadcast[ "result" ];
                                                 EXPECT_EQ( json_get_str( result, "plugin" ), "house_modes_test" );
                                                 EXPECT_EQ( json_get_str( result, "event" ), "system_test_finished" );
                                                 EXPECT_EQ( json_get_str( result, "operation_id" ), "operationId" );
                                                 EXPECT_EQ( json_get_str( result, "type" ), "walk" );
                                                 EXPECT_EQ( json_get_int( result, "time_left" ), 900 );
                                             } } };
    checkBroadcasts( checkers, __LINE__ );
    executeScript( Resources::UnSubscribeScript, __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
