#include "scenes_manager/tests/fixtures/scenes_api_fixture.h"
#include "scenes_manager/tests/resources/generic_scene_json.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

/*

TEST PLAN
DONE    1 Cloned scenes default naming tests
DONE        1.1 Generate default cloned scene name
DONE        1.2. Generate default cloned scene name for cloned scene
DONE        1.3. Generate default cloned scene name with add scenes
DONE        1.4. Generate default cloned scene name with delete scenes
DONE    2 Cloning scenes tests
DONE        2.1 Clone simple scene
DONE        2.2 Clone scenes with block and relations
DONE        2.3 Clone scenes with inner relations in actions

*/
const auto ID = "_id";
class ScenesClonedTests : public ScenesAPIFixture {
public:
    bool createSceneWithName( const std::string& name ) {
        auto request = replaceTokens( SceneEngine::UT::createScene.dump(), { { "__NAME__", name } } );
        Json answer;
        return getComplexScenesAPI().processCreateScene( Json::parse( request ), answer );
    }

    std::string getBlockIdForScene( const JsonObject scene, const std::string blockType, int codeLine ) {
        JsonArray blocks;
        json_get_arr( scene, blockType, blocks );
        EXPECT_TRUE( !blocks.empty() ) << codeLine;
        auto block = blocks.front();
        EXPECT_TRUE( !block.empty() ) << codeLine;
        auto res = json_get_str( block, ID );
        EXPECT_TRUE( !res.empty() ) << codeLine;
        return res;
    }

    Json getBlockForScene( const JsonObject scene, const std::string blockType, int codeLine ) {
        JsonArray blocks;
        json_get_arr( scene, blockType, blocks );
        EXPECT_TRUE( !blocks.empty() ) << codeLine;
        auto block = blocks.front();
        EXPECT_TRUE( !block.empty() ) << codeLine;
        return block;
    }

    std::string getBlockReference( const JsonObject block, const std::string name, int codeLine ) {
        JsonArray fields;
        std::string value;
        json_get( block, "fields", fields );
        std::string sceneId;
        EXPECT_TRUE( !fields.empty() ) << codeLine;
        for( auto field : fields ) {
            EXPECT_TRUE( !field.empty() ) << codeLine;
            if( json_get_str( field, "name" ) == name ) {
                value = json_get_str( field, "value" );
                EXPECT_TRUE( !value.empty() ) << codeLine;
                return value;
            }
        }
        return "";
    }

    std::string getExceptionBlockIdForSceneAndCondition( JsonObject& scene, std::string relatedSceneId, int codeLine ) {
        JsonArray blocks;
        std::string res;
        json_get_arr( scene, "exceptions", blocks );
        for( auto block : blocks ) {
            JsonArray fields;
            json_get_arr( block, "fields", fields );
            for( auto field : fields ) {
                auto value = json_get_str( field, "value" );
                if( relatedSceneId == value ) {
                    res = json_get_str( block, ID );
                    EXPECT_TRUE( !res.empty() ) << codeLine;
                    return res;
                }
            }
        }
        EXPECT_TRUE( !res.empty() ) << codeLine;
        return res;
    }

    std::string getRelationIdForActionBlockForScene( const std::string blockType, JsonObject& scene, int codeLine ) {
        JsonArray blocks;
        std::string res;
        json_get_arr( scene, blockType, blocks );

        for( auto block : blocks ) {
            JsonObject control;
            json_get_obj( block, "control", control );
            if( !control.empty() ) {
                JsonObject exception;
                json_get_obj( control, "exception", exception );
                if( !exception.empty() ) {
                    res = json_get_str( exception, "blockId" );
                    EXPECT_TRUE( !res.empty() ) << codeLine;
                    return res;
                }
            }
        }
        EXPECT_TRUE( !res.empty() ) << codeLine;
        return res;
    }
};

TEST_F( ScenesClonedTests, CreateDefaultclonedSceneName ) {  // 1.1
    std::string name = "scene_name";
    auto clonedName = clonedSceneName( name );
    EXPECT_EQ( clonedName, "scene_name - clone(1)" );
}

TEST_F( ScenesClonedTests, CreateDefaultClonedSceneNameForAlreadyClonedSceneName ) {  // 1.2
    std::string name = "scene_name";
    Json scene = JSON_EMPTY_OBJ;
    auto clonedName = clonedSceneName( name );
    auto clonedClonedName = clonedSceneName( clonedName );
    EXPECT_EQ( clonedClonedName, "scene_name - clone(1) - clone(1)" );
}

TEST_F( ScenesClonedTests, CreateCloneNamesCheckCounting ) {  // 1.3
    std::string name = "scene_name";
    Json scene = JSON_EMPTY_OBJ;
    auto clonedName = clonedSceneName( name );
    EXPECT_EQ( clonedName, "scene_name - clone(1)" );
    createSceneWithName( clonedName );
    clonedName = clonedSceneName( name );
    EXPECT_EQ( clonedName, "scene_name - clone(2)" );
    createSceneWithName( clonedName );
    clonedName = clonedSceneName( name );
    EXPECT_EQ( clonedName, "scene_name - clone(3)" );
}

TEST_F( ScenesClonedTests, sceneCreateCloneNameAfterDeletionPreviousInTheMiddle ) {  // 1.4
    std::string name = "scene_name";
    Json scene = JSON_EMPTY_OBJ;
    auto clonedName = clonedSceneName( name );
    createSceneWithName( clonedName );
    auto clonedName2 = clonedSceneName( name );
    createSceneWithName( clonedName );
    clonedName = clonedSceneName( name );
    createSceneWithName( clonedName );
    getDbClient().remove( HaScenesDBName, JSON( "name" << clonedName2 ) );
    clonedName = clonedSceneName( name );
    EXPECT_EQ( clonedName, "scene_name - clone(2)" );
}

TEST_F( ScenesClonedTests, cloneSimpleScene )  // 2.1
{
    JsonObject answer;
    std::string name = "scene_name";
    bool ret = createSceneWithName( name );
    EXPECT_TRUE( ret );
    std::string idOriginalScene;
    EXPECT_TRUE( json_get( getResult(), ID, idOriginalScene ) );
    auto scene = getDbClient().findOne( HaScenesDBName, JSON( ID << idOriginalScene ), nullptr );
    EXPECT_TRUE( !scene.empty() );
    JsonObject cloneResult;
    bool res = getComplexScenesAPI().processCloneScene( JSON( ID << idOriginalScene ), cloneResult );
    EXPECT_TRUE( res );
    cloneResult = getResult();
    std::string idClonedScene;
    EXPECT_TRUE( json_get( getResult(), "_id", idClonedScene ) );
    auto clonedScene = getDbClient().findOne( HaScenesDBName, JSON( ID << idClonedScene ), nullptr );
    EXPECT_TRUE( !clonedScene.empty() );
    EXPECT_TRUE( idClonedScene != idOriginalScene );
    std::string clonedSceneName;
    EXPECT_EQ( json_get_str( clonedScene, "name" ), "scene_name - clone(1)" );
    auto originalScene = getDbClient().findOne( HaScenesDBName, JSON( ID << idOriginalScene ), nullptr );
    EXPECT_TRUE( !scene.empty() );
}

TEST_F( ScenesClonedTests, cloneScenesWithBlockReferences ) {  // 2.2
    std::string id1;
    std::string id2;

    bool res = createSceneWithName( "name1" );
    EXPECT_TRUE( res );
    EXPECT_TRUE( json_get( getResult(), ID, id1 ) );

    res = createSceneWithName( "name2" );
    EXPECT_TRUE( res );
    EXPECT_TRUE( json_get( getResult(), ID, id2 ) );

    const JsonObject inputSceneData = replaceTokens( SceneEngine::UT::sceneWithExceptionsInActions.dump(),
                                                     { { "__SCENE_ID_1__", id1 }, { "__SCENE_ID_2__", id2 } } );

    Json answer;
    res = getComplexScenesAPI().processCreateScene( Json::parse( inputSceneData ), answer );
    EXPECT_TRUE( res );
    std::string clonedSceneId;
    EXPECT_TRUE( json_get( getResult(), ID, clonedSceneId ) );
    EXPECT_TRUE( res );

    auto originalScene = getResult();
    JsonObject cloneResult;
    bool clonedResult = getComplexScenesAPI().processCloneScene( JSON( ID << clonedSceneId ), cloneResult );
    EXPECT_TRUE( clonedResult );
    auto clonedScene = getResult();

    auto checkBlockIdBetweenScenes = [&]( const std::string blockType, int codeLine ) {
        auto whenBlockIdOrigin = getBlockIdForScene( originalScene, blockType, codeLine );
        auto whenBlockIdCloned = getBlockIdForScene( clonedScene, blockType, codeLine );
        EXPECT_TRUE( whenBlockIdOrigin != whenBlockIdCloned );
    };

    auto exceptionBlockIdOriginal1 = getExceptionBlockIdForSceneAndCondition( originalScene, id2, __LINE__ );
    auto exceptionBlockIdCloned1 = getExceptionBlockIdForSceneAndCondition( clonedScene, id2, __LINE__ );
    auto exceptionBlockIdOriginal2 = getExceptionBlockIdForSceneAndCondition( originalScene, id1, __LINE__ );
    auto exceptionBlockIdCloned2 = getExceptionBlockIdForSceneAndCondition( clonedScene, id1, __LINE__ );
    auto relationIdThenOriginal = getRelationIdForActionBlockForScene( "then", originalScene, __LINE__ );
    auto relationIdThenCloned = getRelationIdForActionBlockForScene( "then", clonedScene, __LINE__ );
    auto relationIdElseOriginal = getRelationIdForActionBlockForScene( "else", originalScene, __LINE__ );
    auto relationIdElseCloned = getRelationIdForActionBlockForScene( "else", clonedScene, __LINE__ );

    // checking of difference block and relation id's between original and cloned scenes:
    checkBlockIdBetweenScenes( "when", __LINE__ );
    checkBlockIdBetweenScenes( "then", __LINE__ );
    checkBlockIdBetweenScenes( "else", __LINE__ );

    EXPECT_TRUE( exceptionBlockIdOriginal1 != exceptionBlockIdCloned1 );
    EXPECT_TRUE( exceptionBlockIdOriginal2 != exceptionBlockIdCloned2 );
    EXPECT_TRUE( exceptionBlockIdOriginal2 != exceptionBlockIdCloned2 );
    EXPECT_TRUE( relationIdThenOriginal != relationIdThenCloned );
    EXPECT_TRUE( relationIdElseOriginal != relationIdElseCloned );

    // checking of cloned scene relations:
    EXPECT_TRUE( relationIdElseCloned == exceptionBlockIdCloned1 );
    EXPECT_TRUE( relationIdThenCloned == exceptionBlockIdCloned2 );
}

TEST_F( ScenesClonedTests, cloneScenesWithInnerReferencesInActionBlocks ) {  // 2.3
    std::string sceneId;
    Json answer;

    EXPECT_TRUE( createSceneWithName( "name1" ) );
    EXPECT_TRUE( json_get( getResult(), ID, sceneId ) );

    const JsonObject inputSceneData = replaceTokens( SceneEngine::UT::sceneWithInnerRelationsInActions.dump(),
                                                     { { "__SCENE_ID__", sceneId }, { "__SCENE_ID_2__", sceneId } } );

    EXPECT_TRUE( getComplexScenesAPI().processCreateScene( Json::parse( inputSceneData ), answer ) );
    auto scene = getResult();
    auto sceneJson = getResult();
    std::string originalSceneId;
    EXPECT_TRUE( json_get( getResult(), ID, originalSceneId ) );
    auto whenBlockId = getBlockIdForScene( sceneJson, "when", __LINE__ );
    const JsonObject editSceneData =
        replaceTokens( SceneEngine::UT::editedSceneWithInnerRelationsInActions.dump(),
                       { { "__SCENE_ID__", originalSceneId }, { "__BLOCK_ID__", whenBlockId } } );
    EXPECT_TRUE( getComplexScenesAPI().processEditScene( Json::parse( editSceneData ), answer ) );
    EXPECT_TRUE( json_get( getResult(), ID, originalSceneId ) );
    auto originalScene = getResult();
    JsonObject cloneResult;
    bool clonedResult = getComplexScenesAPI().processCloneScene( JSON( ID << originalSceneId ), cloneResult );
    EXPECT_TRUE( clonedResult );
    auto clonedScene = getResult();
    std::string clonedSceneId;
    EXPECT_TRUE( json_get( getResult(), ID, clonedSceneId ) );

    auto whenBlockIdCloned = getBlockIdForScene( clonedScene, "when", __LINE__ );
    auto thenBlock = getBlockForScene( clonedScene, "then", __LINE__ );
    auto elseBlock = getBlockForScene( clonedScene, "else", __LINE__ );

    auto thenBlockReferenceSceneIdCloned = getBlockReference( thenBlock, "sceneId", __LINE__ );
    auto thenBlockReferenceBlockIdCloned = getBlockReference( thenBlock, "blockId", __LINE__ );
    auto elseBlockReferenceSceneIdCloned = getBlockReference( elseBlock, "sceneId", __LINE__ );

    EXPECT_TRUE( clonedSceneId != originalSceneId );
    EXPECT_TRUE( clonedSceneId == thenBlockReferenceSceneIdCloned );
    EXPECT_TRUE( clonedSceneId == elseBlockReferenceSceneIdCloned );
    EXPECT_TRUE( whenBlockIdCloned == thenBlockReferenceBlockIdCloned );
}

}  // namespace UT
}  // namespace HubPlatform
