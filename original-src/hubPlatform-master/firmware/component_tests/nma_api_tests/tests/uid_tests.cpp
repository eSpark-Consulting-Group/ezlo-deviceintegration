#include "common_test_resources/resources/uid_resources.h"
#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"

#include "cored/model/cored_item.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/cored_device.h"

#include <gtest/gtest.h>
#include <numeric>

namespace HubPlatform {
namespace Tests {

class UiServiceTests : virtual public SceneManagerTestsFixture {};

TEST_F( UiServiceTests, favoriteList ) {
    using namespace CoreD::Model::Resources;
    auto& coreRemoteInterface = makeRemote( nma() );
    auto deviceId1 = addDevice( coreRemoteInterface,
                                "testDevice1",
                                "smoke",
                                "134_3_24",
                                "security_sensor",
                                "smoke",
                                false /*batteryPowered*/,
                                __LINE__ );
    auto deviceId2 = addDevice( coreRemoteInterface,
                                "testDevice1",
                                "smoke",
                                "134_3_24",
                                "security_sensor",
                                "smoke",
                                false /*batteryPowered*/,
                                __LINE__ );

    auto itemId1 = addItem(
        coreRemoteInterface, deviceId1, "testItem1", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    auto itemId2 = addItem(
        coreRemoteInterface, deviceId2, "testItem2", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    SceneID sceneId1 = createScene( createEmptyWhenBlockSceneJson );
    SceneID sceneId2 = createScene( createEmptyWhenBlockSceneJson );

    auto request = replaceTokens( hubFavoriteSet,
                                  {
                                      { "__DEVICE_ID_1__", deviceId1 },
                                      { "__DEVICE_ID_2__", deviceId2 },
                                      { "__IS_FAVORITE_DEVICE_1__", "true" },
                                      { "__IS_FAVORITE_DEVICE_2__", "true" },
                                      { "__ITEM_ID_1__", itemId1 },
                                      { "__ITEM_ID_2__", itemId2 },
                                      { "__IS_FAVORITE_ITEM_1__", "true" },
                                      { "__IS_FAVORITE_ITEM_2__", "true" },
                                      { "__RULE_ID_1__", sceneId1 },
                                      { "__RULE_ID_2__", sceneId2 },
                                      { "__IS_FAVORITE_RULE_1__", "true" },
                                      { "__IS_FAVORITE_RULE_2__", "true" },
                                  } );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    auto testFavoriteBroadcast = [this]( const std::string& broadcastName,
                                         const std::vector< std::string >& devices,
                                         const std::vector< std::string >& items,
                                         const std::vector< std::string >& rules,
                                         int codeLine ) {
        auto codeLineStr = getCodeLineString( codeLine );
        JsonObject broadcast = waitForBroadcast( broadcastName, codeLineStr );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), broadcastName );

        JsonArray resultIds;
        json_get( broadcast, { "result", "devices" }, resultIds );
        ASSERT_EQ( resultIds.size(), devices.size() ) << codeLineStr;
        for( std::size_t i = 0; i < resultIds.size(); ++i ) {
            EXPECT_EQ( resultIds[ i ], devices[ i ] ) << codeLineStr;
        }
        resultIds.clear();

        json_get( broadcast, { "result", "items" }, resultIds );
        ASSERT_EQ( resultIds.size(), items.size() ) << codeLineStr;
        for( std::size_t i = 0; i < resultIds.size(); ++i ) {
            EXPECT_EQ( resultIds[ i ], items[ i ] ) << codeLineStr;
        }
        resultIds.clear();

        json_get( broadcast, { "result", "rules" }, resultIds );
        ASSERT_EQ( resultIds.size(), rules.size() ) << codeLineStr;
        for( std::size_t i = 0; i < resultIds.size(); ++i ) {
            EXPECT_EQ( resultIds[ i ], rules[ i ] ) << codeLineStr;
        }
        resultIds.clear();
    };

    auto testFavoriteList = [this]( const JsonObject& answer,
                                    const std::vector< std::string >& devices,
                                    const std::vector< std::string >& items,
                                    const std::vector< std::string >& rules,
                                    int codeLine ) {
        auto codeLineStr = getCodeLineString( codeLine );

        JsonArray resultIds;
        json_get( answer, { "result", "favorites", "devices" }, resultIds );
        ASSERT_EQ( resultIds.size(), devices.size() ) << codeLineStr;
        for( std::size_t i = 0; i < resultIds.size(); ++i ) {
            EXPECT_EQ( resultIds[ i ], devices[ i ] ) << codeLineStr;
        }
        resultIds.clear();

        json_get( answer, { "result", "favorites", "items" }, resultIds );
        ASSERT_EQ( resultIds.size(), items.size() ) << codeLineStr;
        for( std::size_t i = 0; i < resultIds.size(); ++i ) {
            EXPECT_EQ( resultIds[ i ], items[ i ] ) << codeLineStr;
        }
        resultIds.clear();

        json_get( answer, { "result", "favorites", "rules" }, resultIds );
        ASSERT_EQ( resultIds.size(), rules.size() ) << codeLineStr;
        for( std::size_t i = 0; i < resultIds.size(); ++i ) {
            EXPECT_EQ( resultIds[ i ], rules[ i ] ) << codeLineStr;
        }
        resultIds.clear();
    };

    testFavoriteBroadcast(
        "hub.favorite.added", { deviceId1, deviceId2 }, { itemId1, itemId2 }, { sceneId1, sceneId2 }, __LINE__ );

    auto getFavoriteList = [this]( int codeLine ) -> JsonObject {
        auto codeLineStr = getCodeLineString( codeLine );
        auto request = replaceTokens(
            hubFavoriteList, { { "__DEVICES__", "devices" }, { "__ITEMS__", "items" }, { "__RULES__", "rules" } } );
        auto answer = nma().sendtoHubAndGetAnswer( request );
        const auto& error = answer[ "error" ];
        EXPECT_TRUE( error.empty() ) << error.dump( 4 ) << ", " << codeLineStr;
        return answer;
    };

    answer = getFavoriteList( __LINE__ );
    testFavoriteList( answer, { deviceId1, deviceId2 }, { itemId1, itemId2 }, { sceneId1, sceneId2 }, __LINE__ );

    request = replaceTokens( hubFavoriteSet,
                             {
                                 { "__DEVICE_ID_1__", deviceId1 },
                                 { "__DEVICE_ID_2__", "deviceId2" },
                                 { "__IS_FAVORITE_DEVICE_1__", "false" },
                                 { "__IS_FAVORITE_DEVICE_2__", "false" },
                                 { "__ITEM_ID_1__", itemId1 },
                                 { "__ITEM_ID_2__", "itemId2" },
                                 { "__IS_FAVORITE_ITEM_1__", "false" },
                                 { "__IS_FAVORITE_ITEM_2__", "false" },
                                 { "__RULE_ID_1__", sceneId1 },
                                 { "__RULE_ID_2__", "sceneId2" },
                                 { "__IS_FAVORITE_RULE_1__", "false" },
                                 { "__IS_FAVORITE_RULE_2__", "false" },
                             } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    testFavoriteBroadcast( "hub.favorite.removed", { deviceId1 }, { itemId1 }, { sceneId1 }, __LINE__ );
    answer = getFavoriteList( __LINE__ );
    testFavoriteList( answer, { deviceId2 }, { itemId2 }, { sceneId2 }, __LINE__ );

    request = replaceTokens( hubFavoriteSet,
                             {
                                 { "__DEVICE_ID_1__", "deviceId1" },
                                 { "__DEVICE_ID_2__", deviceId2 },
                                 { "__IS_FAVORITE_DEVICE_1__", "false" },
                                 { "__IS_FAVORITE_DEVICE_2__", "false" },
                                 { "__ITEM_ID_1__", "itemId1" },
                                 { "__ITEM_ID_2__", itemId2 },
                                 { "__IS_FAVORITE_ITEM_1__", "false" },
                                 { "__IS_FAVORITE_ITEM_2__", "false" },
                                 { "__RULE_ID_1__", "sceneId1" },
                                 { "__RULE_ID_2__", sceneId2 },
                                 { "__IS_FAVORITE_RULE_1__", "false" },
                                 { "__IS_FAVORITE_RULE_2__", "false" },
                             } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    testFavoriteBroadcast( "hub.favorite.removed", { deviceId2 }, { itemId2 }, { sceneId2 }, __LINE__ );
    answer = getFavoriteList( __LINE__ );
    testFavoriteList( answer, {}, {}, {}, __LINE__ );

    request = replaceTokens( hubFavoriteSet,
                             {
                                 { "__DEVICE_ID_1__", deviceId1 },
                                 { "__DEVICE_ID_2__", "deviceId2" },
                                 { "__IS_FAVORITE_DEVICE_1__", "true" },
                                 { "__IS_FAVORITE_DEVICE_2__", "true" },
                                 { "__ITEM_ID_1__", "itemId1" },
                                 { "__ITEM_ID_2__", "itemId2" },
                                 { "__IS_FAVORITE_ITEM_1__", "true" },
                                 { "__IS_FAVORITE_ITEM_2__", "true" },
                                 { "__RULE_ID_1__", "sceneId1" },
                                 { "__RULE_ID_2__", "sceneId2" },
                                 { "__IS_FAVORITE_RULE_1__", "true" },
                                 { "__IS_FAVORITE_RULE_2__", "true" },
                             } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    testFavoriteBroadcast( "hub.favorite.added", { deviceId1 }, {}, {}, __LINE__ );
    answer = getFavoriteList( __LINE__ );
    testFavoriteList( answer, { deviceId1 }, {}, {}, __LINE__ );

    request = replaceTokens( hubFavoriteSet,
                             {
                                 { "__DEVICE_ID_1__", "deviceId1" },
                                 { "__DEVICE_ID_2__", "deviceId2" },
                                 { "__IS_FAVORITE_DEVICE_1__", "true" },
                                 { "__IS_FAVORITE_DEVICE_2__", "true" },
                                 { "__ITEM_ID_1__", itemId1 },
                                 { "__ITEM_ID_2__", "itemId2" },
                                 { "__IS_FAVORITE_ITEM_1__", "true" },
                                 { "__IS_FAVORITE_ITEM_2__", "true" },
                                 { "__RULE_ID_1__", "sceneId1" },
                                 { "__RULE_ID_2__", "sceneId2" },
                                 { "__IS_FAVORITE_RULE_1__", "true" },
                                 { "__IS_FAVORITE_RULE_2__", "true" },
                             } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    testFavoriteBroadcast( "hub.favorite.added", {}, { itemId1 }, {}, __LINE__ );
    answer = getFavoriteList( __LINE__ );
    testFavoriteList( answer, { deviceId1 }, { itemId1 }, {}, __LINE__ );

    request = replaceTokens( hubFavoriteSet,
                             {
                                 { "__DEVICE_ID_1__", "deviceId1" },
                                 { "__DEVICE_ID_2__", "deviceId2" },
                                 { "__IS_FAVORITE_DEVICE_1__", "true" },
                                 { "__IS_FAVORITE_DEVICE_2__", "true" },
                                 { "__ITEM_ID_1__", "itemId1" },
                                 { "__ITEM_ID_2__", "itemId2" },
                                 { "__IS_FAVORITE_ITEM_1__", "true" },
                                 { "__IS_FAVORITE_ITEM_2__", "true" },
                                 { "__RULE_ID_1__", sceneId1 },
                                 { "__RULE_ID_2__", "sceneId2" },
                                 { "__IS_FAVORITE_RULE_1__", "true" },
                                 { "__IS_FAVORITE_RULE_2__", "true" },
                             } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    testFavoriteBroadcast( "hub.favorite.added", {}, {}, { sceneId1 }, __LINE__ );
    answer = getFavoriteList( __LINE__ );
    testFavoriteList( answer, { deviceId1 }, { itemId1 }, { sceneId1 }, __LINE__ );

    request = replaceTokens( hubFavoriteSet,
                             {
                                 { "__DEVICE_ID_1__", "deviceId1" },
                                 { "__DEVICE_ID_2__", deviceId2 },
                                 { "__IS_FAVORITE_DEVICE_1__", "true" },
                                 { "__IS_FAVORITE_DEVICE_2__", "true" },
                                 { "__ITEM_ID_1__", "itemId1" },
                                 { "__ITEM_ID_2__", itemId2 },
                                 { "__IS_FAVORITE_ITEM_1__", "true" },
                                 { "__IS_FAVORITE_ITEM_2__", "true" },
                                 { "__RULE_ID_1__", "sceneId1" },
                                 { "__RULE_ID_2__", sceneId2 },
                                 { "__IS_FAVORITE_RULE_1__", "true" },
                                 { "__IS_FAVORITE_RULE_2__", "true" },
                             } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    testFavoriteBroadcast( "hub.favorite.added", { deviceId2 }, { itemId2 }, { sceneId2 }, __LINE__ );
    answer = getFavoriteList( __LINE__ );
    testFavoriteList( answer, { deviceId1, deviceId2 }, { itemId1, itemId2 }, { sceneId1, sceneId2 }, __LINE__ );

    request = replaceTokens( hubFavoriteSet,
                             {
                                 { "__DEVICE_ID_1__", deviceId1 },
                                 { "__DEVICE_ID_2__", deviceId2 },
                                 { "__IS_FAVORITE_DEVICE_1__", "false" },
                                 { "__IS_FAVORITE_DEVICE_2__", "false" },
                                 { "__ITEM_ID_1__", itemId1 },
                                 { "__ITEM_ID_2__", itemId2 },
                                 { "__IS_FAVORITE_ITEM_1__", "false" },
                                 { "__IS_FAVORITE_ITEM_2__", "false" },
                                 { "__RULE_ID_1__", sceneId1 },
                                 { "__RULE_ID_2__", sceneId2 },
                                 { "__IS_FAVORITE_RULE_1__", "false" },
                                 { "__IS_FAVORITE_RULE_2__", "false" },
                             } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    testFavoriteBroadcast(
        "hub.favorite.removed", { deviceId1, deviceId2 }, { itemId1, itemId2 }, { sceneId1, sceneId2 }, __LINE__ );
    answer = getFavoriteList( __LINE__ );
    testFavoriteList( answer, {}, {}, {}, __LINE__ );

    request = replaceTokens( hubFavoriteDeviceSet,
                             {
                                 { "__DEVICE_ID__", deviceId1 },
                                 { "__IS_FAVORITE_DEVICE__", "true" },
                             } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    testFavoriteBroadcast( "hub.favorite.added", { deviceId1 }, {}, {}, __LINE__ );
    answer = getFavoriteList( __LINE__ );
    testFavoriteList( answer, { deviceId1 }, {}, {}, __LINE__ );

    request = replaceTokens( hubFavoriteItemSet,
                             {
                                 { "__ITEM_ID__", itemId1 },
                                 { "__IS_FAVORITE_ITEM__", "true" },
                             } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    testFavoriteBroadcast( "hub.favorite.added", {}, { itemId1 }, {}, __LINE__ );
    answer = getFavoriteList( __LINE__ );
    testFavoriteList( answer, { deviceId1 }, { itemId1 }, {}, __LINE__ );

    request = replaceTokens( hubFavoriteRuleSet,
                             {
                                 { "__RULE_ID__", sceneId1 },
                                 { "__IS_FAVORITE_RULE__", "true" },
                             } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    testFavoriteBroadcast( "hub.favorite.added", {}, {}, { sceneId1 }, __LINE__ );
    answer = getFavoriteList( __LINE__ );
    testFavoriteList( answer, { deviceId1 }, { itemId1 }, { sceneId1 }, __LINE__ );

    request = replaceTokens( hubFavoriteDeviceSet,
                             {
                                 { "__DEVICE_ID__", deviceId1 },
                                 { "__IS_FAVORITE_DEVICE__", "false" },
                             } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    testFavoriteBroadcast( "hub.favorite.removed", { deviceId1 }, {}, {}, __LINE__ );
    answer = getFavoriteList( __LINE__ );
    testFavoriteList( answer, {}, { itemId1 }, { sceneId1 }, __LINE__ );

    request = replaceTokens( hubFavoriteItemSet,
                             {
                                 { "__ITEM_ID__", itemId1 },
                                 { "__IS_FAVORITE_ITEM__", "false" },
                             } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    testFavoriteBroadcast( "hub.favorite.removed", {}, { itemId1 }, {}, __LINE__ );
    answer = getFavoriteList( __LINE__ );
    testFavoriteList( answer, {}, {}, { sceneId1 }, __LINE__ );

    request = replaceTokens( hubFavoriteRuleSet,
                             {
                                 { "__RULE_ID__", sceneId1 },
                                 { "__IS_FAVORITE_RULE__", "false" },
                             } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    testFavoriteBroadcast( "hub.favorite.removed", {}, {}, { sceneId1 }, __LINE__ );
    answer = getFavoriteList( __LINE__ );
    testFavoriteList( answer, {}, {}, {}, __LINE__ );

    request = replaceTokens( hubFavoriteSet,
                             {
                                 { "__DEVICE_ID_1__", deviceId1 },
                                 { "__DEVICE_ID_2__", deviceId2 },
                                 { "__IS_FAVORITE_DEVICE_1__", "true" },
                                 { "__IS_FAVORITE_DEVICE_2__", "true" },
                                 { "__ITEM_ID_1__", itemId1 },
                                 { "__ITEM_ID_2__", itemId2 },
                                 { "__IS_FAVORITE_ITEM_1__", "true" },
                                 { "__IS_FAVORITE_ITEM_2__", "true" },
                                 { "__RULE_ID_1__", sceneId1 },
                                 { "__RULE_ID_2__", sceneId2 },
                                 { "__IS_FAVORITE_RULE_1__", "true" },
                                 { "__IS_FAVORITE_RULE_2__", "true" },
                             } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    testFavoriteBroadcast(
        "hub.favorite.added", { deviceId1, deviceId2 }, { itemId1, itemId2 }, { sceneId1, sceneId2 }, __LINE__ );
    answer = getFavoriteList( __LINE__ );
    testFavoriteList( answer, { deviceId1, deviceId2 }, { itemId1, itemId2 }, { sceneId1, sceneId2 }, __LINE__ );

    // remove devices to prevent unnecessary update broadcast
    JsonObject broadcast;
    coreRemoteInterface.removeDevice( deviceId1 );
    broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );

    answer = getFavoriteList( __LINE__ );
    testFavoriteList( answer, { deviceId2 }, { itemId2 }, { sceneId1, sceneId2 }, __LINE__ );

    coreRemoteInterface.removeDevice( deviceId2 );
    broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );

    answer = getFavoriteList( __LINE__ );
    testFavoriteList( answer, {}, {}, { sceneId1, sceneId2 }, __LINE__ );

    deleteScenes();

    answer = getFavoriteList( __LINE__ );
    testFavoriteList( answer, {}, {}, {}, __LINE__ );
}

TEST_F( UiServiceTests, checkBroadcastMsgIdField ) {
    // TODO: an echo request/reply/broadcast service would be good for such test
    const std::string createRoomRequest = R"(
    {
        "id": "_ID_",
        "method": "hub.room.create",
        "params": {
            "name": "__NAME__"
        }
    }
    )";

    const std::string deleteRoomRequest = R"(
    {
        "id": "_ID_",
        "method": "hub.room.delete",
        "params": {
            "_id": "__ROOM_ID__"
        }
    }
    )";

    Json request;
    Json answer;
    Json broadcast;

    request = replaceTokens( createRoomRequest, { { "__NAME__", "The room" } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_TRUE( answer[ "error" ].is_null() ) << answer[ "error" ].dump( 4 );
    EXPECT_FALSE( answer[ "result" ].empty() );

    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.room.created" );
    EXPECT_TRUE( json_contains( broadcast, "msg_id" ) );

    const auto roomId = json_get_str( answer[ "result" ], "_id" );
    const auto msgId = json_get_str( broadcast, "msg_id" );

    request = replaceTokens( deleteRoomRequest, { { "__ROOM_ID__", roomId } } );
    nma().sendtoHubAndGetAnswer( request );

    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.room.deleted" );
    EXPECT_TRUE( json_contains( broadcast, "msg_id" ) );
    EXPECT_NE( json_get_str( broadcast, "msg_id" ), msgId );
}

/**********************************************************
 *
 *      hubItemsListFilter
 *
 **********************************************************/
#include <set>
#include <utility>

typedef std::set< std::string > UniqueDeviceIds;

UniqueDeviceIds makeUniqueDeviceIds( JsonObject const& responseItems );

class UiGetItemsTests : virtual public UiServiceTests {
public:
    virtual void setupOwnDevices();
    virtual void removeOwnDevices( int codeLine );

protected:
    typedef CoreDRemoteInterface::CoreDInterface::DeviceID DeviceID;
    typedef CoreDRemoteInterface::CoreDInterface::DeviceID ItemID;

    DeviceID deviceId1, deviceId2;
    ItemID itemId1, itemId21, itemId22;
};

UniqueDeviceIds makeUniqueDeviceIds( JsonObject const& responseItems ) {
    UniqueDeviceIds responseDeviceIds;
    for( auto const& deviceItem : responseItems ) {
        std::string deviceId = deviceItem[ "deviceId" ];
        responseDeviceIds.insert( deviceId );
    }
    return responseDeviceIds;
}

void UiGetItemsTests::setupOwnDevices() {
    using namespace CoreD::Model::Resources;
    auto& coreRemoteInterface = makeRemote( nma() );
    this->deviceId1 = addDevice( coreRemoteInterface,
                                 "testDevice1",
                                 "smoke",
                                 "134_3_24",
                                 "security_sensor",
                                 "smoke",
                                 false /*batteryPowered*/,
                                 __LINE__ );
    this->deviceId2 = addDevice( coreRemoteInterface,
                                 "testDevice1",
                                 "smoke",
                                 "134_3_24",
                                 "security_sensor",
                                 "smoke",
                                 false /*batteryPowered*/,
                                 __LINE__ );

    // Step 2: mocking items
    this->itemId1 = addItem(
        coreRemoteInterface, deviceId1, "testItem1-1", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    this->itemId21 = addItem(
        coreRemoteInterface, deviceId2, "testItem2-1", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    this->itemId22 = addItem(
        coreRemoteInterface, deviceId2, "testItem2-2", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
}

void UiGetItemsTests::removeOwnDevices( int codeLine ) {
    using namespace CoreD::Model::Resources;
    auto& remoteInterface = makeRemote( nma() );

    auto removeSingleDevice = [this, &remoteInterface, codeLine]( std::string const& deviceId ) {
        JsonObject items = remoteInterface.getItemsByDeviceId( deviceId );
        auto itemsCount = items.is_array() ? items.size() : 0;

        removeDevice( remoteInterface, deviceId, itemsCount, 0, codeLine );
    };

    if( !deviceId1.empty() )
        removeSingleDevice( deviceId1 );
    if( !deviceId2.empty() )
        removeSingleDevice( deviceId2 );
}

TEST_F( UiGetItemsTests, hubItemsListFilter ) {
    // Step 0: Remove devices set up by previous tests.
    clearExistingDevices( __LINE__ );
    // Step 1: Mocking devices
    setupOwnDevices();

    const std::string EMPTY_DEVICE_LIST = "";

    auto deviceList = EMPTY_DEVICE_LIST;

    // TODO: Move this outside of a test.
    typedef std::vector< std::string > TDeviceIds;
    struct TTestRec {
    public:
        TTestRec() = default;
        TTestRec( TDeviceIds deviceIds, JsonObject info )
        : m_deviceIds( std::move( deviceIds ) )
        , m_info( std::move( info ) ) {}

    public:
        TDeviceIds m_deviceIds;
        JsonObject m_info;
    };
    typedef std::vector< TTestRec > TTestCases;

    TTestCases tests = {
        { {}, { { "info", "empty deviceIds filter" }, { "expectedItems", 3 }, { "codeLine", __LINE__ } } },
        { { deviceId1, deviceId2 },
          { { "info", "two existing devices listed" }, { "expectedItems", 3 }, { "codeLine", __LINE__ } } },
        { { deviceId1 },
          { { "info", "one/first existing device listed" }, { "expectedItems", 1 }, { "codeLine", __LINE__ } } },
        { { deviceId2 },
          { { "info", "one/second existing devices listed" }, { "expectedItems", 2 }, { "codeLine", __LINE__ } } },
        { { "_non_existent_deviceId" },
          { { "info", "one/non-existent devices listed" }, { "expectedItems", 0 }, { "codeLine", __LINE__ } } },
    };

    auto joinDeviceIds = []( TDeviceIds const& deviceIds ) -> std::string {
        if( deviceIds.empty() )
            return "";
        const std::string quote = std::string( "\"" );
        std::string joined = std::accumulate(
            begin( deviceIds ) + 1,
            end( deviceIds ),
            quote + deviceIds[ 0 ] + quote,
            [&]( const std::string& a, const std::string& b ) { return a + std::string( "," ) + quote + b + quote; } );
        return joined;
    };
    auto fillRequest = [joinDeviceIds]( TDeviceIds const& deviceIds ) {
        auto stringDump = replaceTokensInString(
            hubItemsList,
            { { "__DEVICES__", std::string( "[  " ) + joinDeviceIds( deviceIds ) + std::string( "  ]" ) } } );
        std::string data = hubItemsList;
        std::string devicesPattern = "__DEVICES__";
        std::string arrayToInsert = std::string( "[  " ) + joinDeviceIds( deviceIds ) + std::string( "  ]" );

        auto simpleReplacedString = data.replace( data.find( devicesPattern ), devicesPattern.size(), arrayToInsert );
        JsonObject request = JsonObject::parse( simpleReplacedString );
        return request;
    };

    for( const auto& test : tests ) {
        JsonObject filledRequest = fillRequest( test.m_deviceIds );
        auto codeLineStr = getCodeLineString( test.m_info[ "codeLine" ] );

        // Step 4: Send it and get the answer
        auto answer = nma().sendtoHubAndGetAnswer( filledRequest );

        auto error = answer[ "error" ];
        ASSERT_TRUE( error.empty() ) << error.dump( 4 ) << "code line: " << codeLineStr;

        auto testAnswer = [&test, &answer, &codeLineStr]() {
            int expectedItems;
            JsonObject responseItems;

            try {
                expectedItems = test.m_info[ "expectedItems" ];
                bool hasResult = answer.count( "result" );
                bool hasResultItems = hasResult ? answer[ "result" ].count( "items" ) : false;

                responseItems = answer[ "result" ][ "items" ];
                auto itemsSize = responseItems.size();

                ASSERT_EQ( itemsSize, expectedItems )
                    << "Items size should match number of test.expectedItems" << test.m_info.dump();

                // extract deviceIds from items
                UniqueDeviceIds responseDeviceIds = makeUniqueDeviceIds( responseItems );

                // Loop by this list: all identifiers should be found in request's deviceIds array
                if( !test.m_deviceIds.empty() ) {
                    for( const auto& idString : responseDeviceIds ) {
                        auto bit = begin( test.m_deviceIds ), eit = end( test.m_deviceIds );
                        bool isFoundInRequestParams = std::find( bit, eit, idString ) != eit;

                        ASSERT_TRUE( isFoundInRequestParams ) << codeLineStr;
                    }
                }
                else {
                    ASSERT_TRUE( !responseDeviceIds.empty() ) << codeLineStr;
                }
            }
            catch( std::exception const& e ) {
                auto msg = e.what();
                std::cerr << "Error in testAnswer: " << msg << ", line: " << codeLineStr << "\n";
            }
        };
        testAnswer();
    }

    this->removeOwnDevices( __LINE__ );
}

typedef float Expected;
using TestCase = std::pair< CommonNmaTests::KeyStringList, Expected >;

class HubItemValuePercentApplyTests : public UiGetItemsTests, public ::testing::WithParamInterface< TestCase > {
public:
    void setupOwnDevices() override;

    void SetUp() override;
    void TearDown() override;

protected:
    float m_setpointValue = 27.0f;
    float m_minValue = 0.0f;
    float m_maxValue = 40.0f;
    std::string m_scale = "celsius";
};

void HubItemValuePercentApplyTests::SetUp() {
    UiGetItemsTests::SetUp();
    setupOwnDevices();
}

void HubItemValuePercentApplyTests::TearDown() {
    removeOwnDevices( __LINE__ );
    UiGetItemsTests::TearDown();
}

void HubItemValuePercentApplyTests::setupOwnDevices() {
    auto& remoteInterface = makeRemote( nma() );
    JsonObject broadcast;

    const std::string gatewayId = getMainGateway( remoteInterface, __LINE__ );

    CoreD::Model::Device thermostat( gatewayId, "Z-Wave Thermostat", "thermostat", "139_21586_21570", "hvac", false );
    thermostat.setSubcategory( "hvac" );
    auto thermostatAddResult = remoteInterface.addDevice( thermostat );
    EXPECT_FALSE( thermostatAddResult.first.empty() );
    EXPECT_TRUE( thermostatAddResult.second );

    this->deviceId1 = thermostatAddResult.first;

    broadcast = waitForBroadcast( "hub.device.added",
                                  std::string( __FILE__ ) + ":" + std::to_string( __LINE__ ) + " - no broadcast" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.added" );

    auto valueType = CoreD::Model::ValueType::Enum::Temperature;
    const std::string scale = CoreD::Model::ValueScales::Celsius;
    auto value = CoreD::Model::ValueFactory::createItemValue( valueType, JSON( "value" << 18 << "scale" << scale ) );

    CoreD::Model::Item setpointItem(
        thermostatAddResult.first, "thermostat_setpoint", valueType, true, true, std::move( value ) );

    auto setpointAddResult = remoteInterface.addItem( setpointItem );
    this->itemId1 = setpointAddResult.first;

    EXPECT_FALSE( setpointAddResult.first.empty() );
    EXPECT_TRUE( setpointAddResult.second );
    broadcast = waitForBroadcast( "hub.item.added",
                                  std::string( __FILE__ ) + ":" + std::to_string( __LINE__ ) + " - no broadcast" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.added" );

    Json joNewVal = JSON( "value" << m_setpointValue << "scale" << scale );
    Json joMinVal = JSON( "value" << m_minValue << "scale" << scale );
    Json joMaxVal = JSON( "value" << m_maxValue << "scale" << scale );

    auto makeValueLoader = []( const Json& source ) -> CoreDRemoteInterface::ValueLoader {
        return [&source]( CoreD::Model::ValueType valueType ) {
            return CoreD::Model::ValueFactory::createItemValue( valueType, source );
        };
    };

    remoteInterface.updateItemValueWithMinMax( setpointAddResult.first,
                                               makeValueLoader( joNewVal ),
                                               makeValueLoader( joMinVal ),
                                               makeValueLoader( joMaxVal ) );

    broadcast = waitForBroadcast( "hub.item.updated",
                                  std::string( __FILE__ ) + ":" + std::to_string( __LINE__ ) + " - no broadcast" );
    auto broadcastDump = broadcast.dump();  // dbg
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" );
    Json result;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    EXPECT_EQ( json_get_str( result, "name" ), "thermostat_setpoint" );
    EXPECT_EQ( json_get_str( result, "_id" ), this->itemId1 );

    compareTemperatureValues( Json{ { "value", m_setpointValue }, { "scale", m_scale } }, result, "value" );
    EXPECT_EQ( json_get_str( result, "valueType" ), "temperature" );
}

TEST_P( HubItemValuePercentApplyTests, Perform_HubItemValuePercentApply ) {
    auto thermostatItemId = this->itemId1;

    auto testParam = GetParam();
    auto keyStrings = testParam.first;
    auto replaceItemIdFn = [&]( KeyStringList::value_type& item ) {
        if( item.first == "__ITEMID__" ) {
            item.second = thermostatItemId;
        }
    };

    std::for_each( begin( keyStrings ), end( keyStrings ), replaceItemIdFn );
    auto expectedValue = testParam.second;
    std::string requestString = replaceTokensInString( hubItemValuePercentApply, keyStrings );

    JsonObject request = JsonObject::parse( requestString );
    auto answer = nma().sendtoHubAndGetAnswer( request );

    JsonObject result, newValue, scale, valueType;
    ASSERT_TRUE( json_get_obj( answer, "result", result ) );
    Json tempExpected{ { "value", expectedValue }, { "scale", CoreD::Model::ValueScales::Celsius } };
    compareTemperatureValues( tempExpected, result, "newValue" );
    ASSERT_EQ( json_get_str( result, "valueType" ), "temperature" );

    // check broadcast update here
    JsonObject broadcast =
        waitForBroadcast( "hub.item.updated", "Line: " + std::to_string( __LINE__ ) + " - item updated" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" );

    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    compareTemperatureValues( tempExpected, result, "value", "Value in a broadcast should match" );
}

using KeyStringList = HubPlatform::Tests::CommonNmaTests::KeyStringList;

INSTANTIATE_TEST_CASE_P(
    Prefix,
    HubItemValuePercentApplyTests,
    ::testing::Values(
        std::make_pair< KeyStringList, float >( { { "__ITEMID__", "" }, { "__PERCENT__", "20" } }, 35.0f ),
        std::make_pair< KeyStringList, float >( { { "__ITEMID__", "" }, { "__PERCENT__", "5" } }, 29.0f ),
        std::make_pair< KeyStringList, float >( { { "__ITEMID__", "" }, { "__PERCENT__", "-5" } }, 25.0f ),
        // TODO: set this to actual value when floating-point percents will be actually supported
        std::make_pair< KeyStringList, float >( { { "__ITEMID__", "" }, { "__PERCENT__", "-10.5" } }, 23.0f ) ) );

}  // namespace Tests
}  // namespace HubPlatform
