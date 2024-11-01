#include "scenes_manager/tests/fixtures/scenes_api_fixture.h"
#include "scenes_manager/tests/resources/generic_scene_json.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

/*

TEST PLAN

1. Create a scene
    1.1 Create a simple scene
    1.2 Create a scene with isItemState for scalable item
    1.3 Create a scene with compareNumbers for scalable item

2. Get a scene
3. List existing scenes
    3.1 List scenes with temperature conditions (should be converted to hub's default temperature)

4. Edit a scene
    4.1 Edit a scene that does not exist

5. Delete a scene
6. Convert to block items
7. Modify scenes

DONE    8. Scene block status reset tests
DONE        8.1 Scene block status reset - Ok
DONE            8.1.1 reset status of a block
DONE            8.1.2 reset status of all blocks in scene
DONE        8.2 Scene block status reset errors
DONE            8.2.1 no sceneId parameter
DONE            8.2.2 sceneId is empty
DONE            8.2.3 scene not found
DONE            8.2.4 block not found

DONE    9. Scene status request
DONE        9.1 With valid params
DONE        9.2 With id of non-existing scene
DONE        9.3 Without "sceneId" param
DONE        9.4 With empty "sceneId" param
DONE        9.5 With "sceneId" param of wrong type

DONE    10. Scene block status request
DONE        10.1 global restriction block status reset

*/

class ScenesAPITests : public ScenesAPIFixture {
public:
    static std::string makeError( int line ) {
        return std::string{ __FILE__ }.append( ":" ).append( std::to_string( line ) );
    }
    static void answerHasError( const Json& answer, Errors::Codes errorCode, const std::string& errorData, int line ) {
        Json error;
        EXPECT_TRUE( json_get( answer, "error", error ) ) << makeError( line );
        LOGGER_TRACE( cc::trace( "error = " ) + cc::j( error ) );
        EXPECT_EQ( json_get_int( error, "code", -1 ), static_cast< int >( errorCode ) ) << makeError( line );
        if( !errorData.empty() )
            EXPECT_EQ( json_get_str( error, "data" ), errorData ) << makeError( line );
    }
};

TEST_F( ScenesAPITests, processCreateScene ) /* 1.1 */ {
    JsonObject answer;
    bool ret = getComplexScenesAPI().processCreateScene( SceneEngine::UT::createScene1, answer );
    EXPECT_TRUE( ret );
    std::string id;
    EXPECT_TRUE( json_get( getResult(), "_id", id ) );

    getSceneManager().deleteScene( id, false );
}

TEST_F( ScenesAPITests, processCreateSceneWithScalesItemState ) /* 1.2 */ {
    JsonObject answer;
    bool ret = getComplexScenesAPI().processCreateScene( SceneEngine::UT::createSceneWithScalesItemState, answer );
    EXPECT_TRUE( ret );
    std::string id;
    EXPECT_TRUE( json_get( getResult(), "_id", id ) );

    getSceneManager().deleteScene( id, false );
}

TEST_F( ScenesAPITests, processCreateSceneWithScalesCompareNumbers ) /* 1.3 */ {
    JsonObject answer;
    bool ret = getComplexScenesAPI().processCreateScene( SceneEngine::UT::createSceneWithScalesCompareNumbers, answer );
    EXPECT_TRUE( ret );
    std::string id;
    EXPECT_TRUE( json_get( getResult(), "_id", id ) );

    getSceneManager().deleteScene( id, false );
}

TEST_F( ScenesAPITests, getScene ) /* 2 */ {
    JsonObject answer;
    auto& scenesApi = getComplexScenesAPI();
    bool ret = scenesApi.processCreateScene( SceneEngine::UT::createScene1, answer );
    EXPECT_TRUE( ret );
    std::string id;
    EXPECT_TRUE( json_get( getResult(), "_id", id ) );

    auto scene = scenesApi.getSceneById( Json{ { "_id", id } } );
    EXPECT_FALSE( scene.empty() );

    std::string sceneName;
    EXPECT_TRUE( json_get( scene, "name", sceneName ) );
    EXPECT_STREQ( sceneName.c_str(), "NewR" );

    getSceneManager().deleteScene( id, false );
}

TEST_F( ScenesAPITests, listScenes ) /* 3 */ {
    JsonObject answer;
    auto& scenesApi = getComplexScenesAPI();
    bool ret = scenesApi.processCreateScene( SceneEngine::UT::createScene1, answer );
    EXPECT_TRUE( ret );
    std::string id;
    EXPECT_TRUE( json_get( getResult(), "_id", id ) );

    auto scenes = scenesApi.getScenesList( Json{} );

    EXPECT_FALSE( scenes.empty() );
    std::string sceneName;
    EXPECT_TRUE( json_get( scenes.front(), "name", sceneName ) );
    EXPECT_STREQ( sceneName.c_str(), "NewR" );
    getSceneManager().deleteScene( id, false );
}

TEST_F( ScenesAPITests, listScenesWithTemperatureScale ) /* 3.1 */ {
    JsonObject answer;
    auto& scenesApi = getComplexScenesAPI();
    bool ret = scenesApi.processCreateScene( SceneEngine::UT::createSceneWithScalesCompareNumbers, answer );
    EXPECT_TRUE( ret );
    std::string id;
    EXPECT_TRUE( json_get( getResult(), "_id", id ) );

    auto scenes = scenesApi.getScenesList( Json{} );

    EXPECT_FALSE( scenes.empty() );
    std::string sceneName;
    EXPECT_TRUE( json_get( scenes.front(), "name", sceneName ) );
    EXPECT_STREQ( sceneName.c_str(), "NewR" );
    getSceneManager().deleteScene( id, false );
}

TEST_F( ScenesAPITests, editScene ) /* 4 */ {
    JsonObject answer;
    bool ret = getComplexScenesAPI().processCreateScene( SceneEngine::UT::createScene1, answer );
    EXPECT_TRUE( ret );
    std::string id;
    EXPECT_TRUE( json_get( getResult(), "_id", id ) );

    JsonObject editSceneJson = SceneEngine::UT::editedScene;

    editSceneJson[ "_id" ] = id;
    editSceneJson[ "eo" ][ "_id" ] = id;
    editSceneJson[ "eo" ][ "name" ] = "NewRule2";

    JsonObject editResult;
    bool editRet = getComplexScenesAPI().processEditScene( editSceneJson, editResult );
    EXPECT_TRUE( editRet );

    editResult = getResult();
    std::string sceneName;
    EXPECT_TRUE( json_get( editResult, "name", sceneName ) );
    EXPECT_STREQ( sceneName.c_str(), "NewRule2" );
    getSceneManager().deleteScene( id, false );
}

TEST_F( ScenesAPITests, editSceneThatDoesNotExist ) /* 4.1 */ {
    JsonObject answer;
    bool ret = getComplexScenesAPI().processCreateScene( SceneEngine::UT::createScene1, answer );
    EXPECT_TRUE( ret );
    std::string id;
    EXPECT_TRUE( json_get( getResult(), "_id", id ) );

    JsonObject editSceneJson = SceneEngine::UT::editedScene;

    auto sceneIdThatDoesNotExist = id + "000";
    editSceneJson[ "_id" ] = sceneIdThatDoesNotExist;
    editSceneJson[ "eo" ][ "_id" ] = id;
    editSceneJson[ "eo" ][ "name" ] = "NewRule2";

    JsonObject editResult;
    bool editRet = getComplexScenesAPI().processEditScene( editSceneJson, editResult );
    EXPECT_FALSE( editRet ) << "Should end with an error";

    LOGGER_ERROR( "Scene edit result: " + cc::j( editResult ) );
    JsonObject error;
    EXPECT_TRUE( json_get( editResult, "error", error ) );
    EXPECT_EQ( json_get_int( error, "code", -1 ), -32500 ) << "Should return error 'scene does not exist'";
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.not.exist" );

    getSceneManager().deleteScene( id, false );
}

TEST_F( ScenesAPITests, deleteScene ) /* 5 */ {
    JsonObject answer;
    bool ret = getComplexScenesAPI().processCreateScene( SceneEngine::UT::createScene1, answer );
    EXPECT_TRUE( ret );
    std::string id1, id2;
    EXPECT_TRUE( json_get( getResult(), "_id", id1 ) );

    JsonObject answer2;
    ret = getComplexScenesAPI().processCreateScene( SceneEngine::UT::createScene2, answer2 );
    EXPECT_TRUE( ret );
    EXPECT_TRUE( json_get( getResult(), "_id", id2 ) );

    JsonObject params = {
        { "_id", id1 },
        { "sender", "victory@ezlo.com" },
        { "permission", { { "devices", "s" }, { "ezlo", "s" }, { "rules", "s" }, { "ui", "s" }, { "users", "s" } } }
    };

    JsonObject deleteSceneAnswer;
    ret = getComplexScenesAPI().processDeleteScene( params, deleteSceneAnswer );
    EXPECT_TRUE( ret );

    //    const auto scenes = getSceneRemoteInterface().getScenesList();
    auto scenes = getComplexScenesAPI().getScenesList( nullptr );
    EXPECT_FALSE( scenes.empty() );

    std::string checkId2;
    EXPECT_TRUE( json_get( scenes.front(), "_id", checkId2 ) );
    EXPECT_STREQ( checkId2.c_str(), id2.c_str() );

    getSceneManager().deleteScene( id1, false );
    getSceneManager().deleteScene( id2, false );
}

TEST_F( ScenesAPITests, convertToBlockItems ) /* 6 */ {
    std::vector< JsonObject > dbItems = {
        R"(
        {
            "_id": "5e46f2537f0000469989769a",
            "deviceId": "5e46f2537f00004699897692",
            "enum": [],
            "name": "user_lock_operation",
            "deviceCategory": "door_lock",
            "deviceSubcategory": "locker",
            "sourceIds": [
                "HUB:scene_blocks/templates/scene_blocks.json"
            ]
        }
        )"_json
    };
    std::vector< JsonObject > dbOptionItems = {
        R"(
        {
            "_id": "5e46f3357f00004c70b4856e",
            "blockId": "0001-000000c7",
            "deviceId": "5e46f2537f00004699897692",
            "itemId": "5e46f2537f0000469989769a",
            "optionItemName": "user_codes",
            "optionItemValue": "5e46f2537f00004699897696",
            "optionFieldName": "itemId",
            "sourceId": "HUB:scene_blocks/templates/scene_blocks.json"
        }
        )"_json
    };
    SceneEngine::SceneBlocks::ItemsToGet items = convertToBlockItems( dbItems, dbOptionItems );
    ASSERT_EQ( items.size(), 1 );
    SceneEngine::SceneBlocks::ItemToGet item = items.front();
    EXPECT_EQ( item.item.id, "5e46f2537f0000469989769a" );
    EXPECT_EQ( item.item.deviceId, "5e46f2537f00004699897692" );
    EXPECT_EQ( item.item.name, "user_lock_operation" );
    EXPECT_EQ( item.item.device.category, "door_lock" );
    EXPECT_EQ( item.item.device.subcategory, "locker" );
    EXPECT_TRUE( item.item.enumList.empty() );
    auto it = item.sortedOptionItemsByBlockId.find( "0001-000000c7" );
    ASSERT_TRUE( it != item.sortedOptionItemsByBlockId.end() );
    EXPECT_EQ( it->second.name, "user_codes" );
    EXPECT_EQ( it->second.value, "5e46f2537f00004699897696" );
    EXPECT_EQ( it->second.fieldName, "itemId" );

    items = convertToBlockItems( dbItems, {} );
    ASSERT_EQ( items.size(), 1 );
    item = items.front();
    EXPECT_EQ( item.item.id, "5e46f2537f0000469989769a" );
    EXPECT_EQ( item.item.deviceId, "5e46f2537f00004699897692" );
    EXPECT_EQ( item.item.name, "user_lock_operation" );
    EXPECT_EQ( item.item.device.category, "door_lock" );
    EXPECT_EQ( item.item.device.subcategory, "locker" );
    EXPECT_TRUE( item.item.enumList.empty() );
    EXPECT_TRUE( item.sortedOptionItemsByBlockId.empty() );

    dbItems.push_back(
        R"(
        {
            "_id": "5e46f2537f0000469989769b",
            "deviceId": "5e46f2537f00004699897693",
            "enum": [],
            "name": "user_lock_operation",
            "deviceCategory": "door_lock1",
            "deviceSubcategory": "locker1",
            "sourceIds": [
                "HUB:scene_blocks/templates/scene_blocks.json"
            ]
        }
        )"_json );

    dbOptionItems.push_back(
        R"(
        {
            "_id": "5e46f3357f00004c70b485f",
            "blockId": "0001-000000c7",
            "deviceId": "5e46f2537f00004699897693",
            "itemId": "5e46f2537f0000469989769b",
            "optionItemName": "user_codes",
            "optionItemValue": "5e46f2537f00004699897697",
            "optionFieldName": "itemId",
            "sourceId": "HUB:scene_blocks/templates/scene_blocks.json"
        }
        )"_json );

    items = convertToBlockItems( dbItems, dbOptionItems );
    ASSERT_EQ( items.size(), 2 );
    item = items[ 0 ];
    EXPECT_EQ( item.item.id, "5e46f2537f0000469989769a" );
    EXPECT_EQ( item.item.deviceId, "5e46f2537f00004699897692" );
    EXPECT_EQ( item.item.name, "user_lock_operation" );
    EXPECT_EQ( item.item.device.category, "door_lock" );
    EXPECT_EQ( item.item.device.subcategory, "locker" );
    EXPECT_TRUE( item.item.enumList.empty() );
    it = item.sortedOptionItemsByBlockId.find( "0001-000000c7" );
    ASSERT_TRUE( it != item.sortedOptionItemsByBlockId.end() );
    EXPECT_EQ( it->second.name, "user_codes" );
    EXPECT_EQ( it->second.value, "5e46f2537f00004699897696" );
    item = items[ 1 ];
    EXPECT_EQ( item.item.id, "5e46f2537f0000469989769b" );
    EXPECT_EQ( item.item.deviceId, "5e46f2537f00004699897693" );
    EXPECT_EQ( item.item.name, "user_lock_operation" );
    EXPECT_EQ( item.item.device.category, "door_lock1" );
    EXPECT_EQ( item.item.device.subcategory, "locker1" );
    EXPECT_TRUE( item.item.enumList.empty() );
    it = item.sortedOptionItemsByBlockId.find( "0001-000000c7" );
    ASSERT_TRUE( it != item.sortedOptionItemsByBlockId.end() );
    EXPECT_EQ( it->second.name, "user_codes" );
    EXPECT_EQ( it->second.value, "5e46f2537f00004699897697" );
    EXPECT_EQ( it->second.fieldName, "itemId" );

    dbItems.push_back(
        R"(
        {
            "_id": "5e46f2537f0000469989769c",
            "deviceId": "5e46f2537f00004699897694",
            "enum": [],
            "name": "user_lock_operation1",
            "deviceCategory": "door_lock2",
            "deviceSubcategory": "locker2",
            "sourceIds": [
                "HUB:scene_blocks/templates/scene_blocks.json"
            ]
        }
        )"_json );

    dbOptionItems.push_back(
        R"(
        {
            "_id": "5e46f3357f00004c70b4860",
            "blockId": "0001-000000c7",
            "deviceId": "5e46f2537f00004699897694",
            "itemId": "5e46f2537f0000469989769c",
            "optionItemName": "user_codes1",
            "optionItemValue": "5e46f2537f00004699897698",
            "optionFieldName": "itemId",
            "sourceId": "HUB:scene_blocks/templates/scene_blocks.json"
        }
        )"_json );

    items = convertToBlockItems( dbItems, dbOptionItems );
    ASSERT_EQ( items.size(), 3 );
    item = items[ 0 ];
    EXPECT_EQ( item.item.id, "5e46f2537f0000469989769a" );
    EXPECT_EQ( item.item.deviceId, "5e46f2537f00004699897692" );
    EXPECT_EQ( item.item.name, "user_lock_operation" );
    EXPECT_EQ( item.item.device.category, "door_lock" );
    EXPECT_EQ( item.item.device.subcategory, "locker" );
    EXPECT_TRUE( item.item.enumList.empty() );
    it = item.sortedOptionItemsByBlockId.find( "0001-000000c7" );
    ASSERT_TRUE( it != item.sortedOptionItemsByBlockId.end() );
    EXPECT_EQ( it->second.name, "user_codes" );
    EXPECT_EQ( it->second.value, "5e46f2537f00004699897696" );
    EXPECT_EQ( it->second.fieldName, "itemId" );
    item = items[ 1 ];
    EXPECT_EQ( item.item.id, "5e46f2537f0000469989769b" );
    EXPECT_EQ( item.item.deviceId, "5e46f2537f00004699897693" );
    EXPECT_EQ( item.item.name, "user_lock_operation" );
    EXPECT_EQ( item.item.device.category, "door_lock1" );
    EXPECT_EQ( item.item.device.subcategory, "locker1" );
    EXPECT_TRUE( item.item.enumList.empty() );
    it = item.sortedOptionItemsByBlockId.find( "0001-000000c7" );
    ASSERT_TRUE( it != item.sortedOptionItemsByBlockId.end() );
    EXPECT_EQ( it->second.name, "user_codes" );
    EXPECT_EQ( it->second.value, "5e46f2537f00004699897697" );
    EXPECT_EQ( it->second.fieldName, "itemId" );
    item = items[ 2 ];
    EXPECT_EQ( item.item.id, "5e46f2537f0000469989769c" );
    EXPECT_EQ( item.item.deviceId, "5e46f2537f00004699897694" );
    EXPECT_EQ( item.item.name, "user_lock_operation1" );
    EXPECT_EQ( item.item.device.category, "door_lock2" );
    EXPECT_EQ( item.item.device.subcategory, "locker2" );
    EXPECT_TRUE( item.item.enumList.empty() );
    it = item.sortedOptionItemsByBlockId.find( "0001-000000c7" );
    ASSERT_TRUE( it != item.sortedOptionItemsByBlockId.end() );
    EXPECT_EQ( it->second.name, "user_codes1" );
    EXPECT_EQ( it->second.value, "5e46f2537f00004699897698" );
    EXPECT_EQ( it->second.fieldName, "itemId" );
}

TEST_F( ScenesAPITests, modifyScenes ) /* 7 */ {
    std::function< void( Json& ) > removeBlockIds = [&removeBlockIds]( Json& blocks ) {
        if( blocks.is_array() ) {
            for( auto& block : blocks ) {
                block.erase( "_id" );
                block.erase( "actionGroup" );
                std::string methodName;
                json_get( block, std::vector< Json >{ "blockOptions", "method", "name" }, methodName );
                if( SceneEngine::Model::isLogicOperator( methodName ) ) {
                    removeBlockIds( block[ "fields" ][ 0 ][ "value" ] );
                }
            }
        }
        else {
            blocks.erase( "_id" );
            blocks.erase( "actionGroup" );
            std::string methodName;
            json_get( blocks, std::vector< Json >{ "blockOptions", "method", "name" }, methodName );
            if( SceneEngine::Model::isLogicOperator( methodName ) ) {
                removeBlockIds( blocks[ "fields" ][ 0 ][ "value" ] );
            }
        }
    };

    JsonObject answer;
    std::string id1, id2, id3;

    JsonObject replaceTokenCommand = R"(
    {
        "command": "replace_token",
        "to_replace": "rapid_temperature_fail",
        "replace_by": "rapid_temperature_fall"
    }
    )"_json;

    JsonObject replaceOptionLabelCommand = R"(
    {
        "command": "replace_option_label",
        "language_tag": "rapid_temperature_fail",
        "replace_by_text": "Rapid temperature fall"
    }
    )"_json;

    bool ret = getComplexScenesAPI().processCreateScene( SceneEngine::UT::createScene1, answer );
    ASSERT_TRUE( ret );
    ASSERT_TRUE( json_get( getResult(), "_id", id1 ) );
    auto expectedScene1 = getDbClient().findOne( HaScenesDBName, JSON( "_id" << id1 ), nullptr );

    ret = getComplexScenesAPI().processCreateScene( SceneEngine::UT::createScene2, answer );
    ASSERT_TRUE( ret );
    ASSERT_TRUE( json_get( getResult(), "_id", id2 ) );
    auto expectedScene2 = getDbClient().findOne( HaScenesDBName, JSON( "_id" << id2 ), nullptr );

    ret = getComplexScenesAPI().processCreateScene( SceneEngine::UT::createScene3, answer );
    ASSERT_TRUE( ret );
    ASSERT_TRUE( json_get( getResult(), "_id", id3 ) );
    getComplexScenesAPI().modifyScenes( JSON( "commands" << JSON_ARRAY_( replaceTokenCommand ) ) );
    getComplexScenesAPI().modifyScenes( JSON( "commands" << JSON_ARRAY_( replaceOptionLabelCommand ) ) );
    auto scene1 = getDbClient().findOne( HaScenesDBName, JSON( "_id" << id1 ), nullptr );
    auto scene2 = getDbClient().findOne( HaScenesDBName, JSON( "_id" << id2 ), nullptr );
    auto scene3 = getDbClient().findOne( HaScenesDBName, JSON( "_id" << id3 ), nullptr );
    scene3.erase( "generator" );
    scene3.erase( "_id" );
    removeBlockIds( scene3[ "when" ] );
    EXPECT_EQ( scene1.dump(), expectedScene1.dump() );
    EXPECT_EQ( scene2.dump(), expectedScene2.dump() );
    EXPECT_EQ( scene3.dump(), SceneEngine::UT::createScene3Expected.dump() );

    ret = getComplexScenesAPI().processDeleteScene( JSON( "_id" << id3 ), answer );
    ASSERT_TRUE( ret );
    ret = getComplexScenesAPI().processCreateScene( SceneEngine::UT::createScene3, answer );
    ASSERT_TRUE( ret );
    ASSERT_TRUE( json_get( getResult(), "_id", id3 ) );
    getComplexScenesAPI().modifyScenes(
        JSON( "commands" << JSON_ARRAY_( replaceTokenCommand, replaceOptionLabelCommand ) ) );
    scene1 = getDbClient().findOne( HaScenesDBName, JSON( "_id" << id1 ), nullptr );
    scene2 = getDbClient().findOne( HaScenesDBName, JSON( "_id" << id2 ), nullptr );
    scene3 = getDbClient().findOne( HaScenesDBName, JSON( "_id" << id3 ), nullptr );
    scene3.erase( "_id" );
    scene3.erase( "generator" );
    removeBlockIds( scene3[ "when" ] );
    EXPECT_EQ( scene1.dump(), expectedScene1.dump() );
    EXPECT_EQ( scene2.dump(), expectedScene2.dump() );
    EXPECT_EQ( scene3.dump(), SceneEngine::UT::createScene3Expected.dump() );

    ret = getComplexScenesAPI().processDeleteScene( JSON( "_id" << id3 ), answer );
    ASSERT_TRUE( ret );
    ret = getComplexScenesAPI().processCreateScene( SceneEngine::UT::createScene3, answer );
    ASSERT_TRUE( ret );
    ASSERT_TRUE( json_get( getResult(), "_id", id3 ) );
    getComplexScenesAPI().modifyScenes( JSON( "blockIds"
                                              << JSON_ARRAY_( "0001-00000022" ) << "commands"
                                              << JSON_ARRAY_( replaceTokenCommand, replaceOptionLabelCommand ) ) );
    scene1 = getDbClient().findOne( HaScenesDBName, JSON( "_id" << id1 ), nullptr );
    scene2 = getDbClient().findOne( HaScenesDBName, JSON( "_id" << id2 ), nullptr );
    scene3 = getDbClient().findOne( HaScenesDBName, JSON( "_id" << id3 ), nullptr );
    scene3.erase( "_id" );
    scene3.erase( "generator" );
    removeBlockIds( scene3[ "when" ] );
    EXPECT_EQ( scene1.dump(), expectedScene1.dump() );
    EXPECT_EQ( scene2.dump(), expectedScene2.dump() );
    EXPECT_EQ( scene3.dump(), SceneEngine::UT::createScene3Expected.dump() );

    ret = getComplexScenesAPI().processDeleteScene( JSON( "_id" << id3 ), answer );
    ASSERT_TRUE( ret );
    ret = getComplexScenesAPI().processCreateScene( SceneEngine::UT::createScene3, answer );
    ASSERT_TRUE( ret );
    ASSERT_TRUE( json_get( getResult(), "_id", id3 ) );
    getComplexScenesAPI().modifyScenes( JSON( "blockIds"
                                              << JSON_ARRAY_( "0001-00000023" ) << "commands"
                                              << JSON_ARRAY_( replaceTokenCommand, replaceOptionLabelCommand ) ) );
    scene1 = getDbClient().findOne( HaScenesDBName, JSON( "_id" << id1 ), nullptr );
    scene2 = getDbClient().findOne( HaScenesDBName, JSON( "_id" << id2 ), nullptr );
    scene3 = getDbClient().findOne( HaScenesDBName, JSON( "_id" << id3 ), nullptr );
    scene3.erase( "_id" );
    scene3.erase( "generator" );
    removeBlockIds( scene3[ "when" ] );
    EXPECT_EQ( scene1.dump(), expectedScene1.dump() );
    EXPECT_EQ( scene2.dump(), expectedScene2.dump() );
    EXPECT_EQ( scene3.dump(), SceneEngine::UT::createScene3.dump() );
}

TEST_F( ScenesAPITests, sceneBlockStatusResetOk ) /* 8.1.1 */ {
    // valid case of latch
    Json answer;
    Json sceneRequest = SceneEngine::UT::createScene1;
    Json whenBlock = SceneEngine::UT::intervalCondition;

    const char* blockIdValue = "blockIdValue";
    {
        const char* goodSettings = R"("latch": { "enabled": true })";
        auto functionOperatorStr =
            replaceTokens( SceneEngine::UT::functionOperatorWhenBlock,
                           { { "__BLOCKS__", whenBlock.dump() }, { "__FUNCTION__", goodSettings } } );

        Json functionOperator = Json::parse( functionOperatorStr );
        functionOperator[ "_id" ] = blockIdValue;
        Log::info( "#  Function Latch: " + cc::j( functionOperator ) );
        sceneRequest[ "when" ] = { functionOperator };
    }

    bool ret = getComplexScenesAPI().processCreateScene( sceneRequest, answer );
    std::string id1;
    ASSERT_TRUE( ret );
    ASSERT_TRUE( json_get( getResult(), "_id", id1 ) );

    Json params = { { "sceneId", id1 }, { "blockId", blockIdValue } };
    auto& scenesApi = getComplexScenesAPI();
    answer = scenesApi.processBlockStatusReset( params );
    Json error;

    EXPECT_FALSE( json_get( answer, "error", error ) ) << "Should be no errors";

    ret = getComplexScenesAPI().processDeleteScene( JSON( "_id" << id1 ), answer );
    ASSERT_TRUE( ret );
}

TEST_F( ScenesAPITests, resetStatusOfAllSceneBlocksOk ) /* 8.1.2 */ {
    Json answer;
    Json sceneRequest = SceneEngine::UT::createScene1;
    Json whenBlock = SceneEngine::UT::intervalCondition;

    {
        std::string functionOperatorStr = replaceTokens(
            SceneEngine::UT::functionOperatorWhenBlock,
            { { "__BLOCKS__", whenBlock.dump() }, { "__FUNCTION__", R"("latch": { "enabled": true })" } } );

        Json functionOperator = Json::parse( functionOperatorStr );
        sceneRequest[ "when" ] = { functionOperator, functionOperator, functionOperator, functionOperator };
    }

    const bool sceneCreated = getComplexScenesAPI().processCreateScene( sceneRequest, answer );
    std::string sceneId;
    ASSERT_TRUE( sceneCreated );
    ASSERT_TRUE( json_get( getResult(), "_id", sceneId ) );

    Json params = { { "sceneId", sceneId } };
    answer = getComplexScenesAPI().processBlockStatusReset( params );

    Json error;
    EXPECT_FALSE( json_get( answer, "error", error ) ) << "Should be no errors";

    const bool sceneDeleted = getComplexScenesAPI().processDeleteScene( JSON( "_id" << sceneId ), answer );
    ASSERT_TRUE( sceneDeleted );
}

TEST_F( ScenesAPITests, sceneBlockStatusResetNoSceneIdParameter ) /* 8.2.1 */ {
    // sceneId is not a in a request -> scene doesn't exist
    Json params = { { "blockId", "blockIdValue" } };
    auto& scenesApi = getComplexScenesAPI();
    Json answer = scenesApi.processBlockStatusReset( params );

    answerHasError( answer, Errors::Codes::INVALID_REQUEST, "rpc.params.notfound.sceneId", __LINE__ );
}

TEST_F( ScenesAPITests, sceneBlockStatusResetSceneIdEmpty ) /* 8.2.2 */ {
    // 1 - sceneId is empty -> scene doesn't exist
    Json params = { { "sceneId", "" }, { "blockId", "blockIdValue" } };
    auto& scenesApi = getComplexScenesAPI();
    Json answer = scenesApi.processBlockStatusReset( params );

    answerHasError( answer, Errors::Codes::APPLICATION_ERROR, "ezlo.scenes.not.exist", __LINE__ );
}

TEST_F( ScenesAPITests, sceneBlockStatusResetSceneNotFoundByName ) /* 8.2.3 */
{                                                                  // 2 - scene with this id doesn't exist
    Json params = { { "sceneId", "noSuchScene" }, { "blockId", "blockIdValue" } };
    auto& scenesApi = getComplexScenesAPI();
    Json answer = scenesApi.processBlockStatusReset( params );

    answerHasError( answer, Errors::Codes::APPLICATION_ERROR, "ezlo.scenes.not.exist", __LINE__ );
}

TEST_F( ScenesAPITests, sceneBlockStatusResetSceneBlockNotFound ) /* 8.2.4 */ {
    Json answer;
    Json sceneRequest = SceneEngine::UT::createScene1;
    Json whenBlock = SceneEngine::UT::intervalCondition;

    const char* blockIdValue = "blockIdValue";
    {
        const char* goodSettings = R"("latch": { "enabled": true })";
        auto functionOperatorStr =
            replaceTokens( SceneEngine::UT::functionOperatorWhenBlock,
                           { { "__BLOCKS__", whenBlock.dump() }, { "__FUNCTION__", goodSettings } } );

        Json functionOperator = Json::parse( functionOperatorStr );
        functionOperator[ "_id" ] = blockIdValue;
        Log::info( "#  Function Latch: " + cc::j( functionOperator ) );
        sceneRequest[ "when" ] = { functionOperator };
    }

    bool ret = getComplexScenesAPI().processCreateScene( sceneRequest, answer );
    std::string id1;
    ASSERT_TRUE( ret );
    ASSERT_TRUE( json_get( getResult(), "_id", id1 ) );

    Json params = { { "sceneId", id1 }, { "blockId", "blockIdDoesNotExist" } };
    auto& scenesApi = getComplexScenesAPI();
    answer = scenesApi.processBlockStatusReset( params );

    answerHasError( answer, Errors::Codes::APPLICATION_ERROR, "scenes.block.not.exist", __LINE__ );

    ret = getComplexScenesAPI().processDeleteScene( JSON( "_id" << id1 ), answer );
    ASSERT_TRUE( ret );
}

TEST_F( ScenesAPITests, sceneStatusGetValidParams ) /* 9.1 */ {
    Json answer;

    bool ret = getComplexScenesAPI().processCreateScene( SceneEngine::UT::createSceneForStatusGetRequest, answer );
    std::string sceneId;
    ASSERT_TRUE( ret );
    ASSERT_TRUE( json_get( getResult(), "_id", sceneId ) );

    const Json& whenBlocks = getResult()[ "when" ];
    ASSERT_EQ( whenBlocks.size(), 4 );

    const Json& exceptions = getResult()[ "exceptions" ];
    ASSERT_EQ( exceptions.size(), 1 );

    ASSERT_TRUE( whenBlocks[ 1 ][ "fields" ].is_array() );
    ASSERT_FALSE( whenBlocks[ 1 ][ "fields" ].empty() );
    ASSERT_TRUE( whenBlocks[ 1 ][ "fields" ][ 0 ][ "value" ].is_array() );
    ASSERT_FALSE( whenBlocks[ 1 ][ "fields" ][ 0 ][ "value" ].empty() );

    ASSERT_TRUE( whenBlocks[ 2 ][ "fields" ].is_array() );
    ASSERT_FALSE( whenBlocks[ 2 ][ "fields" ].empty() );
    ASSERT_TRUE( whenBlocks[ 2 ][ "fields" ][ 0 ][ "value" ].is_object() );

    ASSERT_TRUE( exceptions[ 0 ][ "fields" ].is_array() );
    ASSERT_FALSE( exceptions[ 0 ][ "fields" ].empty() );
    ASSERT_TRUE( exceptions[ 0 ][ "fields" ][ 0 ][ "value" ].is_array() );
    ASSERT_FALSE( exceptions[ 0 ][ "fields" ][ 0 ][ "value" ].empty() );

    const std::string blockId0 = json_get_str( whenBlocks[ 0 ], "_id" );
    const std::string blockId1 = json_get_str( whenBlocks[ 1 ], "_id" );
    const std::string blockId2 = json_get_str( whenBlocks[ 1 ][ "fields" ][ 0 ][ "value" ][ 0 ], "_id" );
    const std::string blockId3 = json_get_str( whenBlocks[ 2 ], "_id" );
    const std::string blockId4 = json_get_str( whenBlocks[ 2 ][ "fields" ][ 0 ][ "value" ], "_id" );
    const std::string blockId5 = json_get_str( whenBlocks[ 3 ], "_id" );
    const std::string blockId6 = json_get_str( exceptions[ 0 ], "_id" );
    const std::string blockId7 = json_get_str( exceptions[ 0 ][ "fields" ][ 0 ][ "value" ][ 0 ], "_id" );

    const std::string expected = replaceTokens( SceneEngine::UT::createSceneForStatusGetExpectedResult.dump(),
                                                { { "__SCENE_ID__", sceneId },
                                                  { "__BLOCK0_ID__", blockId0 },
                                                  { "__BLOCK1_ID__", blockId1 },
                                                  { "__BLOCK2_ID__", blockId2 },
                                                  { "__BLOCK3_ID__", blockId3 },
                                                  { "__BLOCK4_ID__", blockId4 },
                                                  { "__BLOCK5_ID__", blockId5 },
                                                  { "__BLOCK6_ID__", blockId6 },
                                                  { "__BLOCK7_ID__", blockId7 } } );
    auto expectedJson = Json::parse( expected );
    expectedJson[ "status" ][ "globalRestriction" ] = JSON_ARRAY_();

    Json params = { { "sceneId", sceneId } };
    auto& scenesApi = getComplexScenesAPI();
    answer = scenesApi.processScenesStatusGet( params );

    Json error;
    EXPECT_FALSE( json_get( answer, "error", error ) ) << "Should be no errors";

    EXPECT_EQ( expectedJson, answer );

    ret = getComplexScenesAPI().processDeleteScene( JSON( "_id" << sceneId ), answer );
    ASSERT_TRUE( ret );
}

TEST_F( ScenesAPITests, sceneStatusGetInvalidSceneId ) /* 9.2 */ {
    Json params = { { "sceneId", "invalidSceneId" } };
    Json answer = getComplexScenesAPI().processScenesStatusGet( params );
    answerHasError( answer, Errors::Codes::APPLICATION_ERROR, "ezlo.scenes.not.exist", __LINE__ );
}

TEST_F( ScenesAPITests, sceneStatusGetAbsentSceneId ) /* 9.3 */ {
    Json params = {};
    Json answer = getComplexScenesAPI().processScenesStatusGet( params );
    answerHasError( answer, Errors::Codes::INVALID_REQUEST, "rpc.params.notfound.sceneId", __LINE__ );
}

TEST_F( ScenesAPITests, sceneStatusGetEmptySceneId ) /* 9.4 */ {
    Json params = { { "sceneId", "" } };
    Json answer = getComplexScenesAPI().processScenesStatusGet( params );
    answerHasError( answer, Errors::Codes::INVALID_REQUEST, "rpc.params.empty.sceneId", __LINE__ );
}

TEST_F( ScenesAPITests, sceneStatusGetSceneIdWrongType ) /* 9.5 */ {
    Json params = { { "sceneId", 3 } };
    Json answer = getComplexScenesAPI().processScenesStatusGet( params );
    answerHasError( answer, Errors::Codes::INVALID_PARAMS, "rpc.params.invalid.sceneId", __LINE__ );
}

TEST_F( ScenesAPITests, sceneGlobalRestrictionBlockStatusResetOk ) /* 10.1 */ {
    Json answer;
    Json sceneRequest = SceneEngine::UT::createSceneWithGlobalRestriction;
    Json globalRestriction = SceneEngine::UT::intervalCondition;

    const char* blockIdValue = "blockIdValue";
    {
        const char* goodSettings = R"("latch": { "enabled": true })";
        auto functionOperatorStr =
            replaceTokens( SceneEngine::UT::functionOperatorWhenBlock,
                           { { "__BLOCKS__", globalRestriction.dump() }, { "__FUNCTION__", goodSettings } } );

        Json functionOperator = Json::parse( functionOperatorStr );
        functionOperator[ "_id" ] = blockIdValue;
        functionOperator[ "blockType" ] = "globalRestriction";
        Log::info( "#  Function Latch: " + cc::j( functionOperator ) );
        sceneRequest[ "exceptions" ] = { functionOperator };
    }

    bool ret = getComplexScenesAPI().processCreateScene( sceneRequest, answer );
    std::string id1;
    ASSERT_TRUE( ret );
    ASSERT_TRUE( json_get( getResult(), "_id", id1 ) );

    Json params = { { "sceneId", id1 }, { "blockId", blockIdValue } };
    auto& scenesApi = getComplexScenesAPI();
    answer = scenesApi.processBlockStatusReset( params );
    Json error;

    EXPECT_FALSE( json_get( answer, "error", error ) ) << "Should be no errors";

    ret = getComplexScenesAPI().processDeleteScene( JSON( "_id" << id1 ), answer );
    ASSERT_TRUE( ret );
}

TEST_F( ScenesAPITests, listScenesWithFilter ) /* 11 */ {
    {
        JsonObject answer;
        auto& scenesApi = getComplexScenesAPI();
        bool ret = scenesApi.processCreateScene( SceneEngine::UT::createScene1, answer );
        EXPECT_TRUE( ret );
        std::string id;
        JsonObject generator;
        EXPECT_TRUE( json_get( getResult(), "_id", id ) );
        EXPECT_TRUE( json_get( getResult(), "generator", generator ) );
        EXPECT_EQ( json_get_str( generator, "role" ), "user" );

        auto scenes = scenesApi.getScenesList(
            JSON( "filters" << JSON( "include" << JSON( "generator" << JSON( "role"
                                                                             << "system" ) ) ) ) );

        EXPECT_TRUE( scenes.empty() );

        ret = getComplexScenesAPI().processDeleteScene( JSON( "_id" << id ), answer );
        ASSERT_TRUE( ret );
    }

    {
        JsonObject answer;
        auto& scenesApi = getComplexScenesAPI();
        auto scene = SceneEngine::UT::createScene1;
        scene[ "generator" ] = JSON( "role"
                                     << "system"
                                     << "name"
                                     << "test"
                                     << "version"
                                     << "test" );
        bool ret = scenesApi.processCreateScene( scene, answer );
        EXPECT_TRUE( ret );
        std::string id;
        JsonObject generator;
        EXPECT_TRUE( json_get( getResult(), "_id", id ) );
        EXPECT_TRUE( json_get( getResult(), "generator", generator ) );
        EXPECT_EQ( json_get_str( generator, "role" ), "system" );

        auto scenes = scenesApi.getScenesList(
            JSON( "filters" << JSON( "include" << JSON( "generator" << JSON( "role"
                                                                             << "user" ) ) ) ) );

        EXPECT_TRUE( scenes.empty() );

        ret = getComplexScenesAPI().processDeleteScene( JSON( "_id" << id ), answer );
        ASSERT_TRUE( ret );
    }

    {
        JsonObject answer;
        auto& scenesApi = getComplexScenesAPI();
        auto scene = SceneEngine::UT::createScene1;
        scene[ "generator" ] = JSON( "role"
                                     << "system"
                                     << "name"
                                     << "test"
                                     << "version"
                                     << "test" );
        bool ret = scenesApi.processCreateScene( scene, answer );
        EXPECT_TRUE( ret );
        std::string id1;
        JsonObject generator;
        EXPECT_TRUE( json_get( getResult(), "_id", id1 ) );
        EXPECT_TRUE( json_get( getResult(), "generator", generator ) );
        EXPECT_EQ( json_get_str( generator, "role" ), "system" );

        ret = scenesApi.processCreateScene( SceneEngine::UT::createScene1, answer );
        EXPECT_TRUE( ret );
        std::string id2;
        EXPECT_TRUE( json_get( getResult(), "_id", id2 ) );
        EXPECT_TRUE( json_get( getResult(), "generator", generator ) );
        EXPECT_EQ( json_get_str( generator, "role" ), "user" );

        ret = scenesApi.processCreateScene( SceneEngine::UT::createScene1, answer );
        EXPECT_TRUE( ret );
        std::string id3;
        EXPECT_TRUE( json_get( getResult(), "_id", id3 ) );
        EXPECT_TRUE( json_get( getResult(), "generator", generator ) );
        EXPECT_EQ( json_get_str( generator, "role" ), "user" );

        auto scenes = scenesApi.getScenesList(
            JSON( "filters" << JSON( "include" << JSON( "generator" << JSON( "role"
                                                                             << "user" ) ) ) ) );

        EXPECT_FALSE( scenes.empty() );
        EXPECT_EQ( scenes.size(), 2 );

        ret = getComplexScenesAPI().processDeleteScene( JSON( "_id" << id1 ), answer );
        ASSERT_TRUE( ret );

        ret = getComplexScenesAPI().processDeleteScene( JSON( "_id" << id2 ), answer );
        ASSERT_TRUE( ret );

        ret = getComplexScenesAPI().processDeleteScene( JSON( "_id" << id3 ), answer );
        ASSERT_TRUE( ret );
    }
}

TEST_F( ScenesAPITests, listScenesWithDefaultFilter ) /* 12 */ {
    JsonObject answer;
    auto& scenesApi = getComplexScenesAPI();
    auto scene = SceneEngine::UT::createScene1;
    scene[ "generator" ] = JSON( "role"
                                 << "system"
                                 << "name"
                                 << "test"
                                 << "version"
                                 << "test" );
    bool ret = scenesApi.processCreateScene( scene, answer );
    EXPECT_TRUE( ret );
    std::string id1;
    JsonObject generator;
    EXPECT_TRUE( json_get( getResult(), "_id", id1 ) );
    EXPECT_TRUE( json_get( getResult(), "generator", generator ) );
    EXPECT_EQ( json_get_str( generator, "role" ), "system" );

    ret = scenesApi.processCreateScene( SceneEngine::UT::createScene1, answer );
    EXPECT_TRUE( ret );
    std::string id2;
    EXPECT_TRUE( json_get( getResult(), "_id", id2 ) );
    EXPECT_TRUE( json_get( getResult(), "generator", generator ) );
    EXPECT_EQ( json_get_str( generator, "role" ), "user" );

    ret = scenesApi.processCreateScene( SceneEngine::UT::createScene1, answer );
    EXPECT_TRUE( ret );
    std::string id3;
    EXPECT_TRUE( json_get( getResult(), "_id", id3 ) );
    EXPECT_TRUE( json_get( getResult(), "generator", generator ) );
    EXPECT_EQ( json_get_str( generator, "role" ), "user" );

    auto scenes = scenesApi.getScenesList( Json{} );

    EXPECT_FALSE( scenes.empty() );
    EXPECT_EQ( scenes.size(), 2 );

    for( auto& sceneData : scenes ) {
        EXPECT_TRUE( json_get( sceneData, "generator", generator ) );
        EXPECT_EQ( json_get_str( generator, "role" ), "user" );
    }

    ret = getComplexScenesAPI().processDeleteScene( JSON( "_id" << id1 ), answer );
    ASSERT_TRUE( ret );

    ret = getComplexScenesAPI().processDeleteScene( JSON( "_id" << id2 ), answer );
    ASSERT_TRUE( ret );

    ret = getComplexScenesAPI().processDeleteScene( JSON( "_id" << id3 ), answer );
    ASSERT_TRUE( ret );
}

TEST_F( ScenesAPITests, listScenesWithSystemFilter ) /* 13 */ {
    JsonObject answer;
    auto& scenesApi = getComplexScenesAPI();
    auto scene = SceneEngine::UT::createScene1;
    scene[ "generator" ] = JSON( "role"
                                 << "system"
                                 << "name"
                                 << "test"
                                 << "version"
                                 << "test" );
    bool ret = scenesApi.processCreateScene( scene, answer );
    EXPECT_TRUE( ret );
    std::string id1;
    JsonObject generator;
    EXPECT_TRUE( json_get( getResult(), "_id", id1 ) );
    EXPECT_TRUE( json_get( getResult(), "generator", generator ) );
    EXPECT_EQ( json_get_str( generator, "role" ), "system" );

    ret = scenesApi.processCreateScene( SceneEngine::UT::createScene1, answer );
    EXPECT_TRUE( ret );
    std::string id2;
    EXPECT_TRUE( json_get( getResult(), "_id", id2 ) );
    EXPECT_TRUE( json_get( getResult(), "generator", generator ) );
    EXPECT_EQ( json_get_str( generator, "role" ), "user" );

    ret = scenesApi.processCreateScene( SceneEngine::UT::createScene1, answer );
    EXPECT_TRUE( ret );
    std::string id3;
    EXPECT_TRUE( json_get( getResult(), "_id", id3 ) );
    EXPECT_TRUE( json_get( getResult(), "generator", generator ) );
    EXPECT_EQ( json_get_str( generator, "role" ), "user" );

    auto scenes =
        scenesApi.getScenesList( JSON( "filters" << JSON( "include" << JSON( "generator" << JSON( "role"
                                                                                                  << "system" ) ) ) ) );

    EXPECT_FALSE( scenes.empty() );
    EXPECT_EQ( scenes.size(), 1 );

    for( auto& sceneData : scenes ) {
        EXPECT_TRUE( json_get( sceneData, "generator", generator ) );
        EXPECT_EQ( json_get_str( generator, "role" ), "system" );
    }

    ret = getComplexScenesAPI().processDeleteScene( JSON( "_id" << id1 ), answer );
    ASSERT_TRUE( ret );

    ret = getComplexScenesAPI().processDeleteScene( JSON( "_id" << id2 ), answer );
    ASSERT_TRUE( ret );

    ret = getComplexScenesAPI().processDeleteScene( JSON( "_id" << id3 ), answer );
    ASSERT_TRUE( ret );
}

TEST_F( ScenesAPITests, listScenesWithAllFilter ) /* 14 */ {
    JsonObject answer;
    auto& scenesApi = getComplexScenesAPI();
    auto scene = SceneEngine::UT::createScene1;
    scene[ "generator" ] = JSON( "role"
                                 << "system"
                                 << "name"
                                 << "test"
                                 << "version"
                                 << "test" );
    bool ret = scenesApi.processCreateScene( scene, answer );
    EXPECT_TRUE( ret );
    std::string id1;
    JsonObject generator;
    EXPECT_TRUE( json_get( getResult(), "_id", id1 ) );
    EXPECT_TRUE( json_get( getResult(), "generator", generator ) );
    EXPECT_EQ( json_get_str( generator, "role" ), "system" );

    ret = scenesApi.processCreateScene( SceneEngine::UT::createScene1, answer );
    EXPECT_TRUE( ret );
    std::string id2;
    EXPECT_TRUE( json_get( getResult(), "_id", id2 ) );
    EXPECT_TRUE( json_get( getResult(), "generator", generator ) );
    EXPECT_EQ( json_get_str( generator, "role" ), "user" );

    ret = scenesApi.processCreateScene( SceneEngine::UT::createScene1, answer );
    EXPECT_TRUE( ret );
    std::string id3;
    EXPECT_TRUE( json_get( getResult(), "_id", id3 ) );
    EXPECT_TRUE( json_get( getResult(), "generator", generator ) );
    EXPECT_EQ( json_get_str( generator, "role" ), "user" );

    auto scenes =
        scenesApi.getScenesList( JSON( "filters" << JSON( "include" << JSON( "generator" << JSON( "role"
                                                                                                  << "all" ) ) ) ) );

    EXPECT_FALSE( scenes.empty() );
    EXPECT_EQ( scenes.size(), 3 );

    ret = getComplexScenesAPI().processDeleteScene( JSON( "_id" << id1 ), answer );
    ASSERT_TRUE( ret );

    ret = getComplexScenesAPI().processDeleteScene( JSON( "_id" << id2 ), answer );
    ASSERT_TRUE( ret );

    ret = getComplexScenesAPI().processDeleteScene( JSON( "_id" << id3 ), answer );
    ASSERT_TRUE( ret );
}

TEST_F( ScenesAPITests, listScenesEmpty ) /* 15 */ {
    JsonObject answer;
    auto& scenesApi = getComplexScenesAPI();
    auto scene = SceneEngine::UT::createScene1;
    scene[ "generator" ] = JSON( "role"
                                 << "system"
                                 << "name"
                                 << "test"
                                 << "version"
                                 << "test" );
    bool ret = scenesApi.processCreateScene( scene, answer );
    EXPECT_TRUE( ret );
    std::string id1;
    JsonObject generator;
    EXPECT_TRUE( json_get( getResult(), "_id", id1 ) );
    EXPECT_TRUE( json_get( getResult(), "generator", generator ) );
    EXPECT_EQ( json_get_str( generator, "role" ), "system" );

    auto scenes = scenesApi.getScenesList( JSON( "filters" << JSON( "generator" << JSON( "role"
                                                                                         << "all" ) ) ) );

    EXPECT_TRUE( scenes.empty() );  // due to wrong params passed

    ret = getComplexScenesAPI().processDeleteScene( JSON( "_id" << id1 ), answer );
    ASSERT_TRUE( ret );
}
}  // namespace UT
}  // namespace HubPlatform
