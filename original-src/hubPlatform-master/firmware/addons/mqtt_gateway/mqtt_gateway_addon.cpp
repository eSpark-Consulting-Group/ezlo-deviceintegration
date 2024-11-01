#include "mqtt_gateway/mqtt_gateway_addon.h"

#include "mqtt_gateway/version/mqtt_gateway_addon_version.h"
#include "mqtt_gateway/resources/resources.h"
#include "mqtt_gateway/resources/message_subclasses.h"
#include "mqtt_gateway/mqtt_gateway_exception.h"
#include "mqtt_gateway/mqtt_client/mqtt_client.h"
#include "mqtt_gateway/spread_bus_wrapper/spread_bus_wrapper.h"
#include "mqtt_gateway/utils/utils.h"

#include "mqtt_gateway/request_handlers/hub_device_settings_list.h"

#include "service_interfaces/bus_wrapper.h"

#include "common_utils/backup/backup_helper.h"
#include "common_utils/backup/restore_helper.h"
#include "common_utils/lock_helper.h"
#include "common_utils/cpp14.h"
#include "common_utils/exception_utils.h"
#include "jsonrpc/jsonrpc.h"
#include "json_utils/json_utils.h"
#include "json_utils/json_approximate_size.h"

#include "system_utils/environment.h"

#include <functional>
#include <cassert>

namespace HubPlatform {

namespace {
std::string truncateMessageForLog( const Json& message ) {
    if( jsonApproximateSize( message ) <= 2048 )
        return cc::j( message );
    std::string str = message.dump();
    str.resize( 2048 );
    return cc::note( str + "..." );
}
}  // namespace

const char* const MQTTGatewayAddonName = "mqtt_gateway";
const char* const MQTTGatewayAddonBusName = "addon.mqtt_gateway";

MQTTGatewayAddon::MQTTGatewayAddon()
: Addons::Addon( MQTTGatewayAddonName,
                 std::make_unique< Services::BusWrapper >(
                     Services::BusWrapper::BusTypeTag< HubPlatform::MQTTGateway::SpreadBusWrapper >{} ),
                 nullptr ) {}

CommonUtils::AddonVersion MQTTGatewayAddon::getVersion() const {
    return Addons::MQTTGateway::getVersion();
}

MQTTGatewayAddon::~MQTTGatewayAddon() = default;

void MQTTGatewayAddon::handleMessageFromBus( const Services::Message& message, const std::string& sender ) {
    switch( message.msgClass ) {
    case LOG_CMD:
        handleLogCommand( message, sender );
        break;
    default:
        Services::Service::handleMessageFromBus( message, sender );
        break;
    }
}

void MQTTGatewayAddon::setUp() {
    using namespace std::placeholders;

    Addon::setUp();

    join( GroupsNames::BROADCAST_GROUP );
    join( GroupsNames::CAMERAS_BROADCASTS );
    join( GroupsNames::DEVICE_GROUP );

    m_registeredClients = MQTTGateway::Utils::readClientsFromDB( getDBClient() );
    for( auto& name : m_registeredClients )
        join( name );

    // NOTE: GCC 4.8 has bug that prevents usage of std::bind with Json&&
    // see: https://github.com/nlohmann/json/issues/2147
    m_mqttClient = std::make_unique< MQTTGateway::MQTTClient >(
        *this, [this]( JsonObject&& message ) { handleMQTTMessage( std::move( message ) ); } );
    m_mqttClient->setUp();

    auto* bus = dynamic_cast< MQTTGateway::SpreadBusWrapper* >( getBus() );
    bus->setForwardCallback( std::bind( &MQTTGatewayAddon::forwardToMQTT, this, _1 ) );
}

void MQTTGatewayAddon::tearDown() {
    m_mqttClient->tearDown();
    Addon::tearDown();
}

JsonObject MQTTGatewayAddon::handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) {
    switch( requestType ) {
    case GLOBAL_LOCK: {
        auto& lockHelper = CommonUtils::LockHelper::global();
        lockHelper.tryLock( request );
        return lockHelper.getResponse();
    }
    case GLOBAL_UNLOCK: {
        auto& lockHelper = CommonUtils::LockHelper::global();
        lockHelper.unlock( request );
        return lockHelper.getResponse();
    }
    default:
        return Service::handleRequestFromBus( request, requestType );
    }

    return JSON_EMPTY_OBJ;
}

void MQTTGatewayAddon::handleMQTTMessage( JsonObject&& message ) {
    namespace Fields = MQTTGateway::Resources::MessageFields;
    namespace MQTTMessageClasses = MQTTGateway::Resources::MQTTMessageClasses;

    try {
        if( !json_contains( message, Fields::Result ) ) {
            LOGGER_WARN_RAW( cc::warn( "Invalid MQTT message receved: missing \"result\" field: " ) +
                             cc::j( message ) );
            return;
        }

        const auto messageClass = json_get_str( message, Fields::MessageClass );

        if( messageClass == MQTTMessageClasses::Request ) {
            const auto group = json_get_str( message, Fields::MessageGroup );
            if( group == MQTTGatewayAddonBusName ) {
                LOGGER_DEBUG_RAW( cc::debug( "Receive MQTT request: " ) + truncateMessageForLog( message ) );
                auto messageSubclass = message.at( Fields::MessageSubclass );
                auto& result = message.at( Fields::Result );
                const auto sender = json_get_str( result, Fields::Sender );
                const auto messageId = json_get_str( result, Fields::UUID );
                auto& params = result.at( Fields::RequestBody );

                if( sender.empty() ) {
                    LOGGER_WARN_RAW( cc::warn( "Ignoring request with empty sender" ) );
                    return;
                }

                auto responseBody = handleMQTTRequest( sender, messageId, messageSubclass, params );
                auto response = MQTTGateway::Utils::buildMQTTResponse( sender, messageId, std::move( responseBody ) );
                m_mqttClient->sendMessage( sender, response );
                return;
            }
        }

        forwardToSpread( std::move( message ) );
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to handle MQTT message: exception: " + CommonUtils::whatError( e ) ) );
    }
    catch( ... ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to handle MQTT message: unknown exception" ) );
    }
}

Json MQTTGatewayAddon::handleMQTTRequest( const std::string& sender,
                                          const std::string& messageId,
                                          const std::string& messageSubclass,
                                          Json& params ) {
    namespace Subclass = HubPlatform::MQTTGateway::MessageSubClasses;
    try {
        if( messageSubclass == Subclass::MQTT_CLIENT_REGISTER ) {
            return handleClientRegister( params );
        }
        else if( messageSubclass == Subclass::MQTT_CLIENT_UNREGISTER ) {
            return handleClientUnregister( params );
        }

        if( !m_registeredClients.count( sender ) ) {
            LOGGER_WARN_RAW( cc::warn( "Ignoring request from unregistered client: " ) + cc::note( sender ) );
            return MQTTGateway::Utils::buildErrorResponseBody( Errors::MQTT_CLIENT_NOT_REGISTERED );
        }

        if( messageSubclass == Subclass::HUB_DEVICE_SETTINGS_LIST ) {
            return MQTTGateway::RequestHandlers::HubDeviceSettingsList( *this ).process( params );
        }

        return MQTTGateway::Utils::buildErrorResponseBody( Errors::INTERNAL_OPERATION_ERROR, "Unhandled request" );
    }
    catch( const std::exception& e ) {
        auto error = "Failed to handle MQTT request: " + CommonUtils::whatError( e );
        LOGGER_ERROR_RAW( cc::warn( error ) );
        return MQTTGateway::Utils::buildErrorResponseBody( Errors::INTERNAL_OPERATION_ERROR, error );
    }
}

Json MQTTGatewayAddon::handleClientRegister( const Json& params ) {
    try {
        std::string name = params.at( "name" );

        const auto& maxNameSize = MQTTGateway::Resources::MQTT::MaxTopicNameSize;
        if( name.empty() || name.find( "mqtt." ) != 0 || name.size() >= maxNameSize ) {
            LOGGER_WARN_RAW( cc::warn( "Failed to register MQTT client. Invalid client name: " ) + cc::note( name ) );
            return MQTTGateway::Utils::buildErrorResponseBody( Errors::MQTT_INVALID_CLIENT_NAME );
        }

        LOGGER_INFO_RAW( cc::warn( "Register MQTT client: " ) + cc::note( name ) );
        join( name.c_str() );
        m_registeredClients.emplace( name );
        MQTTGateway::Utils::addClientToDB( getDBClient(), name );

        return JSON_EMPTY_OBJ;
    }
    catch( const std::exception& e ) {
        auto error = "Failed to register MQTT client: " + CommonUtils::whatError( e );
        LOGGER_ERROR_RAW( cc::warn( error ) );
        return MQTTGateway::Utils::buildErrorResponseBody( Errors::INTERNAL_OPERATION_ERROR, error );
    }
}

Json MQTTGatewayAddon::handleClientUnregister( const Json& params ) {
    try {
        std::string name = params.at( "name" );
        if( name.empty() || name.find( "mqtt." ) != 0 ) {
            LOGGER_WARN_RAW( cc::warn( "Failed to unregister MQTT client. Invalid client name: " ) + cc::note( name ) );
            return MQTTGateway::Utils::buildErrorResponseBody( Errors::MQTT_INVALID_CLIENT_NAME );
        }

        LOGGER_INFO_RAW( cc::warn( "Unregister MQTT client: " ) + cc::note( name ) );
        m_registeredClients.erase( name );
        MQTTGateway::Utils::removeClientFromDB( getDBClient(), name );
        return JSON_EMPTY_OBJ;
    }
    catch( const std::exception& e ) {
        auto error = "Failed to unregister MQTT client: " + CommonUtils::whatError( e );
        LOGGER_ERROR_RAW( cc::warn( error ) );
        return MQTTGateway::Utils::buildErrorResponseBody( Errors::INTERNAL_OPERATION_ERROR, error );
    }
}

void MQTTGatewayAddon::forwardToSpread( Json mqttMessage ) {
    namespace Fields = MQTTGateway::Resources::MessageFields;
    namespace Class = MQTTGateway::Resources::MQTTMessageClasses;
    using namespace MQTTGateway::Utils;

    try {
        auto msgGroup = json_get_str( mqttMessage, Fields::MessageGroup );
        auto msgClass = json_get_str( mqttMessage, Fields::MessageClass );
        auto msgSubclass = json_get_str( mqttMessage, Fields::MessageSubclass );
        auto msgId = json_get_str( mqttMessage.at( Fields::Result ), Fields::UUID );

        LOGGER_INFO_RAW( cc::info( "FWD(SPREAD) <<< (MQTT) <<< message id: " ) + cc::note( msgId ) +
                         cc::info( " <<< class: " ) + cc::note( msgClass ) + cc::info( " <<< subclass: " ) +
                         cc::note( msgSubclass ) );

        LOGGER_INFO_RAW( cc::info( "FWD(SPREAD) <<< (MQTT) <<< " ) + truncateMessageForLog( mqttMessage ) );

        json_remove( mqttMessage, Fields::MessageGroup );
        json_remove( mqttMessage, Fields::MessageClass );

        if( msgClass == Class::Request ) {
            std::string sender = mqttMessage.at( Fields::Result ).at( Fields::Sender );

            if( !m_registeredClients.count( sender ) ) {
                LOGGER_WARN_RAW(
                    cc::warn( "Failed to forward MQTT request to spread. Client name is not registered: " ) +
                    cc::note( sender ) );

                std::string uuid = mqttMessage.at( Fields::Result ).at( Fields::UUID );
                auto response = buildMQTTErrorResponse( sender, uuid, Errors::MQTT_CLIENT_NOT_REGISTERED );
                m_mqttClient->sendMessage( sender, response );
                return;
            }

            auto mqttMessageBytes = mqttMessage.dump();

            if( mqttMessageBytes.size() >= MQTTGateway::Resources::MQTT::MaxMessageSize ) {
                std::string uuid = mqttMessage.at( Fields::Result ).at( Fields::UUID );
                auto response = buildMQTTErrorResponse( sender, uuid, Errors::MQTT_MESSAGE_TOO_LONG );
                m_mqttClient->sendMessage( sender, response );
                return;
            }

            getBus()->multicast( msgGroup.c_str(), MessageClasses::REQUEST, mqttMessageBytes );
        }
        else if( msgClass == Class::Response ) {
            getBus()->multicast( msgGroup.c_str(), MessageClasses::RESPONSE, mqttMessage.dump() );
        }
        else if( msgClass == Class::Event ) {
            getBus()->multicast( msgGroup.c_str(), MessageClasses::EVENT, mqttMessage.dump() );
        }
        else {
            LOGGER_WARN_RAW( cc::warn( "Failed to forward message to spread group: " ) + cc::note( msgGroup ) +
                             cc::warn( " : unknown message class: " ) + cc::note( msgClass ) );
        }
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to forward MQTT message to spread: exception: " ) +
                          cc::note( CommonUtils::whatError( e ) ) );
    }
}

bool MQTTGatewayAddon::forwardToMQTT( const bus_rx_data_t* rxData ) {
    namespace Fields = MQTTGateway::Resources::MessageFields;
    namespace Class = MQTTGateway::Resources::MQTTMessageClasses;

    try {
        if( !rxData || rxData->num_groups <= 0 || false == getBus()->isRegularMessage( rxData->service_type ) ) {
            return false;
        }

        auto group = std::string( rxData->target_groups[ 0 ] );
        if( group == getQualifiedName() )
            return false;

        auto data = JsonObject::parse( std::string( rxData->msg, rxData->msg_size ) );
        auto msgClass = rxData->msg_type;

        data[ Fields::MessageGroup ] = group;

        if( msgClass == MessageClasses::REQUEST ) {
            data[ Fields::MessageClass ] = Class::Request;
        }
        else if( msgClass == MessageClasses::RESPONSE ) {
            data[ Fields::MessageClass ] = Class::Response;
        }
        else if( msgClass == MessageClasses::EVENT ) {
            data[ Fields::MessageClass ] = Class::Event;
        }
        else {
            return false;
        }

        auto msgClassStr = json_get_str( data, Fields::MessageClass );
        auto msgSubclassStr = json_get_str( data, Fields::MessageSubclass );
        auto msgId = json_get_str( data.at( Fields::Result ), Fields::UUID );

        LOGGER_INFO_RAW( cc::info( "FWD(SPREAD) >>> (MQTT) >>> message id: " ) + cc::note( msgId ) +
                         cc::info( " >>> class: " ) + cc::note( msgClassStr ) + cc::info( " >>> subclass: " ) +
                         cc::note( msgSubclassStr ) );
        LOGGER_DEBUG_RAW( cc::debug( "FWD(SPREAD) >>> (MQTT) >>> " ) + truncateMessageForLog( data ) );
        m_mqttClient->sendMessage( group, data );
        return true;
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to forward MQTT message: exception: " ) +
                          cc::note( CommonUtils::whatError( e ) ) );
    }
    return false;
}

}  // namespace HubPlatform
