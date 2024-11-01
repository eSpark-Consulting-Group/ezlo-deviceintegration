#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"

/**
 * isGroupState is a method of "when" block
 * It checks the state of an individual when block of another scene by its ID.
 *
 * @case_01 Trigger scene on TRUE state of a group of another scene
 * @case_02 Trigger scene when the state of a group of another scene is CHANGED
 * @case_03 Attempt to create isGroupState pointing to a non-existing scene
 * @case_04 Attempt to create isGroupState pointing to a non-existing scene group
 */

namespace HubPlatform {
namespace Tests {

class SceneEventTest_isGroupState : public SceneManagerTestsFixture {
public:
    void SetUp() override {
        SceneManagerTestsFixture::SetUp();
        callingFile( __FILE__ );
    }
};

/// @case_01
/// @brief
/// The test verifies that isGroupState is triggered for TRUE state of a group of another scene
TEST_F( SceneEventTest_isGroupState, triggerTrueState ) {
    auto dev1 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    auto dev2 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );

    auto scene1Request = createEmptyScene;
    scene1Request[ "params" ][ "when" ].push_back( replaceTokens(
        isItemStateWhenBlock, { { "__ITEM_ID__", dev1.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "10" } } ) );

    Json scene1CreateResult;
    auto scene1Id = createScene( scene1Request, scene1CreateResult, __LINE__ );
    auto groupId = json_get_str( scene1CreateResult[ "when" ][ 0 ], "_id" );

    auto scene2Request = createEmptyScene;
    scene2Request[ "params" ][ "when" ].push_back( replaceTokens(
        isGroupStateWhenBlock, { { "__SCENE__", scene1Id }, { "__GROUP__", groupId }, { "__STATE__", "true" } } ) );
    scene2Request[ "params" ][ "then" ].push_back( replaceTokens(
        setItemValueThenBlock, { { "__ITEM_ID__", dev2.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "11" } } ) );

    auto scene2Id = createScene( scene2Request, __LINE__ );

    auto& coreRemoteInterface = makeRemote( nma() );

    // This will trigger isItemState of scene1 (true) and also isGroupState of scene2 (true)
    setItemValue( coreRemoteInterface, dev1.m_itemId, 10, __LINE__ );

    // Expect both scenes to be triggered, but only scene2 makes side effects, because scene1 doesn't have "then"
    // actions
    checkBroadcasts(
        { [&]( const Json& broadcast ) {
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
          },
          [this, &dev2]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, dev2.m_itemId, 11 ); } },
        __LINE__ );
}

/// @case_02
/// @brief
/// The test verifies that isGroupState is triggered when the state of a group of another scene is CHANGED
TEST_F( SceneEventTest_isGroupState, triggerChangedState ) {
    auto dev1 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    auto dev2 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );

    auto scene1Request = createEmptyScene;
    scene1Request[ "params" ][ "when" ].push_back( replaceTokens(
        isItemStateWhenBlock, { { "__ITEM_ID__", dev1.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "10" } } ) );

    Json scene1CreateResult;
    auto scene1Id = createScene( scene1Request, scene1CreateResult, __LINE__ );
    auto groupId = json_get_str( scene1CreateResult[ "when" ][ 0 ], "_id" );

    auto scene2Request = createEmptyScene;
    scene2Request[ "params" ][ "when" ].push_back( replaceTokens(
        isGroupStateWhenBlock, { { "__SCENE__", scene1Id }, { "__GROUP__", groupId }, { "__STATE__", "changed" } } ) );
    scene2Request[ "params" ][ "then" ].push_back( replaceTokens(
        setItemValueThenBlock, { { "__ITEM_ID__", dev2.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "11" } } ) );

    auto scene2Id = createScene( scene2Request, __LINE__ );

    auto& coreRemoteInterface = makeRemote( nma() );

    // This won't trigger scene2 because the initial state of scene1 (false) remains unchanged
    setItemValue( coreRemoteInterface, dev1.m_itemId, 9, __LINE__ );

    // No other broadcasts expected, because neither scene1 nor scene2 have any "else" actions

    // This will trigger isItemState of scene1, and also it will cause state transition false -> true
    // Thus isGroupState of scene2 will also be triggered(true)
    setItemValue( coreRemoteInterface, dev1.m_itemId, 10, __LINE__ );

    checkBroadcasts(
        { [&]( const Json& broadcast ) {
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
          },
          [this, &dev2]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, dev2.m_itemId, 11 ); } },
        __LINE__ );
}

/// @case_03
/// @brief
/// The test verifies that a scene is not created if scene ID is not correct
TEST_F( SceneEventTest_isGroupState, invalidSceneId ) {
    auto request = createEmptyScene;

    request[ "params" ][ "when" ].push_back( replaceTokens(
        isGroupStateWhenBlock,
        { { "__SCENE__", "unknown_scene_id" }, { "__GROUP__", "unknown_group_id" }, { "__STATE__", "true" } } ) );

    auto error = nma().sendtoHubAndGetAnswer( request )[ "error" ];

    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.not.exist" );
}

/// @case_04
/// @brief
/// The test verifies that a scene is not created if group ID is not correct
TEST_F( SceneEventTest_isGroupState, invalidGroupId ) {
    auto dev1 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );

    auto scene1Request = createEmptyScene;
    scene1Request[ "params" ][ "when" ].push_back( replaceTokens(
        isItemStateWhenBlock, { { "__ITEM_ID__", dev1.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "10" } } ) );

    auto scene1Id = createScene( scene1Request, __LINE__ );

    auto scene2Request = createEmptyScene;

    scene2Request[ "params" ][ "when" ].push_back(
        replaceTokens( isGroupStateWhenBlock,
                       { { "__SCENE__", scene1Id }, { "__GROUP__", "unknown_group_id" }, { "__STATE__", "true" } } ) );

    auto error = nma().sendtoHubAndGetAnswer( scene2Request )[ "error" ];

    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.block.groups.not.exist" );
}

class SceneEventTest_isSceneState : public SceneEventTest_isGroupState {
public:
};

TEST_F( SceneEventTest_isSceneState, keepWorkingAfterEdit ) {
    // create a scene
    // creates an isSceneState
    // run a scene
    // check progress broadcasts
    auto sceneRequest = createEmptyScene;
    sceneRequest[ "params" ][ "name" ] = "main_scene";
    auto sceneId = createScene( sceneRequest, __LINE__ );

    auto sceneControlRequest = createEmptyScene;
    sceneControlRequest[ "params" ][ "name" ] = "control_scene";

    sceneControlRequest[ "params" ][ "when" ].push_back(
        replaceTokens( isSceneStateWhenBlock, { { "__SCENE_ID__", sceneId }, { "__STATE__", "finished" } } ) );

    auto sceneControlId = createScene( sceneControlRequest, __LINE__ );

    runSceneManually( sceneId, __LINE__ );

    testRunProgressBroadcast( sceneRequest, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( sceneRequest, sceneId, "finished", __LINE__ );
    testRunProgressBroadcast( sceneControlRequest, sceneControlId, "started", __LINE__ );
    testRunProgressBroadcast( sceneControlRequest, sceneControlId, "finished", __LINE__ );

    // edit a scene
    // check progress broadcasts
    editScene( sceneId, sceneRequest[ "params" ], __LINE__ );

    runSceneManually( sceneId, __LINE__ );

    testRunProgressBroadcast( sceneRequest, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( sceneRequest, sceneId, "finished", __LINE__ );
    testRunProgressBroadcast( sceneControlRequest, sceneControlId, "started", __LINE__ );
    testRunProgressBroadcast( sceneControlRequest, sceneControlId, "finished", __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform