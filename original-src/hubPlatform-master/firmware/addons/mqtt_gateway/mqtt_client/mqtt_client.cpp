#include "mqtt_gateway/mqtt_client/mqtt_client.h"

#include "mqtt_gateway/mqtt_gateway_exception.h"
#include "mqtt_gateway/resources/resources.h"

#include "common_utils/exception_utils.h"
#include "json_utils/json_utils.h"
#include "jsonrpc/jsonrpc.h"
#include "mqtt_client/mqtt_utils.h"

#include <cstdlib>
#include <unistd.h>

namespace HubPlatform {
namespace MQTTGateway {

MQTTClient::MQTTClient( Services::Service& service, MessageCallback callback )
: m_service( service )
, m_callback( std::move( callback ) ) {
    if( !m_callback )
        throw MQTTGatewayError( "Invalid callback was passed to MQTTClient" );
}

void MQTTClient::connect() {
    using namespace MQTTGateway::Resources;
    using namespace HubPlatform::Common;

    try {
        std::string clientId = "mqtt.gateway_" + std::to_string( getpid() );

        auto mqttCallback = [this]( Common::MQTTMessage&& mqttMsg ) {
            m_service.addTask( [this, mqttMsg]() {
                try {
                    auto jsonMsg = parseJson( Common::MQTTUtils::getStrPayload( mqttMsg ) );
                    m_callback( std::move( jsonMsg ) );
                }
                catch( const std::exception& e ) {
                    LOGGER_ERROR_RAW( cc::error( "Failed to process MQTT message: exception: " ) +
                                      cc::note( CommonUtils::whatError( e ) ) );
                }
                catch( ... ) {
                    LOGGER_ERROR_RAW( cc::error( "Failed to process MQTT message: Unknown exception" ) );
                }
            } );
        };

        if( !m_mqttClient.isConnected() ) {
            m_mqttClient.connect( clientId, getBrokerURI(), "", "", std::move( mqttCallback ) );
        }

        m_mqttClient.subscribe( Resources::MQTT::AddonTopic );
        LOGGER_INFO_RAW( cc::info( "MQTT client connected" ) );

        return;
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to setup MQTT connection, exception: " ) +
                          cc::note( CommonUtils::whatError( e ) ) );
    }
    catch( ... ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to setup MQTT connection: unknown exception" ) );
    }
    m_service.createPostponedTask( [this]( Services::PostponedTask& ) { connect(); } )
        ->start( std::chrono::seconds( 10 ), false );
}

const std::string& MQTTClient::getBrokerURI() {
    static const char* envBrokerPort = std::getenv( Resources::MQTT::EnvBrokerPort );
    static const std::string uri =
        ( !envBrokerPort ) ? Resources::MQTT::BrokerURI : "tcp://127.0.0.1:" + std::string( envBrokerPort );
    return uri;
}

void MQTTClient::setUp() {
    connect();
}

void MQTTClient::tearDown() {
    m_mqttClient.disconnect();
}

void MQTTClient::sendMessage( const std::string& topic, const JsonObject& msg ) {
    m_mqttClient.publish( Common::MQTTUtils::createMessage( topic, msg.dump() ) );
}

}  // namespace MQTTGateway
}  // namespace HubPlatform
