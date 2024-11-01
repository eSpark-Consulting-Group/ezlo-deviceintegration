#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"

/**
DONE    1. acquired plugin count test
DONE    2. acquired plugin deletion error test
 */

namespace HubPlatform {
namespace Tests {

class AnonymousPluginsTest : public SceneManagerTestsFixture {
public:
    void SetUp() override;
    void checkAcquiredScript( const std::string& scriptId, int count, bool isExist, int codeLine );
    std::string addPluginScript( const std::string& name, const std::string& code, int codeLine );
    void sendEditSceneAndAnswer( const JsonObject& newNameSceneJson, int codeLine );
    void deleteScript( const std::string& scriptId, int codeLine );
};

void AnonymousPluginsTest::SetUp() {
    SceneManagerTestsFixture::SetUp();
    callingFile( __FILE__ );
}

void AnonymousPluginsTest::checkAcquiredScript( const std::string& scriptId, int count, bool isExist, int codeLine ) {
    auto codeLineStr = getCodeLineString( codeLine );
    auto request = replaceTokens( scenesScriptsGetRequest, { { "__ID__", scriptId } } );
    auto answer = sendRequestAndCheckAnswer( request, codeLine );

    Json result;
    EXPECT_TRUE( json_get_obj( answer, "result", result ) ) << codeLineStr;

    std::string id;
    if( isExist ) {
        id = json_get_str( result, "_id" );
        EXPECT_EQ( id, scriptId ) << codeLine;
        EXPECT_EQ( json_get_int( result, "acquired" ), count ) << codeLineStr;
    }
    else {
        EXPECT_EQ( json_get_int( result, "acquired" ), count ) << codeLineStr;
    }
}

std::string AnonymousPluginsTest::addPluginScript( const std::string& name, const std::string& code, int codeLine ) {
    auto codeLineStr = getCodeLineString( codeLine );
    auto request = replaceTokens( addScriptRequest, { { "__NAME__", name }, { "__CODE__", code } } );
    auto answer = sendRequestAndCheckAnswer( request, codeLine );
    Json result;
    EXPECT_TRUE( json_get_obj( answer, "result", result ) ) << codeLineStr;
    std::string scriptId;
    EXPECT_TRUE( json_get( result, "_id", scriptId ) ) << codeLineStr;

    return scriptId;
}

void AnonymousPluginsTest::sendEditSceneAndAnswer( const JsonObject& newNameSceneJson, int codeLine ) {
    auto codeLineStr = getCodeLineString( codeLine );
    auto answer = nma().sendtoHubAndGetAnswer( newNameSceneJson );
    const auto& error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 ) << codeLineStr;
    JsonObject newNameSceneBroadcast = waitForBroadcast( "hub.scene.changed", NMA_TEST_MAKE_MESSAGE( "editScene" ) );
    const JsonObject& newNameSceneResult = newNameSceneBroadcast[ "result" ];
}

void AnonymousPluginsTest::deleteScript( const std::string& scriptId, int codeLine ) {
    auto codeLineStr = getCodeLineString( codeLine );
    auto request = replaceTokens( deleteScriptRequest, { { "__ID__", scriptId } } );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    const auto& error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 ) << codeLineStr;
}

TEST_F( AnonymousPluginsTest, pluginAciquiredCountTest ) {
    CoreDRemoteInterface::CoreDInterface& coredRemote = makeRemote();

    // create two scripts
    std::string scriptId1 = addPluginScript( "script_1", "return 1", __LINE__ );
    std::string scriptId2 = addPluginScript( "script_2", "return 2", __LINE__ );

    const Json scriptParams = {};
    Json runPluginScript1 = replaceTokens( runCustomScriptThenBlock, { { "__SCRIPT_ID__", scriptId1 } } );
    Json runPluginScript2 = replaceTokens( runCustomScriptThenBlock, { { "__SCRIPT_ID__", scriptId2 } } );

    Json sceneJson = createEmptyScene;
    sceneJson[ "params" ][ "name" ] = "scene";
    sceneJson[ "params" ][ "then" ].push_back( runPluginScript1 );
    sceneJson[ "params" ][ "then" ].push_back( runPluginScript1 );

    Json sceneResult;

    // add few scenes with different blocks which use script1
    const SceneID sceneId = createScene( sceneJson, sceneResult, __LINE__ );
    checkAcquiredScript( scriptId1, 2, true, __LINE__ );

    const SceneID sceneId1 = createScene( sceneJson, sceneResult, __LINE__ );
    checkAcquiredScript( scriptId1, 4, true, __LINE__ );

    // edit scene1 with empty actions
    auto editScene1 = Tests::editScene;
    editScene1[ "params" ][ "_id" ] = sceneId;
    editScene1[ "params" ][ "eo" ] = createEmptyScene[ "params" ];
    editScene1[ "params" ][ "eo" ][ "name" ] = "emptyScene";
    sendEditSceneAndAnswer( editScene1, __LINE__ );

    checkAcquiredScript( scriptId1, 2, true, __LINE__ );

    // edit scene2: add two different blocks with script1 and script2
    auto editScene2 = Tests::editScene;
    editScene2[ "params" ][ "_id" ] = sceneId1;
    editScene2[ "params" ][ "eo" ] = createEmptyScene[ "params" ];
    editScene2[ "params" ][ "eo" ][ "name" ] = "emptyScene";
    editScene2[ "params" ][ "eo" ][ "then" ].push_back( runPluginScript1 );
    editScene2[ "params" ][ "eo" ][ "then" ].push_back( runPluginScript2 );
    sendEditSceneAndAnswer( editScene2, __LINE__ );

    checkAcquiredScript( scriptId1, 1, true, __LINE__ );
    checkAcquiredScript( scriptId2, 1, true, __LINE__ );

    // delete all scenes
    deleteScene( sceneId, __LINE__ );
    deleteScene( sceneId1, __LINE__ );

    checkAcquiredScript( scriptId1, 0, false, __LINE__ );
    checkAcquiredScript( scriptId2, 0, false, __LINE__ );

    // check scripts deletion
    deleteScript( scriptId1, __LINE__ );
    deleteScript( scriptId2, __LINE__ );
}

TEST_F( AnonymousPluginsTest, pluginDeleteErrorTest ) {
    std::string scriptId1 = addPluginScript( "script_1", "return 1", __LINE__ );

    const Json scriptParams = {};
    Json runPluginScript1 = replaceTokens( runCustomScriptThenBlock, { { "__SCRIPT_ID__", scriptId1 } } );

    Json sceneJson = createEmptyScene;
    sceneJson[ "params" ][ "name" ] = "scene";
    sceneJson[ "params" ][ "then" ].push_back( runPluginScript1 );

    Json sceneResult;
    auto sceneId = createScene( sceneJson, sceneResult, __LINE__ );
    auto request = replaceTokens( deleteScriptRequest, { { "__ID__", scriptId1 } } );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    const auto& error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "anonymous.plugin.acquired" );
    EXPECT_EQ( json_get_str( error, "message" ), "Anonymous plugin acquired!" );

    deleteScene( sceneId, __LINE__ );
    deleteScript( scriptId1, __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
