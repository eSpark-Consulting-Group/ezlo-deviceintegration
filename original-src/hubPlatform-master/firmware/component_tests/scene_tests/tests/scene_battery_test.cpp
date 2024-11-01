#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"

#include "common_test_resources/resources/scene_manager_resources.h"
#include "hardware_manager/battery/battery_info.h"
#include "database_client/dbtools.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN

DONE        1.1 when block isBatteryState
DONE            1.1.1 Success case
DONE            1.1.2 Errors
DONE        1.2 when block isBatteryLevel
DONE            1.2.1 Success case 1
DONE            1.2.2 Success case 2
DONE            1.2.3 Success case 3
DONE            1.2.4 Success case 4
DONE            1.2.5 Success case 5
DONE            1.2.6 Success case 6
DONE            1.2.7 Success case 7
DONE            1.2.8 Success case 8
DONE            1.2.9 Errors
*/

class SceneBatteryTests : public SceneManagerTestsFixture {
public:
    void SetUp() override {
        SceneManagerTestsFixture::SetUp();
        callingFile( __FILE__ );
    }
};

TEST_F( SceneBatteryTests, isBatteryState ) {  // 1.1.1
    auto& coreRemoteInterface = makeRemote( nma() );

    auto checkBatteryCondition = replaceTokensInString( checkBatteryConditionBlock, { { "__STATE__", "full" } } );

    auto checkBatteryStatusScene = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isBatteryState" }, { "__WHEN_BLOCKS__", checkBatteryCondition }, { "__THEN_BLOCKS__", "" } } );

    auto sceneId = createScene( checkBatteryStatusScene );

    HubPlatform::HardwareManager::BatteryInfo info;

    info.status = HubPlatform::HardwareManager::BatteryStatus::Charging;
    nma().sendBroadcast( HUB_BATTERY_INFO_UPDATED, info.toJson() );

    info.status = HubPlatform::HardwareManager::BatteryStatus::Full;
    nma().sendBroadcast( HUB_BATTERY_INFO_UPDATED, info.toJson() );

    testRunProgressBroadcast( checkBatteryStatusScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( checkBatteryStatusScene, sceneId, "finished", __LINE__ );
}

TEST_F( SceneBatteryTests, isBatteryStateError ) {  // 1.1.2
    auto& coreRemoteInterface = makeRemote( nma() );

    auto checkBatteryCondition = replaceTokensInString( checkBatteryConditionBlock, { { "__STATE__", "" } } );

    auto checkBatteryStatusScene = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isBatteryState" }, { "__WHEN_BLOCKS__", checkBatteryCondition }, { "__THEN_BLOCKS__", "" } } );

    auto answer = nma().sendtoHubAndGetAnswer( checkBatteryStatusScene );
    auto error = answer[ "error" ];
    EXPECT_FALSE( error.empty() ) << error.dump( 4 );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.block.when.wrong" );
    EXPECT_EQ( json_get_str( error, "message" ), "Scene is ill-formed. Can't parse when block" );
}

TEST_F( SceneBatteryTests, isBatteryLevelTest1 ) {  // 1.2.1
    testBatteryLevelWithComparator( ">", 30, 10, __LINE__ );
}

TEST_F( SceneBatteryTests, isBatteryLevelTest2 ) {  // 1.2.2
    testBatteryLevelWithComparator( "<", 10, 30, __LINE__ );
}

TEST_F( SceneBatteryTests, isBatteryLevelTest3 ) {  // 1.2.3
    testBatteryLevelWithComparator( ">=", 30, 10, __LINE__ );
}

TEST_F( SceneBatteryTests, isBatteryLevelTest4 ) {  // 1.2.4
    testBatteryLevelWithComparator( ">=", 20, 10, __LINE__ );
}

TEST_F( SceneBatteryTests, isBatteryLevelTest5 ) {  // 1.2.5
    testBatteryLevelWithComparator( "==", 20, 30, __LINE__ );
}

TEST_F( SceneBatteryTests, isBatteryLevelTest6 ) {  // 1.2.6
    testBatteryLevelWithComparator( "<=", 10, 30, __LINE__ );
}

TEST_F( SceneBatteryTests, isBatteryLevelTest7 ) {  // 1.2.7
    testBatteryLevelWithComparator( "<=", 20, 30, __LINE__ );
}

TEST_F( SceneBatteryTests, isBatteryLevelTest8 ) {  // 1.2.8
    testBatteryLevelWithComparator( "!=", 10, 20, __LINE__ );
}

TEST_F( SceneBatteryTests, isBatteryLevelError ) {  // 1.2.9
    auto& coreRemoteInterface = makeRemote( nma() );
    auto checkBatteryCondition = replaceTokensInString( checkBatteryLevelConditionBlock, { { "__COMP__", "" } } );
    auto checkBatteryLevelScene = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isBatteryLevel" }, { "__WHEN_BLOCKS__", checkBatteryCondition }, { "__THEN_BLOCKS__", "" } } );

    auto answer = nma().sendtoHubAndGetAnswer( checkBatteryLevelScene );
    auto error = answer[ "error" ];
    EXPECT_FALSE( error.empty() ) << error.dump( 4 );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.block.when.wrong" );
    EXPECT_EQ( json_get_str( error, "message" ), "Scene is ill-formed. Can't parse when block" );
}

}  // namespace Tests
}  // namespace HubPlatform
