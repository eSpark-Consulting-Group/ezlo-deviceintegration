#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"

/**
 * @case_01 Request to run "then" block
 * @case_02 Request to run "thenGroups" block
 * @case_03 Invalid "sceneId" field value
 * @case_04 Invalid action group ID
 */

namespace HubPlatform {
namespace Tests {

class SceneActionBlockTest_runScene : public SceneManagerTestsFixture {
public:
    void SetUp() override {
        SceneManagerTestsFixture::SetUp();
        callingFile( __FILE__ );
    }
};

/// @case_01
/// @brief
/// runScene action is a method of action block
/// Executes another scene by its id
/// The test verifies a valid request to run then block
TEST_F( SceneActionBlockTest_runScene, thenBlock ) {
    auto scene1Request = createEmptyScene;
    SceneID scene1Id = createScene( scene1Request );

    auto scene2Request = createEmptyScene;
    scene2Request[ "params" ][ "then" ].push_back(
        replaceTokens( runSceneThenBlock, { { "__SCENE_ID__", scene1Id } } ) );

    SceneID scene2Id = createScene( scene2Request );

    runSceneManually( scene2Id, __LINE__ );

    checkBroadcasts( { [&]( const Json& broadcast ) {
                          return isRunProgressBroadcast( broadcast, scene1Request, scene1Id, "started" );
                      },
                       [&]( const Json& broadcast ) {
                           return isRunProgressBroadcast( broadcast, scene1Request, scene1Id, "finished" );
                       },
                       [&]( const Json& broadcast ) {
                           return isRunProgressBroadcast( broadcast, scene2Request, scene2Id, "started" );
                       },
                       [&]( const Json& broadcast ) {
                           return isRunProgressBroadcast( broadcast, scene2Request, scene2Id, "finished" );
                       } },
                     __LINE__ );
}

/// @case_02
/// @brief
/// runScene is an action block, it executes another scene by its id
/// The test verifies runScene that executes thenGroups block
TEST_F( SceneActionBlockTest_runScene, thenGroups ) {
    auto dev1 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );

    auto scene1Request = createEmptyScene;

    std::string actionGroupIdOfWhenBlock = "0";

    auto whenBlock = replaceTokens(
        isItemStateWhenBlock, { { "__ITEM_ID__", dev1.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "10" } } );

    // Action group will be linked to this when block
    whenBlock[ "actionGroup" ] = actionGroupIdOfWhenBlock;

    scene1Request[ "params" ][ "when" ].push_back( std::move( whenBlock ) );
    scene1Request[ "params" ][ "thenGroups" ][ actionGroupIdOfWhenBlock ].push_back( replaceTokens(
        setItemValueThenBlock, { { "__ITEM_ID__", dev1.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "12" } } ) );

    Json scene1CreateResult;
    auto scene1Id = createScene( scene1Request, scene1CreateResult, __LINE__ );

    actionGroupIdOfWhenBlock = json_get_str( scene1CreateResult[ "when" ][ 0 ], "actionGroup" );

    auto scene2Request = createEmptyScene;
    scene2Request[ "params" ][ "then" ].push_back( replaceTokens(
        runSceneThenGroupsBlock, { { "__SCENE_ID__", scene1Id }, { "__GROUP__", actionGroupIdOfWhenBlock } } ) );

    SceneID scene2Id = createScene( scene2Request );

    runSceneManually( scene2Id, __LINE__ );

    checkBroadcasts(
        { [&]( const Json& broadcast ) {
             return isRunProgressBroadcast( broadcast, scene2Request, scene2Id, "started" );
         },
          [&]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, scene2Request, scene2Id, "finished" );
          },
          [this, &dev1]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, dev1.m_itemId, 12 ); } },
        __LINE__ );
}

/// @case_03
/// @brief
/// runScene is an action block, it executes another scene by its id
/// The test verifies a request with invalid "sceneId" field value
TEST_F( SceneActionBlockTest_runScene, invalidSceneId ) {
    auto sceneRequest = createEmptyScene;

    sceneRequest[ "params" ][ "then" ].push_back( replaceTokens( runSceneThenBlock, { { "__SCENE_ID__", "" } } ) );

    auto response = nma().sendtoHubAndGetAnswer( sceneRequest );

    EXPECT_EQ( json_get_str( response[ "error" ], "data" ), "ezlo.scenes.not.exist" )
        << "response=" << response[ "error" ];
}

/// @case_04
/// @brief
/// runScene is an action block, it executes another scene by its id
/// The test verifies a request to run thenGroups block with invalid "group" field value
TEST_F( SceneActionBlockTest_runScene, invalidActionGroupId ) {
    auto scene1Request = createEmptyScene;
    SceneID scene1Id = createScene( scene1Request );

    auto scene2Request = createEmptyScene;
    scene2Request[ "params" ][ "then" ].push_back( replaceTokens(
        runSceneThenGroupsBlock, { { "__SCENE_ID__", scene1Id }, { "__GROUP__", "not_existing_action_group" } } ) );

    auto response = nma().sendtoHubAndGetAnswer( scene2Request );

    EXPECT_EQ( json_get_str( response[ "error" ], "data" ), "ezlo.scenes.block.groups.not.exist" )
        << "response=" << response[ "error" ];
}

}  // namespace Tests
}  // namespace HubPlatform
