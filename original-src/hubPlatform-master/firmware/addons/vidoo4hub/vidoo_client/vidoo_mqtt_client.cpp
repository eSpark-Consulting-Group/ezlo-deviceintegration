#include "vidoo4hub/vidoo_client/vidoo_mqtt_client.h"

#if __HUB_TYPE_VHUB__

#include "vidoo4hub/vidoo4hub_exception.h"
#include "vidoo4hub/resources/vidoo4hub_resources.h"

#include "json_utils/json_utils.h"
#include "jsonrpc/jsonrpc.h"
#include "common_utils/exception_utils.h"
#include "mqtt_client/mqtt_utils.h"

#include <unistd.h>

namespace HubPlatform {
namespace Vidoo4Hub {

VidooMQTTClient::VidooMQTTClient( Services::Service& service, MessageCallback callback )
: m_service( service )
, m_callback( std::move( callback ) ) {
    if( !m_callback )
        throw Vidoo4HubError( "Invalid callback was passed to VidooMQTTClient" );
}

void VidooMQTTClient::connect() {
    using namespace Vidoo4Hub::Resources;
    using namespace HubPlatform::Vidoo4Hub;
    using namespace HubPlatform::Common;

    try {
        std::string clientId = "vidoo4hub_" + std::to_string( getpid() );

        auto mqttCallback = [this]( Common::MQTTMessage&& mqttMsg ) {
            m_service.addTask( [this, mqttMsg]() {
                try {
                    auto jsonMsg = parseJson( Common::MQTTUtils::getStrPayload( mqttMsg ) );
                    m_callback( jsonMsg );
                }
                catch( const std::exception& e ) {
                    LOGGER_ERROR_RAW( cc::error( "Failed to process MQTT message: " ) +
                                      cc::note( CommonUtils::whatError( e ) ) );
                }
                catch( ... ) {
                    LOGGER_ERROR_RAW( cc::error( "Failed to process MQTT message: Unknown exception" ) );
                }
            } );
        };

        if( !m_mqttClient.isConnected() )
            m_mqttClient.connect( clientId, Resources::MQTT::brokerURI, "", "", std::move( mqttCallback ) );
        m_mqttClient.subscribe( Resources::MQTT::addonTopic );
        LOGGER_INFO_RAW( cc::info( "MQTT client connected" ) );

        LOGGER_DEBUG_RAW( cc::debug( "Sending camera list request" ) );
        sendMessage( prepareMethod( RPC::requestCameraList ) );

        LOGGER_DEBUG_RAW( cc::debug( "Sending hotzone list request" ) );
        sendMessage( prepareMethod( RPC::requestHotzoneList ) );

        LOGGER_DEBUG_RAW( cc::debug( "Sending people list request" ) );
        sendMessage( prepareMethod( RPC::requestPeopleList ) );

        LOGGER_DEBUG_RAW( cc::debug( "Sending object list request" ) );
        sendMessage( prepareMethod( RPC::requestObjectList ) );

        LOGGER_DEBUG_RAW( cc::debug( "Sending qrcode list request" ) );
        sendMessage( prepareMethod( RPC::requestQRCodeList ) );
        return;
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to setup MQTT connection: " ) + cc::note( CommonUtils::whatError( e ) ) );
    }
    catch( ... ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to setup MQTT connection: unknown exception" ) );
    }
    m_service.createPostponedTask( [this]( Services::PostponedTask& ) { connect(); } )
        ->start( std::chrono::seconds( 10 ), false );
}

void VidooMQTTClient::setUp() {
    connect();
}

void VidooMQTTClient::tearDown() {
    m_mqttClient.disconnect();
}

void VidooMQTTClient::sendMessage( const JsonObject& msg ) {
    using namespace Vidoo4Hub::Resources;
    m_mqttClient.publish( Common::MQTTUtils::createMessage( MQTT::vidooTopic, msg.dump() ) );
}

}  // namespace Vidoo4Hub
}  // namespace HubPlatform

#endif  // __HUB_TYPE_VHUB__
