#include "zwave/serialization/zwave_node_json_serializer.h"

#include "zwave/resources/zwave_resources.h"
#include "zwave/serialization/zwave_cmd_class_serialization_visitor.h"
#include "json_utils/json_utils.h"

#include <znet/znet_command_class_visitor.hpp>

#include <znet/znet_node.hpp>
#include <znet/converters/zwave_plus_icon_types_converter.hpp>
#include <znet/converters/generic_types_converter.hpp>
#include <znet/converters/specific_types_converter.hpp>
#include <znet/converters/security_modes_converter.hpp>

namespace HubPlatform {
namespace Zwave {

using namespace Resources::Fields;
using namespace znet;

ZwaveNodeJsonSerializer::ZwaveNodeJsonSerializer( const znet::ZNode& zwaveNode )
: m_zwaveNode( zwaveNode ) {}

Json ZwaveNodeJsonSerializer::serialize() const {
    Json result;
    auto& node = const_cast< znet::ZNode& >( m_zwaveNode );
    result[ Node::Id ] = std::to_string( node.getId() );
    result[ Node::ManufacturerId ] = node.getManufacturerId();
    result[ Node::ProductType ] = node.getProductType();
    result[ Node::ProductId ] = node.getProductId();
    result[ Node::BasicType ] = node.getBasicType();
    result[ Node::Security ] = convertSecurityToToken( node.getSecurity() );
    result[ Node::SecurityClassStarted ] = convertSecurityToToken( node.getSecurityClassStarted() );
    result[ Node::IsReachable ] = node.isReachable() == znet::ReachableState::Reachable;
    result[ Node::IsBatteryPowered ] = node.isBatteryPowered();
    result[ Node::Channels ] = serializeChannels();
    result[ Node::IconType ] = serializeIconType( node.getZwavePlusIconType() );
    result[ Node::IsSmartInclude ] = node.isSmartStart();
    result[ Node::IsRediscoverRequired ] = node.isRediscoverRequired();
    result[ Node::CommandSpan ] = node.getCommandSpan();
    serializeGenricAndSpecificTypes( node.getGenericType(), node.getSpecificType(), result );

    auto wakeUpInterval = node.getWakeUpInterval();
    if( wakeUpInterval ) {
        result[ Node::WakeUpInterval ] = *wakeUpInterval;
    }

    return result;
}

JsonArray ZwaveNodeJsonSerializer::serializeChannels() const {
    JsonArray channels;

    for( uint8_t channelId = 0; channelId != ZNET_CHANNEL_ID_INVALID;
         channelId = m_zwaveNode.getNextChannelId( channelId ) ) {
        channels.push_back(
            JSON( Channel::Id << channelId << Channel::Classes << serializeCommandClasses( channelId ) ) );
    }
    return channels;
}

JsonArray ZwaveNodeJsonSerializer::serializeCommandClasses( std::uint8_t channelId ) const {
    JsonArray commandClasses;

    ZwaveCmdClassSerializationVisitor serializer;
    const auto commandClassCount = m_zwaveNode.getCommandClassCount( channelId );
    for( std::size_t commandClassIdx{ 0 }; commandClassIdx < commandClassCount; ++commandClassIdx ) {
        auto cc = m_zwaveNode.getCommandClassByIndex( channelId, commandClassIdx );
        cc->accept( serializer );
        commandClasses.push_back( serializer.takeResult() );
    }

    return commandClasses;
}

Json ZwaveNodeJsonSerializer::serializeIconType( uint16_t iconType ) const {
    if( !iconType )
        return nullptr;

    auto iconTypeEnum = static_cast< ZwavePlusIconType >( iconType );
    char const* iconName = zwave_plus_icon_types_converter::typeToString( iconTypeEnum );
    if( iconName != nullptr ) {
        return iconName;
    }
    else {
        LOGGER_DEBUG_RAW( "Unknown Zwave+ icon type: " + std::to_string( iconType ) );
        return nullptr;
    }
}

void ZwaveNodeJsonSerializer::serializeGenricAndSpecificTypes( uint8_t genericType,
                                                               uint8_t specificType,
                                                               Json& result ) const {
    auto genericTypeEnum = static_cast< GenericType >( genericType );
    result[ Node::GenericType ] = generic_types_converter::typeToString( genericTypeEnum );
    result[ Node::SpecificType ] = specific_types_converter::typeToString( genericTypeEnum, specificType );
}

JsonArray ZwaveNodeJsonSerializer::convertSecurityToToken( znet::SecurityMode::Class security ) const {
    return JSON_ARRAY( znet::security_modes_converter::typeToString( security ) );
}

}  // namespace Zwave
}  // namespace HubPlatform
