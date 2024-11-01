#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "common_test_resources/resources/cored_resources.h"
#include "cored/model/resources/cored_resources.h"

#include "jsonrpc/jsonrpc.h"
#include <gtest/gtest.h>

#include "tiny/tiny_consolecolors.h"

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN

DONE            1 isDeviceState trigger test 'armed' success
DONE            2 isDeviceState trigger test 'reachable' success
DONE            3 isDeviceState trigger test event initialize success
DONE            4 isDeviceState trigger test event test both events combination with reset
DONE            5 isDeviceState trigger test failed (bad input params checking)
*/

class SceneManagerDeviceStateTests : public SceneManagerTestsFixture {
public:
    void SetUp() override;
    void TearDown() override;
};

void SceneManagerDeviceStateTests::SetUp() {
    SceneManagerTestsFixture::SetUp();
    callingFile( __FILE__ );
}
void SceneManagerDeviceStateTests::TearDown() {
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

TEST_F( SceneManagerDeviceStateTests, isDeviceStateTriggeredArmedSuccess ) /*1*/ {
    using namespace CoreD::Model::Resources;
    auto& coreRemoteInterface = makeRemote();
    auto deviceId = createSecuritySensorDevice( "testDevice", "smoke" );

    auto isDeviceState = replaceTokensInString( isDeviceStateWhenBlockArmed,
                                                { { "__DEVICE_ID__", deviceId }, { "__ARMED__", "false" } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isDeviceState" }, { "__WHEN_BLOCKS__", isDeviceState }, { "__THEN_BLOCKS__", "" } } );
    auto sceneId = createScene( request, __LINE__ );

    // set armed
    coreRemoteInterface.setDeviceArmed( deviceId, false );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, false } }, __LINE__ );

    // triggered
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    coreRemoteInterface.setDeviceArmed( deviceId, true );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, true } }, __LINE__ );

    deleteScene( sceneId, __LINE__ );
}

TEST_F( SceneManagerDeviceStateTests, isDeviceStateTriggeredReachableSuccess ) /*2*/ {
    using namespace CoreD::Model::Resources;
    auto& coreRemoteInterface = makeRemote();

    auto deviceId = createSecuritySensorDevice( "testDevice", "smoke" );
    auto isDeviceState = replaceTokensInString( isDeviceStateWhenBlockReachable,
                                                { { "__DEVICE_ID__", deviceId }, { "__REACHABLE__", "false" } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isDeviceState" }, { "__WHEN_BLOCKS__", isDeviceState }, { "__THEN_BLOCKS__", "" } } );
    auto sceneId = createScene( request, __LINE__ );

    // set reachable
    coreRemoteInterface.updateReachableState( deviceId, false );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ReachableFieldName, false } }, __LINE__ );

    // triggered
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );
}

TEST_F( SceneManagerDeviceStateTests, isDeviceStateEventInitializeSuccess ) /*3*/ {
    using namespace CoreD::Model::Resources;
    auto& coreRemoteInterface = makeRemote();
    auto deviceId = createSecuritySensorDevice( "testDevice", "smoke" );
    {
        auto isDeviceState = replaceTokensInString(
            isDeviceStateWhenBlock,
            { { "__DEVICE_ID__", deviceId }, { "__REACHABLE__", "true" }, { "__ARMED__", "true" } } );
        auto request = replaceTokens(
            createSceneRequest,
            { { "__NAME__", "isDeviceState" }, { "__WHEN_BLOCKS__", isDeviceState }, { "__THEN_BLOCKS__", "" } } );
        auto sceneId = createScene( request, __LINE__ );

        auto sceneState = getSceneStatus( sceneId, __LINE__ );

        JsonObject result;
        JsonObject status;
        JsonArray whenBlocks;
        EXPECT_TRUE( json_get_obj( sceneState, "result", result ) ) << getCodeLineString( __LINE__ );
        EXPECT_TRUE( json_get_obj( result, "status", status ) ) << getCodeLineString( __LINE__ );
        EXPECT_TRUE( json_get_arr( status, "when", whenBlocks ) ) << getCodeLineString( __LINE__ );

        for( auto block : whenBlocks ) {
            std::string state;
            EXPECT_TRUE( json_get( block, "state", state ) ) << getCodeLineString( __LINE__ );
            EXPECT_EQ( state, "true" ) << getCodeLineString( __LINE__ );
        }

        const std::string Name = "Name1";

        // invoke update device
        coreRemoteInterface.updateDeviceName( deviceId, Name );
        checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::NameFieldName, Name } }, __LINE__ );

        // Created device with 'true' states for reachable and armed params but event is no triggered.
        deleteScene( sceneId, __LINE__ );
    }
}

TEST_F( SceneManagerDeviceStateTests, testArmedAndReachableEventsCombination ) /*4*/ {
    using namespace CoreD::Model::Resources;
    auto& coreRemoteInterface = makeRemote();
    auto deviceId = createSecuritySensorDevice( "testDevice", "smoke" );

    auto isDeviceState = replaceTokensInString(
        isDeviceStateWhenBlock,
        { { "__DEVICE_ID__", deviceId }, { "__ARMED__", "false" }, { "__REACHABLE__", "false" } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isDeviceState" }, { "__WHEN_BLOCKS__", isDeviceState }, { "__THEN_BLOCKS__", "" } } );
    auto sceneId = createScene( request, __LINE__ );

    // set armed
    coreRemoteInterface.setDeviceArmed( deviceId, false );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, false } }, __LINE__ );
    // there is no triggered

    // set reachable
    coreRemoteInterface.updateReachableState( deviceId, false );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ReachableFieldName, false } }, __LINE__ );

    // triggered
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    // reset armed (reset event trigger)
    coreRemoteInterface.setDeviceArmed( deviceId, true );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, true } }, __LINE__ );

    // set armed again
    coreRemoteInterface.setDeviceArmed( deviceId, false );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, false } }, __LINE__ );

    // triggered
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    coreRemoteInterface.setDeviceArmed( deviceId, true );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, true } }, __LINE__ );

    deleteScene( sceneId, __LINE__ );
}

TEST_F( SceneManagerDeviceStateTests, isDeviceStateTriggeredFailed ) /*5*/ {
    const std::string deviceId = "112233";
    auto isDeviceState = replaceTokensInString(
        isDeviceStateWhenBlock,
        { { "__DEVICE_ID__", deviceId }, { "__ARMED__", "123" }, { "__REACHABLE__", "false" } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isDeviceState" }, { "__WHEN_BLOCKS__", isDeviceState }, { "__THEN_BLOCKS__", "" } } );

    sendRequestAndCheckFailedAnswer( request, Errors::BAD_PARAMS, "armed", __LINE__ );
    isDeviceState =
        replaceTokensInString( isDeviceStateWhenBlock,
                               { { "__DEVICE_ID__", deviceId }, { "__ARMED__", "true" }, { "__REACHABLE__", "123" } } );
    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isDeviceState" }, { "__WHEN_BLOCKS__", isDeviceState }, { "__THEN_BLOCKS__", "" } } );

    sendRequestAndCheckFailedAnswer( request, Errors::BAD_PARAMS, "reachable", __LINE__ );
    isDeviceState = replaceTokensInString( isDeviceStateWhenBlockFailed, { { "__DEVICE_ID__", "100500" } } );
    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isDeviceState" }, { "__WHEN_BLOCKS__", isDeviceState }, { "__THEN_BLOCKS__", "" } } );

    sendRequestAndCheckFailedAnswer( request, Errors::BAD_PARAMS, "device", __LINE__ );
    isDeviceState = replaceTokensInString( isDeviceStateWhenBlockFailed, { { "__DEVICE_ID__", R"("100500")" } } );
    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isDeviceState" }, { "__WHEN_BLOCKS__", isDeviceState }, { "__THEN_BLOCKS__", "" } } );
    sendRequestAndCheckFailedAnswer( request, Errors::BAD_PARAMS, "Not found condition params", __LINE__ );

    request = replaceTokens( createSceneRequest,
                             { { "__NAME__", "isDeviceState" },
                               { "__WHEN_BLOCKS__", isDeviceStateWhenBlockFailedDevice },
                               { "__THEN_BLOCKS__", "" } } );

    sendRequestAndCheckFailedAnswer( request, Errors::BAD_PARAMS, "Not found 'device' parameter", __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
