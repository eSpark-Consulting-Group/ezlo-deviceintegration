#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"

#include "common_test_resources/resources/scene_manager_resources.h"
#include "hardware_manager/battery/battery_info.h"
#include "database_client/dbtools.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN

DONE        1.1 when block isFirmwareUpdate
DONE            1.1.1 Success case
DONE            1.1.2 Errors
*/

class SceneFirmwareUpdateTests : public SceneManagerTestsFixture {
public:
    void SetUp() override {
        SceneManagerTestsFixture::SetUp();
        callingFile( __FILE__ );
    }
};

TEST_F( SceneFirmwareUpdateTests, isSoftwareUpdateState ) {  // 1.1.1
    auto& coreRemoteInterface = makeRemote( nma() );

    auto softwareUpdateCondition = replaceTokensInString( softwareUpdateConditionBlock, { { "__STATE__", "done" } } );

    auto softwareUpdateScene = replaceTokens( createSceneRequest,
                                              { { "__NAME__", "isFirmwareUpdateState" },
                                                { "__WHEN_BLOCKS__", softwareUpdateCondition },
                                                { "__THEN_BLOCKS__", "" } } );

    auto sceneId = createScene( softwareUpdateScene );

    JsonObject data;
    json_add_null( data, "error" );
    json_add_str( data, "operationId", "operationId" );
    json_add_str( data, "serial", "Serial" );
    json_add_int( data, "completed", 100 );
    json_add_str( data, "status", "finished" );

    nma().sendBroadcast( UPDATING_SOFTWARE, data, GroupsNames::UPDATE_MANAGER_BROADCASTS );

    auto broadcast = waitForBroadcast( "hub.software.update.progress", __FILE__, __LINE__ );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.software.update.progress" ) << __LINE__;

    testRunProgressBroadcast( softwareUpdateScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( softwareUpdateScene, sceneId, "finished", __LINE__ );

    nma().sendBroadcast( PLANNING_SOFTWARE_UPDATE, data, GroupsNames::UPDATE_MANAGER_BROADCASTS );

    broadcast = waitForBroadcast( "hub.software.update.actions", __FILE__, __LINE__ );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.software.update.actions" ) << __LINE__;

    testRunProgressBroadcast( softwareUpdateScene, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( softwareUpdateScene, sceneId, "finished", __LINE__ );
}

TEST_F( SceneFirmwareUpdateTests, isSoftwareUpdateStateFail ) {  // 1.1.2
    auto& coreRemoteInterface = makeRemote( nma() );

    auto softwareUpdateCondition = replaceTokensInString( softwareUpdateConditionBlock, { { "__STATE__", "done" } } );

    auto softwareUpdateScene = replaceTokens( createSceneRequest,
                                              { { "__NAME__", "isFirmwareUpdateState" },
                                                { "__WHEN_BLOCKS__", softwareUpdateCondition },
                                                { "__THEN_BLOCKS__", "" } } );

    auto sceneId = createScene( softwareUpdateScene );

    JsonObject data;
    json_add_null( data, "error" );
    json_add_str( data, "operationId", "operationId" );
    json_add_str( data, "serial", "Serial" );
    json_add_int( data, "completed", 100 );
    json_add_str( data, "status", "started" );

    nma().sendBroadcast( UPDATING_SOFTWARE, data, GroupsNames::UPDATE_MANAGER_BROADCASTS );

    auto broadcast = waitForBroadcast( "hub.software.update.progress", __FILE__, __LINE__ );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.software.update.progress" ) << __LINE__;

    nma().sendBroadcast( PLANNING_SOFTWARE_UPDATE, data, GroupsNames::UPDATE_MANAGER_BROADCASTS );

    broadcast = waitForBroadcast( "hub.software.update.actions", __FILE__, __LINE__ );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.software.update.actions" ) << __LINE__;
}

}  // namespace Tests
}  // namespace HubPlatform
