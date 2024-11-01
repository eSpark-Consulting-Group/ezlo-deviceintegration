#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "hardware_manager/battery/battery_info.h"

namespace HubPlatform {
namespace Tests {

void SceneManagerTestsFixture::SetUp() {
    CoreNmaTest::SetUp();
    updatePredefineGroupsForTest( "instant" );
}
void SceneManagerTestsFixture::TearDown() {
    deleteScenes();
    CoreNmaTest::TearDown();
}

JsonObject SceneManagerTestsFixture::sendRequestAndCheckAnswer( const Json& request, int codeLine ) {
    auto codeLineStr = getCodeLineString( codeLine );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    const auto& error = answer[ "error" ];
    EXPECT_TRUE( error.empty() ) << error.dump( 4 ) << ", " << codeLineStr;
    if( !error.empty() ) {
        throw std::runtime_error( "Received unexpected error in response" );
    }
    return answer;
}

JsonObject SceneManagerTestsFixture::sendRequestAndCheckFailedAnswer( const JsonObject& request, int codeLine ) {
    auto codeLineStr = getCodeLineString( codeLine );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    const auto& error = answer[ "error" ];
    EXPECT_FALSE( error.empty() ) << codeLineStr;
    if( error.empty() ) {
        throw std::runtime_error( "Expected to receive an error in response, but the 'error' is empty" );
    }
    return answer;
}

JsonObject SceneManagerTestsFixture::sendRequestAndCheckFailedAnswer( const JsonObject& request,
                                                                      Errors::Error expectedError,
                                                                      const std::string& expectedMessage,
                                                                      int codeLine ) {
    JsonObject answer = nma().sendtoHubAndGetAnswer( request );
    const JsonObject& errorJson = answer[ "error" ];

    JsonObject expectedErrorJson;
    Errors::get( expectedError, expectedErrorJson );
    expectedErrorJson[ "reason" ] = expectedMessage;
    EXPECT_EQ( errorJson.dump(), expectedErrorJson.dump() ) << getCodeLineString( codeLine );

    if( errorJson.empty() ) {
        throw std::runtime_error( "Expected to receive an error in response, but the 'error' is empty" );
    }

    return answer;
}

SceneManagerTestsFixture::SceneID SceneManagerTestsFixture::createScene( const JsonObject& sceneJson, int codeLine ) {
    JsonObject unused;
    return createScene( sceneJson, unused, codeLine );
}

SceneManagerTestsFixture::SceneID SceneManagerTestsFixture::createScene( const JsonObject& sceneJson,
                                                                         JsonObject& resultFromBroadcast,
                                                                         int codeLine ) {
    auto codeLineStr = getCodeLineString( codeLine );
    sendRequestAndCheckAnswer( sceneJson, codeLine );
    JsonObject broadcast = waitForBroadcast( "hub.scene.added", codeLineStr );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.added" ) << codeLineStr;
    JsonObject result;
    JsonObject params;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << codeLineStr;
    EXPECT_TRUE( json_get_obj( sceneJson, "params", params ) ) << codeLineStr;
    EXPECT_EQ( json_get_bool( result, "enabled" ), json_get_bool( params, "enabled" ) ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "group_id" ), json_get_str( params, "group_id" ) ) << codeLineStr;
    EXPECT_EQ( json_get_bool( result, "is_group" ), json_get_bool( params, "is_group" ) ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "name" ), json_get_str( params, "name" ) ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "parent_id" ), json_get_str( params, "parent_id" ) ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "execPolicy" ), json_get_str( params, "execPolicy" ) ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "icon" ), json_get_str( params, "icon" ) ) << codeLineStr;
    SceneID id = json_get_str( result, "_id" );
    EXPECT_TRUE( !id.empty() ) << codeLineStr;
    m_createdScenes.insert( id );
    resultFromBroadcast = result;
    return id;
}

SceneManagerTestsFixture::SceneID SceneManagerTestsFixture::cloneScene( const JsonObject& request,
                                                                        JsonObject& resultFromBroadcast,
                                                                        int codeLine ) {
    auto codeLineStr = getCodeLineString( codeLine );
    sendRequestAndCheckAnswer( request, codeLine );
    JsonObject broadcast = waitForBroadcast( "hub.scene.added", codeLineStr );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.added" ) << codeLineStr;
    JsonObject result;
    JsonObject params;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << codeLineStr;
    EXPECT_TRUE( json_get_obj( request, "params", params ) ) << codeLineStr;
    SceneID id = json_get_str( result, "_id" );
    EXPECT_TRUE( !id.empty() ) << codeLineStr;
    m_createdScenes.insert( id );
    resultFromBroadcast = result;
    return id;
}

void SceneManagerTestsFixture::editScene( const std::string& sceneId, const JsonObject& sceneJson, int codeLine ) {
    auto codeLineStr = getCodeLineString( codeLine );

    Json editedScene = Tests::editScene;
    editedScene[ "params" ][ "_id" ] = sceneId;
    editedScene[ "params" ][ "eo" ] = sceneJson;
    auto answer = sendRequestAndCheckAnswer( editedScene, codeLine );
    // check answer
    ASSERT_TRUE( answer[ "error" ].is_null() ) << codeLineStr;

    // check broadcast
    checkChangedBroadcastEditRequest( sceneId, editedScene, codeLine );
}

void SceneManagerTestsFixture::testBatteryLevelWithComparator( const std::string& comparator,
                                                               int level1,
                                                               int level2,
                                                               int codeLine ) {
    auto checkBatteryCondition =
        replaceTokensInString( checkBatteryLevelConditionBlock, { { "__COMP__", comparator } } );

    auto checkBatteryLevelScene = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isBatteryLevel" }, { "__WHEN_BLOCKS__", checkBatteryCondition }, { "__THEN_BLOCKS__", "" } } );

    auto sceneId = createScene( checkBatteryLevelScene, codeLine );
    HubPlatform::HardwareManager::BatteryInfo info;
    info.stateOfCharge = level2;
    nma().sendBroadcast( HUB_BATTERY_INFO_UPDATED, info.toJson() );
    info.stateOfCharge = level1;
    nma().sendBroadcast( HUB_BATTERY_INFO_UPDATED, info.toJson() );
    testRunProgressBroadcast( checkBatteryLevelScene, sceneId, "started", codeLine );
    testRunProgressBroadcast( checkBatteryLevelScene, sceneId, "finished", codeLine );
    deleteScene( sceneId );
}

std::string SceneManagerTestsFixture::createSecuritySensorDevice( const std::string& deviceName,
                                                                  const std::string& subcategory ) {
    using namespace CoreD::Model::Resources;
    auto& coreRemoteInterface = makeRemote();
    auto deviceId = addDevice( coreRemoteInterface,
                               deviceName,
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               subcategory,
                               false /*batteryPowered*/,
                               __LINE__ );
    auto itemId =
        addItem( coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    auto securityItemId = addItem(
        coreRemoteInterface, deviceId, "security_threat", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, true } }, __LINE__ );
    return deviceId;
}

void SceneManagerTestsFixture::testRunProgressBroadcast( const JsonObject& createJson,
                                                         const std::string& sceneId,
                                                         const std::string& status,
                                                         int codeLine,
                                                         const JsonArray& notifications,
                                                         std::chrono::seconds timeoutSeconds ) {
    std::string codeLineStr = getCodeLineString( codeLine );
    JsonObject broadcast =
        waitForBroadcast( "hub.scene.run.progress",
                          "testRunProgressBroadcast scene: " + sceneId + ", status: " + status + ", " + codeLineStr,
                          timeoutSeconds );
    testRunProgressBroadcast( broadcast, createJson, sceneId, status, codeLine, notifications );
}

void SceneManagerTestsFixture::testRunProgressBroadcast( const JsonObject& broadcast,
                                                         const JsonObject& createJson,
                                                         const std::string& sceneId,
                                                         const std::string& status,
                                                         int codeLine,
                                                         const JsonArray& notifications ) {
    std::string codeLineStr = getCodeLineString( codeLine );
    JsonObject result = JSON_EMPTY_OBJ;
    JsonArray resultNotifications = JSON_EMPTY_ARR;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << codeLineStr;
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.run.progress" ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "scene_id" ), sceneId ) << codeLineStr;
    EXPECT_EQ( result[ "scene_name" ].dump(), createJson[ "params" ][ "name" ].dump() ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "status" ), status ) << codeLineStr;
    EXPECT_EQ( json_get_bool( result, "userNotification" ), status != "started" ) << codeLineStr;
    EXPECT_TRUE( json_get_arr( result, "notifications", resultNotifications ) ) << codeLineStr;
    EXPECT_EQ( resultNotifications.dump(), notifications.dump() ) << codeLineStr;
}

bool SceneManagerTestsFixture::waitAndCheckRunProgressBroadcast( const JsonObject& createJson,
                                                                 const std::string& sceneId,
                                                                 const std::string& status,
                                                                 int codeLine ) {
    std::string codeLineStr = getCodeLineString( codeLine );
    auto broadcast = waitForBroadcast( "hub.scene.run.progress", codeLineStr );

    JsonObject result = JSON_EMPTY_OBJ;
    JsonArray resultNotifications = JSON_EMPTY_ARR;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << codeLineStr;

    if( json_get_str( broadcast, "msg_subclass" ) == "hub.scene.run.progress" ) {
        if( json_get_str( result, "scene_id" ) == sceneId ) {
            if( result[ "scene_name" ].dump() == createJson[ "params" ][ "name" ].dump() ) {
                if( json_get_str( result, "status" ) == status )
                    return true;
            }
        }
    }
    return false;
}

bool SceneManagerTestsFixture::isRunProgressBroadcast( const JsonObject& broadcast,
                                                       const JsonObject& createJson,
                                                       const std::string& sceneId,
                                                       const std::string& status ) {
    JsonObject result = JSON_EMPTY_OBJ;
    return json_get_obj( broadcast, "result", result ) &&
           json_get_str( broadcast, "msg_subclass" ) == "hub.scene.run.progress" &&
           json_get_str( result, "scene_id" ) == sceneId &&
           result[ "scene_name" ] == createJson[ "params" ][ "name" ] && json_get_str( result, "status" ) == status &&
           json_get_bool( result, "userNotification" ) == ( status != "started" );
}

bool SceneManagerTestsFixture::isRunProgressBroadcast( const JsonObject& broadcast,
                                                       const std::string& sceneId,
                                                       const std::string& status ) {
    JsonObject result = JSON_EMPTY_OBJ;
    return json_get_obj( broadcast, "result", result ) &&
           json_get_str( broadcast, "msg_subclass" ) == "hub.scene.run.progress" &&
           json_get_str( result, "scene_id" ) == sceneId && json_get_str( result, "status" ) == status &&
           json_get_bool( result, "userNotification" ) == ( status != "started" );
}

bool SceneManagerTestsFixture::isEnableChangedBroadcast( const JsonObject& broadcast,
                                                         const std::string& sceneId,
                                                         bool isEnabled ) {
    JsonObject result = JSON_EMPTY_OBJ;
    return json_get_obj( broadcast, "result", result ) &&
           json_get_str( broadcast, "msg_subclass" ) == "hub.scene.changed" &&
           json_get_str( result, "_id" ) == sceneId && json_get_bool( result, "enabled" ) == isEnabled;
}

void SceneManagerTestsFixture::deleteScenes() {
    auto createdScenes = m_createdScenes;
    for( const auto& id : createdScenes ) {
        deleteScene( id );
    }
}

void SceneManagerTestsFixture::runSceneManually( SceneManagerTestsFixture::SceneID sceneId, int codeLine ) {
    std::string dump = HubPlatform::Tests::runSceneManually.dump();
    std::string toReplace = "__ID__";
    dump.replace( dump.find( toReplace ), toReplace.size(), sceneId );
    sendRequest( JsonObject::parse( dump ), codeLine );
}

void SceneManagerTestsFixture::runSceneManually( SceneManagerTestsFixture::SceneID sceneId,
                                                 const std::string& block,
                                                 int codeLine ) {
    auto request = replaceTokens( HubPlatform::Tests::runSceneManually, { { "__ID__", sceneId } } );
    request[ "params" ][ "block" ] = block;
    sendRequest( request, codeLine );
}

void SceneManagerTestsFixture::checkChangedBroadcast( SceneManagerTestsFixture::SceneID sceneId,
                                                      const JsonObject& expected,
                                                      int codeLine ) {
    std::string codeLineStr = getCodeLineString( codeLine );
    JsonObject broadcast = waitForBroadcast( "hub.scene.changed", codeLineStr );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.changed" ) << codeLineStr;
    JsonObject result;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "_id" ), sceneId ) << codeLineStr;
    EXPECT_EQ( json_get_bool( result, "enabled" ), json_get_bool( expected, "enabled" ) ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "group_id" ), json_get_str( expected, "group_id" ) ) << codeLineStr;
    EXPECT_EQ( json_get_bool( result, "is_group" ), json_get_bool( expected, "is_group" ) ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "name" ), json_get_str( expected, "name" ) ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "parent_id" ), json_get_str( expected, "parent_id" ) ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "icon" ), json_get_str( expected, "icon" ) ) << codeLineStr;
}

void SceneManagerTestsFixture::checkChangedBroadcast( SceneManagerTestsFixture::SceneID sceneId, int codeLine ) {
    std::string codeLineStr = getCodeLineString( codeLine );
    JsonObject broadcast = waitForBroadcast( "hub.scene.changed", codeLineStr );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.changed" ) << codeLineStr;
    JsonObject result;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "_id" ), sceneId ) << codeLineStr;
}

void SceneManagerTestsFixture::checkChangedBroadcastEditRequest( SceneManagerTestsFixture::SceneID sceneId,
                                                                 const JsonObject& editRequest,
                                                                 int codeLine ) {
    std::string codeLineStr = getCodeLineString( codeLine );
    JsonObject params, eo;
    EXPECT_TRUE( json_get_obj( editRequest, "params", params ) ) << codeLineStr;
    EXPECT_TRUE( json_get_obj( params, "eo", eo ) ) << codeLineStr;
    checkChangedBroadcast( sceneId, eo, codeLine );
}

void SceneManagerTestsFixture::checkChangedBroadcastEnableRequest( SceneManagerTestsFixture::SceneID sceneId,
                                                                   const JsonObject& scene,
                                                                   bool enable,
                                                                   int codeLine ) {
    std::string codeLineStr = getCodeLineString( codeLine );
    JsonObject params;
    EXPECT_TRUE( json_get_obj( scene, "params", params ) ) << codeLineStr;
    params[ "enabled" ] = enable;
    checkChangedBroadcast( sceneId, params, codeLine );
}

void SceneManagerTestsFixture::checkErrorFieldNotFound( const JsonObject& answer,
                                                        const std::string& fieldName,
                                                        int codeLine ) {
    auto codeLineStr = getCodeLineString( codeLine );
    const auto& error = answer[ "error" ];
    ASSERT_FALSE( error.empty() ) << codeLineStr;
    EXPECT_EQ( json_get_int( error, "code" ), -32600 ) << codeLineStr;
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.notfound." + fieldName ) << codeLineStr;
    EXPECT_EQ( json_get_str( error, "message" ), "Bad request, " + fieldName + " does not exist" ) << codeLineStr;
}

void SceneManagerTestsFixture::checkErrorEmptyField( const JsonObject& answer,
                                                     const std::string& fieldName,
                                                     int codeLine ) {
    auto codeLineStr = getCodeLineString( codeLine );
    const auto& error = answer[ "error" ];
    ASSERT_FALSE( error.empty() ) << codeLineStr;
    EXPECT_EQ( json_get_int( error, "code" ), -32600 ) << codeLineStr;
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.empty." + fieldName ) << codeLineStr;
    EXPECT_EQ( json_get_str( error, "message" ), "Bad request, " + fieldName + " is empty" ) << codeLineStr;
}

std::string SceneManagerTestsFixture::hostName() {
    return "localhost";
}

unsigned& SceneManagerTestsFixture::getPort() {
    return m_port;
}

std::string SceneManagerTestsFixture::makeHostName() {
    return hostName() + ":" + std::to_string( getPort() );
}

std::string SceneManagerTestsFixture::makeUrl( const char* path, const char* hostName, const char* proto ) {
    if( !proto || !*proto )
        proto = "http";
    if( !path || !*path )
        path = "/";
    std::stringstream os;
    os << proto << "://" << ( hostName && *hostName ? hostName : makeHostName() ) << path;
    return os.str();
}

void SceneManagerTestsFixture::deleteScene( const SceneManagerTestsFixture::SceneID& sceneId, int codeLine ) {
    deleteScene(
        m_createdScenes, sceneId, "hub.scenes.delete", "hub.scene.deleted", ( codeLine > -1 ? codeLine : __LINE__ ) );
}

void SceneManagerTestsFixture::deleteScene( std::set< SceneID >& container,
                                            const SceneManagerTestsFixture::SceneID& sceneId,
                                            const std::string& methodName,
                                            const std::string& broadcastName,
                                            int codeLine ) {
    auto codeLineStr = getCodeLineString( codeLine );
    container.erase( sceneId );
    auto deleteSceneJson = HubPlatform::Tests::deleteScene;
    deleteSceneJson[ "method" ] = methodName;
    deleteSceneJson[ "params" ][ "_id" ] = sceneId;
    auto answer = nma().sendtoHubAndGetAnswer( deleteSceneJson );
    const auto& error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 ) << codeLineStr;
    EXPECT_EQ( json_get_str( answer, "method" ), methodName ) << codeLineStr;
    JsonObject broadcast = waitForBroadcast( broadcastName, "deleteScene: " + sceneId );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), broadcastName ) << codeLineStr;
    EXPECT_EQ( broadcast[ "result" ][ "_id" ].dump(), "\"" + sceneId + "\"" ) << codeLineStr;
    if( methodName == "hub.scenes.delete" ) {
        EXPECT_EQ( broadcast[ "result" ][ "syncNotification" ].dump(), "true" ) << codeLineStr;
    }
    else if( methodName == "hub.scenes.global.delete" ) {
        EXPECT_EQ( broadcast[ "result" ].count( "syncNotification" ), 0 );
    }
}

JsonArray SceneManagerTestsFixture::getOptionValues( const JsonObject& block ) const {
    JsonArray options;
    EXPECT_TRUE( json_get( block, { "fields", 1, "options" }, options ) );
    JsonArray values;
    for( const auto& option : options ) {
        values.push_back( json_get_str( option, "value" ) );
    }
    return values;
}

void SceneManagerTestsFixture::testIsSceneStateResult( const std::string& result ) {
    auto& coreRemoteInterface = makeRemote( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 = addItem(
        coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );

    auto setItemValue1 = replaceTokensInString(
        setItemValueThenBlock, { { "__ITEM_ID__", itemId1 }, { "__TYPE__", "bool" }, { "__VALUE__", "false" } } );

    auto setItemValue2 =
        replaceTokensInString( setItemValueThenBlock, { { "__ITEM_ID__", "wrong_id" }, { "__VALUE__", "true" } } );

    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "TriggerScene" },
                                    { "__EXEC_POLICY__", "ignore_result" },
                                    { "__WHEN_BLOCKS__", "" },
                                    { "__THEN_BLOCKS__", setItemValue1 + "," + setItemValue2 } } );
    auto sceneId1 = createScene( request, __LINE__ );

    auto isSceneState =
        replaceTokensInString( isSceneStateWhenBlock, { { "__SCENE_ID__", sceneId1 }, { "__STATE__", result } } );
    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isSceneState" }, { "__WHEN_BLOCKS__", isSceneState }, { "__THEN_BLOCKS__", "" } } );
    auto sceneId2 = createScene( request, __LINE__ );

    runSceneManually( sceneId1, __LINE__ );

    if( result == "partially_finished" || result == "any_result" ) {
        checkBroadcasts( { [this, &sceneId1]( const Json& broadcast ) {
                              return isRunProgressBroadcast( broadcast, sceneId1, "started" );
                          },
                           [this, &sceneId1]( const Json& broadcast ) {
                               return isRunProgressBroadcast( broadcast, sceneId1, "partially_finished" );
                           },
                           [this, &sceneId2]( const Json& broadcast ) {
                               return isRunProgressBroadcast( broadcast, sceneId2, "started" );
                           },
                           [this, &sceneId2]( const Json& broadcast ) {
                               return isRunProgressBroadcast( broadcast, sceneId2, "finished" );
                           } },
                         __LINE__ );
    }
    else {
        checkBroadcasts( { [this, &sceneId1]( const Json& broadcast ) {
                              return isRunProgressBroadcast( broadcast, sceneId1, "started" );
                          },
                           [this, &sceneId1]( const Json& broadcast ) {
                               return isRunProgressBroadcast( broadcast, sceneId1, "partially_finished" );
                           } },
                         __LINE__ );
    }

    deleteScenes();
    //-----------------------------------------------------------------------------------------------------------
    request = replaceTokens( createSceneRequest,
                             { { "__NAME__", "TriggerScene" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", "" } } );
    sceneId1 = createScene( request, __LINE__ );

    isSceneState =
        replaceTokensInString( isSceneStateWhenBlock, { { "__SCENE_ID__", sceneId1 }, { "__STATE__", result } } );
    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isSceneState" }, { "__WHEN_BLOCKS__", isSceneState }, { "__THEN_BLOCKS__", "" } } );
    sceneId2 = createScene( request, __LINE__ );

    runSceneManually( sceneId1, __LINE__ );

    if( result == "finished" || result == "any_result" ) {
        checkBroadcasts( { [this, &sceneId1]( const Json& broadcast ) {
                              return isRunProgressBroadcast( broadcast, sceneId1, "started" );
                          },
                           [this, &sceneId1]( const Json& broadcast ) {
                               return isRunProgressBroadcast( broadcast, sceneId1, "finished" );
                           },
                           [this, &sceneId2]( const Json& broadcast ) {
                               return isRunProgressBroadcast( broadcast, sceneId2, "started" );
                           },
                           [this, &sceneId2]( const Json& broadcast ) {
                               return isRunProgressBroadcast( broadcast, sceneId2, "finished" );
                           } },
                         __LINE__ );
    }
    else {
        checkBroadcasts( { [this, &sceneId1]( const Json& broadcast ) {
                              return isRunProgressBroadcast( broadcast, sceneId1, "started" );
                          },
                           [this, &sceneId1]( const Json& broadcast ) {
                               return isRunProgressBroadcast( broadcast, sceneId1, "finished" );
                           } },
                         __LINE__ );
    }

    deleteScenes();
    //-----------------------------------------------------------------------------------------------------------
    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "TriggerScene" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", setItemValue2 } } );
    sceneId1 = createScene( request, __LINE__ );

    isSceneState =
        replaceTokensInString( isSceneStateWhenBlock, { { "__SCENE_ID__", sceneId1 }, { "__STATE__", result } } );
    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isSceneState" }, { "__WHEN_BLOCKS__", isSceneState }, { "__THEN_BLOCKS__", "" } } );
    sceneId2 = createScene( request, __LINE__ );

    runSceneManually( sceneId1, __LINE__ );

    if( result == "failed" || result == "any_result" ) {
        checkBroadcasts( { [this, &sceneId1]( const Json& broadcast ) {
                              return isRunProgressBroadcast( broadcast, sceneId1, "started" );
                          },
                           [this, &sceneId1]( const Json& broadcast ) {
                               return isRunProgressBroadcast( broadcast, sceneId1, "failed" );
                           },
                           [this, &sceneId2]( const Json& broadcast ) {
                               return isRunProgressBroadcast( broadcast, sceneId2, "started" );
                           },
                           [this, &sceneId2]( const Json& broadcast ) {
                               return isRunProgressBroadcast( broadcast, sceneId2, "finished" );
                           } },
                         __LINE__ );
    }
    else {
        checkBroadcasts( { [this, &sceneId1]( const Json& broadcast ) {
                              return isRunProgressBroadcast( broadcast, sceneId1, "started" );
                          },
                           [this, &sceneId1]( const Json& broadcast ) {
                               return isRunProgressBroadcast( broadcast, sceneId1, "failed" );
                           } },
                         __LINE__ );
    }

    JsonObject broadcast;
    coreRemoteInterface.removeDevice( deviceId );
    broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
}

void SceneManagerTestsFixture::removeBlockIds( Json& blocks ) {
    if( blocks.is_array() ) {
        for( auto& block : blocks ) {
            block.erase( "_id" );
            block.erase( "actionGroup" );
            std::string methodName;
            json_get( block, std::vector< Json >{ "blockOptions", "method", "name" }, methodName );
            if( methodName == "and" || methodName == "or" || methodName == "not" ) {
                removeBlockIds( block[ "fields" ][ 0 ][ "value" ] );
            }
        }
    }
    else {
        blocks.erase( "_id" );
        blocks.erase( "actionGroup" );
        std::string methodName;
        json_get( blocks, std::vector< Json >{ "blockOptions", "method", "name" }, methodName );
        if( methodName == "and" || methodName == "or" || methodName == "not" ) {
            removeBlockIds( blocks[ "fields" ][ 0 ][ "value" ] );
        }
    }
}

void SceneManagerTestsFixture::sendNetworkChangedBroadcast( const std::string& interface, bool isInternetAvailable ) {
    nma().sendBroadcast( HUB_NETWORK_CHANGED,
                         JSON( "interfaces" << JSON_ARRAY_(
                                   JSON( "_id" << interface << "internetAvailable" << isInternetAvailable ) ) ) );
    waitForBroadcast( "hub.network.changed",
                      "hub.network.changed broadcast, code line: " + std::to_string( __LINE__ ),
                      progressBroadcastTimeout );
}

void SceneManagerTestsFixture::setGeoCoordinates() {
    auto answer = nma().sendtoHubAndGetAnswer( HubPlatform::Tests::setGeoCoordinates );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
}

JsonObject SceneManagerTestsFixture::getSunStateRequest( const std::string& type,
                                                         const std::string& sunState,
                                                         const std::string& when,
                                                         const std::string& rangeType,
                                                         const std::string& time ) {
    std::string dump;
    if( type == "daily" ) {
        dump = HubPlatform::Tests::createIsSunStateDailyScene.dump();
        std::string toReplace = "__RANGE_TYPE__";
        dump.replace( dump.find( toReplace ), toReplace.size(), rangeType );
    }
    else if( type == "weekly" ) {
        dump = HubPlatform::Tests::createIsSunStateWeeklyScene.dump();
    }
    else if( type == "monthly" ) {
        dump = HubPlatform::Tests::createIsSunStateMonthlyScene.dump();
    }
    std::string toReplace = "__SUNSTATE__";
    dump.replace( dump.find( toReplace ), toReplace.size(), sunState );
    dump.replace( dump.find( toReplace ), toReplace.size(), sunState );
    toReplace = "__WHEN__";
    dump.replace( dump.find( toReplace ), toReplace.size(), when );
    toReplace = "__TIME__";
    dump.replace( dump.find( toReplace ), toReplace.size(), time );
    return JsonObject::parse( dump );
}

void SceneManagerTestsFixture::enableSceneBlock( const std::string& sceneId,
                                                 const std::string& blockId,
                                                 bool enabled,
                                                 int codeLine ) {
    auto request = replaceTokens(
        setBlockEnabledSetRequest,
        { { "__SCENE_ID__", sceneId }, { "__BLOCK_ID__", blockId }, { "__ENABLED__", enabled ? "true" : "false" } } );
    sendRequest( request, codeLine );
    checkChangedBroadcast( sceneId, codeLine );
}

JsonObject SceneManagerTestsFixture::getSceneStatus( const std::string& sceneId, int codeLine ) {
    auto request = replaceTokens( getSceneStatusRequest, { { "__SCENE_ID__", sceneId } } );
    return sendRequestAndCheckAnswer( request, codeLine );
}

SceneManagerTestsFixture::SceneID SceneManagerTestsFixture::sceneWithBlocks( int codeLine ) {
    auto dev = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, codeLine );
    auto request = createEmptyScene;

    std::string actionGroupIdOfWhenBlock = "0";

    auto whenBlock = replaceTokens( isItemStateWhenBlock,
                                    { { "__ITEM_ID__", dev.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "10" } } );

    // Action group will be linked to this when block
    whenBlock[ "actionGroup" ] = actionGroupIdOfWhenBlock;
    auto whenBlocks = replaceTokens( andOperatorWhenBlock, { { "__BLOCKS__", std::move( whenBlock.dump() ) } } );
    request[ "params" ][ "when" ].push_back( whenBlocks );
    request[ "params" ][ "exceptions" ].push_back( whenBlocks );
    request[ "params" ][ "then" ].push_back( replaceTokens(
        setItemValueThenBlock, { { "__ITEM_ID__", dev.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "11" } } ) );
    request[ "params" ][ "else" ].push_back( replaceTokens(
        setItemValueThenBlock, { { "__ITEM_ID__", dev.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "11" } } ) );
    request[ "params" ][ "thenGroups" ][ actionGroupIdOfWhenBlock ].push_back( replaceTokens(
        setItemValueThenBlock, { { "__ITEM_ID__", dev.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "12" } } ) );
    request[ "params" ][ "elseGroups" ][ actionGroupIdOfWhenBlock ].push_back( replaceTokens(
        setItemValueThenBlock, { { "__ITEM_ID__", dev.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "12" } } ) );

    return createScene( request, codeLine );
}

}  // namespace Tests
}  // namespace HubPlatform
