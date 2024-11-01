#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/model/values/value_factory.h"
#include "hardware_manager/battery/battery_info.h"
#include "database_client/dbtools.h"
#include "base_utils/timeutils.h"

#include <gtest/gtest.h>
#include "tiny/tiny_consolecolors.h"

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN

1. Clone simple
    1.1 Clone simple (empty) scene test
    1.2 Clone scene with a defined name
2 Clone scenes with relations
    2.1 Clone scene with actions groups relations
*/

class SceneCloneTests : public SceneManagerTestsFixture {
public:
    static void SetUpTestCase();
    void SetUp() override;
    void TearDown() override;
};

void SceneCloneTests::TearDown() {
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

void SceneCloneTests::SetUpTestCase() {}

void SceneCloneTests::SetUp() {
    SceneManagerTestsFixture::SetUp();
    callingFile( __FILE__ );
}

/// @brief
///  clone scene ( hub.scenes.clone )
///  check hub.scene.added broadcast

TEST_F( SceneCloneTests, cloneScene ) {  // 1.1
    auto request = replaceTokens( createSceneRequest, { { "__THEN_BLOCKS__", "" }, { "__WHEN_BLOCKS__", "" } } );
    auto wrongRequest = request;
    wrongRequest[ "params" ].erase( "enabled" );
    auto answer = nma().sendtoHubAndGetAnswer( wrongRequest );
    checkErrorFieldNotFound( answer, "enabled", __LINE__ );

    SceneID sceneId = createScene( createSceneJson );
    Json result;
    JsonObject cloneSceneRequest = replaceTokens( cloneSceneJson, { { "__NAME__", "" }, { "__ID__", sceneId } } );
    SceneID clonedSceneId = cloneScene( cloneSceneRequest, result, __LINE__ );
    answer = nma().sendtoHubAndGetAnswer( getScenesList );
    JsonArray scenesList = answer[ "result" ][ "scenes" ];
    ASSERT_EQ( scenesList.size(), 2 );
    EXPECT_EQ( json_get_str( result, "name" ), "newTestRule1 - clone(1)" );
}

TEST_F( SceneCloneTests, cloneSceneWithDefinedName ) {  // 1.2
    auto request = replaceTokens( createSceneRequest, { { "__THEN_BLOCKS__", "" }, { "__WHEN_BLOCKS__", "" } } );
    auto wrongRequest = request;
    wrongRequest[ "params" ].erase( "enabled" );
    auto answer = nma().sendtoHubAndGetAnswer( wrongRequest );
    checkErrorFieldNotFound( answer, "enabled", __LINE__ );

    SceneID sceneId = createScene( createSceneJson );
    Json result;

    auto cloneSceneRequest = replaceTokens( cloneSceneJson, { { "__NAME__", "test" }, { "__ID__", sceneId } } );
    auto clonedSceneId = cloneScene( cloneSceneRequest, result, __LINE__ );
    answer = nma().sendtoHubAndGetAnswer( getScenesList );
    EXPECT_EQ( json_get_str( result, "name" ), "test" );
}

TEST_F( SceneCloneTests, cloneScenesWithThenGroupsRefferences ) {  // 2.1
    auto dev1 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );

    auto sceneRequest = createEmptyScene;
    std::string actionGroupIdOfWhenBlock = "0";

    auto whenBlock = replaceTokens(
        isItemStateWhenBlock, { { "__ITEM_ID__", dev1.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "10" } } );

    whenBlock[ "actionGroup" ] = actionGroupIdOfWhenBlock;

    sceneRequest[ "params" ][ "when" ].push_back( std::move( whenBlock ) );
    sceneRequest[ "params" ][ "thenGroups" ][ actionGroupIdOfWhenBlock ].push_back( replaceTokens(
        setItemValueThenBlock, { { "__ITEM_ID__", dev1.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "12" } } ) );

    Json sceneCreateResult;
    const auto ClonedSceneName = "scene_cloned";
    auto sceneId = createScene( sceneRequest, sceneCreateResult, __LINE__ );

    JsonArray thenGroups;
    json_get_obj( sceneCreateResult, "thenGroups", thenGroups );

    auto getActionGroupId = []( JsonObject thenGroups ) {
        std::string actionGroupId;
        for( auto it = thenGroups.begin(); it != thenGroups.end(); ++it ) {
            actionGroupId = it.key();
        }
        return actionGroupId;
    };

    auto groupIdOrigin = getActionGroupId( thenGroups );
    JsonObject cloneSceneRequest =
        replaceTokens( cloneSceneJson, { { "__NAME__", ClonedSceneName }, { "__ID__", sceneId } } );

    JsonObject result;
    SceneID clonedSceneId = cloneScene( cloneSceneRequest, result, __LINE__ );
    auto answer = nma().sendtoHubAndGetAnswer( getScenesList );

    json_get_obj( result, "thenGroups", thenGroups );
    auto groupIdCloned = getActionGroupId( thenGroups );
    EXPECT_TRUE( !groupIdCloned.empty() );

    JsonObject whenBlockCloned;
    json_get_arr( result, "when", whenBlockCloned );

    auto whenBlockId = json_get_str( whenBlockCloned.front(), "_id" );
    auto whenBlockActionGroupId = json_get_str( whenBlockCloned.front(), "actionGroup" );

    EXPECT_TRUE( groupIdOrigin != groupIdCloned );
    EXPECT_TRUE( groupIdCloned == whenBlockId );
    EXPECT_TRUE( groupIdCloned == whenBlockActionGroupId );
}

}  // namespace Tests
}  // namespace HubPlatform
