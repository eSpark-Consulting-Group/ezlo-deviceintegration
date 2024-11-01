#include "mqtt_gateway_tests/tests/fixtures/mqtt_gateway_fixture.h"

#include "mqtt_gateway_tests/application.h"
#include "mqtt_gateway_tests/tests/resources/mqtt_gateway_resources.h"

#include "json_utils/json_wrapper.h"
#include "common_utils/cpp14.h"
#include "test_utils/test_utils.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

class MQTTGatewayTestSuite : public MQTTGatewayFixture {
public:
    void SetUp() override;
    void TearDown() override;

    const std::string& spreadEndpoint() {
        return m_spreadEndpoint;
    }
    const std::string& mqttEndpoint() {
        return m_mqttEndpoint;
    }

private:
    const std::string m_spreadEndpoint = "test_spread_service";
    const std::string m_mqttEndpoint = "mqtt.test_service";
};

void MQTTGatewayTestSuite::SetUp() {
    MQTTGatewayFixture::SetUp();

    app().join( spreadEndpoint() );
    app().join( GroupsNames::BROADCAST_GROUP );

    mqttClient().subscribe( mqttEndpoint() );
    mqttClient().subscribe( GroupsNames::BROADCAST_GROUP );

    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
}

void MQTTGatewayTestSuite::TearDown() {
    MQTTGatewayFixture::TearDown();
}

TEST_F( MQTTGatewayTestSuite, GatewayTest ) {
    {  // Request from unregistered client
        const std::string messageUUID = app().generateMessageUUID();

        auto request = replaceTokens( Resources::RPC::MQTTRequest,
                                      { { "__MSG_GROUP__", spreadEndpoint() },
                                        { "__MSG_SUBCLASS__", "hub.status.get" },
                                        { "__UUID__", messageUUID },
                                        { "__SENDER__", mqttEndpoint() },
                                        { "__REQUEST_BODY__", "{}" } } );

        ASSERT_NO_THROW( sendMQTTMessage( Resources::MQTTGatewayTopic, request ) );

        ASSERT_TRUE( waitForMQTTMessage() );
        auto response = popMQTTMessage();

        JsonObject expectedError;
        setError( expectedError, Errors::Error::MQTT_CLIENT_NOT_REGISTERED );

        auto expectedResponse = replaceTokens( Resources::RPC::MQTTResponse,
                                               { { "__MSG_GROUP__", mqttEndpoint() },
                                                 { "__UUID__", messageUUID },
                                                 { "__RESPONSE_BODY__", expectedError.dump() } } );

        EXPECT_EQ( response, expectedResponse );
    }

    {  // Register client request with invalid name
        const std::string messageUUID = app().generateMessageUUID();

        auto request = replaceTokens( Resources::RPC::MQTTRequest,
                                      { { "__MSG_GROUP__", Resources::MQTTGatewayAddonName },
                                        { "__MSG_SUBCLASS__", "mqtt.client.register" },
                                        { "__UUID__", messageUUID },
                                        { "__SENDER__", mqttEndpoint() },
                                        { "__REQUEST_BODY__",
                                          JSON( "name"
                                                << "invalid_name" )
                                              .dump() } } );

        ASSERT_NO_THROW( sendMQTTMessage( Resources::MQTTGatewayTopic, request ) );

        ASSERT_TRUE( waitForMQTTMessage() );
        auto response = popMQTTMessage();

        JsonObject expectedError;
        setError( expectedError, Errors::Error::MQTT_INVALID_CLIENT_NAME );

        auto expectedResponse = replaceTokens( Resources::RPC::MQTTResponse,
                                               { { "__MSG_GROUP__", mqttEndpoint() },
                                                 { "__UUID__", messageUUID },
                                                 { "__RESPONSE_BODY__", expectedError.dump() } } );

        EXPECT_EQ( response, expectedResponse );
    }

    {  // Register client request with too long name
        const std::string messageUUID = app().generateMessageUUID();

        auto request = replaceTokens( Resources::RPC::MQTTRequest,
                                      { { "__MSG_GROUP__", Resources::MQTTGatewayAddonName },
                                        { "__MSG_SUBCLASS__", "mqtt.client.register" },
                                        { "__UUID__", messageUUID },
                                        { "__SENDER__", mqttEndpoint() },
                                        { "__REQUEST_BODY__",
                                          JSON( "name"
                                                << "mqtt.too_loooooooooooooooooooooooooooong_name" )
                                              .dump() } } );

        ASSERT_NO_THROW( sendMQTTMessage( Resources::MQTTGatewayTopic, request ) );

        ASSERT_TRUE( waitForMQTTMessage() );
        auto response = popMQTTMessage();

        JsonObject expectedError;
        setError( expectedError, Errors::Error::MQTT_INVALID_CLIENT_NAME );

        auto expectedResponse = replaceTokens( Resources::RPC::MQTTResponse,
                                               { { "__MSG_GROUP__", mqttEndpoint() },
                                                 { "__UUID__", messageUUID },
                                                 { "__RESPONSE_BODY__", expectedError.dump() } } );

        EXPECT_EQ( response, expectedResponse );
    }

    {  // Register client request
        const std::string messageUUID = app().generateMessageUUID();

        auto request = replaceTokens( Resources::RPC::MQTTRequest,
                                      { { "__MSG_GROUP__", Resources::MQTTGatewayAddonName },
                                        { "__MSG_SUBCLASS__", "mqtt.client.register" },
                                        { "__UUID__", messageUUID },
                                        { "__SENDER__", mqttEndpoint() },
                                        { "__REQUEST_BODY__", JSON( "name" << mqttEndpoint() ).dump() } } );

        ASSERT_NO_THROW( sendMQTTMessage( Resources::MQTTGatewayTopic, request ) );

        ASSERT_TRUE( waitForMQTTMessage() );
        auto response = popMQTTMessage();

        auto expectedResponse = replaceTokens(
            Resources::RPC::MQTTResponse,
            { { "__MSG_GROUP__", mqttEndpoint() }, { "__UUID__", messageUUID }, { "__RESPONSE_BODY__", "{}" } } );

        EXPECT_EQ( response, expectedResponse );
    }

    {  // Request from mqtt client to spread service
        const std::string messageUUID = app().generateMessageUUID();
        const std::string params = JSON( "test" << messageUUID ).dump();

        // Echo message
        app().setRequestHandler( MessageSubClasses::GET_HUB_STATUS,
                                 []( const JsonObject& requestParams ) { return requestParams; } );

        auto request = replaceTokens( Resources::RPC::MQTTRequest,
                                      { { "__MSG_GROUP__", spreadEndpoint() },
                                        { "__MSG_SUBCLASS__", "hub.status.get" },
                                        { "__UUID__", messageUUID },
                                        { "__SENDER__", mqttEndpoint() },
                                        { "__REQUEST_BODY__", params } } );

        sendMQTTMessage( Resources::MQTTGatewayTopic, request );

        ASSERT_TRUE( waitForMQTTMessage() );
        auto response = popMQTTMessage();

        auto expectedResponse = replaceTokens(
            Resources::RPC::MQTTResponse,
            { { "__MSG_GROUP__", mqttEndpoint() }, { "__UUID__", messageUUID }, { "__RESPONSE_BODY__", params } } );

        EXPECT_EQ( response, expectedResponse );
    }

    {  // Request from spread service to mqtt client
        const std::string messageUUID = app().generateMessageUUID();
        const std::string params = JSON( "test" << messageUUID ).dump();

        std::promise< JsonObject > responsePromise;
        auto responseFuture = responsePromise.get_future();

        auto responseCallback = [&responsePromise]( const JsonObject& message ) {
            responsePromise.set_value( message );
        };

        app().sendAsyncRequest( mqttEndpoint(),
                                params,
                                MessageSubClasses::GET_HUB_STATUS,
                                std::move( responseCallback ),
                                std::chrono::seconds( 10 ) );

        ASSERT_TRUE( waitForMQTTMessage() );

        {  // echo the request
            auto request = popMQTTMessage();

            auto& requestResult = request.at( "result" );
            auto requestUUID = json_get_str( requestResult, "uuid" );
            auto requestSender = json_get_str( requestResult, "sender" );
            auto requestParams = requestResult.at( "requestBody" );

            auto mqttResponse = replaceTokens( Resources::RPC::MQTTResponse,
                                               { { "__MSG_GROUP__", requestSender },
                                                 { "__UUID__", requestUUID },
                                                 { "__RESPONSE_BODY__", requestParams.dump() } } );

            sendMQTTMessage( Resources::MQTTGatewayTopic, mqttResponse );
        }

        auto status = responseFuture.wait_for( std::chrono::seconds( 15 ) );
        ASSERT_EQ( status, std::future_status::ready );

        auto response = responseFuture.get();
        EXPECT_EQ( response, params );
    }

    {  // Broadcast from mqtt client to spread service
        const std::string messageUUID = app().generateMessageUUID();
        auto params = JSON( "test" << messageUUID );

        auto srcBroadcast = replaceTokens( Resources::RPC::MQTTBroadcast,
                                           { { "__MSG_GROUP__", GroupsNames::BROADCAST_GROUP },
                                             { "__MSG_SUBCLASS__", "DeviceAdded" },
                                             { "__RESULT__", params.dump() } } );

        std::promise< JsonObject > eventPromise;
        auto eventFuture = eventPromise.get_future();

        app().setEventHandler( MessageSubClasses::DEVICE_ADDED,
                               [&eventPromise]( const JsonObject& message ) { eventPromise.set_value( message ); } );

        sendMQTTMessage( Resources::MQTTGatewayTopic, srcBroadcast );

        auto status = eventFuture.wait_for( std::chrono::seconds( 15 ) );
        ASSERT_EQ( status, std::future_status::ready );

        auto broadcastParams = eventFuture.get();
        EXPECT_EQ( broadcastParams, params );

        ASSERT_TRUE( waitForMQTTMessage() )
            << "Spread will forward broadcast back to test service and hence to MQTT clients";
        clearMQTTMessageQueue();
    }

    {  // Broadcast from spread service to mqtt client
        const std::string messageUUID = app().generateMessageUUID();
        auto params = JSON( "test" << messageUUID );

        app().sendBroadcast( MessageSubClasses::HUB_ITEM_ADDED, params, GroupsNames::BROADCAST_GROUP );

        ASSERT_TRUE( waitForMQTTMessage() );
        auto broadcast = popMQTTMessage();

        auto expectedBroadcast = replaceTokens( Resources::RPC::MQTTBroadcast,
                                                { { "__MSG_GROUP__", GroupsNames::BROADCAST_GROUP },
                                                  { "__MSG_SUBCLASS__", "hub.item.added" },
                                                  { "__RESULT__", params.dump() } } );

        EXPECT_EQ( broadcast, expectedBroadcast );
    }
}

}  // namespace Tests
}  // namespace HubPlatform
