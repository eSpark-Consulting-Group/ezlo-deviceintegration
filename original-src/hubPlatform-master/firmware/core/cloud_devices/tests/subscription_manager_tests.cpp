#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "cloud_devices/subscription_manager/subscription_manager.h"
#include "cloud_devices/subscription_manager/subscription_helper.h"
#include "cloud_devices/resources/cloud_devices_resources.h"
#include "cloud_devices/subscription_manager/subscription_utils.h"
#include "cloud_devices/subscription_manager/subscription_exceptions.h"

#include "cloud_devices/tests/fakes/fake_cloud_devices_service.h"
#include "cloud_devices/tests/fakes/fake_cored_remote_interface.h"
#include "cloud_devices/tests/mocks/subscription_helper_mock.h"

/*

TESTPLAN

DONE    1. CloudDevices tests
DONE        1.1  Create subscription
DONE        1.2  Delete subscription
DONE        1.3  Delete unknown subscription
DONE        1.4  Send subscriptions list
DONE        1.5  Send empty subscriptions list
DONE        1.6  Update subscription
DONE        1.7  Remove item by update subscription
DONE        1.8  Create subscription with gateway
DONE            1.8.1  Create subscription with gateway
DONE            1.8.2  Create subscription with ezlo_geofencing gateway
DONE        1.9  Update subscription with gateway
DONE        1.10 Delete subscription with gateway

DONE     2. Geofencing gateway, abstract subscriptions
DONE        2.1 Create ezlo_geofencing subscription, successful

DONE     3. ezlo_geofencing gateway, dispatch commands
DONE        3.1 Success
DONE        3.2 Errors


*/

namespace HubPlatform {
namespace UT {

using namespace CloudDevices;

class SubscriptionManagerTestSuite : public FakeCloudDevicesService {
protected:
    static Json createGateway( const std::string& gatewayName );
    static void SetUpTestCase();
};

Json SubscriptionManagerTestSuite::createGateway( const std::string& gatewayName ) {
    using namespace CloudDevices::Resources::GatewayScripts::ScriptCommand;
    return { { CreateAbstractSubscription, "HUB:" + gatewayName + "/scripts/create_abstract_subscription" },
             { UpdateAbstractSubscription, "HUB:" + gatewayName + "/scripts/update_abstract_subscription" },
             { DeleteAbstractSubscription, "HUB:" + gatewayName + "/scripts/delete_abstract_subscription" },
             { DispatchAbstractSubscription, "HUB:" + gatewayName + "/scripts/dispatch_abstract_subscription" } };
}

void SubscriptionManagerTestSuite::SetUpTestCase() {
    FakeCloudDevicesService::SetUpTestCase();
    Log::setLocalLogLevel( Log::PriorityLevel::TRACE );
}

TEST_F( SubscriptionManagerTestSuite, CreateSubscriptions ) /* 1.1 */ {
    using namespace Resources;

    auto& dbClient = getHostService().getDBClient();
    FakeCoreDRemoteInterface core( getHostService() );
    SubscriptionHelper helper( getHostService(), core );

    SubscriptionManager manager( helper, dbClient );

    auto request1 = Json::parse( R"({
    "abstractUUID": "uuid_1",
    "mappings": [{
        "capability": "capability_1",
        "variable": "variable_1",
        "valueType": "string",
        "dictionaryType": false,
        "value": "value_1"
    }]
    })" );

    auto request2 = Json::parse( R"({
    "abstractUUID": "uuid_2",
    "mappings": [{
        "capability": "capability_2",
        "variable": "variable_2",
        "valueType": "bool",
        "dictionaryType": false,
        "value": true
    },
    {
        "capability": "capability_3",
        "variable": "variable_3",
        "valueType": "int",
        "dictionaryType": true,
        "value": { "key": 3 }
    }]
    })" );

    EXPECT_FALSE( hasError( manager.addSubscription( request1 ) ) );
    EXPECT_FALSE( hasError( manager.addSubscription( request2 ) ) );

    ASSERT_EQ( core.m_devices.size(), 2 );
    ASSERT_EQ( core.m_items.size(), 3 );

    auto table = dbClient.findAll( DB::TableNames::Subscriptions, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( table.size(), 2 );

    for( auto& subscription : table ) {
        auto deviceId = json_get_str( subscription, DB::SubscriptionFields::DeviceId );
        EXPECT_TRUE( core.m_devices.count( deviceId ) ) << "Device Id should be present: " << deviceId;

        auto& mappings = subscription.at( DB::SubscriptionFields::Mappings );
        for( auto& mapping : mappings ) {
            auto itemId = json_get_str( mapping, DB::SubscriptionFields::ItemId );
            EXPECT_TRUE( core.m_items.count( itemId ) ) << "Item Id should be present: " << itemId;
        }
    }

    for( auto& subscription : table ) {
        subscription.at( DB::SubscriptionFields::DeviceId ) = "deviceId";
        auto& mappings = subscription.at( DB::SubscriptionFields::Mappings );
        for( auto& mapping : mappings )
            mapping.at( DB::SubscriptionFields::ItemId ) = "itemId";
    }

    auto expectedSubscription1 = Json::parse( R"({
        "_id": "uuid_1",
        "deviceId": "deviceId",
        "mappings": [{
            "capability": "capability_1",
            "variable": "variable_1",
            "itemId": "itemId"
        }]
    })" );

    auto& subscription1 = table.front();
    EXPECT_EQ( subscription1.dump(), expectedSubscription1.dump() );

    auto expectedSubscription2 = Json::parse( R"({
        "_id": "uuid_2",
        "deviceId": "deviceId",
        "mappings": [{
            "capability": "capability_2",
            "variable": "variable_2",
            "itemId": "itemId"
        },
        {
            "capability": "capability_3",
            "variable": "variable_3",
            "itemId": "itemId"
        }]
    })" );

    auto& subscription2 = table.back();
    EXPECT_EQ( subscription2.dump(), expectedSubscription2.dump() );
}

TEST_F( SubscriptionManagerTestSuite, DeleteSubscription ) /* 1.2 */ {
    using namespace Resources;

    auto& dbClient = getHostService().getDBClient();
    FakeCoreDRemoteInterface core( getHostService() );
    SubscriptionHelper helper( getHostService(), core );

    SubscriptionManager manager( helper, dbClient );

    auto request1 = Json::parse( R"({
    "abstractUUID": "uuid_1",
    "mappings": [{
        "capability": "capability_1",
        "variable": "variable_1",
        "valueType": "string",
        "dictionaryType": false,
        "value": "value_1"
    }]
    })" );

    auto request2 = Json::parse( R"({
    "abstractUUID": "uuid_2",
    "mappings": [{
        "capability": "capability_2",
        "variable": "variable_2",
        "valueType": "string",
        "dictionaryType": false,
        "value": "value_2"
    }]
    })" );

    EXPECT_FALSE( hasError( manager.addSubscription( request1 ) ) );
    EXPECT_FALSE( hasError( manager.addSubscription( request2 ) ) );

    ASSERT_EQ( core.m_devices.size(), 2 );
    ASSERT_EQ( core.m_items.size(), 2 );

    EXPECT_FALSE( hasError( manager.deleteSubscription( JSON( RPCParams::AbstractUUID << "uuid_2" ) ) ) );

    ASSERT_EQ( core.m_devices.size(), 1 );
    ASSERT_EQ( core.m_items.size(), 1 );

    auto table = dbClient.findAll( DB::TableNames::Subscriptions, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( table.size(), 1 );

    auto uuid = json_get_str( table.front(), DB::SubscriptionFields::Id );
    EXPECT_EQ( uuid, "uuid_1" );
}

TEST_F( SubscriptionManagerTestSuite, DeleteUnknownSubscription ) /* 1.3 */ {
    using namespace Resources;

    auto& dbClient = getHostService().getDBClient();
    FakeCoreDRemoteInterface core( getHostService() );
    SubscriptionHelper helper( getHostService(), core );

    SubscriptionManager manager( helper, dbClient );
    EXPECT_TRUE( hasError( manager.deleteSubscription( JSON( RPCParams::AbstractUUID << "unknownUUID" ) ) ) );
}

TEST_F( SubscriptionManagerTestSuite, SendSubscriptionsListToCloud ) /* 1.4 */ {
    using namespace Resources;

    auto& dbClient = getHostService().getDBClient();
    FakeCoreDRemoteInterface core( getHostService() );
    SubscriptionHelper helper( getHostService(), core );
    SubscriptionManager manager( helper, dbClient );

    auto request1 = Json::parse( R"({
    "abstractUUID": "uuid_1",
    "mappings": [{
        "capability": "capability_1",
        "variable": "variable_1",
        "valueType": "string",
        "dictionaryType": false,
        "value": "value_1"
    }]
    })" );

    auto request2 = Json::parse( R"({
    "abstractUUID": "uuid_2",
    "mappings": [{
        "capability": "capability_2",
        "variable": "variable_2",
        "valueType": "bool",
        "dictionaryType": false,
        "value": true
    },
    {
        "capability": "capability_3",
        "variable": "variable_3",
        "valueType": "int",
        "dictionaryType": false,
        "value": 3
    },
    {
        "capability": "capability_4",
        "variable": "variable_4",
        "valueType": "int",
        "dictionaryType": true,
        "value": { "key": 4 }
    }]
    })" );

    EXPECT_FALSE( hasError( manager.addSubscription( request1 ) ) );
    EXPECT_FALSE( hasError( manager.addSubscription( request2 ) ) );

    manager.sendSubscriptionList();
    auto message = getFakeResponseBus().getMessage();

    EXPECT_EQ( message.group, GroupsNames::UI_GROUP );
    EXPECT_EQ( message.msgSubClass, CLOUD_MESSAGE );

    auto& data = message.data;
    ASSERT_TRUE( !data.empty() );

    auto& requestBody = data.at( "requestBody" );
    ASSERT_TRUE( !requestBody.empty() );

    EXPECT_EQ( json_get_int( requestBody, API::General::Api ), 1 );
    EXPECT_EQ( json_get_str( requestBody, API::General::Method ), CloudMethods::CloudControllerSubscribeAbstracts );

    auto& params = requestBody.at( API::General::Params );
    auto& abstracts = params.at( CloudRPCParams::Abstracts );
    EXPECT_EQ( abstracts.size(), 2 );

    for( auto& abstract : abstracts ) {
        auto deviceId = json_get_str( abstract, CloudRPCParams::DeviceId );
        EXPECT_TRUE( core.m_devices.count( deviceId ) ) << "Device id should be present: " << deviceId;
        abstract.at( CloudRPCParams::DeviceId ) = "device_id";

        auto& mappings = abstract.at( CloudRPCParams::Mappings );
        for( auto& mapping : mappings ) {
            auto itemId = json_get_str( mapping, CloudRPCParams::ItemId );
            EXPECT_TRUE( core.m_items.count( itemId ) ) << "Item Id should be present: " << itemId;
            mapping.at( CloudRPCParams::ItemId ) = "item_id";
        }
    }

    auto expectedParams = Json::parse( R"({
        "abstracts": [{
            "abstract_uuid": "uuid_1",
            "device_id": "device_id",
            "mappings": [{
                "capability": "capability_1",
                "item_id": "item_id",
                "value": "value_1",
                "variable": "variable_1"
            }]
        },
        {
            "abstract_uuid": "uuid_2",
            "device_id": "device_id",
            "mappings": [{
                "capability": "capability_2",
                "item_id": "item_id",
                "value": 1,
                "variable": "variable_2"
            },
            {
                "capability": "capability_3",
                "item_id": "item_id",
                "value": 3,
                "variable": "variable_3"
            },
            {
                "capability": "capability_4",
                "item_id": "item_id",
                "value": "{\"key\":4}",
                "variable": "variable_4"
            }]
        }]
    })" );

    EXPECT_EQ( params.dump(), expectedParams.dump() );
}

TEST_F( SubscriptionManagerTestSuite, SendEmptySubscriptionsListToCloud ) /* 1.5 */ {
    using namespace Resources;

    auto& dbClient = getHostService().getDBClient();
    FakeCoreDRemoteInterface core( getHostService() );
    SubscriptionHelper helper( getHostService(), core );
    SubscriptionManager manager( helper, dbClient );

    manager.sendSubscriptionList();
    auto message = getFakeResponseBus().getMessage();

    auto& data = message.data;
    auto& requestBody = data.at( "requestBody" );
    auto& params = requestBody.at( API::General::Params );

    auto expectedParams = JSON( CloudRPCParams::Abstracts << JSON_EMPTY_ARR );

    EXPECT_EQ( params.dump(), expectedParams.dump() );
}

TEST_F( SubscriptionManagerTestSuite, UpdateSubscriptions ) { /* 1.6 */
    using namespace Resources;
    auto& dbClient = getHostService().getDBClient();
    FakeCoreDRemoteInterface core( getHostService() );
    SubscriptionHelper helper( getHostService(), core );

    SubscriptionManager manager( helper, dbClient );

    auto request1 = Json::parse( R"({
            "abstractUUID": "uuid_1",
            "mappings": [{
                "capability": "capability_1",
                "variable": "variable_1",
                "valueType": "int",
                "dictionaryType": false,
                "value": 10,
                "itemId": "itemId"
            }]
    })" );

    EXPECT_FALSE( hasError( manager.addSubscription( request1 ) ) );
    ASSERT_EQ( core.m_devices.size(), 1 );
    ASSERT_EQ( core.m_items.size(), 1 );

    auto table = dbClient.findAll( DB::TableNames::Subscriptions, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( table.size(), 1 );

    auto request2 = Json::parse( R"({
            "abstractUUID": "uuid_1",
            "mappings": [{
                "capability": "capability_1",
                "variable": "variable_1",
                "valueType": "int",
                "dictionaryType": false,
                "value": 20,
                "itemId": "itemId"
            }]
    })" );
    EXPECT_FALSE( hasError( manager.updateSubscription( request2 ) ) );

    for( const auto& item : core.m_items ) {
        auto itemInfo = item.second;
        EXPECT_TRUE( json_get_str( itemInfo, "name" ) == "capability_1.variable_1" );
        EXPECT_TRUE( json_get_int( itemInfo, "value" ) == 20 );
    }

    auto request3 = Json::parse( R"({
            "abstractUUID": "uuid_1",
            "mappings": [{
                "capability": "capability_1",
                "variable": "variable_2",
                "valueType": "int",
                "dictionaryType": false,
                "value": 20,
                "maxValue": 30,
                "minValue": 10,
                "itemId": "itemId",
                "enum": ["one", "two"]
            }]
    })" );
    EXPECT_FALSE( hasError( manager.updateSubscription( request3 ) ) );
    ASSERT_EQ( core.m_devices.size(), 1 );
    ASSERT_EQ( core.m_items.size(), 2 );

    for( const auto& item : core.m_items ) {
        auto itemInfo = item.second;
        std::string name = json_get_str( itemInfo, "name" );
        if( name == "capability_1.variable_2" ) {
            EXPECT_TRUE( json_get_int( itemInfo, "valueMax" ) == 30 );
            EXPECT_TRUE( json_get_int( itemInfo, "value" ) == 20 );
            EXPECT_TRUE( json_get_int( itemInfo, "valueMin" ) == 10 );
            JsonArray arrEnum;
            json_get_arr( itemInfo, "enum", arrEnum );
            EXPECT_EQ( arrEnum.size(), 2 );
        }
    }
}

TEST_F( SubscriptionManagerTestSuite, RemoveItemByUpdateSubscriptions ) { /* 1.7 */
    using namespace Resources;

    auto& dbClient = getHostService().getDBClient();
    FakeCoreDRemoteInterface core( getHostService() );
    SubscriptionHelper helper( getHostService(), core );

    SubscriptionManager manager( helper, dbClient );

    auto request1 = Json::parse( R"({
            "abstractUUID": "uuid_1",
            "mappings": [{
                "capability": "capability_1",
                "variable": "variable_1",
                "valueType": "int",
                "dictionaryType": false,
                "value": 10,
                "itemId": "itemId"
            },
            {
                "capability": "capability_1",
                "variable": "variable_2",
                "valueType": "int",
                "dictionaryType": false,
                "value": 10,
                "itemId": "itemId1"
            }]
    })" );

    EXPECT_FALSE( hasError( manager.addSubscription( request1 ) ) );

    ASSERT_EQ( core.m_devices.size(), 1 );
    ASSERT_EQ( core.m_items.size(), 2 );

    auto table = dbClient.findAll( DB::TableNames::Subscriptions, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( table.size(), 1 );

    auto request2 = Json::parse( R"({
            "abstractUUID": "uuid_1",
            "mappings": [{
                "capability": "capability_1",
                "variable": "variable_1"
            }]
    })" );
    EXPECT_FALSE( hasError( manager.updateSubscription( request2 ) ) );

    ASSERT_EQ( core.m_devices.size(), 1 );
    ASSERT_EQ( core.m_items.size(), 1 );

    auto request3 = Json::parse( R"({
            "abstractUUID": "uuid_1",
            "mappings": [{
                "capability": "capability_1",
                "variable": "variable_2"
            }]
    })" );
    EXPECT_FALSE( hasError( manager.updateSubscription( request3 ) ) );
    // ASSERT_EQ( core.m_devices.size(), 0 ); // TODO check if subscription should be removed if no items left
    ASSERT_EQ( core.m_items.size(), 0 );
}

TEST_F( SubscriptionManagerTestSuite, CreateSubscriptionsWithGateway ) /* 1.8.1 */ {
    using namespace Resources;

    auto& dbClient = getHostService().getDBClient();
    FakeCoreDRemoteInterface core( getHostService() );
    SubscriptionHelperMock helper( getHostService(), core );
    SubscriptionManager manager( helper, dbClient );

    auto request1 = Json::parse( R"({
    "gatewayName": "gateway1",
    "abstractUUID": "uuid_1",
    "mappings": [{
        "capability": "capability_1",
        "variable": "variable_1",
        "valueType": "string",
        "dictionaryType": false,
        "value": "value_1"
    }]
    })" );

    EXPECT_CALL( helper, getGatewayByName( testing::_ ) ).WillOnce( testing::Return( createGateway( "gateway1" ) ) );
    EXPECT_CALL( helper, sendExecutePluginScript( "HUB:gateway1/scripts/create_abstract_subscription", testing::_ ) )
        .Times( 1 );
    EXPECT_FALSE( hasError( manager.addSubscription( request1 ) ) );
}

TEST_F( SubscriptionManagerTestSuite, CreateSubscriptionsWithGeofencingGateway ) /* 1.8.2 */ {
    using namespace Resources;

    auto& dbClient = getHostService().getDBClient();
    FakeCoreDRemoteInterface core( getHostService() );
    SubscriptionHelperMock helper( getHostService(), core );
    SubscriptionManager manager( helper, dbClient );

    auto request1 = Json::parse( R"({
    "gatewayName": "ezlo_geofencing",
    "abstractUUID": "uuid_1",
    "mappings": [{
        "capability": "geofences_all",
        "variable": "geofences",
        "valueType": "geofence",
        "dictionaryType": true,
        "value": {}
    }]
    })" );

    // creation of a subscription
    const Json geofencingGateway = createGateway( "ezlo_geofencing" );
    EXPECT_CALL( helper, getGatewayByName( testing::_ ) ).WillOnce( testing::Return( geofencingGateway ) );
    EXPECT_CALL( helper,
                 sendExecutePluginScript( "HUB:ezlo_geofencing/scripts/create_abstract_subscription", testing::_ ) )
        .Times( 1 );
    EXPECT_FALSE( hasError( manager.addSubscription( request1 ) ) );

    // dispatch of subscription data, success
    JsonArray geofenceInfo = JSON_ARRAY_( "abstract_uuid1", 1, 12345678 );
    Json dispatchCommandParams = JSON( "_id"
                                       << "2"
                                       << "value" << JSON_ARRAY_( geofenceInfo ) << "gatewayName"
                                       << "ezlo_geofencing" );

    // dispatch subscription successfully
    {
        EXPECT_CALL( helper, getGatewayByName( testing::_ ) ).WillOnce( testing::Return( geofencingGateway ) );
        EXPECT_CALL( helper, itemBelongsToGateway( testing::_, testing::_ ) ).WillOnce( testing::Return( true ) );
        EXPECT_CALL(
            helper,
            sendExecutePluginScript( "HUB:ezlo_geofencing/scripts/dispatch_abstract_subscription", testing::_ ) )
            .Times( 1 );
        EXPECT_FALSE( hasError( manager.dispatchSubscription( dispatchCommandParams ) ) );
    }
    // No gateway found
    {
        Json gateway = JSON_EMPTY_OBJ;
        using namespace CloudDevices::Resources::GatewayScripts;

        // The gatewayName field is not  passed
        EXPECT_CALL( helper, getGatewayByName( testing::_ ) ).WillOnce( testing::Return( gateway ) );
        EXPECT_CALL( helper, itemBelongsToGateway( testing::_, testing::_ ) ).Times( 0 );
        EXPECT_CALL(
            helper,
            sendExecutePluginScript( "HUB:ezlo_geofencing/scripts/dispatch_abstract_subscription", testing::_ ) )
            .Times( 0 );
        ASSERT_THROW( manager.dispatchSubscription( dispatchCommandParams ), GatewayNotFoundError )
            << "Should throw if the is no gateway";
    }

    // dispatch of subscription data, no gateway script for dispatch
    {
        Json gatewayWithoutDispatchScript = geofencingGateway;
        using namespace CloudDevices::Resources::GatewayScripts;
        json_remove( gatewayWithoutDispatchScript, ScriptCommand::DispatchAbstractSubscription );

        EXPECT_CALL( helper, getGatewayByName( testing::_ ) )
            .WillOnce( testing::Return( gatewayWithoutDispatchScript ) );
        EXPECT_CALL( helper, itemBelongsToGateway( testing::_, testing::_ ) ).Times( 0 );
        EXPECT_CALL(
            helper,
            sendExecutePluginScript( "HUB:ezlo_geofencing/scripts/dispatch_abstract_subscription", testing::_ ) )
            .Times( 0 );
        ASSERT_THROW( manager.dispatchSubscription( dispatchCommandParams ), BadGatewayError )
            << "Should throw if the gateway has no dispatch command script";
    }

    // Bad subscription parameters: _id, value
    {
        using namespace CloudDevices::Resources::GatewayScripts;

        EXPECT_CALL( helper, getGatewayByName( testing::_ ) ).WillOnce( testing::Return( geofencingGateway ) );
        EXPECT_CALL( helper, itemBelongsToGateway( testing::_, testing::_ ) ).Times( 0 );
        EXPECT_CALL(
            helper,
            sendExecutePluginScript( "HUB:ezlo_geofencing/scripts/dispatch_abstract_subscription", testing::_ ) )
            .Times( 0 );
        Json dispatchCommandWoRequiredField = dispatchCommandParams;
        json_remove( dispatchCommandWoRequiredField, "_id" );

        ASSERT_THROW( manager.dispatchSubscription( dispatchCommandWoRequiredField ), SubscriptionBadParamError )
            << "Required param is missing: _id";

        EXPECT_CALL( helper, getGatewayByName( testing::_ ) ).WillOnce( testing::Return( geofencingGateway ) );
        EXPECT_CALL( helper, itemBelongsToGateway( testing::_, testing::_ ) ).Times( 0 );
        EXPECT_CALL(
            helper,
            sendExecutePluginScript( "HUB:ezlo_geofencing/scripts/dispatch_abstract_subscription", testing::_ ) )
            .Times( 0 );
        dispatchCommandWoRequiredField = dispatchCommandParams;
        json_remove( dispatchCommandWoRequiredField, "value" );
        ASSERT_THROW( manager.dispatchSubscription( dispatchCommandWoRequiredField ), SubscriptionBadParamError )
            << "Required param is missing: value";
    }
}

TEST_F( SubscriptionManagerTestSuite, UpdateSubscriptionsWithGateway ) /* 1.8 */ {
    using namespace Resources;

    auto& dbClient = getHostService().getDBClient();
    FakeCoreDRemoteInterface core( getHostService() );
    SubscriptionHelperMock helper( getHostService(), core );
    SubscriptionManager manager( helper, dbClient );

    auto subscriptionRecord = Json::parse( R"({
    "_id": "uuid_1",
    "deviceId": "abcd",
    "mappings": [
        {
            "capability": "capability_1",
            "variable": "variable_1",
            "itemId": "itemId_1"
        }
    ]})" );

    SubscriptionUtils::storeSubscriptionToDb( dbClient, subscriptionRecord );

    auto request1 = Json::parse( R"({
    "gatewayName": "gateway1",
    "abstractUUID": "uuid_1",
    "mappings": [{
        "capability": "capability_1",
        "variable": "variable_1",
        "valueType": "string",
        "dictionaryType": false,
        "value": "value_1"
    }]
    })" );

    EXPECT_CALL( helper, getGatewayByName( testing::_ ) ).WillOnce( testing::Return( createGateway( "gateway1" ) ) );
    EXPECT_CALL( helper, sendExecutePluginScript( "HUB:gateway1/scripts/update_abstract_subscription", testing::_ ) )
        .Times( 1 );
    EXPECT_FALSE( hasError( manager.updateSubscription( request1 ) ) );
}

TEST_F( SubscriptionManagerTestSuite, DeleteSubscriptionsWithGateway ) /* 1.8 */ {
    using namespace Resources;

    auto& dbClient = getHostService().getDBClient();
    FakeCoreDRemoteInterface core( getHostService() );
    SubscriptionHelperMock helper( getHostService(), core );
    SubscriptionManager manager( helper, dbClient );

    auto subscriptionRecord = Json::parse( R"({
    "_id": "uuid_1",
    "deviceId": "abcd",
    "mappings": [
        {
            "capability": "capability_1",
            "variable": "variable_1",
            "itemId": "itemId_1"
        }
    ]})" );

    SubscriptionUtils::storeSubscriptionToDb( dbClient, subscriptionRecord );

    auto request1 = Json::parse( R"({
    "gatewayName": "gateway1",
    "abstractUUID": "uuid_1"
    })" );

    EXPECT_CALL( helper, getGatewayByName( testing::_ ) ).WillOnce( testing::Return( createGateway( "gateway1" ) ) );
    EXPECT_CALL( helper, sendExecutePluginScript( "HUB:gateway1/scripts/delete_abstract_subscription", testing::_ ) )
        .Times( 1 );
    EXPECT_FALSE( hasError( manager.deleteSubscription( request1 ) ) );
}

TEST_F( SubscriptionManagerTestSuite, GeofencingCreateSubscriptions ) /* 2.1 */ {
    using namespace Resources;

    auto& dbClient = getHostService().getDBClient();
    FakeCoreDRemoteInterface core( getHostService() );
    SubscriptionHelper helper( getHostService(), core );

    SubscriptionManager manager( helper, dbClient );

    auto request1 = R"({
    "abstractUUID": "geofences_all_uuid_1",
    "mappings": [{
        "capability": "geofences_all",
        "variable": "geofences",
        "valueType": "geofence",
        "dictionaryType": true,
        "value": {}
    }]
    })"_json;

    auto request2 = Json::parse( R"({
    "abstractUUID": "uuid_2",
    "mappings": [{
        "capability": "capability_2",
        "variable": "variable_2",
        "valueType": "bool",
        "dictionaryType": false,
        "value": true
    },
    {
        "capability": "capability_3",
        "variable": "variable_3",
        "valueType": "int",
        "dictionaryType": true,
        "value": { "key": 3 }
    }]
    })" );

    EXPECT_FALSE( hasError( manager.addSubscription( request1 ) ) );
    EXPECT_FALSE( hasError( manager.addSubscription( request2 ) ) );

    ASSERT_EQ( core.m_devices.size(), 2 );
    ASSERT_EQ( core.m_items.size(), 3 );

    auto table = dbClient.findAll( DB::TableNames::Subscriptions, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( table.size(), 2 );

    for( auto& subscription : table ) {
        auto deviceId = json_get_str( subscription, DB::SubscriptionFields::DeviceId );
        EXPECT_TRUE( core.m_devices.count( deviceId ) ) << "Device Id should be present: " << deviceId;

        auto& mappings = subscription.at( DB::SubscriptionFields::Mappings );
        for( auto& mapping : mappings ) {
            auto itemId = json_get_str( mapping, DB::SubscriptionFields::ItemId );
            EXPECT_TRUE( core.m_items.count( itemId ) ) << "Item Id should be present: " << itemId;
        }
    }

    for( auto& subscription : table ) {
        subscription.at( DB::SubscriptionFields::DeviceId ) = "deviceId";
        auto& mappings = subscription.at( DB::SubscriptionFields::Mappings );
        for( auto& mapping : mappings )
            mapping.at( DB::SubscriptionFields::ItemId ) = "itemId";
    }

    auto expectedSubscription1 = R"({
        "_id": "geofences_all_uuid_1",
        "deviceId": "deviceId",
        "mappings": [{
            "capability": "geofences_all",
            "variable": "geofences",
            "itemId": "itemId"
        }]
    })"_json;

    auto& subscription1 = table.front();
    EXPECT_EQ( subscription1.dump(), expectedSubscription1.dump() );

    auto expectedSubscription2 = Json::parse( R"({
        "_id": "uuid_2",
        "deviceId": "deviceId",
        "mappings": [{
            "capability": "capability_2",
            "variable": "variable_2",
            "itemId": "itemId"
        },
        {
            "capability": "capability_3",
            "variable": "variable_3",
            "itemId": "itemId"
        }]
    })" );

    auto& subscription2 = table.back();
    EXPECT_EQ( subscription2.dump(), expectedSubscription2.dump() );

    manager.sendSubscriptionList();
    auto message = getFakeResponseBus().getMessage();

    EXPECT_EQ( message.group, GroupsNames::UI_GROUP );
    EXPECT_EQ( message.msgSubClass, CLOUD_MESSAGE );

    auto& data = message.data;
    LOGGER_INFO( "sendSubscriptionList payload: " + message.data.dump( 4 ) );

    ASSERT_TRUE( !data.empty() );

    auto& requestBody = data.at( "requestBody" );
    ASSERT_TRUE( !requestBody.empty() );

    EXPECT_EQ( json_get_int( requestBody, API::General::Api ), 1 );
    EXPECT_EQ( json_get_str( requestBody, API::General::Method ), CloudMethods::CloudControllerSubscribeAbstracts );

    auto& results = requestBody.at( API::General::Params );
    auto& abstracts = results.at( CloudRPCParams::Abstracts );
    EXPECT_EQ( abstracts.size(), 2 );

    Json responseTypeMap, dispatch;
    EXPECT_TRUE( json_get( results, "response_type", responseTypeMap ) ) << "Should have response_type for geofencing";
    EXPECT_EQ( responseTypeMap.count( "geofencing_dispatch" ), 1 );
    EXPECT_TRUE( json_get( responseTypeMap, "geofencing_dispatch", dispatch ) );

    // structure of response_type
    EXPECT_TRUE( dispatch.is_object() );
    EXPECT_EQ( dispatch.count( "data_path" ), 1 );
    EXPECT_EQ( dispatch.count( "template" ), 1 );
    EXPECT_EQ( dispatch.count( "value_path" ), 1 );
}

Json makeGeofeicingSubscription() {
    return Json::parse( R"({
    "gatewayName": "ezlo_geofencing",
    "abstractUUID": "uuid_1",
    "mappings": [{
        "capability": "geofences_all",
        "variable": "geofences",
        "valueType": "geofence",
        "dictionaryType": true,
        "value": {}
    }]
    })" );
}

TEST_F( SubscriptionManagerTestSuite, GeofencingDispatchOk ) /* 3.1 */ {
    using namespace Resources;

    auto& dbClient = getHostService().getDBClient();
    FakeCoreDRemoteInterface core( getHostService() );
    SubscriptionHelperMock helper( getHostService(), core );
    SubscriptionManager manager( helper, dbClient );

    const Json request1 = makeGeofeicingSubscription();
    // creation of a subscription
    const Json geofencingGateway = createGateway( "ezlo_geofencing" );
    EXPECT_CALL( helper, getGatewayByName( testing::_ ) ).WillOnce( testing::Return( geofencingGateway ) );
    EXPECT_CALL( helper,
                 sendExecutePluginScript( "HUB:ezlo_geofencing/scripts/create_abstract_subscription", testing::_ ) )
        .Times( 1 );
    EXPECT_FALSE( hasError( manager.addSubscription( request1 ) ) );

    // dispatch of subscription data, success
    JsonArray geofenceInfo = JSON_ARRAY_( "abstract_uuid1", 1, 12345678 );
    Json dispatchCommandParams = JSON( "_id"
                                       << "2"
                                       << "value" << JSON_ARRAY_( geofenceInfo ) << "gatewayName"
                                       << "ezlo_geofencing" );

    // dispatch subscription successfully
    {
        EXPECT_CALL( helper, getGatewayByName( testing::_ ) ).WillOnce( testing::Return( geofencingGateway ) );
        EXPECT_CALL( helper, itemBelongsToGateway( testing::_, testing::_ ) ).WillOnce( testing::Return( true ) );
        EXPECT_CALL(
            helper,
            sendExecutePluginScript( "HUB:ezlo_geofencing/scripts/dispatch_abstract_subscription", testing::_ ) )
            .Times( 1 );
        EXPECT_FALSE( hasError( manager.dispatchSubscription( dispatchCommandParams ) ) );
    }
}

TEST_F( SubscriptionManagerTestSuite, GeofencingDispatchErrors ) /* 3.2 */ {
    using namespace Resources;

    auto& dbClient = getHostService().getDBClient();
    FakeCoreDRemoteInterface core( getHostService() );
    SubscriptionHelperMock helper( getHostService(), core );
    SubscriptionManager manager( helper, dbClient );

    const Json request1 = makeGeofeicingSubscription();
    const Json geofencingGateway = createGateway( "ezlo_geofencing" );
    // creation of a subscription
    {
        EXPECT_CALL( helper, getGatewayByName( testing::_ ) ).WillOnce( testing::Return( geofencingGateway ) );
        EXPECT_CALL( helper,
                     sendExecutePluginScript( "HUB:ezlo_geofencing/scripts/create_abstract_subscription", testing::_ ) )
            .Times( 1 );
        EXPECT_FALSE( hasError( manager.addSubscription( request1 ) ) );
    }

    // dispatch of subscription data, success
    JsonArray geofenceInfo = JSON_ARRAY_( "abstract_uuid1", 1, 12345678 );
    Json dispatchCommandParams = JSON( "_id"
                                       << "2"
                                       << "value" << JSON_ARRAY_( geofenceInfo ) << "gatewayName"
                                       << "ezlo_geofencing" );

    // No gateway found
    {
        Json gateway = JSON_EMPTY_OBJ;
        using namespace CloudDevices::Resources::GatewayScripts;

        // The gatewayName field is not  passed
        EXPECT_CALL( helper, getGatewayByName( testing::_ ) ).WillOnce( testing::Return( gateway ) );
        EXPECT_CALL( helper, itemBelongsToGateway( testing::_, testing::_ ) ).Times( 0 );
        EXPECT_CALL(
            helper,
            sendExecutePluginScript( "HUB:ezlo_geofencing/scripts/dispatch_abstract_subscription", testing::_ ) )
            .Times( 0 );
        ASSERT_THROW( manager.dispatchSubscription( dispatchCommandParams ), GatewayNotFoundError )
            << "Should throw if the is no gateway";
    }

    // dispatch of subscription data, no gateway script for dispatch
    {
        Json gatewayWithoutDispatchScript = geofencingGateway;
        using namespace CloudDevices::Resources::GatewayScripts;
        json_remove( gatewayWithoutDispatchScript, ScriptCommand::DispatchAbstractSubscription );

        EXPECT_CALL( helper, getGatewayByName( testing::_ ) )
            .WillOnce( testing::Return( gatewayWithoutDispatchScript ) );
        EXPECT_CALL( helper, itemBelongsToGateway( testing::_, testing::_ ) ).Times( 0 );
        EXPECT_CALL(
            helper,
            sendExecutePluginScript( "HUB:ezlo_geofencing/scripts/dispatch_abstract_subscription", testing::_ ) )
            .Times( 0 );
        ASSERT_THROW( manager.dispatchSubscription( dispatchCommandParams ), BadGatewayError )
            << "Should throw if the gateway has no dispatch command script";
    }

    // Bad subscription parameters: _id, value
    {
        using namespace CloudDevices::Resources::GatewayScripts;

        EXPECT_CALL( helper, getGatewayByName( testing::_ ) ).WillOnce( testing::Return( geofencingGateway ) );
        EXPECT_CALL( helper, itemBelongsToGateway( testing::_, testing::_ ) ).Times( 0 );
        EXPECT_CALL(
            helper,
            sendExecutePluginScript( "HUB:ezlo_geofencing/scripts/dispatch_abstract_subscription", testing::_ ) )
            .Times( 0 );
        Json dispatchCommandWoRequiredField = dispatchCommandParams;
        json_remove( dispatchCommandWoRequiredField, "_id" );

        ASSERT_THROW( manager.dispatchSubscription( dispatchCommandWoRequiredField ), SubscriptionBadParamError )
            << "Required param is missing: _id";

        EXPECT_CALL( helper, getGatewayByName( testing::_ ) ).WillOnce( testing::Return( geofencingGateway ) );
        EXPECT_CALL( helper, itemBelongsToGateway( testing::_, testing::_ ) ).Times( 0 );
        EXPECT_CALL(
            helper,
            sendExecutePluginScript( "HUB:ezlo_geofencing/scripts/dispatch_abstract_subscription", testing::_ ) )
            .Times( 0 );
        dispatchCommandWoRequiredField = dispatchCommandParams;
        json_remove( dispatchCommandWoRequiredField, "value" );
        ASSERT_THROW( manager.dispatchSubscription( dispatchCommandWoRequiredField ), SubscriptionBadParamError )
            << "Required param is missing: value";
    }
}

}  // namespace UT
}  // namespace HubPlatform
