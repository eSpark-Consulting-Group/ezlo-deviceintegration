#include "mqtt_client/mqtt_client.h"

#include "mqtt_client/mqtt_utils.h"
#include "mqtt_client/mqtt_paho_wrapper.h"
#include "mqtt_client/mqtt_client_exception.h"
#include "mqtt_client/mqtt_callback.h"

#include "mqtt/async_client.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "common_utils/cpp14.h"

#include <iostream>

namespace HubPlatform {
namespace Common {
namespace {
constexpr std::chrono::seconds connectTimeout( 5 );
constexpr std::chrono::seconds disconnectTimeout( 5 );
constexpr std::chrono::seconds subscribeTimeout( 5 );
constexpr std::chrono::seconds unsubscribeTimeout( 5 );
}  // namespace

MQTTClient::MQTTClient() = default;

MQTTClient::~MQTTClient() {
    try {
        disconnect();
        if( m_subscribeThread.joinable() )
            m_subscribeThread.join();
    }
    catch( const MQTTClientError& e ) {
        Log::warn( cc::warn( "MQTTClient: failed to disconnect: " ) + cc::note( e.what() ) );
    }
    catch( const std::exception& e ) {
        Log::warn( cc::warn( "MQTTClient: Failed to disconnect: " ) + cc::note( e.what() ) );
    }
    catch( ... ) {
        Log::warn( "MQTTClient: Failed to disconnect: unknown exception" );
    }
}

void MQTTClient::connect( std::string clientID,
                          std::string serverURI,
                          std::string username,
                          std::string password,
                          MQTTMessageCallback messageCallback ) {
    if( isConnected() )
        disconnect();

    m_client = MQTTPahoWrapper::createClient( std::move( serverURI ), std::move( clientID ) );

    m_callback = std::make_unique< MQTTCallback::MQTTCallback >(
        std::bind( &MQTTClient::onConnected, this, std::placeholders::_1 ),
        std::bind( &MQTTClient::onConnectionLost, this, std::placeholders::_1 ) );
    MQTTPahoWrapper::setClientCallback( *m_client, *m_callback );

    m_messageCallback = std::move( messageCallback );
    MQTTPahoWrapper::setMessageCallback( *m_client, [this]( mqtt::const_message_ptr msg ) {
        try {
            if( !msg ) {
                Log::warn( cc::warn( "MQTTClient: Failed to process message: nullptr received" ) );
                return;
            }
            m_messageCallback( MQTTPahoWrapper::convertMessage( msg ) );
        }
        catch( const std::exception& e ) {
            Log::warn( cc::warn( "MQTTClient: Failed to process message: " ) + cc::note( e.what() ) );
        }
        catch( ... ) {
            Log::warn( cc::warn( "MQTTClient: Failed to process message: " ) + cc::note( "unknown exception" ) );
        }
    } );

    MQTTPahoWrapper::connect( *m_client, std::move( username ), std::move( password ), connectTimeout );
    m_isConnected = true;
}

void MQTTClient::disconnect() {
    if( m_client && isConnected() ) {
        MQTTPahoWrapper::disconnect( *m_client, disconnectTimeout );
    }
    m_isConnected = false;
}

void MQTTClient::subscribe( const std::string& topic ) {
    if( !isConnected() )
        throw MQTTClientError( "Failed to subscribe: client is not connected" );
    MQTTPahoWrapper::subscribe( *m_client, topic, subscribeTimeout );
    std::lock_guard< std::mutex > lock( m_subscriptionsMutex );
    m_subscriptions.emplace( topic );
}

void MQTTClient::unsubscribe( const std::string& topic ) {
    if( isConnected() ) {
        MQTTPahoWrapper::unsubscribe( *m_client, topic, unsubscribeTimeout );
        std::lock_guard< std::mutex > lock( m_subscriptionsMutex );
        auto it = m_subscriptions.find( topic );
        if( it != m_subscriptions.cend() )
            m_subscriptions.erase( it );
    }
}

void MQTTClient::publish( const MQTTMessage& message ) {
    if( !isConnected() )
        throw MQTTClientError( "Failed to publish: client is not connected" );
    MQTTPahoWrapper::publish( *m_client, message );
}

void MQTTClient::publish( const std::string& topic, const std::string& payload ) {
    publish( MQTTUtils::createMessage( topic, payload ) );
}

bool MQTTClient::isConnected() {
    return m_client && m_isConnected;
}

void MQTTClient::onConnected( const std::string& reason ) {
    m_isConnected = true;
    Log::debug( cc::debug( "MQTT client connected" ) );
    resubscribe();
}

void MQTTClient::onConnectionLost( const std::string& reason ) {
    m_isConnected = false;
    Log::warn( cc::warn( "MQTT Client connection lost: reason: " + reason ) );
}

void MQTTClient::resubscribe() {
    if( !isConnected() && m_subscriptions.empty() )
        return;

    if( m_subscribeThread.joinable() )
        m_subscribeThread.join();

    m_subscribeThread = std::thread( [this]() {
        std::lock_guard< std::mutex > lock( m_subscriptionsMutex );
        for( auto& subscription : m_subscriptions ) {
            try {
                LOGGER_DEBUG_RAW( cc::debug( "MQTT Client resubscribe to: " + subscription ) );
                MQTTPahoWrapper::subscribe( *m_client, subscription, subscribeTimeout );
            }
            catch( const std::exception& e ) {
                std::string error( e.what() );
                std::string errorType( typeid( e ).name() );
                LOGGER_WARN_RAW( cc::warn( "MQTT Client failed to resubscribe to: " + subscription + ": " + "(" +
                                           errorType + "): " + cc::note( error ) ) );
            }
            catch( ... ) {
                LOGGER_WARN_RAW(
                    cc::warn( "MQTT Client failed to resubscribe to: " + subscription + ": unknown error" ) );
            }
        }
    } );
}

}  // namespace Common
}  // namespace HubPlatform
