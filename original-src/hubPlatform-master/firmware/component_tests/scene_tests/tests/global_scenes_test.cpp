#include "scene_tests/application.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/cored_resources.h"

#include "jsonrpc/message.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN

DONE    1. Global Scenes
DONE        1.1 hub.scenes.global.create
DONE        1.2 hub.scenes.global.list
DONE        1.3 hub.scenes.global.delete
DONE        1.4 hub.scenes.global.edit
DONE        1.5 hub.scenes.global.enabled.set
DONE        1.6 hub.scenes.global.run
DONE        1.7 events test
DONE            1.7.1 isItemState event
DONE            1.7.2 isItemArmedState event
DONE            1.7.3 isButtonState event
DONE            1.7.4 hasAtLeastOneDictionaryValue event
DONE            1.7.5 and operator event
DONE        1.8 actions test
DONE            1.8.1 setItemValue action
DONE            1.8.2 setItemValue multicast action
DONE            1.8.3 setDeviceArmed action
DONE        1.9 events and actions test
DONE            1.9.1 event and action with different ids
DONE            1.9.2 event and action with same ids
DONE            1.9.3 event and action with different invalid ids
DONE            1.9.4 event and action with same invalid ids

*/

static const std::chrono::seconds RequestTimeout( 2 );

class GlobalScenesTests : public SceneManagerTestsFixture {
public:
    using ControllerId = std::string;
    using ItemId = std::string;
    using SubscriptionData = std::map< ControllerId, std::set< ItemId > >;
    using SerialToUuidMapping = std::map< ControllerId, std::string >;

public:
    void SetUp() override {
        SceneManagerTestsFixture::SetUp();
        nma().join( GroupsNames::EXTERNAL_CONTROLLERS );
        nma().setRequestsProcessFunction( [this]( rpc::Message& message ) {
            LOGGER_TRACE( "request method: " + message.getMethod() );
            LOGGER_TRACE( "request params: " + cc::j( message.getParams() ) );
            if( message.getMethod() != "cloud.controller_raw_command" ) {
                return;
            }
            std::unique_lock< std::mutex > lock( m_requestMutex );
            m_requestMessages.push_back( message.getMessage() );
            lock.unlock();
            m_requestCondVar.notify_all();
        } );
        nma().setInternalMethodCallProcessFunction(
            [this]( const JsonObject& json, MessageSubClasses requestType, const std::string& /*sender*/ ) {
                if( requestType == EXTERNAL_CONTROLLERS_UNSUBSCRIBE ) {
                    LOGGER_TRACE( "controller unsubsctibe request: " + cc::j( json ) );
                    std::unique_lock< std::mutex > lock( m_extControllerMutex );
                    m_extControllerUnsubscriptions.push_back( json );
                }
            } );

        callingFile( __FILE__ );
    }
    void TearDown() override {
        deleteGlobalScenes( __LINE__ );
        nma().resetRequestsProcessFunction();
        nma().resetInternalMethodCallProcessFunction();
        SceneManagerTestsFixture::TearDown();
    }
    JsonObject waitForCloudRequest( int codeLine, std::chrono::seconds timeout = RequestTimeout ) {
        auto codeLineStr = getCodeLineString( codeLine );
        std::unique_lock< std::mutex > lock( m_requestMutex );
        if( m_requestMessages.empty() ) {
            if( !m_requestCondVar.wait_for( lock, timeout, [this] { return !m_requestMessages.empty(); } ) ) {
                ADD_FAILURE() << "no cloud request is received: " << codeLineStr;
                throw std::exception();
            }
        }
        auto tmp = m_requestMessages.front();
        m_requestMessages.pop_front();
        return tmp;
    }
    JsonObject waitForExtControllerRequest( int codeLine, std::chrono::seconds timeout = RequestTimeout ) {
        auto codeLineStr = getCodeLineString( codeLine );
        std::unique_lock< std::mutex > lock( m_extControllerMutex );
        if( m_extControllerUnsubscriptions.empty() ) {
            if( !m_extControllerCondVar.wait_for(
                    lock, timeout, [this] { return !m_extControllerUnsubscriptions.empty(); } ) ) {
                ADD_FAILURE() << "no external controller request is received: " << codeLineStr;
                throw std::exception();
            }
        }
        auto tmp = m_extControllerUnsubscriptions.front();
        m_extControllerUnsubscriptions.pop_front();
        return tmp;
    }
    JsonObject readCommand( const Json& request, const std::string& controllerId, const std::string& codeLineStr ) {
        const std::string& controllerUuid = m_serialToUuidMapping[ controllerId ];

        JsonObject params, payload;
        EXPECT_TRUE( json_get( request, "params", params ) ) << codeLineStr;
        EXPECT_FALSE( controllerUuid.empty() );
        EXPECT_EQ( json_get_str( params, "controller_uuid" ), controllerUuid ) << codeLineStr;
        EXPECT_TRUE( json_get( params, "command", payload ) ) << codeLineStr;
        return payload;
    }

    void checkUnsubscription( SubscriptionData subsciptions ) {  // very weird code
        m_subscriptions = std::move( subsciptions );
    }
    void checkItemValueSet( const Json& request,
                            const std::string& controllerId,
                            const std::string& itemId,
                            const Json& value,
                            int codeLine ) {
        auto codeLineStr = getCodeLineString( codeLine );
        const JsonObject command = readCommand( request, controllerId, codeLineStr );
        EXPECT_EQ( json_get_str( command, "method" ), "hub.item.value.set" ) << codeLineStr;

        JsonObject itemParams;
        EXPECT_TRUE( json_get( command, "params", itemParams ) ) << codeLineStr;
        EXPECT_EQ( json_get_str( itemParams, "_id" ), itemId ) << codeLineStr;
        EXPECT_EQ( itemParams[ "value" ], value ) << codeLineStr;
    }
    void checkItemValueSet( const Json& request,
                            const std::string& controllerId,
                            const std::vector< std::string >& itemIds,
                            const Json& value,
                            int codeLine ) {
        auto codeLineStr = getCodeLineString( codeLine );
        const JsonObject command = readCommand( request, controllerId, codeLineStr );
        EXPECT_EQ( json_get_str( command, "method" ), "hub.item.value.set" ) << codeLineStr;

        JsonObject itemParams;
        EXPECT_TRUE( json_get( command, "params", itemParams ) ) << codeLineStr;
        EXPECT_EQ( itemParams[ "ids" ], itemIds ) << codeLineStr;
        EXPECT_EQ( itemParams[ "value" ], value ) << codeLineStr;
    }
    void checkDeviceArmedSet( const Json& request,
                              const std::string& controllerId,
                              const std::string& deviceId,
                              bool armed,
                              int codeLine ) {
        auto codeLineStr = getCodeLineString( codeLine );
        const JsonObject command = readCommand( request, controllerId, codeLineStr );
        EXPECT_EQ( json_get_str( command, "method" ), "hub.device.armed.set" ) << codeLineStr;

        JsonObject deviceParams;
        EXPECT_TRUE( json_get( command, "params", deviceParams ) ) << codeLineStr;
        EXPECT_EQ( json_get_str( deviceParams, "_id" ), deviceId ) << codeLineStr;
        EXPECT_EQ( deviceParams[ "armed" ], armed ) << codeLineStr;
    }
    std::string getMessageUuid( const Json& request ) {
        return request[ "params" ][ "command" ][ "id" ];
    }
    SceneID createGlobalScene( const JsonObject& sceneJson, int codeLine ) {
        auto codeLineStr = getCodeLineString( codeLine );
        sendRequestAndCheckAnswer( sceneJson, codeLine );
        JsonObject broadcast = waitForBroadcast( "hub.scene.global.added", codeLineStr );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.global.added" ) << codeLineStr;
        JsonObject result;
        JsonObject params;
        EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
        EXPECT_TRUE( json_get_obj( sceneJson, "params", params ) );
        EXPECT_EQ( json_get_bool( result, "enabled" ), json_get_bool( params, "enabled" ) ) << codeLineStr;
        EXPECT_EQ( json_get_str( result, "name" ), json_get_str( params, "name" ) ) << codeLineStr;
        EXPECT_EQ( json_get_str( result, "tag" ), json_get_str( params, "tag" ) ) << codeLineStr;
        EXPECT_EQ( json_get_str( result, "execPolicy" ), json_get_str( params, "execPolicy" ) ) << codeLineStr;
        EXPECT_EQ( json_get_str( result, "_id" ), json_get_str( params, "_id" ) ) << codeLineStr;
        SceneID id = json_get_str( result, "_id" );
        EXPECT_TRUE( !id.empty() );
        m_createdGlobalScenes.insert( id );
        return id;
    }
    void deleteGlobalScenes( int codeLine ) {
        auto createdScenes = m_createdGlobalScenes;
        for( const auto& id : createdScenes ) {
            deleteGlobalScene( id, codeLine );
        }
    }
    void deleteGlobalScene( const SceneManagerTestsFixture::SceneID& sceneId, int codeLine ) {
        auto codeLineStr = getCodeLineString( codeLine );
        SceneManagerTestsFixture::deleteScene(
            m_createdGlobalScenes, sceneId, "hub.scenes.global.delete", "hub.scene.global.removed", codeLine );
        std::size_t size = 0;
        for( const auto& subscription : m_subscriptions ) {
            size += subscription.second.size();
        }
        for( std::size_t i = 0; i < size; i++ ) {
            auto request = waitForExtControllerRequest( codeLine );
            auto controllerId = json_get_str( request, "controllerId" );
            auto controllerIt = m_subscriptions.find( controllerId );
            if( controllerIt == m_subscriptions.end() ) {
                ADD_FAILURE() << "Unexpected controllerId " + controllerId + " unsubscription\n" << codeLineStr;
                continue;
            }
            auto& ids = controllerIt->second;
            if( request.count( "itemsIds" ) ) {
                Json requestedItemIds;
                EXPECT_TRUE( json_get_arr( request, "itemsIds", requestedItemIds ) );
                EXPECT_FALSE( requestedItemIds.empty() );
                for( const auto& requestedItemId : requestedItemIds ) {
                    auto itemId = requestedItemId.get< std::string >();
                    auto itemIt = ids.find( itemId );
                    if( itemIt == ids.end() ) {
                        ADD_FAILURE() << "Unexpected itemId " + itemId + " unsubscription\n" << codeLineStr;
                    }
                }
            }
            else if( request.count( "devicesIds" ) ) {
                Json requestedDeviceIds;
                EXPECT_TRUE( json_get_arr( request, "devicesIds", requestedDeviceIds ) );
                EXPECT_FALSE( requestedDeviceIds.empty() );
                for( const auto& requestedDeviceId : requestedDeviceIds ) {
                    auto deviceId = requestedDeviceId.get< std::string >();
                    auto deviceIt = ids.find( deviceId );
                    if( deviceIt == ids.end() ) {
                        ADD_FAILURE() << "Unexpected deviceId " + deviceId + " unsubscription\n" << codeLineStr;
                    }
                }
            }
            else {
                ADD_FAILURE() << "No ids are received\n" << codeLineStr;
            }
        }
    }

    void checkEnabledGlobalScene( const JsonObject& broadcast, bool enabled, int codeLine ) {
        auto codeLineStr = getCodeLineString( codeLine );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.global.changed" ) << codeLineStr;
        JsonObject result;
        EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << codeLineStr;
        EXPECT_EQ( result.size(), 1 ) << codeLineStr;
        EXPECT_EQ( json_get_bool( result, "enabled" ), enabled ) << codeLineStr;
    }

    void runGlobalSceneManually( SceneManagerTestsFixture::SceneID sceneId, int codeLine ) {
        auto request = replaceTokens( runGlobalSceneRequest, { { "__ID__", sceneId } } );
        sendRequest( request, codeLine );
    }

    void checkGlobalRunProgressBroadcast( const JsonObject& createJson, const std::string& status, int codeLine ) {
        std::string codeLineStr = getCodeLineString( codeLine );
        JsonObject broadcast = waitForBroadcast( "hub.scene.global.run.progress", codeLineStr );
        checkGlobalRunProgressBroadcast( broadcast, createJson, status, codeLine );
    }
    void checkGlobalRunProgressBroadcast( const JsonObject& broadcast,
                                          const JsonObject& createJson,
                                          const std::string& status,
                                          int codeLine ) {
        std::string codeLineStr = getCodeLineString( codeLine );
        JsonObject result, params;
        EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << codeLineStr;
        EXPECT_TRUE( json_get_obj( createJson, "params", params ) ) << codeLineStr;
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.global.run.progress" ) << codeLineStr;
        EXPECT_EQ( json_get_str( result, "sceneId" ), json_get_str( params, "_id" ) ) << codeLineStr;
        EXPECT_EQ( json_get_str( result, "status" ), status ) << codeLineStr;
        EXPECT_EQ( json_get_str( result, "tag" ), json_get_str( params, "tag" ) ) << codeLineStr;
    }
    void sendBroadcastToExtDeviceGroup( MessageSubClasses eventType, const JsonObject& broadcast ) {
        Services::Message message = { GroupsNames::EXTERNAL_DEVICE_GROUP, EVENT, eventType, broadcast };
        nma().send( message );
    }

    void checkNoBroadcastIsReceived( int codeLine ) {
        createScene( createEmptyWhenBlockSceneJson, codeLine );
    }

    void sendEmptyResponse( const Json& request ) {
        auto messageUuid = getMessageUuid( request );
        auto response = JSON( "uuid" << messageUuid << "responseBody" << JSON( "result" << JSON_EMPTY_OBJ ) );
        Services::Message message = { GroupsNames::SCENE_MANAGER, RESPONSE, RESPONSE_RESULT, response };
        nma().send( message );
    }
    void sendSerialToUuidMapping( SerialToUuidMapping mapping ) {
        Json request = cloudAccessKeysSyncResponseTemplate;
        Json& clients = request[ "result" ][ "clients" ];
        Json& peers = request[ "result" ][ "peers" ];
        for( auto& entry : mapping ) {
            const std::string& serial = entry.first;
            const std::string& uuid = entry.second;
            clients[ uuid ] = JSON_ARRAY_( uuid + "_access_token_hash" );
            peers[ uuid ] = Json{ { "id", serial }, { "tokens", JSON_ARRAY_( uuid + "_access_token" ) } };
        };

        m_serialToUuidMapping = std::move( mapping );
        nma().sendtoHub( request, ComponentTestApplication::ConnectionType::NMA );
    }

private:
    std::set< SceneID > m_createdGlobalScenes;

    std::mutex m_requestMutex;
    std::condition_variable m_requestCondVar;
    std::deque< JsonObject > m_requestMessages;

    std::mutex m_extControllerMutex;
    std::condition_variable m_extControllerCondVar;
    std::deque< JsonObject > m_extControllerUnsubscriptions;

    SubscriptionData m_subscriptions;
    SerialToUuidMapping m_serialToUuidMapping;
};

TEST_F( GlobalScenesTests, createScene ) {  // 1.1
    auto request = replaceTokens( createGlobalSceneRequest, { { "__THEN_BLOCKS__", "" }, { "__WHEN_BLOCKS__", "" } } );
    auto wrongRequest = request;
    wrongRequest[ "params" ].erase( "_id" );
    auto answer = nma().sendtoHubAndGetAnswer( wrongRequest );
    checkErrorFieldNotFound( answer, "_id", __LINE__ );

    wrongRequest = request;
    wrongRequest[ "params" ].erase( "enabled" );
    answer = nma().sendtoHubAndGetAnswer( wrongRequest );
    checkErrorFieldNotFound( answer, "enabled", __LINE__ );

    request = replaceTokens(
        createGlobalSceneRequest,
        { { "__ID__", "abc1" }, { "__TAG__", "testRule/1" }, { "__THEN_BLOCKS__", "" }, { "__WHEN_BLOCKS__", "" } } );
    auto sceneId = createGlobalScene( request, __LINE__ );

    answer = nma().sendtoHubAndGetAnswer( request );
    const auto& error = answer[ "error" ];
    EXPECT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.overlapping" );
    EXPECT_EQ( json_get_str( error, "message" ), "Scene is overlapping with other scene" );

    deleteGlobalScene( sceneId, __LINE__ );
}

TEST_F( GlobalScenesTests, sceneList ) {  // 1.2
    std::map< std::string, Json > globalScenes;
    std::string id;
    id = "abc1";
    auto request = replaceTokens(
        createGlobalSceneRequest,
        { { "__ID__", id }, { "__TAG__", "testRule/1" }, { "__THEN_BLOCKS__", "" }, { "__WHEN_BLOCKS__", "" } } );
    createGlobalScene( request, __LINE__ );
    request[ "params" ][ "global" ] = true;
    globalScenes.emplace( id, request[ "params" ] );

    id = "abc2";
    request[ "params" ][ "_id" ] = id;
    request[ "params" ][ "tag" ] = "testRule/2";
    createGlobalScene( request, __LINE__ );
    request[ "params" ][ "global" ] = true;
    globalScenes.emplace( id, request[ "params" ] );

    id = "abc3";
    request[ "params" ][ "_id" ] = id;
    request[ "params" ][ "tag" ] = "testRule/3";
    createGlobalScene( request, __LINE__ );
    request[ "params" ][ "global" ] = true;
    globalScenes.emplace( id, request[ "params" ] );

    std::map< std::string, Json > scenes;
    request = replaceTokens( createSceneRequest,
                             { { "__NAME__", "name1" }, { "__THEN_BLOCKS__", "" }, { "__WHEN_BLOCKS__", "" } } );
    id = createScene( request, __LINE__ );
    request[ "params" ][ "_id" ] = id;
    scenes.emplace( id, request[ "params" ] );

    request[ "params" ][ "name" ] = "name2";
    id = createScene( request, __LINE__ );
    request[ "params" ][ "_id" ] = id;
    scenes.emplace( id, request[ "params" ] );

    request[ "params" ][ "name" ] = "name3";
    id = createScene( request, __LINE__ );
    request[ "params" ][ "_id" ] = id;
    scenes.emplace( id, request[ "params" ] );

    auto checkSceneList =
        [this]( const Json& answer, const std::map< std::string, Json >& expectedScenes, bool onlyId, int codeLine ) {
            auto codeLineStr = getCodeLineString( codeLine );
            const JsonArray& sceneList = answer[ "result" ][ "scenes" ];
            ASSERT_EQ( sceneList.size(), expectedScenes.size() ) << codeLineStr;
            for( const auto& scene : sceneList ) {
                auto it = expectedScenes.find( json_get_str( scene, "_id" ) );
                ASSERT_TRUE( it != expectedScenes.end() ) << codeLineStr;
                if( onlyId ) {
                    EXPECT_EQ( scene.size(), 1 ) << "onlyId filter failed, line: " << codeLineStr;
                }
                else {
                    auto sceneData = scene;
                    sceneData.erase( "generator" );
                    EXPECT_EQ( it->second.dump(), sceneData.dump() )
                        << "Scenes list request failed, line: " << codeLineStr;
                }
            }
        };

    Json answer;
    answer = nma().sendtoHubAndGetAnswer( getScenesList );
    checkSceneList( answer, scenes, false, __LINE__ );

    request = replaceTokens( getGlobalScenesListRequest, { { "__ONLY_ID__", "false" } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    checkSceneList( answer, globalScenes, false, __LINE__ );

    request = replaceTokens( getGlobalScenesListRequest, { { "__ONLY_ID__", "true" } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    checkSceneList( answer, globalScenes, true, __LINE__ );
}

TEST_F( GlobalScenesTests, sceneDelete ) {  // 1.3
    auto request = replaceTokens( createGlobalSceneRequest,
                                  { { "__ID__", "abc1" }, { "__THEN_BLOCKS__", "" }, { "__WHEN_BLOCKS__", "" } } );
    auto globalSceneId = createGlobalScene( request, __LINE__ );

    request = replaceTokens( createSceneRequest, { { "__THEN_BLOCKS__", "" }, { "__WHEN_BLOCKS__", "" } } );
    auto sceneId = createScene( request, __LINE__ );

    request = replaceTokens( deleteSceneRequest, { { "__ID__", "abc1" } } );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    ASSERT_FALSE( answer[ "error" ].empty() ) << "Global scene should not be deleted with local scenes API";

    request = replaceTokens( deleteGlobalSceneRequest, { { "__ID__", sceneId } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    ASSERT_FALSE( answer[ "error" ].empty() ) << "Local scene should not be deleted with global scenes API";
}

TEST_F( GlobalScenesTests, sceneEdit ) {  // 1.4
    auto request = replaceTokens(
        createGlobalSceneRequest,
        { { "__ID__", "abc1" }, { "__TAG__", "testRule/1" }, { "__THEN_BLOCKS__", "" }, { "__WHEN_BLOCKS__", "" } } );
    auto globalSceneId = createGlobalScene( request, __LINE__ );

    request = replaceTokens( editGlobalScenesRequest,
                             { { "__ID__", globalSceneId },
                               { "__ENABLED__", "false" },
                               { "__TAG__", "testRule/2" },
                               { "__EXEC_POLICY__", "check_result" },
                               { "__THEN_BLOCKS__", "" },
                               { "__WHEN_BLOCKS__", "" } } );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    const auto& error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    JsonObject broadcast = waitForBroadcast( "hub.scene.global.changed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.global.changed" );
    JsonObject result;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    EXPECT_EQ( json_get_str( result, "_id" ), globalSceneId );
    EXPECT_EQ( json_get_bool( result, "enabled" ), false );
    EXPECT_EQ( json_get_str( result, "tag" ), "testRule/2" );
    EXPECT_EQ( json_get_str( result, "exec_policy" ), "check_result" );
    EXPECT_EQ( result[ "then" ].dump(), JSON_ARRAY_().dump() );
    EXPECT_EQ( result[ "when" ].dump(), JSON_ARRAY_().dump() );
}

TEST_F( GlobalScenesTests, sceneEnableSet ) {  // 1.5
    auto request = replaceTokens( createGlobalSceneRequest,
                                  { { "__ID__", "abc1" }, { "__THEN_BLOCKS__", "" }, { "__WHEN_BLOCKS__", "" } } );
    auto globalSceneId = createGlobalScene( request, __LINE__ );

    request = replaceTokens( createSceneRequest,
                             { { "__NAME__", "usualScene" }, { "__THEN_BLOCKS__", "" }, { "__WHEN_BLOCKS__", "" } } );
    auto usualScene = request;
    auto sceneId = createScene( request, __LINE__ );

    request = replaceTokens( setEnableSceneRequest, { { "__ID__", globalSceneId }, { "__ENABLED__", "false" } } );
    sendRequestAndCheckFailedAnswer( request, __LINE__ );

    request = replaceTokens( enableGlobalSceneRequest, { { "__ID__", sceneId }, { "__ENABLED__", "false" } } );
    sendRequestAndCheckFailedAnswer( request, __LINE__ );

    request = replaceTokens( setEnableSceneRequest, { { "__ID__", sceneId }, { "__ENABLED__", "false" } } );
    sendRequestAndCheckAnswer( request, __LINE__ );
    checkChangedBroadcastEnableRequest( sceneId, usualScene, false, __LINE__ );

    request = replaceTokens( setEnableSceneRequest, { { "__ID__", sceneId }, { "__ENABLED__", "true" } } );
    sendRequestAndCheckAnswer( request, __LINE__ );
    checkChangedBroadcastEnableRequest( sceneId, usualScene, true, __LINE__ );

    request = replaceTokens( enableGlobalSceneRequest, { { "__ID__", globalSceneId }, { "__ENABLED__", "false" } } );
    sendRequestAndCheckAnswer( request, __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.scene.global.changed", getCodeLineString( __LINE__ ) );
    checkEnabledGlobalScene( broadcast, false, __LINE__ );

    request = replaceTokens( enableGlobalSceneRequest, { { "__ID__", globalSceneId }, { "__ENABLED__", "true" } } );
    sendRequestAndCheckAnswer( request, __LINE__ );
    broadcast = waitForBroadcast( "hub.scene.global.changed", getCodeLineString( __LINE__ ) );
    checkEnabledGlobalScene( broadcast, true, __LINE__ );
}

TEST_F( GlobalScenesTests, sceneRunManually ) {  // 1.6
    auto request = replaceTokens(
        createSceneRequest, { { "__NAME__", "usualScene" }, { "__THEN_BLOCKS__", "" }, { "__WHEN_BLOCKS__", "" } } );
    auto usualScene = request;
    auto sceneId = createScene( request, __LINE__ );
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    request = replaceTokens( runGlobalSceneRequest, { { "__ID__", sceneId } } );
    sendRequestAndCheckFailedAnswer( request, __LINE__ );

    request = replaceTokens( createGlobalSceneRequest,
                             { { "__ID__", "abc1" }, { "__THEN_BLOCKS__", "" }, { "__WHEN_BLOCKS__", "" } } );
    auto globalSceneId = createGlobalScene( request, __LINE__ );
    runGlobalSceneManually( globalSceneId, __LINE__ );
    checkGlobalRunProgressBroadcast( request, "started", __LINE__ );
    checkGlobalRunProgressBroadcast( request, "finished", __LINE__ );

    request = replaceTokens( runSceneRequest, { { "__ID__", globalSceneId } } );
    sendRequestAndCheckFailedAnswer( request, __LINE__ );
}

TEST_F( GlobalScenesTests, isItemStateEvent ) {  // 1.7.1
    auto isItemState = replaceTokensInString( globalIsItemStateWhenBlock,
                                              { { "__CONTROLLER_ID__", "ExternalController-1" },
                                                { "__ITEM_ID__", "item_id-1.1.1" },
                                                { "__TYPE__", "bool" },
                                                { "__VALUE__", "true" } } );
    auto request =
        replaceTokens( createGlobalSceneRequest,
                       { { "__ID__", "abc1" }, { "__THEN_BLOCKS__", "" }, { "__WHEN_BLOCKS__", isItemState } } );
    auto globalSceneId = createGlobalScene( request, __LINE__ );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_ADDED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "value": true
    }
    )"_json );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "value": true
    }
    )"_json );

    checkGlobalRunProgressBroadcast( request, "started", __LINE__ );
    checkGlobalRunProgressBroadcast( request, "finished", __LINE__ );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "value": false
    }
    )"_json );
}

TEST_F( GlobalScenesTests, isItemArmedStateEvent ) {  // 1.7.2
    JsonObject query = R"(
    {
        "_id": "ExternalController-1|device_id-1.1.1",
        "armed":false
    }
    )"_json;
    nma().getDBClient().insert( "db.firmware.external_controllers.devices", query );

    query = R"(
    {
        "_id": "ExternalController-1|item_id-1.1.1",
        "value":false
    }
    )"_json;
    nma().getDBClient().insert( "db.firmware.external_controllers.items", query );

    query = R"(
    {
        "_id": "ExternalController-1|item_id-1.1.2",
        "value":false
    }
    )"_json;
    nma().getDBClient().insert( "db.firmware.external_controllers.items", query );

    auto updateDeviceArmed = []( bool armed ) {
        nma().getDBClient().update( "db.firmware.external_controllers.devices",
                                    JSON( "_id"
                                          << "ExternalController-1|device_id-1.1.1" ),
                                    JSON( "armed" << armed ) );
    };

    auto updateItemState = []( const std::string& itemId, bool state ) {
        nma().getDBClient().update( "db.firmware.external_controllers.items",
                                    JSON( "_id"
                                          << "ExternalController-1|" + itemId ),
                                    JSON( "value" << state ) );
    };

    auto isItemArmedState1 = replaceTokensInString( globalIsItemArmedStateWhenBlock,
                                                    { { "__CONTROLLER_ID__", "ExternalController-1" },
                                                      { "__ITEM_ID__", "item_id-1.1.1" },
                                                      { "__VALUE__", "true" },
                                                      { "__TYPE__", "bool" },
                                                      { "__ARMED__", "true" } } );
    auto isItemArmedState2 = replaceTokensInString( globalIsItemArmedStateWhenBlock,
                                                    { { "__CONTROLLER_ID__", "ExternalController-1" },
                                                      { "__ITEM_ID__", "item_id-1.1.2" },
                                                      { "__VALUE__", "true" },
                                                      { "__TYPE__", "bool" },
                                                      { "__ARMED__", "true" } } );
    auto request = replaceTokens( createGlobalSceneRequest,
                                  { { "__ID__", "abc1" },
                                    { "__THEN_BLOCKS__", "" },
                                    { "__WHEN_BLOCKS__", isItemArmedState1 + ", " + isItemArmedState2 } } );
    auto globalSceneId = createGlobalScene( request, __LINE__ );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_ADDED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "deviceId":"device_id-1.1.1",
        "value": false
    }
    )"_json );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_ADDED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.2",
        "deviceId":"device_id-1.1.1",
        "value": false
    }
    )"_json );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "value": true
    }
    )"_json );

    checkNoBroadcastIsReceived( __LINE__ );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "deviceId":"device_id-1.1.1",
        "value": true
    }
    )"_json );

    checkNoBroadcastIsReceived( __LINE__ );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.2",
        "deviceId":"device_id-1.1.1",
        "value": true
    }
    )"_json );

    checkNoBroadcastIsReceived( __LINE__ );

    sendBroadcastToExtDeviceGroup( HUB_DEVICE_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "device_id-1.1.1",
        "armed": true
    }
    )"_json );

    checkNoBroadcastIsReceived( __LINE__ );

    updateDeviceArmed( true );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "deviceId":"device_id-1.1.1",
        "value": true
    }
    )"_json );

    checkGlobalRunProgressBroadcast( request, "started", __LINE__ );
    checkGlobalRunProgressBroadcast( request, "finished", __LINE__ );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.2",
        "deviceId":"device_id-1.1.1",
        "value": true
    }
    )"_json );

    checkGlobalRunProgressBroadcast( request, "started", __LINE__ );
    checkGlobalRunProgressBroadcast( request, "finished", __LINE__ );

    updateItemState( "item_id-1.1.1", true );
    updateItemState( "item_id-1.1.2", true );

    sendBroadcastToExtDeviceGroup( HUB_DEVICE_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "device_id-1.1.1",
        "armed": true
    }
    )"_json );

    checkGlobalRunProgressBroadcast( request, "started", __LINE__ );
    checkGlobalRunProgressBroadcast( request, "finished", __LINE__ );

    nma().getDBClient().drop( "db.firmware.external_controllers.devices" );
    nma().getDBClient().drop( "db.firmware.external_controllers.items" );
}

TEST_F( GlobalScenesTests, isButtonStateEvent ) {  // 1.7.3
    auto isButtonState = replaceTokensInString( globalIsButtonStateWhenBlock,
                                                { { "__CONTROLLER_ID__", "ExternalController-1" },
                                                  { "__ITEM_ID__", "item_id-1.1.1" },
                                                  { "__VALUE__", "press_1_time" } } );
    auto request =
        replaceTokens( createGlobalSceneRequest,
                       { { "__ID__", "abc1" }, { "__THEN_BLOCKS__", "" }, { "__WHEN_BLOCKS__", isButtonState } } );
    auto globalSceneId = createGlobalScene( request, __LINE__ );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_ADDED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "value": {
            "button_number": 1,
            "button_state": "released"
        }
    }
    )"_json );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "value": {
            "button_number": 1,
            "button_state": "press_1_time"
        }
    }
    )"_json );

    checkGlobalRunProgressBroadcast( request, "started", __LINE__ );
    checkGlobalRunProgressBroadcast( request, "finished", __LINE__ );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "value": {
            "button_number": 1,
            "button_state": "released"
        }
    }
    )"_json );
}

TEST_F( GlobalScenesTests, hasAtLeastOneDictionaryValueEvent ) {  // 1.7.4
    auto isHasAtLeastOneDictionaryValue = replaceTokensInString( globalHasAtLeastOneDictionaryValueWhenBlock,
                                                                 { { "__CONTROLLER_ID__", "ExternalController-1" },
                                                                   { "__ITEM_ID__", "item_id-1.1.1" },
                                                                   { "__TOKEN__", "low_battery" } } );
    auto request = replaceTokens(
        createGlobalSceneRequest,
        { { "__ID__", "abc1" }, { "__THEN_BLOCKS__", "" }, { "__WHEN_BLOCKS__", isHasAtLeastOneDictionaryValue } } );
    auto globalSceneId = createGlobalScene( request, __LINE__ );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_ADDED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1"
    }
    )"_json );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_DICTIONARY_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "operation": "added",
        "element": {
            "1": {
                "value": "low_battery"
            }
        }
    }
    )"_json );

    checkGlobalRunProgressBroadcast( request, "started", __LINE__ );
    checkGlobalRunProgressBroadcast( request, "finished", __LINE__ );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_DICTIONARY_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "operation": "added",
        "element": {
            "2": {
                "value": "low_battery"
            }
        }
    }
    )"_json );

    checkGlobalRunProgressBroadcast( request, "started", __LINE__ );
    checkGlobalRunProgressBroadcast( request, "finished", __LINE__ );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_DICTIONARY_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "operation": "removed",
        "element": {
            "2": {
                "value": "low_battery"
            }
        }
    }
    )"_json );
}

TEST_F( GlobalScenesTests, andOperatorEvent ) {  // 1.7.5
    auto isItemState = replaceTokensInString( globalIsItemStateWhenBlock,
                                              { { "__CONTROLLER_ID__", "ExternalController-1" },
                                                { "__ITEM_ID__", "item_id-1.1.1" },
                                                { "__TYPE__", "bool" },
                                                { "__VALUE__", "true" } } );
    auto isHasAtLeastOneDictionaryValue = replaceTokensInString( globalHasAtLeastOneDictionaryValueWhenBlock,
                                                                 { { "__CONTROLLER_ID__", "ExternalController-1" },
                                                                   { "__ITEM_ID__", "item_id-1.1.2" },
                                                                   { "__TOKEN__", "low_battery" } } );
    auto andOperator = replaceTokensInString(
        andOperatorWhenBlock, { { "__BLOCKS__", isItemState + "," + isHasAtLeastOneDictionaryValue } } );
    auto request =
        replaceTokens( createGlobalSceneRequest,
                       { { "__ID__", "abc1" }, { "__THEN_BLOCKS__", "" }, { "__WHEN_BLOCKS__", andOperator } } );
    auto globalSceneId = createGlobalScene( request, __LINE__ );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_ADDED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "value": false
    }
    )"_json );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_ADDED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.2"
    }
    )"_json );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "value": true
    }
    )"_json );

    checkNoBroadcastIsReceived( __LINE__ );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "value": false
    }
    )"_json );

    checkNoBroadcastIsReceived( __LINE__ );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_DICTIONARY_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.2",
        "operation": "added",
        "element": {
            "1": {
                "value": "low_battery"
            }
        }
    }
    )"_json );

    checkNoBroadcastIsReceived( __LINE__ );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "value": true
    }
    )"_json );

    checkGlobalRunProgressBroadcast( request, "started", __LINE__ );
    checkGlobalRunProgressBroadcast( request, "finished", __LINE__ );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "value": false
    }
    )"_json );

    checkNoBroadcastIsReceived( __LINE__ );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "value": true
    }
    )"_json );

    checkGlobalRunProgressBroadcast( request, "started", __LINE__ );
    checkGlobalRunProgressBroadcast( request, "finished", __LINE__ );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_DICTIONARY_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.2",
        "operation": "removed",
        "element": {
            "1": {
                "value": "low_battery"
            }
        }
    }
    )"_json );

    checkNoBroadcastIsReceived( __LINE__ );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_DICTIONARY_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.2",
        "operation": "added",
        "element": {
            "1": {
                "value": "low_battery"
            }
        }
    }
    )"_json );

    checkGlobalRunProgressBroadcast( request, "started", __LINE__ );
    checkGlobalRunProgressBroadcast( request, "finished", __LINE__ );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_DICTIONARY_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.2",
        "operation": "removed",
        "element": {
            "1": {
                "value": "low_battery"
            }
        }
    }
    )"_json );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "value": false
    }
    )"_json );
}

TEST_F( GlobalScenesTests, setItemValueAction ) {  // 1.8.1
    sendSerialToUuidMapping( { { "ExternalController-1", "ExternalController-1-UUID" } } );

    auto setItemValue = replaceTokensInString( globalSetItemValueThenBlock,
                                               { { "__CONTROLLER_ID__", "ExternalController-1" },
                                                 { "__EXEC_POLICY__", "check_result" },
                                                 { "__ITEM_ID__", "item_id-1.1.1" },
                                                 { "__TYPE__", "bool" },
                                                 { "__VALUE__", "true" } } );
    auto request =
        replaceTokens( createGlobalSceneRequest,
                       { { "__ID__", "abc1" }, { "__THEN_BLOCKS__", setItemValue }, { "__WHEN_BLOCKS__", "" } } );
    auto globalSceneId = createGlobalScene( request, __LINE__ );
    checkUnsubscription( { { "ExternalController-1", { "item_id-1.1.1" } } } );

    runGlobalSceneManually( globalSceneId, __LINE__ );
    checkGlobalRunProgressBroadcast( request, "started", __LINE__ );
    checkGlobalRunProgressBroadcast( request, "failed", __LINE__ );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_ADDED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "value": false
    }
    )"_json );
    runGlobalSceneManually( globalSceneId, __LINE__ );
    auto cloudRequest = waitForCloudRequest( __LINE__ );
    checkItemValueSet( cloudRequest, "ExternalController-1", "item_id-1.1.1", true, __LINE__ );
    sendEmptyResponse( cloudRequest );
    checkGlobalRunProgressBroadcast( request, "started", __LINE__ );
    checkGlobalRunProgressBroadcast( request, "finished", __LINE__ );
}

TEST_F( GlobalScenesTests, setItemValueMulticastAction ) {  // 1.8.2
    sendSerialToUuidMapping( { { "ExternalController-1", "ExternalController-1-UUID" } } );

    auto setItemValue1 = replaceTokensInString( globalSetItemValueThenBlock,
                                                { { "__CONTROLLER_ID__", "ExternalController-1" },
                                                  { "__EXEC_POLICY__", "check_result" },
                                                  { "__ITEM_ID__", "item_id-1.1.1" },
                                                  { "__TYPE__", "bool" },
                                                  { "__VALUE__", "true" } } );

    auto setItemValue2 = replaceTokensInString( globalSetItemValueThenBlock,
                                                { { "__CONTROLLER_ID__", "ExternalController-1" },
                                                  { "__EXEC_POLICY__", "check_result" },
                                                  { "__ITEM_ID__", "item_id-1.1.2" },
                                                  { "__TYPE__", "bool" },
                                                  { "__VALUE__", "true" } } );
    auto request = replaceTokens( createGlobalSceneRequest,
                                  { { "__ID__", "abc1" },
                                    { "__THEN_BLOCKS__", setItemValue1 + "," + setItemValue2 },
                                    { "__WHEN_BLOCKS__", "" } } );
    auto globalSceneId = createGlobalScene( request, __LINE__ );
    checkUnsubscription( { { "ExternalController-1", { "item_id-1.1.1", "item_id-1.1.2" } } } );

    runGlobalSceneManually( globalSceneId, __LINE__ );
    checkGlobalRunProgressBroadcast( request, "started", __LINE__ );
    checkGlobalRunProgressBroadcast( request, "failed", __LINE__ );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_ADDED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "value": false
    }
    )"_json );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_ADDED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.2",
        "value": false
    }
    )"_json );

    runGlobalSceneManually( globalSceneId, __LINE__ );
    auto cloudRequest = waitForCloudRequest( __LINE__ );
    checkItemValueSet( cloudRequest,
                       "ExternalController-1",
                       std::vector< std::string >{ "item_id-1.1.1", "item_id-1.1.2" },
                       true,
                       __LINE__ );
    sendEmptyResponse( cloudRequest );
    checkGlobalRunProgressBroadcast( request, "started", __LINE__ );
    checkGlobalRunProgressBroadcast( request, "finished", __LINE__ );
}

TEST_F( GlobalScenesTests, setDeviceArmedAction ) {  // 1.8.3
    sendSerialToUuidMapping( { { "ExternalController-1", "ExternalController-1-UUID" } } );

    auto setDeviceArmed = replaceTokensInString( globalSetDeviceArmed,
                                                 { { "__CONTROLLER_ID__", "ExternalController-1" },
                                                   { "__EXEC_POLICY__", "check_result" },
                                                   { "__DEVICE_ID__", "device_id-1.1.1" },
                                                   { "__VALUE__", "true" } } );
    auto request =
        replaceTokens( createGlobalSceneRequest,
                       { { "__ID__", "abc1" }, { "__THEN_BLOCKS__", setDeviceArmed }, { "__WHEN_BLOCKS__", "" } } );
    auto globalSceneId = createGlobalScene( request, __LINE__ );

    runGlobalSceneManually( globalSceneId, __LINE__ );
    checkGlobalRunProgressBroadcast( request, "started", __LINE__ );
    checkGlobalRunProgressBroadcast( request, "failed", __LINE__ );

    sendBroadcastToExtDeviceGroup( HUB_DEVICE_ADDED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "device_id-1.1.1",
        "armed": false
    }
    )"_json );
    runGlobalSceneManually( globalSceneId, __LINE__ );
    auto cloudRequest = waitForCloudRequest( __LINE__ );
    checkDeviceArmedSet( cloudRequest, "ExternalController-1", "device_id-1.1.1", "true", __LINE__ );
    sendEmptyResponse( cloudRequest );
    checkGlobalRunProgressBroadcast( request, "started", __LINE__ );
    checkGlobalRunProgressBroadcast( request, "finished", __LINE__ );
}

TEST_F( GlobalScenesTests, eventAndActionWithDifferentIds ) {  // 1.9.1
    sendSerialToUuidMapping( { { "ExternalController-1", "ExternalController-1-UUID" } } );

    auto isItemState = replaceTokensInString( globalIsItemStateWhenBlock,
                                              { { "__CONTROLLER_ID__", "ExternalController-1" },
                                                { "__ITEM_ID__", "item_id-1.1.1" },
                                                { "__TYPE__", "bool" },
                                                { "__VALUE__", "true" } } );

    auto setItemValue = replaceTokensInString( globalSetItemValueThenBlock,
                                               { { "__CONTROLLER_ID__", "ExternalController-1" },
                                                 { "__EXEC_POLICY__", "check_result" },
                                                 { "__ITEM_ID__", "item_id-1.1.2" },
                                                 { "__VALUE__", "true" } } );
    auto request = replaceTokens(
        createGlobalSceneRequest,
        { { "__ID__", "abc1" }, { "__THEN_BLOCKS__", setItemValue }, { "__WHEN_BLOCKS__", isItemState } } );
    auto globalSceneId = createGlobalScene( request, __LINE__ );
    checkUnsubscription( { { "ExternalController-1", { "item_id-1.1.1", "item_id-1.1.2", "" } } } );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_ADDED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "value": false
    }
    )"_json );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_ADDED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.2",
        "value": false
    }
    )"_json );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "value": true
    }
    )"_json );

    auto cloudRequest = waitForCloudRequest( __LINE__ );
    checkItemValueSet( cloudRequest, "ExternalController-1", "item_id-1.1.2", true, __LINE__ );
    sendEmptyResponse( cloudRequest );
    checkGlobalRunProgressBroadcast( request, "started", __LINE__ );
    checkGlobalRunProgressBroadcast( request, "finished", __LINE__ );
}

TEST_F( GlobalScenesTests, eventAndActionWithSameIds ) {  // 1.9.2
    sendSerialToUuidMapping( { { "ExternalController-1", "ExternalController-1-UUID" } } );

    auto isItemState = replaceTokensInString( globalIsItemStateWhenBlock,
                                              { { "__CONTROLLER_ID__", "ExternalController-1" },
                                                { "__ITEM_ID__", "item_id-1.1.1" },
                                                { "__TYPE__", "bool" },
                                                { "__VALUE__", "true" } } );

    auto setItemValue = replaceTokensInString( globalSetItemValueThenBlock,
                                               { { "__CONTROLLER_ID__", "ExternalController-1" },
                                                 { "__EXEC_POLICY__", "check_result" },
                                                 { "__ITEM_ID__", "item_id-1.1.1" },
                                                 { "__VALUE__", "true" } } );
    auto request = replaceTokens(
        createGlobalSceneRequest,
        { { "__ID__", "abc1" }, { "__THEN_BLOCKS__", setItemValue }, { "__WHEN_BLOCKS__", isItemState } } );
    auto globalSceneId = createGlobalScene( request, __LINE__ );
    checkUnsubscription( { { "ExternalController-1", { "item_id-1.1.1", "" } } } );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_ADDED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "value": false
    }
    )"_json );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_ADDED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "value": false
    }
    )"_json );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "value": true
    }
    )"_json );

    auto cloudRequest = waitForCloudRequest( __LINE__ );
    checkItemValueSet( cloudRequest, "ExternalController-1", "item_id-1.1.1", true, __LINE__ );
    sendEmptyResponse( cloudRequest );
    checkGlobalRunProgressBroadcast( request, "started", __LINE__ );
    checkGlobalRunProgressBroadcast( request, "finished", __LINE__ );
}

TEST_F( GlobalScenesTests, eventAndActionWithDifferentInvalidIds ) {  // 1.9.3
    auto isItemState = replaceTokensInString( globalIsItemStateWhenBlock,
                                              { { "__CONTROLLER_ID__", "ExternalController-1" },
                                                { "__ITEM_ID__", "item_id-1.1.1" },
                                                { "__TYPE__", "bool" },
                                                { "__VALUE__", "true" } } );

    auto setItemValue = replaceTokensInString( globalSetItemValueThenBlock,
                                               { { "__CONTROLLER_ID__", "ExternalController-1" },
                                                 { "__EXEC_POLICY__", "check_result" },
                                                 { "__ITEM_ID__", "item_id-1.1.2" },
                                                 { "__VALUE__", "true" } } );
    auto request = replaceTokens(
        createGlobalSceneRequest,
        { { "__ID__", "abc1" }, { "__THEN_BLOCKS__", setItemValue }, { "__WHEN_BLOCKS__", isItemState } } );
    auto globalSceneId = createGlobalScene( request, __LINE__ );
    checkUnsubscription( { { "ExternalController-1", { "item_id-1.1.1", "item_id-1.1.2" } } } );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "value": true
    }
    )"_json );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "value": false
    }
    )"_json );
}

TEST_F( GlobalScenesTests, eventAndActionWithSameInvalidIds ) {  // 1.9.4
    auto isItemState = replaceTokensInString( globalIsItemStateWhenBlock,
                                              { { "__CONTROLLER_ID__", "ExternalController-1" },
                                                { "__ITEM_ID__", "item_id-1.1.1" },
                                                { "__TYPE__", "bool" },
                                                { "__VALUE__", "true" } } );

    auto setItemValue = replaceTokensInString( globalSetItemValueThenBlock,
                                               { { "__CONTROLLER_ID__", "ExternalController-1" },
                                                 { "__EXEC_POLICY__", "check_result" },
                                                 { "__ITEM_ID__", "item_id-1.1.1" },
                                                 { "__VALUE__", "true" } } );
    auto request = replaceTokens(
        createGlobalSceneRequest,
        { { "__ID__", "abc1" }, { "__THEN_BLOCKS__", setItemValue }, { "__WHEN_BLOCKS__", isItemState } } );
    auto globalSceneId = createGlobalScene( request, __LINE__ );
    checkUnsubscription( { { "ExternalController-1", { "item_id-1.1.1" } } } );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "value": true
    }
    )"_json );

    sendBroadcastToExtDeviceGroup( HUB_ITEM_UPDATED, R"(
    {
        "controllerId":"ExternalController-1",
        "_id": "item_id-1.1.1",
        "value": false
    }
    )"_json );
}

}  // namespace Tests
}  // namespace HubPlatform
