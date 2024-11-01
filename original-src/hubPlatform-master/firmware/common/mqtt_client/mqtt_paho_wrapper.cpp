#include "mqtt_client/mqtt_paho_wrapper.h"

#include "mqtt_client/mqtt_client_exception.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace Common {
namespace MQTTPahoWrapper {

const int minReconnectTimoutSec = 5;
const int maxReconnectTimoutSec = 10;

MQTTMessage convertMessage( mqtt::const_message_ptr msg ) {
    try {
        if( !msg )
            throw MQTTClientError( "Failed to convert message: nullptr received" );

        auto& payloadRef = msg->get_payload_ref();
        if( payloadRef.empty() )
            return MQTTMessage{ msg->get_topic(), {} };

        const char* data = static_cast< const char* >( payloadRef.data() );
        size_t payloadLen = sizeof( *payloadRef.data() ) * payloadRef.size();
        return MQTTMessage{ msg->get_topic(), { data, data + payloadLen } };
    }
    catch( const mqtt::exception& e ) {
        throw MQTTClientError( "Failed to convert message: " + e.to_string() );
    }
}

std::unique_ptr< mqtt::async_client > createClient( std::string serverURI, std::string clientID ) {
    try {
        return std::make_unique< mqtt::async_client >( std::move( serverURI ), std::move( clientID ) );
    }
    catch( const mqtt::exception& e ) {
        throw MQTTClientError( "Failed to create client: " + e.to_string() );
    }
}

void setClientCallback( mqtt::async_client& client, mqtt::callback& callback ) {
    try {
        client.set_callback( callback );
    }
    catch( const mqtt::exception& e ) {
        throw MQTTClientError( "Failed to set client callback: " + e.to_string() );
    }
}

void setMessageCallback( mqtt::async_client& client, std::function< void( mqtt::const_message_ptr msg ) > callback ) {
    try {
        client.set_message_callback( std::move( callback ) );
    }
    catch( const mqtt::exception& e ) {
        throw MQTTClientError( "Failed to set message callback: " + e.to_string() );
    }
}

void connect( mqtt::async_client& client,
              std::string username,
              std::string password,
              std::chrono::milliseconds timeout ) {
    try {
        mqtt::connect_options options;
        options.set_clean_session( false );
        options.set_keep_alive_interval( 30 );
        options.set_automatic_reconnect( minReconnectTimoutSec, maxReconnectTimoutSec );
        if( !username.empty() && !password.empty() ) {
            options.set_user_name( std::move( username ) );
            options.set_password( std::move( password ) );
        }

        if( !client.connect( options )->wait_for( timeout ) )
            throw MQTTClientError( "Failed to connect" );
    }
    catch( const mqtt::exception& e ) {
        throw MQTTClientError( "Failed to connect: " + e.to_string() );
    }
}

void disconnect( mqtt::async_client& client, std::chrono::milliseconds timeout ) {
    try {
        if( !client.disconnect()->wait_for( timeout ) )
            throw MQTTClientError( "Failed to disconnect" );
    }
    catch( const mqtt::exception& e ) {
        throw MQTTClientError( "Failed to disconnect: " + e.to_string() );
    }
}

void subscribe( mqtt::async_client& client, const std::string& topic, std::chrono::milliseconds timeout ) {
    try {
        if( !client.subscribe( topic, 0 )->wait_for( timeout ) )
            throw MQTTClientError( "Failed to subscribe" );
    }
    catch( const mqtt::exception& e ) {
        throw MQTTClientError( "Failed to subscribe: " + e.to_string() );
    }
}

void unsubscribe( mqtt::async_client& client, const std::string& topic, std::chrono::milliseconds timeout ) {
    try {
        if( !client.unsubscribe( topic )->wait_for( timeout ) )
            throw MQTTClientError( "Failed to unsubscribe" );
    }
    catch( const mqtt::exception& e ) {
        throw MQTTClientError( "Failed to unsubscribe: " + e.to_string() );
    }
}

void publish( mqtt::async_client& client, const MQTTMessage& message ) {
    try {
        if( !client.publish( message.topic, message.payload.data(), message.payload.size(), 0, false ) )
            throw MQTTClientError( "Failed to publish" );
    }
    catch( const mqtt::exception& e ) {
        throw MQTTClientError( "Failed to publish: " + e.to_string() );
    }
}

bool isConnected( mqtt::async_client& client ) {
    try {
        return client.is_connected();
    }
    catch( const mqtt::exception& e ) {
        throw MQTTClientError( "Failed to check connection: " + e.to_string() );
    }
}

}  // namespace MQTTPahoWrapper
}  // namespace Common
}  // namespace HubPlatform
