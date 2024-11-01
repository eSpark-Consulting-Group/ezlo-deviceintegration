#include "mqtt_gateway_tests/tests/fixtures/mqtt_gateway_fixture.h"

#include "mqtt_gateway_tests/application.h"
#include "mqtt_gateway_tests/tests/resources/mqtt_gateway_resources.h"

#include "mqtt_client/mqtt_utils.h"
#include "common_utils/cpp14.h"
#include "jsonrpc/message.h"
#include "base_utils/utils.h"

namespace HubPlatform {
namespace Tests {

void MQTTGatewayFixture::SetUp() {
    setupMQTTClient();
    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
}

void MQTTGatewayFixture::TearDown() {}

MQTTGatewayTestsApplication& MQTTGatewayFixture::app() {
    return MQTTGatewayTestsApplication::get();
}

MQTTGatewayFixture::MQTTClient& MQTTGatewayFixture::mqttClient() {
    return *m_mqttClient;
}

void MQTTGatewayFixture::setupMQTTClient() {
    m_mqttClient = std::make_unique< HubPlatform::Common::MQTTClient >();
    std::string clientId = "mqtt_gateway_client_" + std::to_string( getpid() );

    auto mqttCallback = [this]( HubPlatform::Common::MQTTMessage&& msg ) {
        try {
            std::lock_guard< std::mutex > lock( m_mutex );
            auto jsonMsg = parseJson( HubPlatform::Common::MQTTUtils::getStrPayload( msg ) );
            m_mqttMessages.emplace_back( std::move( jsonMsg ) );
        }
        catch( const std::exception& e ) {
            LOGGER_ERROR_RAW( cc::error( "Failed to process MQTT message: exception: " ) + cc::note( e.what() ) );
        }
        catch( ... ) {
            LOGGER_ERROR_RAW( cc::error( "Failed to process MQTT message: Unknown exception" ) );
        }
    };

    std::string brokerURI = "tcp://127.0.0.1:" + app().mqttBrokerPort();
    m_mqttClient->connect( clientId, brokerURI, "", "", std::move( mqttCallback ) );

    auto condition = [this] {
        std::lock_guard< std::mutex > lock( m_mutex );
        return m_mqttClient->isConnected();
    };
    if( !waitForCondition( condition, Resources::MQTTConnectTimeout, std::chrono::milliseconds( 100 ) ) ) {
        throw MQTTGatewayFixtureError( "Failed to connect to MQTT broker" + brokerURI );
    }
}

bool MQTTGatewayFixture::waitForMQTTMessage() {
    return waitForCondition(
        [this] {
            std::lock_guard< std::mutex > lock( m_mutex );
            return !m_mqttMessages.empty();
        },
        Resources::MQTTMessageTimeout,
        std::chrono::milliseconds( 50 ) );
}

JsonObject MQTTGatewayFixture::popMQTTMessage() {
    std::lock_guard< std::mutex > lock( m_mutex );
    if( m_mqttMessages.empty() ) {
        throw MQTTGatewayFixtureError( "Unable to pop MQTT message from empty queue" );
    }
    JsonObject message = std::move( m_mqttMessages.front() );
    m_mqttMessages.pop_front();
    return message;
}

void MQTTGatewayFixture::clearMQTTMessageQueue() {
    std::lock_guard< std::mutex > lock( m_mutex );
    m_mqttMessages.clear();
}

void MQTTGatewayFixture::sendMQTTMessage( const std::string& topic, const JsonObject& msg ) {
    m_mqttClient->publish( topic, msg.dump() );
}

}  // namespace Tests
}  // namespace HubPlatform
