#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "common_test_resources/resources/cored_resources.h"

#include "cored/model/values/value_factory.h"
#include <gtest/gtest.h>
#include "tiny/tiny_consolecolors.h"

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN
DONE    1. Test set scene meta request, get meta in scenes params test and check broadcast
DONE    2. Test set scene meta blocks request, get meta in scenes block test and check broadcast
DONE    3. Test add scene with metadata, and check broadcasts 'add' and 'change', test run scenes with metadata.

*/

class SceneSetMetadataTests : public SceneManagerTestsFixture {
public:
    static void SetUpTestCase();
    void SetUp() override;
    void checkSetParamsMeta( const std::string& sceneId, int codeLine );
    void checkSetBlockMeta( const std::string& blockName, const std::string& sceneId, int codeLine );
    void checkSetBlockGroupMeta( const std::string& blockName, const std::string& sceneId, int codeLine );
};

void SceneSetMetadataTests::checkSetParamsMeta( const std::string& sceneId, int codeLine ) {
    auto request = replaceTokens( setMetaRequest, { { "__SCENE_ID__", sceneId } } );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto codeLineStr = getCodeLineString( codeLine );
    auto broadcast = waitForBroadcast( "hub.scene.changed", codeLineStr );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << codeLineStr;
    JsonObject result;
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.changed" ) << codeLineStr;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << codeLineStr;
    JsonObject blockMeta;
    EXPECT_TRUE( json_get_obj( result, "meta", blockMeta ) ) << codeLineStr;
    answer = nma().sendtoHubAndGetAnswer( getScenesList );
    JsonArray scenesList = answer[ "result" ][ "scenes" ];
    JsonArray scene = scenesList.front();
    EXPECT_TRUE( json_get_obj( result, "meta", blockMeta ) ) << codeLineStr;
};

void SceneSetMetadataTests::checkSetBlockMeta( const std::string& blockName,
                                               const std::string& sceneId,
                                               int codeLine ) {
    JsonObject result;
    auto codeLineStr = getCodeLineString( codeLine );
    auto answer = nma().sendtoHubAndGetAnswer( getScenesList );
    JsonArray scenesList = answer[ "result" ][ "scenes" ];
    ASSERT_EQ( scenesList.size(), 1 ) << codeLineStr;
    auto block = scenesList.front()[ blockName ].front();
    JsonObject blockMeta;
    auto blockId = json_get_str( block, "_id" );
    auto request = replaceTokens( setBlockMetaRequest, { { "__SCENE_ID__", sceneId }, { "__BLOCK_ID__", blockId } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    const auto& error = answer[ "error" ];
    EXPECT_TRUE( error.empty() ) << codeLineStr;
    auto broadcast = waitForBroadcast( "hub.scene.changed", codeLineStr );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.changed" ) << codeLineStr;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << codeLineStr;
    EXPECT_TRUE( json_get_obj( result[ blockName ].front(), "blockMeta", blockMeta ) ) << codeLineStr;
    answer = nma().sendtoHubAndGetAnswer( getScenesList );
    scenesList = answer[ "result" ][ "scenes" ];
    block = scenesList.front()[ blockName ].front();
    EXPECT_TRUE( json_get_obj( block, "blockMeta", blockMeta ) ) << codeLineStr;
}

void SceneSetMetadataTests::checkSetBlockGroupMeta( const std::string& blockName,
                                                    const std::string& sceneId,
                                                    int codeLine ) {
    auto codeLineStr = getCodeLineString( codeLine );
    JsonObject result;
    auto answer = nma().sendtoHubAndGetAnswer( getScenesList );
    JsonArray scenesList = answer[ "result" ][ "scenes" ];
    ASSERT_EQ( scenesList.size(), 1 ) << codeLineStr;
    JsonArray block = scenesList.front()[ blockName ].front().front();
    JsonObject blockMeta;
    auto whenBlockId = json_get_str( block, "_id" );
    auto request =
        replaceTokens( setBlockMetaRequest, { { "__SCENE_ID__", sceneId }, { "__BLOCK_ID__", whenBlockId } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    const auto& error = answer[ "error" ];
    EXPECT_TRUE( error.empty() ) << codeLineStr;

    auto broadcast = waitForBroadcast( "hub.scene.changed", codeLineStr );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.changed" ) << codeLineStr;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << codeLineStr;
    EXPECT_TRUE( json_get_obj( result[ blockName ].front().front(), "blockMeta", blockMeta ) ) << codeLineStr;

    answer = nma().sendtoHubAndGetAnswer( getScenesList );
    scenesList = answer[ "result" ][ "scenes" ];
    block = scenesList.front()[ blockName ].front().front();
    EXPECT_TRUE( json_get_obj( block, "blockMeta", blockMeta ) ) << codeLineStr;
}

void SceneSetMetadataTests::SetUpTestCase() {}

void SceneSetMetadataTests::SetUp() {
    SceneManagerTestsFixture::SetUp();
    callingFile( __FILE__ );
}

TEST_F( SceneSetMetadataTests, setMetaRequest ) {
    SceneID sceneId = sceneWithBlocks( __LINE__ );
    checkSetParamsMeta( sceneId, __LINE__ );
    deleteScene( sceneId );
}

TEST_F( SceneSetMetadataTests, setMetaBlockRequest ) {
    SceneID sceneId = sceneWithBlocks( __LINE__ );
    auto codeLineStr = getCodeLineString( __LINE__ );
    JsonObject result;
    checkSetBlockMeta( "when", sceneId, __LINE__ );
    checkSetBlockMeta( "then", sceneId, __LINE__ );
    checkSetBlockMeta( "else", sceneId, __LINE__ );
    checkSetBlockMeta( "exceptions", sceneId, __LINE__ );
    checkSetBlockGroupMeta( "thenGroups", sceneId, __LINE__ );
    checkSetBlockGroupMeta( "elseGroups", sceneId, __LINE__ );
    deleteScene( sceneId );
}

TEST_F( SceneSetMetadataTests, checkMetaBlockAfterUpdateScene ) {
    SceneID sceneId = createScene( createSceneWithMetaInParamsRequest );
    JsonObject blockMeta;
    auto codeLineStr = getCodeLineString( __LINE__ );
    // Add scene with metadata in params
    sendRequestAndCheckAnswer( createSceneWithMetaInParamsRequest, __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.scene.added", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.added" );
    JsonObject result;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << getCodeLineString( __LINE__ );
    SceneID id = json_get_str( result, "_id" );
    // check then metadata is exist in broadcast
    EXPECT_TRUE( json_get_obj( result, "meta", blockMeta ) ) << getCodeLineString( __LINE__ );
    // check then metadata is exist in broadcast
    auto answer = nma().sendtoHubAndGetAnswer( getScenesList );
    JsonArray scenesList = answer[ "result" ][ "scenes" ];
    JsonArray scene = scenesList.front();

    EXPECT_TRUE( json_get_obj( scene, "meta", blockMeta ) ) << getCodeLineString( __LINE__ );
    auto getRequest = replaceTokens( getScene, { { "__SCENE_ID__", sceneId } } );
    answer = nma().sendtoHubAndGetAnswer( getRequest );
    scene = answer[ "result" ];

    EXPECT_TRUE( json_get_obj( scene, "meta", blockMeta ) ) << getCodeLineString( __LINE__ );
    // run scene and check that scene run successfully
    auto request = replaceTokens( HubPlatform::Tests::runSceneManually, { { "__ID__", sceneId } } );
    answer = nma().sendtoHubAndGetAnswer( request, ComponentTestApplication::ConnectionType::NMA );
    testRunProgressBroadcast( createEmptyWhenBlockSceneJson, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( createEmptyWhenBlockSceneJson, sceneId, "finished", __LINE__ );
    // edit scene
    std::string dump = HubPlatform::Tests::createSceneWithMetaInParamsEditSceneRequest.dump();
    std::string toReplace = "__SCENE_ID__";
    dump.replace( dump.find( toReplace ), toReplace.size(), sceneId );
    JsonObject editScene = JsonObject::parse( dump );
    sendRequestAndCheckAnswer( editScene, __LINE__ );
    broadcast = waitForBroadcast( "hub.scene.changed", getCodeLineString( __LINE__ ) );
    // check 'changed' broadcast
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.changed" ) << getCodeLineString( __LINE__ );
    EXPECT_TRUE( json_get_obj( result, "meta", blockMeta ) ) << getCodeLineString( __LINE__ );
    deleteScene( sceneId );
}

}  // namespace Tests
}  // namespace HubPlatform
