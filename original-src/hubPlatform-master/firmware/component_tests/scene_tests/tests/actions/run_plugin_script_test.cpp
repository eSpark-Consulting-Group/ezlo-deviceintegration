#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"

/**

TEST-PLAN

DONE    1. run plugin script
DONE    2. invalid script path, scene failed

 */

namespace HubPlatform {
namespace Tests {

class RunPluginScriptActionTest : public SceneManagerTestsFixture {
public:
    void SetUp() override;
};

void RunPluginScriptActionTest::SetUp() {
    SceneManagerTestsFixture::SetUp();
    callingFile( __FILE__ );
}

TEST_F( RunPluginScriptActionTest, runScript ) {
    CoreDRemoteInterface::CoreDInterface& coredRemote = makeRemote();

    // return devices list in a broadcast
    const std::string scriptId = "HUB:item_control/scripts/get_devices_list";
    const Json scriptParams = { { "testSelector", "get_devices" } };
    Json sceneJson = createEmptyScene;
    sceneJson[ "params" ][ "name" ] = "runPluginScript/runScript";
    Json runPluginScript = replaceTokens(
        runPluginScriptThenBlock, { { "__SCRIPT_ID__", scriptId }, { "__UNQUOTED_OBJECT__", scriptParams.dump() } } );
    sceneJson[ "params" ][ "then" ].push_back( std::move( runPluginScript ) );

    Json sceneResult;
    const SceneID sceneId = createScene( sceneJson, sceneResult, __LINE__ );

    // test scene
    runSceneManually( sceneId, __LINE__ );

    testRunProgressBroadcast( sceneJson, sceneId, "started", __LINE__ );
    checkBroadcasts( { [&]( const Json& broadcast ) {
                          std::string subclass = json_get_str( broadcast, "msg_subclass" );
                          return subclass == "hub.extensions.plugin.ui_broadcast";
                      },
                       [&]( const Json& broadcast ) {
                           return isRunProgressBroadcast( broadcast, sceneJson, sceneId, "finished" );
                       } },
                     __LINE__ );

    deleteScene( sceneId, __LINE__ );
}

TEST_F( RunPluginScriptActionTest, scriptError ) {
    CoreDRemoteInterface::CoreDInterface& coredRemote = makeRemote();

    // return devices list in a broadcast
    const std::string scriptId = "HUB:item_control/scripts/non_existent_script_name";
    const Json scriptParams = { { "testSelector", "get_devices" } };
    Json sceneJson = createEmptyScene;
    sceneJson[ "params" ][ "name" ] = "runPluginScript/scriptError";

    Json runPluginScript = replaceTokens(
        runPluginScriptThenBlock, { { "__SCRIPT_ID__", scriptId }, { "__UNQUOTED_OBJECT__", scriptParams.dump() } } );
    sceneJson[ "params" ][ "then" ].push_back( std::move( runPluginScript ) );

    Json sceneResult;
    const SceneID sceneId = createScene( sceneJson, sceneResult, __LINE__ );

    // test scene
    runSceneManually( sceneId, __LINE__ );

    testRunProgressBroadcast( sceneJson, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( sceneJson, sceneId, "failed", __LINE__ );

    deleteScene( sceneId, __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
