#include "common_test_resources/fixtures/core_test_fixture.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "json_utils/json_wrapper.h"
#include "cloud_devices_tests/application.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

class CloudAbstractDeviceTest : public CoreNmaTest {
public:
    void SetUp() override;
    void TearDown() override;
};

void CloudAbstractDeviceTest::SetUp() {
    CoreNmaTest::SetUp();
    clearExistingDevices( __LINE__ );
    callingFile( __FILE__ );
}

void CloudAbstractDeviceTest::TearDown() {
    clearExistingDevices( __LINE__ );
    CoreNmaTest::TearDown();
}

TEST_F( CloudAbstractDeviceTest, addAbstractDevice ) {
    JsonObject broadcast;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "cloud",
                               "134_3_24",
                               "abstract",
                               "abstract",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId =
        addItem( coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    Json subscriptionCreateCommand = R"({
        "method": "hub.cloud.subscription.create",
        "id": "_ID_",
        "params": {
            "abstractUUID": "1234-abcd-...",
            "mappings": [
                {
                    "capability": "testItem1",
                    "variable": "status",
                    "valueType": "int",
                    "dictionaryType": false,
                    "value": 1
                }
           ]
        }
    })"_json;

    Json answer = nma().sendtoHubAndGetAnswer( subscriptionCreateCommand );
    broadcast = waitForBroadcast( "hub.gateway.added", NMA_TEST_MAKE_MESSAGE( "expected gateway.created" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.gateway.added" );
    broadcast = waitForBroadcast( "hub.device.added", NMA_TEST_MAKE_MESSAGE( "expected device.created" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.added" );
    broadcast = waitForBroadcast( "hub.item.added", NMA_TEST_MAKE_MESSAGE( "expected item.created" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.added" );

    coreRemoteInterface.removeItem( itemId );
    broadcast = waitForBroadcast( "hub.item.removed", NMA_TEST_MAKE_MESSAGE( "expected item.removed" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );

    coreRemoteInterface.removeDevice( deviceId );
    broadcast = waitForBroadcast( "hub.device.removed", NMA_TEST_MAKE_MESSAGE( "expected device.removed" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
}

TEST_F( CloudAbstractDeviceTest, createSubscriptionWithTooLongCapabilityName ) {
    JsonObject broadcast;

    Json subscriptionCreateCommand = R"({
        "method": "hub.cloud.subscription.create",
        "id": "_ID_",
        "params": {
        "abstractUUID": "13c50adb-e278-4f70-93c9-a468c0ca7bcf",
        "mappings": [
            {
                "capability": "custom_subscription_eyJ1dWlkIjoiMTNjNTBhZGItZTI3OC00ZjcwLTkzYzktYTQ2OGMwY2E3YmNmIiwibmFtZSI6ImNyZWF0ZUlzc3VlQ2FsbGJhY2siLCJwYXJhbXMiOnsicHJvamVjdF9pZCI6InNkMjEzMjEzIn0sImV2ZW50X25hbWUiOiJpc3N1ZSJ9",
                "variable": "value",
                "valueType": "string",
                "dictionaryType": false,
                "value": ""
            }
                    ]
        }
    })"_json;

    Json answer = nma().sendtoHubAndGetAnswer( subscriptionCreateCommand );

    broadcast = waitForBroadcast( "hub.device.added", NMA_TEST_MAKE_MESSAGE( "expected device.created" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.added" );

    Json error;
    ASSERT_TRUE( json_get( answer, "error", error ) );
    EXPECT_EQ( error[ "code" ], static_cast< int >( Errors::Codes::INVALID_PARAMS ) );
    EXPECT_EQ( error[ "data" ], "invalid.subscription.capability.name" );
}

TEST_F( CloudAbstractDeviceTest, createSubscriptionWithTooLongVariableName ) {
    JsonObject broadcast;

    Json subscriptionCreateCommand = R"({
        "method": "hub.cloud.subscription.create",
        "id": "_ID_",
        "params": {
        "abstractUUID": "13c50adb-e278-4f70-93c9-a468c0ca7bcf",
        "mappings": [
            {
                "capability": "custom_subscription",
                "variable": "value_eyJ1dWlkIjoiMTNjNTBhZGItZTI3OC00ZjcwLTkzYzktYTQ2OGMwY2E3YmNmIiwibmFtZSI6ImNyZWF0ZUlzc3VlQ2FsbGJhY2siLCJwYXJhbXMiOnsicHJvamVjdF9pZCI6InNkMjEzMjEzIn0sImV2ZW50X25hbWUiOiJpc3N1ZSJ9",
                "valueType": "string",
                "dictionaryType": false,
                "value": ""
            }
                    ]
        }
    })"_json;

    Json answer = nma().sendtoHubAndGetAnswer( subscriptionCreateCommand );

    broadcast = waitForBroadcast( "hub.device.added", NMA_TEST_MAKE_MESSAGE( "expected device.created" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.added" );

    Json error;
    ASSERT_TRUE( json_get( answer, "error", error ) );
    EXPECT_EQ( error[ "code" ], static_cast< int >( Errors::Codes::INVALID_PARAMS ) );
    EXPECT_EQ( error[ "data" ], "invalid.subscription.variable.name" );
}

}  // namespace Tests
}  // namespace HubPlatform
