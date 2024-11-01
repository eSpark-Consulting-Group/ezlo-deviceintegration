#include "zwave/zwave_client.h"

#include "zwave/common/zwave_exceptions.h"
#include "zwave/zwave_addon.h"
#include "zwave/resources/zwave_resources.h"
#include "zwave/common/auth_mode_utils.h"
#include "zwave/zwave_node_manager.h"
#include "zwave/provisioning/provisioning_list_provider.h"
#include "zwave/provisioning/provisioning_list_utils.h"
#include "zwave/serialization/zwave_node_json_serializer.h"
#include "zwave/serialization/zwave_cmd_class_serialization_visitor.h"
#include "common_utils/firmware_info_helper.h"

#include "jsonrpc/jsonrpc.h"

#include <znet/znet_alarm_silence.hpp>
#include <znet/znet_alarm_sensor.hpp>

#include <znet/cc_0x32_meter.hpp>
#include <znet/cc_0x86_version.hpp>
#include <znet/cc_anti_theft_base.hpp>

#include <znet/converters/door_lock.hpp>
#include <znet/converters/value_change_command_converter.hpp>
#include <znet/converters/manufacturer_names_converter.hpp>
#include <znet/converters/manufacturer_names_converter.hpp>
#include <znet/converters/thermostat_setpoint_converter.hpp>
#include <znet/converters/thermostat_setback_converter.hpp>
#include <znet/converters/user_code.hpp>
#include <znet/converters/color_switch_converter.hpp>
#include <znet/converters/schedule_converter.hpp>
#include <znet/enums/schedule.hpp>
#include <znet/znet_week_day.hpp>
#include <znet/znet_meter.hpp>
#include <znet/znet_multicast_mode.hpp>
#include <znet/optional.hpp>
#include <tiny/include/tiny/tiny_utils.h>

namespace HubPlatform {
namespace Zwave {

namespace {
Json makeScheduleEntryLockParams( uint8_t nodeId, uint8_t channelId, uint8_t userId, uint8_t slotId, Json value = {} ) {
    Json params = Json::object();

    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ChannelId ] = channelId;

    params[ Resources::RPCParams::SlotId ] = slotId;
    params[ Resources::RPCParams::UserId ] = userId;

    if( !value.empty() ) {
        params[ Resources::RPCParams::Value ] = std::move( value );
    }

    return params;
}

}  // namespace

ZwaveClient::ZwaveClient( Addons::Addon& addon )
: ScriptClient( Addons::Addon::createQualifiedName( ZwaveAddon::Name ), addon ) {}

void ZwaveClient::startInclude( const std::string& deviceSettingFilePath ) {
    auto method = prepareMethod( Resources::RPC::StartInclude );
    method[ "params" ] = JSON( Resources::RPCParams::DeviceSettingFilePath << deviceSettingFilePath );
    sendMethodCall( std::move( method ) );
}

void ZwaveClient::startRediscover( const uint8_t nodeId, const std::string& deviceSettingFilePath ) {
    if( !hasNode( nodeId ) )
        throw NodeNotFoundError{ nodeId };

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::DeviceSettingFilePath ] = deviceSettingFilePath;
    sendMethodCall( ZWAVE_START_REDISCOVER, std::move( params ) );
}

void ZwaveClient::startExclude() {
    sendMethodCall( prepareMethod( Resources::RPC::StartExclude ) );
}

void ZwaveClient::stopInclude() {
    sendMethodCall( prepareMethod( Resources::RPC::StopInclude ) );
}

void ZwaveClient::stopExclude() {
    sendMethodCall( prepareMethod( Resources::RPC::StopExclude ) );
}

void ZwaveClient::startLearn() {
    sendMethodCall( prepareMethod( Resources::RPC::StartLearn ) );
}

void ZwaveClient::stopLearn() {
    sendMethodCall( prepareMethod( Resources::RPC::StopLearn ) );
}

void ZwaveClient::stopSecurityInclude() {
    sendMethodCall( ZWAVE_STOP_SECURITY_INCLUDE, JSON_EMPTY_OBJ );
}

void ZwaveClient::startExclude( const uint8_t nodeId ) {
    if( !hasNode( nodeId ) )
        throw NodeNotFoundError{ nodeId };

    auto method = prepareMethod( Resources::RPC::StartExclude );
    method[ "params" ] = JSON( Resources::RPCParams::NodeId << nodeId );
    sendMethodCall( std::move( method ) );
}

void ZwaveClient::setAuthenticationMode( const std::vector< std::string >& modes ) {
    for( const auto& mode : modes )
        AuthModeUtils::checkAuthMode( mode.c_str() );

    auto method = prepareMethod( Resources::RPC::SetAuthenticationMode );
    method[ "params" ] = JSON( Resources::RPCParams::Modes << modes );
    sendMethodCall( std::move( method ) );
}

void ZwaveClient::setDeviceSpecificKey( uint16_t key ) {
    auto method = prepareMethod( Resources::RPC::SetDeviceSpecificKey );
    method[ "params" ] = JSON( Resources::RPCParams::Key << key );
    sendMethodCall( std::move( method ) );
}

void ZwaveClient::setDevicesPollIntervals( bool useFixedWakeUpInterval,
                                           uint32_t wakeUpInterval,
                                           uint32_t alwaysOnDevicesInetrval,
                                           uint8_t attemptsCount ) {
    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::UseFixedWakeUpInterval ] = useFixedWakeUpInterval;
    params[ Resources::RPCParams::WakeUpInterval ] = wakeUpInterval;
    params[ Resources::RPCParams::AlwaysOnDevicesInterval ] = alwaysOnDevicesInetrval;
    params[ Resources::RPCParams::AttemptsCount ] = attemptsCount;

    sendMethodCall( ZWAVE_SET_DEVICES_POLL_INTERVALS, std::move( params ) );
}

void ZwaveClient::setUnresponsiveDeviceIgnoreInterval( uint32_t interval ) {
    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::Interval ] = interval;

    sendMethodCall( ZWAVE_SET_UNRESPONSIVE_DEVICE_IGNORE_INTERVAL, std::move( params ) );
}

void ZwaveClient::setDefaultDeviceConfigurationsFile( const std::string& deviceSettingFilePath ) {
    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::DeviceSettingFilePath ] = deviceSettingFilePath;

    sendMethodCall( ZWAVE_DEFAULT_DEVICE_CONFIGURATIONS_FILE, std::move( params ) );
}

void ZwaveClient::setDoorLockMode( uint8_t nodeId, uint8_t channelId, const std::string& doorLockMode ) {
    doSimpleRequest(
        ZWAVE_SET_DOOR_LOCK_MODE, nodeId, channelId, ZNET_COMMAND_CLASS_DOOR_LOCK, [&doorLockMode]( Json& params ) {
            using namespace znet::door_lock_converter;
            const auto modeOpt = modeFromString( doorLockMode.c_str() );
            if( !modeOpt ) {
                throw InvalidDoorLockMode( doorLockMode );
            }
            params[ Resources::RPCParams::Value ] = static_cast< uint8_t >( *modeOpt );
        } );
}

void ZwaveClient::requestDoorLockState( uint8_t nodeId, uint8_t channelId ) {
    doSimpleRequest( ZWAVE_REQUEST_DOOR_LOCK_STATE, nodeId, channelId, ZNET_COMMAND_CLASS_DOOR_LOCK );
}

void ZwaveClient::setDoorLockConfiguration( uint8_t nodeId,
                                            uint8_t channelId,
                                            const DoorLockConfiguration& configuration,
                                            const DoorLockExtendedConfigurationOpt& extendedConfiguration ) {
    doSimpleRequest(
        ZWAVE_SET_DOOR_LOCK_CONFIGURATION,
        nodeId,
        channelId,
        ZNET_COMMAND_CLASS_DOOR_LOCK,
        [&configuration, &extendedConfiguration]( Json& params ) {
            if( configuration.lockTimeoutOpt.second ) {
                params[ Resources::RPCParams::LockTimeout ] = configuration.lockTimeoutOpt.first;
            }

            params[ Resources::RPCParams::EnabledInsideHandleIds ] = configuration.enabledInsideHandleIdsBitmask;
            params[ Resources::RPCParams::EnabledOutsideHandleIds ] = configuration.enabledOutsideHandleIdsBitmask;

            if( extendedConfiguration.second ) {
                params[ Resources::RPCParams::HoldAndReleaseTimeout ] =
                    extendedConfiguration.first.holdAndReleaseTimeout;
                params[ Resources::RPCParams::AutoRelockTimeout ] = extendedConfiguration.first.autoRelockTimeout;
                params[ Resources::RPCParams::TwistAssistEnabled ] = extendedConfiguration.first.isTwistAssistEnabled;
                params[ Resources::RPCParams::BlockToBlockEnabled ] = extendedConfiguration.first.isBlockToBlockEnabled;
            }
        } );
}

void ZwaveClient::requestDoorLockConfiguration( uint8_t nodeId, uint8_t channelId ) {
    doSimpleRequest( ZWAVE_REQUEST_DOOR_LOCK_CONFIGURATION, nodeId, channelId, ZNET_COMMAND_CLASS_DOOR_LOCK );
}

Json ZwaveClient::getAllNodesIds() const {
    const JsonObject fieldsToReturn = JSON( Resources::DB::Fields::Id << "id" );
    const auto data = getDBClient().findAll( Resources::DB::TableNames::Devices, JSON_EMPTY_OBJ, &fieldsToReturn );
    JsonArray result = Json::array();
    for( const auto& object : data ) {
        result.push_back( json_get_str( object, "id" ) );
    }
    return result;
}

Json ZwaveClient::getNodeAsJson( const uint8_t nodeId ) const {
    auto node = getNode( nodeId );
    if( !node ) {
        LOGGER_DEBUG( cc::debug( "Node not found with id: " ) + cc::num10( nodeId ) );
        return nullptr;
    }

    ZwaveNodeJsonSerializer serializer( *node );
    return serializer.serialize();
}

Json ZwaveClient::getNodeInfoAsJson( uint8_t nodeId ) const {
    Json node = getNodeAsJson( nodeId );
    if( node.is_null() ) {
        LOGGER_DEBUG( cc::debug( "Node not found with id: " ) + cc::num10( nodeId ) );
        return nullptr;
    }

    json_remove( node, Resources::Fields::Node::Channels );
    return node;
}

std::string ZwaveClient::getManufacturerName( const uint16_t manufacturerId ) const {
    return znet::manufacturer_names_converter::manufacturerIdToName( manufacturerId );
}

Json ZwaveClient::getCommandClassAsJson( const uint8_t nodeId, const uint8_t channelId, const uint8_t ccId ) const {
    auto node = getNode( nodeId );
    if( !node ) {
        LOGGER_DEBUG( cc::debug( "Node not found with id: " ) + cc::num10( nodeId ) );
        return nullptr;
    }

    auto cc = node->getCommandClass( channelId, ccId );

    if( !cc ) {
        LOGGER_DEBUG( cc::debug( "Node not found with id: " ) + cc::num10( nodeId ) + cc::debug( ", channel id: " ) +
                      cc::num10( channelId ) + cc::debug( ", class id: " ) + cc::num10( ccId ) );
        return nullptr;
    }

    ZwaveCmdClassSerializationVisitor serializer;
    cc->accept( serializer );
    return serializer.takeResult();
}

Json ZwaveClient::nodeBase64ToJson( const std::string& nodeBase64 ) const {
    auto nodeBaseJson = JSON( Resources::DB::Fields::Base64Data << nodeBase64 );
    auto node = NodeManager::deserializeNode( nodeBaseJson, nullptr, nullptr, nullptr );

    if( node ) {
        ZwaveNodeJsonSerializer serializer( *node );
        return serializer.serialize();
    }
    else {
        return {};
    }
}

void ZwaveClient::setValue( const uint8_t nodeId,
                            const uint8_t classId,
                            const uint8_t channelId,
                            const Json& value,
                            const common::optional< uint32_t >& durationOpt ) {
    if( !hasNode( nodeId, channelId, classId ) )
        throw NodeNotFoundError{ nodeId, channelId, classId };

    auto method = prepareMethod( Resources::RPC::SetValue );
    auto jsonParams = JSON( Resources::RPCParams::NodeId << nodeId << Resources::RPCParams::ClassId << classId
                                                         << Resources::RPCParams::ChannelId << channelId
                                                         << Resources::RPCParams::Value << value );
    if( durationOpt.has_value() ) {
        jsonParams[ Resources::RPCParams::Duration ] = *durationOpt;
    }

    method[ "params" ] = jsonParams;
    sendMethodCall( std::move( method ) );
}

void ZwaveClient::setValueMulticast( const std::vector< uint8_t >& nodeId,
                                     const uint8_t classId,
                                     const uint8_t channelId,
                                     Json value,
                                     const std::string& mode ) {
    doMulticastRequest( ZWAVE_SET_VALUE_MULTICAST, nodeId, classId, channelId, std::move( value ), mode );
}

void ZwaveClient::setTokenValue( const uint8_t nodeId,
                                 const uint8_t classId,
                                 const uint8_t channelId,
                                 const std::string& value ) {
    if( !hasNode( nodeId, channelId, classId ) )
        throw NodeNotFoundError{ nodeId, channelId, classId };

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ClassId ] = classId;
    params[ Resources::RPCParams::ChannelId ] = channelId;
    params[ Resources::RPCParams::Value ] = value;

    sendMethodCall( ZWAVE_SET_TOKEN_VALUE, std::move( params ) );
}

void ZwaveClient::setTokenValueMulticast( const std::vector< uint8_t >& nodeId,
                                          const uint8_t classId,
                                          const uint8_t channelId,
                                          std::string value,
                                          const std::string& mode ) {
    doMulticastRequest( ZWAVE_SET_TOKEN_VALUE_MULTICAST, nodeId, classId, channelId, std::move( value ), mode );
}

void ZwaveClient::setRgbValue( const uint8_t nodeId,
                               const uint8_t classId,
                               const uint8_t channelId,
                               const Json& value ) {
    if( !hasNode( nodeId, channelId, classId ) )
        throw NodeNotFoundError{ nodeId, channelId, classId };

    auto method = prepareMethod( Resources::RPC::SetRgbValue );
    method[ "params" ] = JSON( Resources::RPCParams::NodeId << nodeId << Resources::RPCParams::ClassId << classId
                                                            << Resources::RPCParams::ChannelId << channelId
                                                            << Resources::RPCParams::Value << value );

    sendMethodCall( std::move( method ) );
}

void ZwaveClient::setRgbValueMulticast( const std::vector< uint8_t >& nodeId,
                                        const uint8_t classId,
                                        const uint8_t channelId,
                                        Json value,
                                        const std::string& mode ) {
    doMulticastRequest( ZWAVE_SET_RGB_VALUE_MULTICAST, nodeId, classId, channelId, std::move( value ), mode );
}

void ZwaveClient::setSoundSwitchConfiguration( const uint8_t nodeId,
                                               const uint8_t channelId,
                                               const uint8_t volume,
                                               const uint8_t defaultToneId ) {
    if( !hasNode( nodeId, channelId ) )
        throw NodeNotFoundError{ nodeId, channelId };

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ChannelId ] = channelId;
    params[ Resources::RPCParams::Value ] =
        JSON( Resources::ScriptEvent::Param::Sound::Volume
              << volume << Resources::ScriptEvent::Param::Sound::DefaultToneId << defaultToneId );

    sendMethodCall( ZWAVE_SET_SOUND_SWITCH_CONFIGURATION, std::move( params ) );
}

void ZwaveClient::requestSoundSwitchConfiguration( const uint8_t nodeId, const uint8_t channelId ) {
    if( !hasNode( nodeId, channelId ) )
        throw NodeNotFoundError{ nodeId, channelId };

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ChannelId ] = channelId;

    sendMethodCall( ZWAVE_REQUEST_SOUND_SWITCH_CONFIGURATION, std::move( params ) );
}

void ZwaveClient::setSoundSwitchTonePlay( const uint8_t nodeId,
                                          const uint8_t channelId,
                                          const uint8_t toneId,
                                          const uint8_t volume ) {
    if( !hasNode( nodeId, channelId ) )
        throw NodeNotFoundError{ nodeId, channelId };

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ChannelId ] = channelId;
    params[ Resources::RPCParams::Value ] = JSON( Resources::ScriptEvent::Param::Sound::ToneId
                                                  << toneId << Resources::ScriptEvent::Param::Sound::Volume << volume );

    sendMethodCall( ZWAVE_SET_SOUND_SWITCH_TONE_PLAY, std::move( params ) );
}

void ZwaveClient::requestSoundSwitchTonePlay( const uint8_t nodeId, const uint8_t channelId ) {
    if( !hasNode( nodeId, channelId ) )
        throw NodeNotFoundError{ nodeId, channelId };

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ChannelId ] = channelId;

    sendMethodCall( ZWAVE_REQUEST_SOUND_SWITCH_TONE_PLAY, std::move( params ) );
}

void ZwaveClient::setSetpointValue( uint8_t nodeId,
                                    uint8_t classId,
                                    uint8_t channelId,
                                    uint8_t setpointType,
                                    const std::string& scale,
                                    float value ) {
    if( !hasNode( nodeId, channelId, classId ) )
        throw NodeNotFoundError{ nodeId, channelId, classId };

    const auto scaleOpt = znet::thermostat_setpoint_converter::scaleFromString( scale.c_str() );
    if( !scaleOpt.second )
        throw ValueError{ "Unsupported setpoint scale: " + scale };

    auto method = prepareMethod( Resources::RPC::SetSetpointValue );
    method[ "params" ] =
        JSON( Resources::RPCParams::NodeId
              << nodeId << Resources::RPCParams::ClassId << classId << Resources::RPCParams::ChannelId << channelId
              << Resources::RPCParams::SetpointType << setpointType << Resources::RPCParams::Scale << scaleOpt.first
              << Resources::RPCParams::Value << value );

    sendMethodCall( std::move( method ) );
}

void ZwaveClient::setConfigurationValue( uint8_t nodeId, uint16_t paramNumber, uint8_t paramSize, int64_t value ) {
    if( !hasNode( nodeId, ZNET_CHANNEL_ID_ROOT, ZNET_COMMAND_CLASS_CONFIGURATION ) )
        throw NodeNotFoundError{ nodeId, ZNET_CHANNEL_ID_ROOT, ZNET_COMMAND_CLASS_CONFIGURATION };

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ParamNumber ] = paramNumber;
    params[ Resources::RPCParams::ParamSize ] = paramSize;
    params[ Resources::RPCParams::Value ] = value;

    sendMethodCall( ZWAVE_SET_CONFIGURATION_VALUE, std::move( params ) );
}

void ZwaveClient::addConfiguration( uint8_t nodeId, uint8_t paramNumber, uint8_t paramSize ) {
    if( !hasNode( nodeId, ZNET_CHANNEL_ID_ROOT, ZNET_COMMAND_CLASS_CONFIGURATION ) )
        throw NodeNotFoundError{ nodeId, ZNET_CHANNEL_ID_ROOT, ZNET_COMMAND_CLASS_CONFIGURATION };

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ParamNumber ] = paramNumber;
    params[ Resources::RPCParams::ParamSize ] = paramSize;

    sendMethodCall( ZWAVE_ADD_CONFIGURATION, std::move( params ) );
}

void ZwaveClient::removeConfiguration( uint8_t nodeId, uint8_t paramNumber ) {
    if( !hasNode( nodeId, ZNET_CHANNEL_ID_ROOT, ZNET_COMMAND_CLASS_CONFIGURATION ) )
        throw NodeNotFoundError{ nodeId, ZNET_CHANNEL_ID_ROOT, ZNET_COMMAND_CLASS_CONFIGURATION };

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ParamNumber ] = paramNumber;

    sendMethodCall( ZWAVE_REMOVE_CONFIGURATION, std::move( params ) );
}

void ZwaveClient::addNodeToConfigurationGroup( uint8_t nodeId,
                                               uint8_t channelId,
                                               uint8_t groupId,
                                               uint8_t destinationNode,
                                               uint8_t destinationEndpoint ) {
    if( !hasNode( nodeId, channelId, ZNET_COMMAND_CLASS_ASSOCIATION ) &&
        !hasNode( nodeId, channelId, ZNET_COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION ) ) {
        throw NodeNotFoundError{ nodeId, channelId };
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ChannelId ] = channelId;
    params[ Resources::RPCParams::GroupId ] = groupId;
    params[ Resources::RPCParams::DestinationNode ] = destinationNode;
    params[ Resources::RPCParams::DestinationEndpoint ] = destinationEndpoint;

    sendMethodCall( ZWAVE_ADD_NODE_TO_ASSOCIATION_GROUP, std::move( params ) );
}

void ZwaveClient::removeNodeFromConfigurationGroup( uint8_t nodeId,
                                                    uint8_t channelId,
                                                    uint8_t groupId,
                                                    uint8_t destinationNode,
                                                    uint8_t destinationEndpoint ) {
    if( !hasNode( nodeId, channelId, ZNET_COMMAND_CLASS_ASSOCIATION ) &&
        !hasNode( nodeId, channelId, ZNET_COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION ) ) {
        throw NodeNotFoundError{ nodeId, channelId };
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ChannelId ] = channelId;
    params[ Resources::RPCParams::GroupId ] = groupId;
    params[ Resources::RPCParams::DestinationNode ] = destinationNode;
    params[ Resources::RPCParams::DestinationEndpoint ] = destinationEndpoint;

    sendMethodCall( ZWAVE_REMOVE_NODE_FROM_ASSOCIATION_GROUP, std::move( params ) );
}

void ZwaveClient::resetConfigurationValue( uint8_t nodeId, uint16_t paramNumber ) {
    if( !hasNode( nodeId, ZNET_CHANNEL_ID_ROOT, ZNET_COMMAND_CLASS_CONFIGURATION ) )
        throw NodeNotFoundError{ nodeId, ZNET_CHANNEL_ID_ROOT, ZNET_COMMAND_CLASS_CONFIGURATION };

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ParamNumber ] = paramNumber;

    sendMethodCall( ZWAVE_RESET_CONFIGURATION_VALUE, std::move( params ) );
}

void ZwaveClient::resetAllConfigurationValues( uint8_t nodeId ) {
    if( !hasNode( nodeId, ZNET_CHANNEL_ID_ROOT, ZNET_COMMAND_CLASS_CONFIGURATION ) )
        throw NodeNotFoundError{ nodeId, ZNET_CHANNEL_ID_ROOT, ZNET_COMMAND_CLASS_CONFIGURATION };

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;

    sendMethodCall( ZWAVE_RESET_ALL_CONFIGURATION_VALUES, std::move( params ) );
}

void ZwaveClient::requestConfigurationValue( uint8_t nodeId, uint16_t paramNumber ) {
    if( !hasNode( nodeId, ZNET_CHANNEL_ID_ROOT, ZNET_COMMAND_CLASS_CONFIGURATION ) )
        throw NodeNotFoundError{ nodeId, ZNET_CHANNEL_ID_ROOT, ZNET_COMMAND_CLASS_CONFIGURATION };

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ParamNumber ] = paramNumber;

    sendMethodCall( ZWAVE_REQUEST_CONFIGURATION_VALUE, std::move( params ) );
}

void ZwaveClient::switchMultilevelLevelChange( uint8_t nodeId,
                                               uint8_t channelId,
                                               const std::string& command,
                                               const common::optional< uint32_t >& durationOpt ) {
    if( !hasNode( nodeId, channelId, ZNET_COMMAND_CLASS_SWITCH_MULTILEVEL ) )
        throw NodeNotFoundError{ nodeId, channelId, ZNET_COMMAND_CLASS_SWITCH_MULTILEVEL };

    const auto commandOpt = znet::stringToValueChangeCommand( command.c_str() );
    if( !commandOpt )
        throw ValueError{ "Unsupported switch multilevel command: " + command };

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ChannelId ] = channelId;
    params[ Resources::RPCParams::Command ] = static_cast< uint8_t >( *commandOpt );

    if( durationOpt.has_value() ) {
        params[ Resources::RPCParams::Duration ] = *durationOpt;
    }

    sendMethodCall( ZWAVE_SWITCH_MULTILEVEL_LEVEL_CHANGE, std::move( params ) );
}

void ZwaveClient::colorSwitchLevelChange( uint8_t nodeId,
                                          uint8_t channelId,
                                          const std::string& componentId,
                                          const std::string& command,
                                          const common::optional< uint32_t >& durationOpt ) {
    if( !hasNode( nodeId, channelId, ZNET_COMMAND_CLASS_SWITCH_COLOR ) )
        throw NodeNotFoundError{ nodeId, channelId, ZNET_COMMAND_CLASS_SWITCH_COLOR };

    const auto colorComponentIdOpt = znet::color_switch_converter::strToColorComponentId( componentId.c_str() );
    if( !colorComponentIdOpt ) {
        throw ValueError{ "Unsupported switch color component: " + componentId };
    }

    const auto commandOpt = znet::stringToValueChangeCommand( command.c_str() );
    if( !commandOpt ) {
        throw ValueError{ "Unsupported switch color command: " + command };
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ChannelId ] = channelId;
    params[ Resources::RPCParams::ComponentId ] = static_cast< uint8_t >( *colorComponentIdOpt );
    params[ Resources::RPCParams::Command ] = static_cast< uint8_t >( *commandOpt );

    if( durationOpt.has_value() ) {
        params[ Resources::RPCParams::Duration ] = *durationOpt;
    }

    sendMethodCall( ZWAVE_COLOR_SWITCH_LEVEL_CHANGE, std::move( params ) );
}

void ZwaveClient::requestValue( const uint8_t nodeId, const uint8_t classId, const uint8_t channelId ) {
    if( !hasNode( nodeId, channelId, classId ) )
        throw NodeNotFoundError{ nodeId, channelId, classId };

    auto method = prepareMethod( Resources::RPC::RequestValue );
    method[ "params" ] = JSON( Resources::RPCParams::NodeId << nodeId << Resources::RPCParams::ClassId << classId
                                                            << Resources::RPCParams::ChannelId << channelId );
    sendMethodCall( std::move( method ) );
}

void ZwaveClient::requestSetpointValue( uint8_t nodeId, uint8_t channelId, uint8_t setpointType ) {
    if( !hasNode( nodeId, channelId ) )
        throw NodeNotFoundError{ nodeId, channelId };

    auto method = prepareMethod( Resources::RPC::RequestSetpointValue );
    method[ "params" ] = JSON( Resources::RPCParams::NodeId << nodeId << Resources::RPCParams::ChannelId << channelId
                                                            << Resources::RPCParams::SetpointType << setpointType );

    sendMethodCall( std::move( method ) );
}

void ZwaveClient::requestUserCode( uint8_t nodeId, uint8_t channelId, uint8_t userId ) {
    doSimpleRequest(
        ZWAVE_REQUEST_USER_CODE, nodeId, channelId, ZNET_COMMAND_CLASS_USER_CODE, [userId]( Json& params ) {
            params[ Resources::RPCParams::UserId ] = userId;
        } );
}

void ZwaveClient::requestUserCodes( uint8_t nodeId, uint8_t channelId ) {
    doSimpleRequest( ZWAVE_REQUEST_USER_CODES, nodeId, channelId, ZNET_COMMAND_CLASS_USER_CODE );
}

void ZwaveClient::requestUserCodesChecksum( uint8_t nodeId, uint8_t channelId ) {
    doSimpleRequest( ZWAVE_REQUEST_USER_CODES_CHECKSUM, nodeId, channelId, ZNET_COMMAND_CLASS_USER_CODE );
}

uint16_t ZwaveClient::calculateUserCodesChecksum( const std::vector< UserCodeElement >& userCodes ) {
    std::vector< znet_user_code_report_t > znetUserCodes;
    znetUserCodes.reserve( userCodes.size() );

    std::transform(
        userCodes.cbegin(),
        userCodes.cend(),
        std::back_inserter( znetUserCodes ),
        []( const UserCodeElement& userCode ) {
            const auto status = znet::user_code_converter::userIdStatusFromString( userCode.status.c_str() );

            if( !status ) {
                throw ValueError{ "Unsupported user code mode: " + userCode.status };
            }

            if( userCode.code.size() > 0xFF ) {
                throw ValueError{ "User code is too long: " + userCode.code };
            }

            return znet_user_code_report_t{ .user_id = userCode.id,
                                            .user_id_status = static_cast< uint8_t >( *status ),
                                            .user_code_size = static_cast< uint8_t >( userCode.code.size() ),
                                            .user_code = reinterpret_cast< const uint8_t* >( userCode.code.data() ) };
        } );

    return calculate_user_codes_checksum( znetUserCodes.data(),
                                          static_cast< znet_supported_users_t >( znetUserCodes.size() ) );
}

void ZwaveClient::setMasterCode( uint8_t nodeId, uint8_t channelId, const std::string& masterCode ) {
    doSimpleRequest(
        ZWAVE_SET_MASTER_CODE, nodeId, channelId, ZNET_COMMAND_CLASS_USER_CODE, [&masterCode]( Json& params ) {
            params[ Resources::RPCParams::MasterCode ] = masterCode;
        } );
}

void ZwaveClient::setUserCodesKeypadMode( uint8_t nodeId, uint8_t channelId, const std::string& mode ) {
    const auto modeOpt = znet::user_code_converter::keypadModeFromString( mode.c_str() );

    if( !modeOpt ) {
        throw ValueError( "Invalid user code keypad mode: " + mode );
    }

    doSimpleRequest(
        ZWAVE_SET_USER_CODES_KEYPAD_MODE, nodeId, channelId, ZNET_COMMAND_CLASS_USER_CODE, [&modeOpt]( Json& params ) {
            params[ Resources::RPCParams::Mode ] = static_cast< uint8_t >( *modeOpt );
        } );
}

void ZwaveClient::requestUserCodesKeypadMode( uint8_t nodeId, uint8_t channelId ) {
    doSimpleRequest( ZWAVE_REQUEST_USER_CODES_KEYPAD_MODE, nodeId, channelId, ZNET_COMMAND_CLASS_USER_CODE );
}

void ZwaveClient::requestMasterCode( uint8_t nodeId, uint8_t channelId ) {
    doSimpleRequest( ZWAVE_REQUEST_MASTER_CODE, nodeId, channelId, ZNET_COMMAND_CLASS_USER_CODE );
}

void ZwaveClient::requestAlarmSensorValue( uint8_t nodeId, uint8_t channelId, const std::string& sensorName ) {
    if( !hasNode( nodeId, channelId, ZNET_COMMAND_CLASS_SENSOR_ALARM ) ) {
        throw NodeNotFoundError{ nodeId, channelId };
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ChannelId ] = channelId;

    const auto sensorType = znet::getNotificationType( sensorName );

    if( !sensorType ) {
        throw InvalidSensorType( sensorName );
    }

    params[ Resources::RPCParams::SensorType ] = static_cast< uint8_t >( *sensorType );

    sendMethodCall( ZWAVE_REQUEST_SENSOR_ALARM_VALUE, std::move( params ) );
}

void ZwaveClient::requestAllNodesValues() {
    sendMethodCall( ZWAVE_REQUEST_ALL_NODES_VALUES, JSON_EMPTY_OBJ );
}

void ZwaveClient::requestNodeValues( uint8_t nodeId,
                                     std::chrono::milliseconds requestDelay,
                                     size_t unsetPinCodesCountToStop,
                                     bool force ) {
    if( !hasNode( nodeId ) ) {
        throw NodeNotFoundError{ nodeId };
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::RequestDelay ] = requestDelay.count();
    params[ Resources::RPCParams::UnsetPinCodesCountToStop ] = unsetPinCodesCountToStop;
    params[ Resources::RPCParams::Force ] = force;

    sendMethodCall( ZWAVE_REQUEST_NODE_VALUES, std::move( params ) );
}

void ZwaveClient::requestAssociationGroup( uint8_t nodeId, uint8_t channelId, uint8_t groupId ) {
    if( !hasNode( nodeId, channelId, ZNET_COMMAND_CLASS_ASSOCIATION ) &&
        !hasNode( nodeId, channelId, ZNET_COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION ) ) {
        throw NodeNotFoundError{ nodeId, channelId };
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ChannelId ] = channelId;
    params[ Resources::RPCParams::GroupId ] = groupId;

    sendMethodCall( ZWAVE_REQUEST_ASSOCIATION_GROUP, std::move( params ) );
}

void ZwaveClient::sendIdentify( uint8_t nodeId, uint8_t channelId ) {
    if( !hasNode( nodeId, channelId, ZNET_COMMAND_CLASS_INDICATOR ) ) {
        throw NodeNotFoundError{ nodeId, channelId };
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ChannelId ] = channelId;

    sendMethodCall( ZWAVE_SEND_IDENTIFY, std::move( params ) );
}

void ZwaveClient::setUserCode( uint8_t nodeId, uint8_t channelId, const UserCodeElement& userCode ) {
    if( !hasNode( nodeId, channelId, ZNET_COMMAND_CLASS_USER_CODE ) ) {
        throw NodeNotFoundError{ nodeId, channelId };
    }

    const auto userIdStatusOpt = znet::user_code_converter::userIdStatusFromString( userCode.status.c_str() );
    if( !userIdStatusOpt ) {
        throw InvalidUserIdStatus{ userCode.status };
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ChannelId ] = channelId;
    params[ Resources::RPCParams::UserId ] = userCode.id;
    params[ Resources::RPCParams::UserIdStatus ] = static_cast< uint8_t >( *userIdStatusOpt );
    params[ Resources::RPCParams::UserCode ] = userCode.code;

    sendMethodCall( ZWAVE_SET_USER_CODE, std::move( params ) );
}

void ZwaveClient::setSilentSensors( uint8_t nodeId,
                                    uint8_t channelId,
                                    uint16_t delay,
                                    const std::vector< std::string >& sensorNames ) {
    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ChannelId ] = channelId;
    params[ Resources::RPCParams::Delay ] = delay;

    const auto disablingMode = znet::SensorsSoundDisablingMode::UsingBitmask;
    params[ Resources::RPCParams::SoundDisablingMode ] = static_cast< uint8_t >( disablingMode );

    const auto mask = znet::getExtendedAlarmSensorTypesMask(
        sensorNames.begin(), sensorNames.end(), []( const std::string& unknownSensor ) {
            throw InvalidSensorType( unknownSensor );
        } );

    params[ Resources::RPCParams::NotificationsMask ] = tiny::tools::base64::encode( mask.m_mask, mask.m_length );
    sendMethodCall( ZWAVE_SET_SILENT_SENSORS, std::move( params ) );
}

void ZwaveClient::setWeekDayEntryLockSchedule( uint8_t nodeId,
                                               uint8_t channelId,
                                               uint8_t userId,
                                               uint8_t slotId,
                                               WeekDayEntryLockScheduleOpt valueOpt ) {
    if( !hasNode( nodeId, channelId, ZNET_COMMAND_CLASS_SCHEDULE_ENTRY_LOCK ) ) {
        throw NodeNotFoundError{ nodeId, channelId };
    }

    Json jsonValue;

    if( valueOpt.second ) {
        const auto weekDayOpt = znet::getWeekDayByName( valueOpt.first.weekDay );

        if( !weekDayOpt ) {
            throw InvalidWeekDay{ valueOpt.first.weekDay };
        }

        jsonValue[ Resources::RPCParams::WeekDay ] = static_cast< uint8_t >( *weekDayOpt );
        jsonValue[ Resources::RPCParams::StartHour ] = valueOpt.first.startHour;
        jsonValue[ Resources::RPCParams::StartMin ] = valueOpt.first.startMin;
        jsonValue[ Resources::RPCParams::StopHour ] = valueOpt.first.stopHour;
        jsonValue[ Resources::RPCParams::StopMin ] = valueOpt.first.stopMin;
    }

    sendMethodCall( ZWAVE_SET_WEEK_DAY_ENTRY_LOCK_SCHEDULE,
                    makeScheduleEntryLockParams( nodeId, channelId, userId, slotId, std::move( jsonValue ) ) );
}

void ZwaveClient::setYearDayEntryLockSchedule( uint8_t nodeId,
                                               uint8_t channelId,
                                               uint8_t userId,
                                               uint8_t slotId,
                                               YearDayEntryLockScheduleOpt valueOpt ) {
    if( !hasNode( nodeId, channelId, ZNET_COMMAND_CLASS_SCHEDULE_ENTRY_LOCK ) ) {
        throw NodeNotFoundError{ nodeId, channelId };
    }

    Json jsonValue;

    if( valueOpt.second ) {
        jsonValue[ Resources::RPCParams::StartYear ] = valueOpt.first.startYear;
        jsonValue[ Resources::RPCParams::StartMonth ] = valueOpt.first.startMonth;
        jsonValue[ Resources::RPCParams::StartDay ] = valueOpt.first.startDay;
        jsonValue[ Resources::RPCParams::StartHour ] = valueOpt.first.startHour;
        jsonValue[ Resources::RPCParams::StartMin ] = valueOpt.first.startMin;
        jsonValue[ Resources::RPCParams::StopYear ] = valueOpt.first.stopYear;
        jsonValue[ Resources::RPCParams::StopMonth ] = valueOpt.first.stopMonth;
        jsonValue[ Resources::RPCParams::StopDay ] = valueOpt.first.stopDay;
        jsonValue[ Resources::RPCParams::StopHour ] = valueOpt.first.stopHour;
        jsonValue[ Resources::RPCParams::StopMin ] = valueOpt.first.stopMin;
    }

    sendMethodCall( ZWAVE_SET_YEAR_DAY_ENTRY_LOCK_SCHEDULE,
                    makeScheduleEntryLockParams( nodeId, channelId, userId, slotId, std::move( jsonValue ) ) );
}

void ZwaveClient::setDailyRepeatingEntryLockSchedule( uint8_t nodeId,
                                                      uint8_t channelId,
                                                      uint8_t userId,
                                                      uint8_t slotId,
                                                      DailyRepeatingEntryLockScheduleOpt valueOpt ) {
    if( !hasNode( nodeId, channelId, ZNET_COMMAND_CLASS_SCHEDULE_ENTRY_LOCK ) ) {
        throw NodeNotFoundError{ nodeId, channelId };
    }

    Json jsonValue;

    if( valueOpt.second ) {
        jsonValue[ Resources::RPCParams::WeekDays ] = znet::getWeekDaysMask(
            valueOpt.first.weekDays.begin(), valueOpt.first.weekDays.end(), []( const std::string& weekDayName ) {
                throw InvalidWeekDay{ weekDayName };
            } );

        jsonValue[ Resources::RPCParams::StartHour ] = valueOpt.first.startHour;
        jsonValue[ Resources::RPCParams::StartMin ] = valueOpt.first.startMin;
        jsonValue[ Resources::RPCParams::DurationHour ] = valueOpt.first.durationHour;
        jsonValue[ Resources::RPCParams::DurationMin ] = valueOpt.first.durationMin;
    }

    sendMethodCall( ZWAVE_SET_DAILY_REPEATING_ENTRY_LOCK_SCHEDULE,
                    makeScheduleEntryLockParams( nodeId, channelId, userId, slotId, std::move( jsonValue ) ) );
}

void ZwaveClient::setEntryLockScheduleEnable( uint8_t nodeId, uint8_t channelId, uint8_t userId, bool enable ) {
    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ChannelId ] = channelId;
    params[ Resources::RPCParams::UserId ] = userId;
    params[ Resources::RPCParams::Enable ] = enable;

    sendMethodCall( ZWAVE_SET_ENTRY_LOCK_SCHEDULE_ENABLE, std::move( params ) );
}

std::pair< bool, std::string > ZwaveClient::isScheduleStructureAcceptable(
    uint8_t nodeId,
    uint8_t channelId,
    uint8_t scheduleId,
    const common::optional< uint8_t >& blockIdOpt,
    Json value ) {
    using namespace Resources::RPCParams;

    bool result = false;

    try {
        if( !json_contains( value, Schedule::Commands ) ) {
            return { false,
                     std::string( "The field " ) + std::string( Schedule::Commands ) + std::string( " is absent" ) };
        }

        const auto jsonNodeData = getDBClient().findOne( Resources::DB::TableNames::Devices,
                                                         JSON( Resources::DB::Fields::Id << std::to_string( nodeId ) ),
                                                         nullptr );

        if( jsonNodeData.empty() ) {
            return { false, std::string( "Node's data is absent" ) };
        }
        std::unique_ptr< znet::ZNode > node = NodeManager::deserializeNode( jsonNodeData, nullptr, nullptr, nullptr );

        if( !node ) {
            return { false, std::string( "Node's info is absent" ) };
        }

        const auto params = getCommandClassAsJson( nodeId, channelId, 0x53 );

        auto blocksIt = params.find( Schedule::ScheduleBlocks );
        if( blocksIt == params.end() ) {
            return { false, std::string( "Block's info is absent" ) };
        }

        const auto& blocks = *blocksIt;

        struct SupportStartTime {
            bool supportStartNow : 1;
            bool supportStartHourMinute : 1;
            bool supportCalendarTime : 1;
            bool supportWeekdays : 1;
            bool supportTimeFromNow : 1;
            bool supportRecurringMode : 1;
        };

        SupportStartTime supportStartTime;

        for( const auto& block : blocks ) {
            auto timeSupportIt = block.find( Schedule::ScheduleTimeSupport );

            if( timeSupportIt == block.end() ) {
                return { false, std::string( "Support time's info is absent" ) };
            }
            const auto& timeSupport = *timeSupportIt;
            for( const auto& type : timeSupport ) {
                znet::ScheduleStartTimeSupport supportType = znet::ScheduleStartTimeSupport::Undefined;

                if( !type.is_string() ) {
                    continue;
                }

                supportType = znet::schedule_start_time_support_converter::stringToStartTimeSupport(
                    type.get< std::string >().c_str() );

                switch( supportType ) {
                case znet::ScheduleStartTimeSupport::StartNow:
                    supportStartTime.supportStartNow = true;
                    break;

                case znet::ScheduleStartTimeSupport::StartHourMinute:
                    supportStartTime.supportStartHourMinute = true;
                    break;

                case znet::ScheduleStartTimeSupport::CalendarTime:
                    supportStartTime.supportCalendarTime = true;
                    break;

                case znet::ScheduleStartTimeSupport::Weekdays:
                    supportStartTime.supportWeekdays = true;
                    break;

                case znet::ScheduleStartTimeSupport::TimeFromNow:
                    supportStartTime.supportTimeFromNow = true;
                    break;

                case znet::ScheduleStartTimeSupport::RecurringMode:
                    supportStartTime.supportRecurringMode = true;
                    break;
                }
            }
        }

        // has hours and minutes data
        bool hasHoursAndMinutes = false;

        auto hourandminuteIt = value.find( Schedule::Hourandminute );
        if( hourandminuteIt != value.end() ) {
            auto& hourandminute = ( *hourandminuteIt );

            hasHoursAndMinutes = hourandminute.find( Schedule::StartHour ) != hourandminute.end() &&
                                 hourandminute.find( Schedule::StartMinute ) != hourandminute.end();
        }

        // has year data
        bool hasYear = false;
        // has month data
        bool hasMonth = false;
        // has day of month data
        bool hasDayOfMonth = false;

        auto yeardayIt = value.find( Schedule::Yearday );
        if( yeardayIt != value.end() ) {
            auto& yearday = ( *yeardayIt );
            hasYear = yearday.find( Schedule::StartYear ) != yearday.end();
            hasMonth = yearday.find( Schedule::StartMonth ) != yearday.end();
            hasDayOfMonth = yearday.find( Schedule::StartDayOfMonth ) != yearday.end();
        }

        // has weekdays data
        bool hasWeekdays = value.find( Schedule::Weekdays ) != value.end();

        if( supportStartTime.supportStartNow ) {
            // any parameter has NOT been set. Can start now
            if( !hasHoursAndMinutes && !hasYear && !hasMonth && !hasDayOfMonth && !hasWeekdays ) {
                return { true, {} };
            }
        }
        if( supportStartTime.supportStartHourMinute ) {
            // if hours and minutes has been set.
            if( hasHoursAndMinutes ) {
                if( !supportStartTime.supportCalendarTime && !hasYear && !hasMonth && !hasDayOfMonth &&
                    !supportStartTime.supportWeekdays && !hasWeekdays ) {
                    // And day, month, year and weekdays must NOT be set
                    return { true, {} };
                }
                else if( supportStartTime.supportCalendarTime &&
                         ( ( !hasYear && !hasMonth && hasDayOfMonth ) || ( !hasYear && hasMonth && hasDayOfMonth ) ||
                           ( hasYear && hasMonth && hasDayOfMonth ) ) ) {
                    // And day or day + month or  day + month + year can be set additionally
                    return { true, {} };
                }
                else if( supportStartTime.supportWeekdays && hasWeekdays ) {
                    // And weekdays can be set additionally
                    return { true, {} };
                }
            }
        }
        if( supportStartTime.supportCalendarTime && !hasHoursAndMinutes && !hasWeekdays &&
            ( ( !hasYear && !hasMonth && hasDayOfMonth ) || ( !hasYear && hasMonth && hasDayOfMonth ) ||
              ( hasYear && hasMonth && hasDayOfMonth ) ) ) {
            // day or day + month or  day + month + year has been set.  But hours and minutes and weekdays has NOT been
            // set
            return { true, {} };
        }
        if( supportStartTime.supportWeekdays && hasWeekdays && !hasHoursAndMinutes && !hasYear && !hasMonth &&
            !hasDayOfMonth ) {
            // if weekdays has been set only
            return { true, {} };
        }
        if( supportStartTime.supportTimeFromNow && hasHoursAndMinutes && !hasYear && !hasMonth && hasDayOfMonth &&
            !hasWeekdays ) {
            // if hours and minutes and day has been set.  But month and year and weekdays has NOT been set
            return { true, {} };
        }
        if( supportStartTime.supportRecurringMode && hasHoursAndMinutes && !hasYear && !hasMonth && !hasDayOfMonth &&
            !hasWeekdays ) {
            // if hours and minutes and day has been set. But day and month and year and weekdays has NOT been set
            return { true, {} };
        }
    }
    catch( ... ) {
        return { false, { std::string( "Checking of schedule's data is failed" ) } };
    }

    return { false, { std::string( "The start time's option isn't supported" ) } };
}

void ZwaveClient::setSchedule( uint8_t nodeId,
                               uint8_t channelId,
                               uint8_t scheduleId,
                               const common::optional< uint8_t >& blockIdOpt,
                               Json value ) {
    if( !hasNode( nodeId, channelId, ZNET_COMMAND_CLASS_SCHEDULE ) ) {
        throw NodeNotFoundError{ nodeId, channelId };
    }

    Json params;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ChannelId ] = channelId;
    params[ Resources::RPCParams::Schedule::ScheduleId ] = scheduleId;

    if( blockIdOpt.has_value() ) {
        params[ Resources::RPCParams::Schedule::ScheduleIdBlock ] = *blockIdOpt;
    }

    if( !value.empty() ) {
        params[ Resources::RPCParams::Value ] = std::move( value );

        auto result = isScheduleStructureAcceptable( nodeId, channelId, scheduleId, blockIdOpt, value );
        if( !result.first ) {
            throw ValueError( result.second );
        }
    }

    sendMethodCall( ZWAVE_SET_SCHEDULE, std::move( params ) );
}

void ZwaveClient::requestSchedule( uint8_t nodeId,
                                   uint8_t channelId,
                                   uint8_t scheduleId,
                                   const common::optional< uint8_t >& blockIdOpt,
                                   const common::optional< bool >& activeIdCtlOpt ) {
    if( !hasNode( nodeId, channelId, ZNET_COMMAND_CLASS_SCHEDULE ) ) {
        throw NodeNotFoundError{ nodeId, channelId };
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ChannelId ] = channelId;
    params[ Resources::RPCParams::Schedule::ScheduleId ] = scheduleId;

    if( blockIdOpt ) {
        params[ Resources::RPCParams::Schedule::ScheduleIdBlock ] = *blockIdOpt;
    }

    if( activeIdCtlOpt ) {
        params[ Resources::RPCParams::Schedule::AidRoControl ] = *activeIdCtlOpt;
    }

    sendMethodCall( ZWAVE_REQUEST_SCHEDULE, std::move( params ) );
}

void ZwaveClient::setScheduleState( uint8_t nodeId,
                                    uint8_t channelId,
                                    uint8_t scheduleId,
                                    const common::optional< uint8_t >& blockIdOpt,
                                    uint8_t scheduleState ) {
    if( !hasNode( nodeId, channelId, ZNET_COMMAND_CLASS_SCHEDULE ) ) {
        throw NodeNotFoundError{ nodeId, channelId };
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ChannelId ] = channelId;
    params[ Resources::RPCParams::Schedule::ScheduleId ] = scheduleId;
    if( blockIdOpt ) {
        params[ Resources::RPCParams::Schedule::ScheduleIdBlock ] = *blockIdOpt;
    }
    params[ Resources::RPCParams::Schedule::ScheduleState ] = scheduleState;

    sendMethodCall( ZWAVE_SET_SCHEDULE_STATE, std::move( params ) );
}

void ZwaveClient::requestScheduleState( uint8_t nodeId,
                                        uint8_t channelId,
                                        const common::optional< uint8_t >& blockIdOpt ) {
    if( !hasNode( nodeId, channelId, ZNET_COMMAND_CLASS_SCHEDULE ) ) {
        throw NodeNotFoundError{ nodeId, channelId };
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ChannelId ] = channelId;
    if( blockIdOpt ) {
        params[ Resources::RPCParams::Schedule::ScheduleIdBlock ] = *blockIdOpt;
    }

    sendMethodCall( ZWAVE_REQUEST_SCHEDULE_STATE, std::move( params ) );
}

void ZwaveClient::requestWeekDayEntryLockSchedule( uint8_t nodeId, uint8_t channelId, uint8_t userId, uint8_t slotId ) {
    if( !hasNode( nodeId, channelId, ZNET_COMMAND_CLASS_SCHEDULE_ENTRY_LOCK ) ) {
        throw NodeNotFoundError{ nodeId, channelId };
    }

    sendMethodCall( ZWAVE_REQUEST_WEEK_DAY_ENTRY_LOCK_SCHEDULE,
                    makeScheduleEntryLockParams( nodeId, channelId, userId, slotId ) );
}

void ZwaveClient::requestYearDayEntryLockSchedule( uint8_t nodeId, uint8_t channelId, uint8_t userId, uint8_t slotId ) {
    if( !hasNode( nodeId, channelId, ZNET_COMMAND_CLASS_SCHEDULE_ENTRY_LOCK ) ) {
        throw NodeNotFoundError{ nodeId, channelId };
    }

    sendMethodCall( ZWAVE_REQUEST_YEAR_DAY_ENTRY_LOCK_SCHEDULE,
                    makeScheduleEntryLockParams( nodeId, channelId, userId, slotId ) );
}

void ZwaveClient::requestDailyRepeatingEntryLockSchedule( uint8_t nodeId,
                                                          uint8_t channelId,
                                                          uint8_t userId,
                                                          uint8_t slotId ) {
    if( !hasNode( nodeId, channelId, ZNET_COMMAND_CLASS_SCHEDULE_ENTRY_LOCK ) ) {
        throw NodeNotFoundError{ nodeId, channelId };
    }

    sendMethodCall( ZWAVE_REQUEST_DAILY_REPEATING_ENTRY_LOCK_SCHEDULE,
                    makeScheduleEntryLockParams( nodeId, channelId, userId, slotId ) );
}

void ZwaveClient::setBarrierSignalValue( uint8_t nodeId,
                                         uint8_t channelId,
                                         const std::string& signalName,
                                         uint8_t value ) {
    if( !hasNode( nodeId, channelId, ZNET_COMMAND_CLASS_BARRIER_OPERATOR ) ) {
        throw NodeNotFoundError{ nodeId, channelId };
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ChannelId ] = channelId;
    params[ Resources::RPCParams::SignalName ] = signalName;
    params[ Resources::RPCParams::Value ] = value;

    sendMethodCall( ZWAVE_SET_BARRIER_SIGNAL_VALUE, std::move( params ) );
}

void ZwaveClient::requestBarrierSignalValue( uint8_t nodeId, uint8_t channelId, const std::string& signalName ) {
    if( !hasNode( nodeId, channelId, ZNET_COMMAND_CLASS_BARRIER_OPERATOR ) ) {
        throw NodeNotFoundError{ nodeId, channelId };
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ChannelId ] = channelId;
    params[ Resources::RPCParams::SignalName ] = signalName;

    sendMethodCall( ZWAVE_REQUEST_BARRIER_SIGNAL_VALUE, std::move( params ) );
}

void ZwaveClient::setThermostatSetbackValue( uint8_t nodeId,
                                             uint8_t channelId,
                                             const std::string& overrideType,
                                             const std::string& mode,
                                             const ThermostatSetbackValueOpt& valueOpt ) {
    doSimpleRequest( ZWAVE_SET_THERMOSTAT_SETBACK_VALUE,
                     nodeId,
                     channelId,
                     ZNET_COMMAND_CLASS_THERMOSTAT_SETBACK,
                     [&]( Json& params ) {
                         const auto overrideTypeOpt =
                             znet::thermostat_setback::stringToOverrideType( overrideType.c_str() );
                         if( !overrideTypeOpt ) {
                             throw ValueError{ "Unsupported thermostat setback override type: " + overrideType };
                         }

                         const auto modeOpt = znet::thermostat_setback::stringToMode( mode.c_str() );
                         if( !modeOpt ) {
                             throw ValueError{ "Unsupported thermostat setback mode: " + mode };
                         }

                         params[ Resources::RPCParams::OverrideType ] = static_cast< uint8_t >( *overrideTypeOpt );
                         params[ Resources::RPCParams::Mode ] = static_cast< uint8_t >( *modeOpt );

                         if( valueOpt.second ) {
                             params[ Resources::RPCParams::Value ] = valueOpt.first;
                         }
                     } );
}

void ZwaveClient::reset() {
    sendMethodCall( prepareMethod( Resources::RPC::Reset ) );
}

void ZwaveClient::reconstructNetwork() {
    sendMethodCall( ZWAVE_RECONSTRUCT_NETWORK, JSON_EMPTY_OBJ );
}

void ZwaveClient::resetMeter( uint8_t nodeId, uint8_t channelId, int value, uint8_t size ) {
    doSimpleRequest( ZWAVE_RESET_METER, nodeId, channelId, ZNET_COMMAND_CLASS_METER, [value, size]( Json& params ) {
        params[ Resources::RPCParams::Value ] = value;
        params[ Resources::RPCParams::ParamSize ] = size;
    } );
}

void ZwaveClient::requestMeter( uint8_t nodeId, uint8_t channelId, const std::string& scale ) {
    auto node = getNode( nodeId );

    if( !node ) {
        throw NodeNotFoundError{ nodeId, channelId, ZNET_COMMAND_CLASS_METER };
    }

    auto cc = node->getCommandClass< znet::ZCc0x32Meter >( channelId );

    if( !cc ) {
        throw NodeNotFoundError{ nodeId, channelId, ZNET_COMMAND_CLASS_METER };
    }

    const auto meterScale = znet::meter::stringToScale( cc->getType(), scale.c_str() );

    if( !meterScale ) {
        throw ValueError( "Invalid meter scale: " + scale );
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ChannelId ] = channelId;
    params[ Resources::RPCParams::Scale ] = static_cast< uint8_t >( *meterScale );

    sendMethodCall( ZWAVE_REQUEST_METER, std::move( params ) );
}

Json ZwaveClient::getProvisioningInfoAsJson( uint8_t nodeId ) const {
    auto entry = getProvisioningListEntry( nodeId );

    Json result;
    result[ "node_id" ] = nodeId;
    result[ "name" ] = entry.m_fields.m_name ? *entry.m_fields.m_name : "";
    result[ "room" ] = entry.m_fields.m_room ? *entry.m_fields.m_room : "";
    return result;
}

void ZwaveClient::setProvisioningInfo( uint8_t nodeId,
                                       const std::unique_ptr< std::string >& name,
                                       const std::unique_ptr< std::string >& room ) {
    auto entry = getProvisioningListEntry( nodeId );

    if( !name && !room ) {
        throw ValueError( "Neither name nor room were specified" );
    }

    auto params = JSON_EMPTY_OBJ;
    if( name ) {
        params[ "name" ] = *name;
    }
    if( room ) {
        params[ "room" ] = *room;
    }

    auto method = prepareMethod( Resources::RPC::ProvisioningSet );
    method[ "params" ] = std::move( params );
    method[ "params" ][ "_id" ] = ProvisioningListUtils::idToString( entry.m_provisioningId );

    sendRequest( ADDON_MESSAGE, std::move( method ) );
}

Json ZwaveClient::getInfo() const {
    return sendRequest( ZWAVE_GET_INFO, {} );
}

std::string ZwaveClient::getFirmwareRegion() const {
    return FirmwareInfoHelper::getFirmwareRegion();
}

void ZwaveClient::updateNodesFirmware( const std::vector< uint8_t >& nodeIds,
                                       const std::vector< NodeFirmwareInfo >& firmwares ) {
    for( auto nodeId : nodeIds ) {
        if( !hasNode( nodeId, ZNET_CHANNEL_ID_ROOT, ZNET_COMMAND_CLASS_FIRMWARE_UPDATE_MD ) ) {
            throw NodeNotFoundError{ nodeId, ZNET_CHANNEL_ID_ROOT, ZNET_COMMAND_CLASS_FIRMWARE_UPDATE_MD };
        }
    }

    if( firmwares.empty() ) {
        throw ValueError( "No passed firmwares" );
    }

    auto params = JSON_EMPTY_OBJ;

    params[ Resources::RPCParams::NodeIds ] = nodeIds;
    auto& jsFirmwares = params[ Resources::RPCParams::Firmwares ] = Json::array();

    for( auto& firmware : firmwares ) {
        auto jsFirmware = Json::object();
        jsFirmware[ Resources::RPCParams::Target ] = firmware.target;
        jsFirmware[ Resources::RPCParams::Checksum ] = firmware.checksum;
        jsFirmware[ Resources::RPCParams::Image ] = firmware.image;
        jsFirmwares.push_back( std::move( jsFirmware ) );
    }

    sendMethodCall( ZWAVE_UPDATE_NODES_FIRMWARE, std::move( params ) );
}

void ZwaveClient::downloadNodeFirmware( uint8_t nodeId, uint8_t target, const std::string& path ) {
    if( !hasNode( nodeId, ZNET_CHANNEL_ID_ROOT, ZNET_COMMAND_CLASS_FIRMWARE_UPDATE_MD ) ) {
        throw NodeNotFoundError{ nodeId, ZNET_CHANNEL_ID_ROOT, ZNET_COMMAND_CLASS_FIRMWARE_UPDATE_MD };
    }

    if( path.empty() ) {
        throw ValueError( "File path is empty" );
    }

    auto params = JSON_EMPTY_OBJ;

    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::Target ] = target;
    params[ Resources::RPCParams::Path ] = path;

    sendMethodCall( ZWAVE_DOWNLOAD_NODE_FIRMWARE, std::move( params ) );
}

void ZwaveClient::requestNotificationValue( uint8_t nodeId,
                                            uint8_t channelId,
                                            const std::string& notificationType,
                                            const std::string& eventType ) {
    doSimpleRequest(
        ZWAVE_REQUEST_NOTIFICATION_VALUE, nodeId, channelId, ZNET_COMMAND_CLASS_NOTIFICATION, [&]( Json& params ) {
            const auto type = znet::getNotificationType( notificationType );
            if( !type ) {
                throw ValueError( "Notification type " + notificationType + " is unknown" );
            }

            if( !eventType.empty() ) {
                const auto event = znet::getNotificationEvent( *type, eventType );

                if( !event ) {
                    throw ValueError( "Notification event type " + eventType + " is unknown" );
                }

                params[ Resources::RPCParams::NotificationEvent ] = *event;
            }
            else {
                params[ Resources::RPCParams::NotificationEvent ] = 0;
            }

            params[ Resources::RPCParams::NotificationType ] = static_cast< uint8_t >( *type );
        } );
}

void ZwaveClient::requestAlarmV1Value( uint8_t nodeId, uint8_t channelId, uint8_t alarmType ) {
    doSimpleRequest(
        ZWAVE_REQUEST_ALARM_V1_VALUE, nodeId, channelId, ZNET_COMMAND_CLASS_NOTIFICATION, [&]( Json& params ) {
            params[ Resources::RPCParams::NotificationType ] = alarmType;
        } );
}

void ZwaveClient::checkReachableStatus( uint8_t nodeId ) {
    if( !hasNode( nodeId ) ) {
        throw NodeNotFoundError{ nodeId };
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;

    sendMethodCall( ZWAVE_CHECK_REACHABLE_STATUS, std::move( params ) );
}

void ZwaveClient::checkNodeValues( uint8_t nodeId, std::chrono::milliseconds requestDelay, const Json& ccToCheck ) {
    if( !hasNode( nodeId ) ) {
        throw NodeNotFoundError{ nodeId };
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::RequestDelay ] = requestDelay.count();
    params[ Resources::RPCParams::CcToCheck ] = ccToCheck;

    sendMethodCall( ZWAVE_CHECK_NODE_VALUES, std::move( params ) );
}

void ZwaveClient::lockAntiTheft( uint8_t nodeId, uint8_t channelId, std::string magicCode, std::string hint ) {
    auto bytesMagicCode = znet::utils::hexToBytes( magicCode );

    if( !bytesMagicCode || bytesMagicCode->size() < znet::ZCcAntiTheftBase::MinMagicCodeLength ||
        bytesMagicCode->size() > znet::ZCcAntiTheftBase::MaxMagicCodeLength ) {
        throw ValueError( "Invalid magic code " + magicCode );
    }

    auto bytesHint = znet::utils::hexToBytes( hint );

    if( !bytesHint || bytesHint->size() < znet::ZCcAntiTheftBase::MinMagicCodeLength ||
        bytesHint->size() > znet::ZCcAntiTheftBase::MaxMagicCodeLength ) {
        throw ValueError( "Invalid hint " + magicCode );
    }

    doSimpleRequest( ZWAVE_LOCK_ANTI_THEFT, nodeId, channelId, ZNET_COMMAND_CLASS_ANTITHEFT, [&]( Json& params ) {
        params[ Resources::RPCParams::MagicCode ] = std::move( magicCode );
        params[ Resources::RPCParams::Hint ] = std::move( hint );
    } );
}

void ZwaveClient::lockAntiTheftDefault( uint8_t nodeId, uint8_t channelId ) {
    doSimpleRequest( ZWAVE_LOCK_ANTI_THEFT_DEFAULT, nodeId, channelId, ZNET_COMMAND_CLASS_ANTITHEFT );
}

void ZwaveClient::unlockAntiTheft( uint8_t nodeId, uint8_t channelId, uint8_t classId, std::string magicCode ) {
    if( classId != ZNET_COMMAND_CLASS_ANTITHEFT && classId != ZNET_COMMAND_CLASS_ANTITHEFT_UNLOCK ) {
        throw ValueError( "Invalid cc " + std::to_string( classId ) );
    }

    if( !znet::utils::hexToBytes( magicCode ) ) {
        throw ValueError( "Invalid magic code " + magicCode );
    }

    doSimpleRequest( ZWAVE_UNLOCK_ANTI_THEFT, nodeId, channelId, classId, [&]( Json& params ) {
        params[ Resources::RPCParams::MagicCode ] = std::move( magicCode );
        params[ Resources::RPCParams::ClassId ] = classId;
    } );
}
void ZwaveClient::unlockAntiTheftDefault( uint8_t nodeId, uint8_t channelId, uint8_t classId, std::string hint ) {
    if( classId != ZNET_COMMAND_CLASS_ANTITHEFT && classId != ZNET_COMMAND_CLASS_ANTITHEFT_UNLOCK ) {
        throw ValueError( "Invalid cc " + std::to_string( classId ) );
    }

    auto bytesHint = znet::utils::hexToBytes( hint );

    if( !bytesHint || bytesHint->size() != znet::ZCcAntiTheftBase::MaxMagicCodeLength ) {
        throw ValueError( "Invalid hint " + hint );
    }

    doSimpleRequest( ZWAVE_UNLOCK_ANTI_THEFT_DEFAULT, nodeId, channelId, classId, [&]( Json& params ) {
        params[ Resources::RPCParams::Hint ] = std::move( hint );
        params[ Resources::RPCParams::ClassId ] = classId;
    } );
}

void ZwaveClient::addDeviceSpecificConfiguration( const std::string& deviceTypeId, const Json& configuration ) {
    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::DeviceTypeId ] = deviceTypeId;
    params[ Resources::RPCParams::Configuration ] = configuration;

    sendMethodCall( ZWAVE_ADD_DEVICE_SPECIFIC_CONFIGURATION, std::move( params ) );
}

void ZwaveClient::addDeviceSpecificAssociation( const std::string& deviceTypeId, const Json& association ) {
    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::DeviceTypeId ] = deviceTypeId;
    params[ Resources::RPCParams::Association ] = association;

    sendMethodCall( ZWAVE_ADD_DEVICE_SPECIFIC_ASSOCIATION, std::move( params ) );
}

void ZwaveClient::setThermostatMode( uint8_t nodeId,
                                     uint8_t channelId,
                                     uint8_t mode,
                                     std::string manufacturerSpecificData ) {
    if( !znet::utils::hexToBytes( manufacturerSpecificData ) ) {
        throw ValueError( "Invalid manufacturer specific hex data " + manufacturerSpecificData );
    }

    doSimpleRequest(
        ZWAVE_SET_THERMOSTAT_MODE, nodeId, channelId, ZNET_COMMAND_CLASS_THERMOSTAT_MODE, [&]( Json& params ) {
            params[ Resources::RPCParams::Mode ] = std::move( mode );
            params[ Resources::RPCParams::ManufacturerSpecificData ] = std::move( manufacturerSpecificData );
        } );
}

void ZwaveClient::clearDeviceSpecific() {
    sendMethodCall( ZWAVE_CLEAR_DEVICE_SPECIFIC, JSON_EMPTY_OBJ );
}

void ZwaveClient::synchronizeDeviceTime( uint8_t nodeId ) {
    if( !hasNode( nodeId ) ) {
        throw NodeNotFoundError{ nodeId };
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;

    sendMethodCall( ZWAVE_SYNCHRONIZE_DEVICE_TIME, std::move( params ) );
}

void ZwaveClient::setWakeUpInterval( uint8_t nodeId, uint32_t value ) {
    if( !hasNode( nodeId ) ) {
        throw NodeNotFoundError{ nodeId };
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::Value ] = value;

    sendMethodCall( ZWAVE_SET_WAKE_UP_INTERVAL, std::move( params ) );
}

bool ZwaveClient::hasNode( uint8_t nodeId ) const {
    return static_cast< bool >( getNode( nodeId ) );
}

bool ZwaveClient::hasNode( uint8_t nodeId, uint8_t channelId ) const {
    const auto node = getNode( nodeId );
    if( !node )
        return false;

    auto channelOpt = node->getChannel( channelId );
    return channelOpt && *channelOpt;
}

bool ZwaveClient::hasNode( uint8_t nodeId, uint8_t channelId, uint8_t classId ) const {
    const auto node = getNode( nodeId );
    if( !node )
        return false;

    return static_cast< bool >( node->getCommandClass( channelId, classId ) );
}

std::unique_ptr< znet::ZNode > ZwaveClient::getNode( uint8_t nodeId ) const {
    const auto jsonNodeData = getRawNode( nodeId );
    if( jsonNodeData.empty() )
        return nullptr;

    return NodeManager::deserializeNode( jsonNodeData, nullptr, nullptr, nullptr );
}

JsonObject ZwaveClient::getRawNode( uint8_t nodeId ) const {
    return getDBClient().findOne(
        Resources::DB::TableNames::Devices, JSON( Resources::DB::Fields::Id << std::to_string( nodeId ) ), nullptr );
}

const IDbClient& ZwaveClient::getDBClient() const {
    return getHost().getDBClient();
}

void ZwaveClient::doSimpleRequest( MessageSubClasses requestClass,
                                   uint8_t nodeId,
                                   uint8_t channelId,
                                   uint8_t classId,
                                   std::function< void( Json& ) > paramsModifier ) {
    if( !hasNode( nodeId, channelId, classId ) ) {
        throw NodeNotFoundError{ nodeId, channelId, classId };
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ChannelId ] = channelId;

    if( paramsModifier ) {
        paramsModifier( params );
    }

    sendMethodCall( requestClass, std::move( params ) );
}

znet::ProvisioningListEntry ZwaveClient::getProvisioningListEntry( uint8_t nodeId ) const {
    znet::ProvisioningListEntry entry;
    ProvisioningListProvider provider( getHost().getDBClient(), Resources::DB::TableNames::Provisioning );
    if( !provider.findProvisioningListEntryByNodeID( nodeId, entry ) )
        throw ProvisioningListEntryNotFound( nodeId );

    return entry;
}

void ZwaveClient::doMulticastRequest( MessageSubClasses requestClass,
                                      const std::vector< uint8_t >& nodeId,
                                      uint8_t classId,
                                      uint8_t channelId,
                                      Json value,
                                      const std::string& mode ) {
    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::ClassId ] = classId;
    params[ Resources::RPCParams::ChannelId ] = channelId;
    params[ Resources::RPCParams::Value ] = std::move( value );

    if( !mode.empty() ) {
        if( auto modeOpt = znet::stringToMulticastMode( mode.c_str() ) ) {
            params[ Resources::RPCParams::Mode ] = static_cast< uint8_t >( *modeOpt );
        }
        else {
            throw ValueError( "Invalid multicast mode: " + mode );
        }
    }

    LOGGER_DEBUG( "ZwaveClient::doMulticastRequest method call: " + params.dump() );

    sendMethodCall( requestClass, std::move( params ) );
}

}  // namespace Zwave
}  // namespace HubPlatform
