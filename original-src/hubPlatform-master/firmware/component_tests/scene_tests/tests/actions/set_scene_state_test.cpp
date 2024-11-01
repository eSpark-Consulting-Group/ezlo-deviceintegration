#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"

/**
 * @case_01 Disable another scene
 * @case_01 Invalid "sceneId" or "enabled" fields
 */

namespace HubPlatform {
namespace Tests {

class SceneActionBlockTest_setSceneState : public SceneManagerTestsFixture {
public:
    void SetUp() override {
        SceneManagerTestsFixture::SetUp();
        callingFile( __FILE__ );
    }
};

/// @case_01
/// @brief
/// setSceneState action is a method of action block
/// Enables or disables selected scene
/// The test verifies a valid request that disables a scene
TEST_F( SceneActionBlockTest_setSceneState, disableScene ) {
    auto scene1Request = createEmptyScene;
    SceneID scene1Id = createScene( scene1Request );

    auto scene2Request = createEmptyScene;
    scene2Request[ "params" ][ "then" ].push_back(
        replaceTokens( setSceneStateThenBlock, { { "__SCENE_ID__", scene1Id }, { "__ENABLED__", "false" } } ) );

    SceneID scene2Id = createScene( scene2Request );

    runSceneManually( scene2Id, __LINE__ );

    checkBroadcasts(
        { [&]( const Json& broadcast ) {
             return isRunProgressBroadcast( broadcast, scene2Request, scene2Id, "started" );
         },
          [&]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, scene2Request, scene2Id, "finished" );
          },
          [&]( const Json& broadcast ) { return isEnableChangedBroadcast( broadcast, scene1Id, false ); } },
        __LINE__ );
}

/// @case_02
/// @brief
/// setSceneState action is a method of action block
/// Enables or disables selected scene
/// The test verifies a request with invalid "sceneId" or "enabled" field values
TEST_F( SceneActionBlockTest_setSceneState, invalidArgs ) {
    auto scene1Request = createEmptyScene;

    scene1Request[ "params" ][ "then" ].push_back(
        replaceTokens( setSceneStateThenBlock, { { "__SCENE_ID__", "" }, { "__ENABLED__", "false" } } ) );

    auto createResponse = nma().sendtoHubAndGetAnswer( scene1Request );
    EXPECT_EQ( json_get_str( createResponse[ "error" ], "data" ), "ezlo.scenes.not.exist" )
        << "response=" << createResponse[ "error" ];

    SceneID scene2Id = createScene( createEmptyScene );

    scene1Request[ "params" ][ "then" ][ 0 ][ "fields" ][ 0 ][ "value" ] = scene2Id;              // sceneId
    scene1Request[ "params" ][ "then" ][ 0 ][ "fields" ][ 1 ][ "value" ] = "invalid_bool_value";  // enabled

    createResponse = nma().sendtoHubAndGetAnswer( scene1Request );
    EXPECT_EQ( json_get_str( createResponse[ "error" ], "data" ), "rpc.params.wrong_field" )
        << "response=" << createResponse[ "error" ];
}

}  // namespace Tests
}  // namespace HubPlatform
