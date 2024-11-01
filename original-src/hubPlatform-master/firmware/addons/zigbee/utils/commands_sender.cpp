#include "zigbee/utils/commands_sender.h"
#include "zigbee/resources/zigbee_resources.h"
#include "zigbee/model/enums/attribute_type.h"
#include "zigbee/model/enums/attributes/thermostat_system_mode.h"
#include "zigbee/model/enums/attributes/thermostat_fan_mode.h"
#include "zigbee/model/enums/commands/ias_ace_arm_response.h"
#include "zigbee/model/enums/commands/ias_ace_panel_status.h"
#include "zigbee/model/enums/commands/ias_ace_alarm_status.h"
#include "zigbee/model/enums/commands/user_status.h"

#include "log_utils/ezlo_log.h"

#include <cassert>

extern "C" {
#include "Zigbee/definitions.h"
#include "protocol/zigbee/app/framework/include/af.h"
#include "protocol/zigbee/app/framework/include/af-types.h"
#include "app/util/zigbee-framework/zigbee-device-common.h"
#include "app/framework/plugin/device-table/device-table.h"
}

namespace HubPlatform {
namespace Zigbee {

template <>
size_t CommandsSender::addDataToMessage< uint16_t >( uint8_t* data, size_t startIndex, uint16_t reportableChange ) {
    data[ startIndex ] = LOW_BYTE( reportableChange );
    data[ ++startIndex ] = HIGH_BYTE( reportableChange );

    return 2;
}

template <>
size_t CommandsSender::addDataToMessage< uint32_t >( uint8_t* data, size_t startIndex, uint32_t reportableChange ) {
    data[ startIndex ] = BYTE_0( reportableChange );
    data[ ++startIndex ] = BYTE_1( reportableChange );
    data[ ++startIndex ] = BYTE_2( reportableChange );
    data[ ++startIndex ] = BYTE_3( reportableChange );

    return 4;
}

template <>
size_t CommandsSender::addDataToMessage< uint8_t >( uint8_t* data, size_t startIndex, uint8_t reportableChange ) {
    data[ startIndex ] = reportableChange;

    return 1;
}

template <>
size_t CommandsSender::addDataToMessage< int8_t >( uint8_t* data, size_t startIndex, int8_t reportableChange ) {
    data[ startIndex ] = reportableChange;

    return 1;
}

template <>
size_t CommandsSender::addDataToMessage< int16_t >( uint8_t* data, size_t startIndex, int16_t reportableChange ) {
    data[ startIndex ] = LOW_BYTE( reportableChange );
    data[ ++startIndex ] = HIGH_BYTE( reportableChange );

    return 2;
}

void CommandsSender::sendCommand( uint16_t nodeId,
                                  uint8_t endpointId,
                                  uint16_t clusterId,
                                  const std::string& commandName,
                                  const Json& commandParams ) {
    switch( static_cast< ClusterId >( clusterId ) ) {
    case ClusterId::OnOff:
        if( !commandParams.empty() ) {
            Log::print( Log::PriorityLevel::WARN, "commandParams will be ignored for %s command", commandName.c_str() );
        }
        sendOnOffClusterCommand( nodeId, endpointId, commandName );
        break;

    case ClusterId::LevelControl:
        sendLevelControlClusterCommand( nodeId, endpointId, commandName, commandParams );
        break;

    case ClusterId::ColorControl:
        sendColorControlClusterCommand( nodeId, endpointId, commandName, commandParams );
        break;

    case ClusterId::IASAce:
        sendIasAceClusterCommand( nodeId, endpointId, commandName, commandParams );
        break;

    case ClusterId::PollControl:
        sendPollControlClusterCommand( nodeId, endpointId, commandName, commandParams );
        break;

    case ClusterId::DoorLock:
        sendDoorLockClusterCommand( nodeId, endpointId, commandName, commandParams );
        break;
    case ClusterId::ApplianceEventsAndAlerts:
        sendGetAlertsResponseCommand( nodeId, endpointId, commandName );
        break;

    default:
        Log::print( Log::PriorityLevel::DEBUG,
                    "Unsupported cluster command: clusterName: %s, commandName: %s",
                    toString( static_cast< ClusterId >( clusterId ) ),
                    commandName.c_str() );
    }
}

void CommandsSender::setAttributeValue( uint16_t nodeId,
                                        uint8_t endpointId,
                                        uint16_t clusterId,
                                        uint16_t attributeId,
                                        uint8_t attributeValueType,
                                        const Json& value ) {
    switch( clusterId ) {
    case ZCL_THERMOSTAT_CLUSTER_ID:
        setThermostatAttribute( nodeId, endpointId, attributeId, attributeValueType, value );
        break;

    case ZCL_FAN_CONTROL_CLUSTER_ID:
        setFanControlAttribute( nodeId, endpointId, attributeId, attributeValueType, value );
        break;

    case ZCL_POLL_CONTROL_CLUSTER_ID:
        setPollControlAttribute( nodeId, endpointId, attributeId, attributeValueType, value );
        break;

    case ZCL_OCCUPANCY_SENSING_CLUSTER_ID:
        setOccupancySensingAttribute( nodeId, endpointId, attributeId, attributeValueType, value );
        break;

    default:
        Log::print( Log::PriorityLevel::DEBUG,
                    "Unsupported cluster attribute for set: clusterName: %s, attributeName: %s",
                    toString( static_cast< ClusterId >( clusterId ) ),
                    toString( static_cast< ClusterId >( clusterId ), attributeId ) );
    }
}

void CommandsSender::sendBindingCommand( uint16_t nodeId, uint8_t endpointId, uint16_t clusterId ) {
    EmberEUI64 destinationEui64, sourceEui64;
    emberAfGetEui64( destinationEui64 );
    emberAfDeviceTableGetEui64FromNodeId( nodeId, sourceEui64 );
    auto status = emberBindRequest( nodeId,       // device who gets this request - SOURCE device, emits data
                                    sourceEui64,  // source device EUI64
                                    endpointId,
                                    clusterId,
                                    UNICAST_BINDING,           // binding type
                                    destinationEui64,          // destination eui IN the binding
                                    0,                         // groupId for new binding
                                    emberAfPrimaryEndpoint(),  // destination endpoint
                                    EMBER_AF_DEFAULT_APS_OPTIONS );

    Log::print( Log::PriorityLevel::DEBUG,
                "sendBindingCommand finished: nodeId: 0x%04x, endpointId: 0x%02x, clusterName: %s, status: - 0x%02x",
                nodeId,
                endpointId,
                toString( static_cast< ClusterId >( clusterId ) ),
                status );
}

void CommandsSender::sendConfigureAttributeReportingCommand( uint16_t nodeId,
                                                             uint8_t endpointId,
                                                             uint16_t clusterId,
                                                             uint16_t attributeId,
                                                             bool isMfgSpecific,
                                                             uint16_t mfgCode ) {
    uint8_t record[ 3 ];
    addDataToMessage< uint8_t >( record, 0, 0x00 );
    addDataToMessage( record, 1, attributeId );

    if( isMfgSpecific ) {
        emberAfFillExternalManufacturerSpecificBuffer(
            ( ZCL_GLOBAL_COMMAND | ZCL_MANUFACTURER_SPECIFIC_MASK | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER ),
            clusterId,
            mfgCode,
            ZCL_CONFIGURE_REPORTING_COMMAND_ID,
            "b",
            record,
            3 );
    }
    else {
        emberAfFillCommandGlobalClientToServerConfigureReporting( clusterId, record, 3 );
    }
    sendCommandWithEndpoint( nodeId, endpointId );

    Log::print( Log::PriorityLevel::DEBUG,
                "Configure attribute finished: nodeId: 0x%04x, endpointId: 0x%02x, clusterName: %s, attributeName: %s,",
                nodeId,
                endpointId,
                toString( static_cast< ClusterId >( clusterId ) ),
                toString( static_cast< ClusterId >( clusterId ), attributeId ) );
}

void CommandsSender::sendConfigureAttributeReportingCommand( uint16_t nodeId,
                                                             uint8_t endpointId,
                                                             uint16_t clusterId,
                                                             uint16_t attributeId,
                                                             AttributeValueType attributeValueType,
                                                             const uint16_t minInterval,
                                                             const uint16_t maxInterval,
                                                             const Json& reportableChange,
                                                             bool isMfgSpecific,
                                                             uint16_t mfgCode ) {
    auto valueTypeInt = static_cast< uint8_t >( attributeValueType );

    if( reportableChange.empty() ) {
        sendConfigureAttributeReportingCommandInternal( nodeId,
                                                        endpointId,
                                                        clusterId,
                                                        attributeId,
                                                        valueTypeInt,
                                                        minInterval,
                                                        maxInterval,
                                                        isMfgSpecific,
                                                        mfgCode );
    }
    else {
        switch( attributeValueType ) {
        case AttributeValueType::Uint16Data:
        case AttributeValueType::Bitmap16Data:
        case AttributeValueType::Enum16Data: {
            uint16_t change = reportableChange.get< uint16_t >();
            sendConfigureAttributeReportingCommandInternal( nodeId,
                                                            endpointId,
                                                            clusterId,
                                                            attributeId,
                                                            valueTypeInt,
                                                            minInterval,
                                                            maxInterval,
                                                            change,
                                                            isMfgSpecific,
                                                            mfgCode );
        } break;

        case AttributeValueType::Int16Data: {
            int16_t change = reportableChange.get< int16_t >();
            sendConfigureAttributeReportingCommandInternal( nodeId,
                                                            endpointId,
                                                            clusterId,
                                                            attributeId,
                                                            valueTypeInt,
                                                            minInterval,
                                                            maxInterval,
                                                            change,
                                                            isMfgSpecific,
                                                            mfgCode );
        } break;

        case AttributeValueType::Uint8Data:
        case AttributeValueType::Bitmap8Data:
        case AttributeValueType::Enum8Data: {
            uint8_t change = reportableChange.get< uint8_t >();
            sendConfigureAttributeReportingCommandInternal( nodeId,
                                                            endpointId,
                                                            clusterId,
                                                            attributeId,
                                                            valueTypeInt,
                                                            minInterval,
                                                            maxInterval,
                                                            change,
                                                            isMfgSpecific,
                                                            mfgCode );
        } break;

        case AttributeValueType::Int8Data: {
            int8_t change = reportableChange.get< int8_t >();
            sendConfigureAttributeReportingCommandInternal( nodeId,
                                                            endpointId,
                                                            clusterId,
                                                            attributeId,
                                                            valueTypeInt,
                                                            minInterval,
                                                            maxInterval,
                                                            change,
                                                            isMfgSpecific,
                                                            mfgCode );
        } break;

        default:
            Log::error( std::string{ "Unsupported attribute value type: " } + toString( attributeValueType ) );
        }
    }

    Log::print( Log::PriorityLevel::DEBUG,
                "Configure attribute finished: nodeId: 0x%04x, endpointId: 0x%02x, clusterName: %s, attributeName: %s, "
                "attributeValueType: 0x%02x, minInterval: %d, maxInterval: %d, reportableChange: %s",
                nodeId,
                endpointId,
                toString( static_cast< ClusterId >( clusterId ) ),
                toString( static_cast< ClusterId >( clusterId ), attributeId ),
                attributeValueType,
                minInterval,
                maxInterval,
                reportableChange.dump().c_str() );
}

bool CommandsSender::requestAttribute( uint16_t nodeId,
                                       uint8_t endpointId,
                                       uint16_t clusterId,
                                       uint16_t attributeId,
                                       bool isMfgSpecific,
                                       uint16_t mfgCode ) {
    std::vector< uint16_t > attributes = { attributeId };
    return requestAttributes( nodeId, endpointId, clusterId, attributes, isMfgSpecific, mfgCode );
}

bool CommandsSender::requestAttributes( uint16_t nodeId,
                                        uint8_t endpointId,
                                        uint16_t clusterId,
                                        const std::vector< uint16_t >& attributes,
                                        bool isMfgSpecific,
                                        uint16_t mfgCode ) {
    std::vector< uint8_t > data;
    for( auto attr : attributes ) {
        data.push_back( LOW_BYTE( attr ) );
        data.push_back( HIGH_BYTE( attr ) );
    }

    if( isMfgSpecific ) {
        emberAfFillExternalManufacturerSpecificBuffer(
            ( ZCL_GLOBAL_COMMAND | ZCL_MANUFACTURER_SPECIFIC_MASK | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER ),
            clusterId,
            mfgCode,
            ZCL_READ_ATTRIBUTES_COMMAND_ID,
            "b",
            &data.front(),
            data.size() * sizeof( uint8_t ) );
    }
    else {
        emberAfFillCommandGlobalClientToServerReadAttributes(
            clusterId, &data.front(), data.size() * sizeof( uint8_t ) );
    }

    // TODO determine source endpoint correctly or leave as 0x01
    emberAfSetCommandEndpoints( 1, endpointId );
    auto status = emberAfSendCommandUnicast( EMBER_OUTGOING_DIRECT, nodeId );
    return ( EMBER_SUCCESS == status );
}

bool CommandsSender::discoverAttributes( uint16_t nodeId,
                                         uint8_t endpointId,
                                         uint16_t clusterId,
                                         uint16_t startAttribute,
                                         uint8_t maxAttributesCount,
                                         bool isMfgSpecific,
                                         uint16_t mfgCode ) {
    if( isMfgSpecific ) {
        emberAfFillExternalManufacturerSpecificBuffer(
            ( ZCL_GLOBAL_COMMAND | ZCL_MANUFACTURER_SPECIFIC_MASK | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER ),
            clusterId,
            mfgCode,
            ZCL_DISCOVER_ATTRIBUTES_COMMAND_ID,
            "vu",
            startAttribute,
            maxAttributesCount );
    }
    else {
        emberAfFillCommandGlobalClientToServerDiscoverAttributes( clusterId, startAttribute, maxAttributesCount );
    }

    emberAfSetCommandEndpoints( endpointId, endpointId );
    auto status = emberAfSendCommandUnicast( EMBER_OUTGOING_DIRECT, nodeId );
    return EMBER_SUCCESS == status;
}

bool CommandsSender::discoverCommandsReceived( uint16_t nodeId,
                                               uint8_t endpointId,
                                               uint16_t clusterId,
                                               uint16_t startCommand,
                                               uint8_t maxCommandsCount,
                                               CommandDirection direction,
                                               bool isMfgSpecific,
                                               uint16_t mfgCode ) {
    // TODO support mfg specific request

    if( direction == CommandDirection::ClientToServer ) {
        emberAfFillCommandGlobalClientToServerDiscoverCommandsReceived( clusterId, startCommand, maxCommandsCount );
    }
    else {
        assert( direction == CommandDirection::ServerToClient );
        emberAfFillCommandGlobalServerToClientDiscoverCommandsReceived( clusterId, startCommand, maxCommandsCount );
    }

    emberAfSetCommandEndpoints( endpointId, endpointId );
    auto status = emberAfSendCommandUnicast( EMBER_OUTGOING_DIRECT, nodeId );
    return EMBER_SUCCESS == status;
}

bool CommandsSender::discoverCommandsGenerated( uint16_t nodeId,
                                                uint8_t endpointId,
                                                uint16_t clusterId,
                                                uint16_t startCommand,
                                                uint8_t maxCommandsCount,
                                                CommandDirection direction,
                                                bool isMfgSpecific,
                                                uint16_t mfgCode ) {
    // TODO support mfg specific request

    if( direction == CommandDirection::ClientToServer ) {
        emberAfFillCommandGlobalClientToServerDiscoverCommandsGenerated( clusterId, startCommand, maxCommandsCount );
    }
    else {
        assert( direction == CommandDirection::ServerToClient );
        emberAfFillCommandGlobalServerToClientDiscoverCommandsGenerated( clusterId, startCommand, maxCommandsCount );
    }

    emberAfSetCommandEndpoints( endpointId, endpointId );
    auto status = emberAfSendCommandUnicast( EMBER_OUTGOING_DIRECT, nodeId );
    return EMBER_SUCCESS == status;
}

void CommandsSender::sendOnOffClusterCommand( uint16_t nodeId, uint8_t endpointId, const std::string& commandName ) {
    if( commandName == Resources::Fields::Command::Names::OnOff::On ) {
        sendOnOffClusterOnCommand( nodeId, endpointId );
    }
    else if( commandName == Resources::Fields::Command::Names::OnOff::Off ) {
        sendOnOffClusterOffCommand( nodeId, endpointId );
    }
    else {
        Log::print( Log::PriorityLevel::ERROR, "Unsupported On/Off command name: %s", commandName.c_str() );
    }
}

void CommandsSender::sendOnOffClusterOnCommand( uint16_t nodeId, uint8_t endpointId ) {
    emberAfFillCommandOnOffClusterOn();
    sendCommandWithEndpoint( nodeId, endpointId );
}

void CommandsSender::sendOnOffClusterOffCommand( uint16_t nodeId, uint8_t endpointId ) {
    emberAfFillCommandOnOffClusterOff();
    sendCommandWithEndpoint( nodeId, endpointId );
}

void CommandsSender::sendLevelControlClusterCommand( uint16_t nodeId,
                                                     uint8_t endpointId,
                                                     const std::string& commandName,
                                                     const Json& commandParams ) {
    if( commandName == Resources::Fields::Command::Names::LevelControl::MoveToLevelWithOnOff ) {
        sendLevelControlClusterMoveToLevelCommand( nodeId, endpointId, commandParams.get< uint8_t >() );
    }
    else if( commandName == Resources::Fields::Command::Names::LevelControl::MoveWithOnOff ) {
        sendLevelControlClusterMoveCommand( nodeId, endpointId, commandParams.get< std::string >() );
    }
    else if( commandName == Resources::Fields::Command::Names::LevelControl::StopWithOnOff ) {
        sendLevelControlClusterStopMovingCommand( nodeId, endpointId );
    }
    else {
        Log::print( Log::PriorityLevel::ERROR, "Unsupported Level Control command name: %s", commandName.c_str() );
    }
}

void CommandsSender::sendLevelControlClusterMoveToLevelCommand( uint16_t nodeId, uint8_t endpointId, uint8_t level ) {
    emberAfFillCommandLevelControlClusterMoveToLevelWithOnOff( level, 0xffff );
    sendCommandWithEndpoint( nodeId, endpointId );
}

void CommandsSender::sendLevelControlClusterMoveCommand( uint16_t nodeId,
                                                         uint8_t endpointId,
                                                         const std::string& direction ) {
    uint8_t convertedDirection;
    if( direction == Resources::CommandParams::LevelControlCluster::UpParam ) {
        convertedDirection = 0x00;
    }
    else if( direction == Resources::CommandParams::LevelControlCluster::DownParam ) {
        convertedDirection = 0x01;
    }
    else {
        Log::print( Log::PriorityLevel::ERROR, "Unsupported Level Control direction: %s", direction.c_str() );
        return;
    }

    emberAfFillCommandLevelControlClusterMoveWithOnOff( convertedDirection, 0xff );
    sendCommandWithEndpoint( nodeId, endpointId );
}

void CommandsSender::sendLevelControlClusterStopMovingCommand( uint16_t nodeId, uint8_t endpointId ) {
    emberAfFillCommandLevelControlClusterStopWithOnOff();
    sendCommandWithEndpoint( nodeId, endpointId );
}

void CommandsSender::sendColorControlClusterCommand( uint16_t nodeId,
                                                     uint8_t endpointId,
                                                     const std::string& commandName,
                                                     const Json& commandParams ) {
    if( commandName == Resources::Fields::Command::Names::ColorControl::MoveToColor ) {
        sendColorControlClusterMoveToColorCommand(
            nodeId,
            endpointId,
            commandParams[ Resources::CommandParams::ColorControlCluster::CurrentX ].get< uint16_t >(),
            commandParams[ Resources::CommandParams::ColorControlCluster::CurrentY ].get< uint16_t >() );
    }
    else if( commandName == Resources::Fields::Command::Names::ColorControl::MoveToColorTemperature ) {
        sendColorControlClusterMoveToTemperatureCommand( nodeId, endpointId, commandParams.get< uint16_t >() );
    }
    else {
        Log::print( Log::PriorityLevel::ERROR, "Unsupported Color Control command name: %s", commandName.c_str() );
    }
}

void CommandsSender::sendColorControlClusterMoveToColorCommand( uint16_t nodeId,
                                                                uint8_t endpointId,
                                                                uint16_t currentX,
                                                                uint16_t currentY ) {
    emberAfFillExternalBuffer( ( ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER ),
                               ZCL_COLOR_CONTROL_CLUSTER_ID,
                               ZCL_MOVE_TO_COLOR_COMMAND_ID,
                               "vvv",
                               currentX,
                               currentY,
                               2 );

    sendCommandWithEndpoint( nodeId, endpointId );
}

void CommandsSender::sendColorControlClusterMoveToTemperatureCommand( uint16_t nodeId,
                                                                      uint8_t endpointId,
                                                                      uint16_t temperature ) {
    emberAfFillExternalBuffer( ( ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER ),
                               ZCL_COLOR_CONTROL_CLUSTER_ID,
                               ZCL_MOVE_TO_COLOR_TEMPERATURE_COMMAND_ID,
                               "vv",
                               temperature,
                               2 );

    sendCommandWithEndpoint( nodeId, endpointId );
}

void CommandsSender::setThermostatAttribute( uint16_t nodeId,
                                             uint8_t endpointId,
                                             uint16_t attributeId,
                                             uint8_t attributeValueType,
                                             const Json& value ) {
    switch( static_cast< ThermostatClusterAttributeId >( attributeId ) ) {
    case ThermostatClusterAttributeId::SystemMode:
        setThermostatSystemMode( nodeId, endpointId, attributeId, attributeValueType, value );
        break;

    case ThermostatClusterAttributeId::OccupiedCoolingSetpoint:
    case ThermostatClusterAttributeId::OccupiedHeatingSetpoint:
        setThermostatSetpoint( nodeId, endpointId, attributeId, attributeValueType, value );
        break;

    case ThermostatClusterAttributeId::LocalTemperatureCalibration:
        setThermostatLocalTemperatureCalibration( nodeId, endpointId, attributeId, attributeValueType, value );
        break;

    default:
        Log::print( Log::PriorityLevel::DEBUG,
                    "Unsupported thermostat attribute to set: %s",
                    toString( ClusterId::Thermostat, attributeId ) );
    }
}

void CommandsSender::setThermostatSystemMode( uint16_t nodeId,
                                              uint8_t endpointId,
                                              uint16_t attributeId,
                                              uint8_t attributeValueType,
                                              const Json& value ) {
    ThermostatSystemMode mode = thermostatSystemModeFromString( value.get< std::string >() );
    assert( mode != ThermostatSystemMode::Unknown );

    uint8_t record[ 4 ];
    addDataToMessage( record, 0, attributeId );
    addDataToMessage( record, 2, attributeValueType );
    addDataToMessage( record, 3, static_cast< uint8_t >( mode ) );

    emberAfFillCommandGlobalClientToServerWriteAttributes( ZCL_THERMOSTAT_CLUSTER_ID, record, 4 );

    sendCommandWithEndpoint( nodeId, endpointId );
}

void CommandsSender::setThermostatSetpoint( uint16_t nodeId,
                                            uint8_t endpointId,
                                            uint16_t attributeId,
                                            uint8_t attributeValueType,
                                            const Json& value ) {
    int16_t data = value.get< int16_t >();

    uint8_t record[ 5 ];
    addDataToMessage( record, 0, attributeId );
    addDataToMessage( record, 2, attributeValueType );
    addDataToMessage( record, 3, data );

    emberAfFillCommandGlobalClientToServerWriteAttributes( ZCL_THERMOSTAT_CLUSTER_ID, record, 5 );

    sendCommandWithEndpoint( nodeId, endpointId );
}

void CommandsSender::setThermostatLocalTemperatureCalibration( uint16_t nodeId,
                                                               uint8_t endpointId,
                                                               uint16_t attributeId,
                                                               uint8_t attributeValueType,
                                                               const Json& value ) {
    int8_t data = value.get< int8_t >();

    uint8_t record[ 4 ];
    addDataToMessage( record, 0, attributeId );
    addDataToMessage( record, 2, attributeValueType );
    addDataToMessage( record, 3, data );

    emberAfFillCommandGlobalClientToServerWriteAttributes( ZCL_THERMOSTAT_CLUSTER_ID, record, 4 );

    sendCommandWithEndpoint( nodeId, endpointId );
}

void CommandsSender::setFanControlAttribute( uint16_t nodeId,
                                             uint8_t endpointId,
                                             uint16_t attributeId,
                                             uint8_t attributeValueType,
                                             const Json& value ) {
    switch( static_cast< FanControlClusterAttributeId >( attributeId ) ) {
    case FanControlClusterAttributeId::FanMode:
        setFanControlFanMode( nodeId, endpointId, attributeId, attributeValueType, value );
        break;

    default:
        Log::print( Log::PriorityLevel::DEBUG,
                    "Unsupported fan control attribute to set: %s",
                    toString( ClusterId::FanControl, attributeId ) );
    }
}

void CommandsSender::setFanControlFanMode( uint16_t nodeId,
                                           uint8_t endpointId,
                                           uint16_t attributeId,
                                           uint8_t attributeValueType,
                                           const Json& value ) {
    ThermostatFanMode mode = thermostatFanModeFromString( value.get< std::string >() );
    assert( mode != ThermostatFanMode::Unknown );

    uint8_t record[ 4 ];
    addDataToMessage( record, 0, attributeId );
    addDataToMessage( record, 2, attributeValueType );
    addDataToMessage( record, 3, static_cast< uint8_t >( mode ) );

    emberAfFillCommandGlobalClientToServerWriteAttributes( ZCL_FAN_CONTROL_CLUSTER_ID, record, 4 );

    sendCommandWithEndpoint( nodeId, endpointId );
}

void CommandsSender::setPollControlAttribute( uint16_t nodeId,
                                              uint8_t endpointId,
                                              uint16_t attributeId,
                                              uint8_t attributeValueType,
                                              const Json& value ) {
    switch( static_cast< PollControlClusterAttributeId >( attributeId ) ) {
    case PollControlClusterAttributeId::CheckInInterval:
        setPollControlCheckInIntervalAttribute( nodeId, endpointId, attributeId, attributeValueType, value );
        break;

    default:
        Log::print( Log::PriorityLevel::DEBUG,
                    "Unsupported poll control attribute to set: %s",
                    toString( ClusterId::FanControl, attributeId ) );
    }
}

void CommandsSender::setPollControlCheckInIntervalAttribute( uint16_t nodeId,
                                                             uint8_t endpointId,
                                                             uint16_t attributeId,
                                                             uint8_t attributeValueType,
                                                             const Json& value ) {
    uint8_t record[ 7 ];
    addDataToMessage( record, 0, attributeId );
    addDataToMessage( record, 2, attributeValueType );
    addDataToMessage( record, 3, value.get< uint32_t >() );

    emberAfFillCommandGlobalClientToServerWriteAttributes( ZCL_POLL_CONTROL_CLUSTER_ID, record, 7 );

    sendCommandWithEndpoint( nodeId, endpointId );
}

void CommandsSender::sendPollControlClusterCommand( uint16_t nodeId,
                                                    uint8_t endpointId,
                                                    const std::string& commandName,
                                                    const Json& commandParams ) {
    if( commandName == Resources::Fields::Command::Names::PollControl::CheckInResponse ) {
        sendPollControlClusterCheckInResponseCommand(
            nodeId,
            endpointId,
            commandParams[ Resources::CommandParams::PollControlCluster::StartFastPollingMode ].get< bool >(),
            commandParams[ Resources::CommandParams::PollControlCluster::FastPollTimeout ].get< uint16_t >() );
    }
    else if( commandName == Resources::Fields::Command::Names::PollControl::FastPollStop ) {
        sendPollControlClusterFastPollStopCommand( nodeId, endpointId );
    }
    else {
        Log::print( Log::PriorityLevel::ERROR, "Unsupported Poll Control command name: %s", commandName.c_str() );
    }
}

void CommandsSender::sendGetAlertsResponseCommand( uint16_t nodeId,
                                                   uint8_t endpointId,
                                                   const std::string& commandName ) {
    if( commandName == Resources::Fields::Command::Names::ApplianceEventsAndAlerts::GetAlertsResponse ) {
        emberAfFillCommandApplianceEventsAndAlertClusterGetAlerts();
        sendCommandWithEndpoint( nodeId, endpointId );
    }
    else {
        Log::print( Log::PriorityLevel::ERROR, "Unsupported Alerts Response command name: %s", commandName.c_str() );
    }
}

void CommandsSender::sendPollControlClusterCheckInResponseCommand( uint16_t nodeId,
                                                                   uint8_t endpointId,
                                                                   bool startFastPollingMode,
                                                                   uint16_t fastPollTimeout ) {
    emberAfFillCommandPollControlClusterCheckInResponse( startFastPollingMode, fastPollTimeout );
    sendCommandWithEndpoint( nodeId, endpointId );
}

void CommandsSender::sendPollControlClusterFastPollStopCommand( uint16_t nodeId, uint8_t endpointId ) {
    emberAfFillCommandPollControlClusterFastPollStop();
    sendCommandWithEndpoint( nodeId, endpointId );
}

void CommandsSender::sendIasAceClusterCommand( uint16_t nodeId,
                                               uint8_t endpointId,
                                               const std::string& commandName,
                                               const Json& commandParams ) {
    if( commandName == Resources::Fields::Command::Names::IasAce::ArmResponse ) {
        sendIasAceClusterArmResponseCommand(
            nodeId, endpointId, iasAceArmResponseFromString( commandParams.get< std::string >() ) );
    }
    else if( commandName == Resources::Fields::Command::Names::IasAce::GetPanelStatusResponse ) {
        sendIasAceClusterGetPanelStatusResponseCommand(
            nodeId, endpointId, iasAcePanelStatusFromString( commandParams.get< std::string >() ) );
    }
    else if( commandName == Resources::Fields::Command::Names::IasAce::PanelStatusChanged ) {
        auto secondsRemainingIt = commandParams.find( Resources::CommandParams::IasAceCluster::SecondsRemaining );
        auto alarmStatusIt = commandParams.find( Resources::CommandParams::IasAceCluster::AlarmStatus );

        sendIasAceClusterPanelStatusChangedCommand(
            nodeId,
            endpointId,
            iasAcePanelStatusFromString(
                commandParams[ Resources::CommandParams::IasAceCluster::PanelStatus ].get< std::string >() ),
            secondsRemainingIt != commandParams.end() ? secondsRemainingIt->get< uint8_t >() : 0,
            alarmStatusIt != commandParams.end() ? iasAceAlarmStatusFromString( alarmStatusIt->get< std::string >() ) :
                                                   IasAceAlarmStatus::NoAlarm );
    }
    else {
        Log::print( Log::PriorityLevel::ERROR, "Unsupported IAS Ace command name: %s", commandName.c_str() );
    }
}

void CommandsSender::sendIasAceClusterArmResponseCommand( uint16_t nodeId,
                                                          uint8_t endpointId,
                                                          const IasAceArmResponse& status ) {
    if( status == IasAceArmResponse::Unknown ) {
        return;
    }

    emberAfFillCommandIasAceClusterArmResponse( static_cast< uint8_t >( status ) );

    sendCommandWithEndpoint( nodeId, endpointId );
}

void CommandsSender::sendIasAceClusterGetPanelStatusResponseCommand( uint16_t nodeId,
                                                                     uint8_t endpointId,
                                                                     const IasAcePanelStatus& status ) {
    if( status == IasAcePanelStatus::Unknown ) {
        return;
    }

    emberAfFillCommandIasAceClusterGetPanelStatusResponse( static_cast< uint8_t >( status ), 0x00, 0x01, 0x00 );

    sendCommandWithEndpoint( nodeId, endpointId );
}

void CommandsSender::sendIasAceClusterPanelStatusChangedCommand( uint16_t nodeId,
                                                                 uint8_t endpointId,
                                                                 const IasAcePanelStatus& status,
                                                                 uint8_t secondsReamining,
                                                                 const IasAceAlarmStatus& alarmStatus ) {
    if( status == IasAcePanelStatus::Unknown || alarmStatus == IasAceAlarmStatus::Unknown ) {
        return;
    }

    emberAfFillCommandIasAceClusterPanelStatusChanged(
        static_cast< uint8_t >( status ), secondsReamining, 0x01, static_cast< uint8_t >( alarmStatus ) );

    sendCommandWithEndpoint( nodeId, endpointId );
}

void CommandsSender::sendDoorLockClusterCommand( uint16_t nodeId,
                                                 uint8_t endpointId,
                                                 const std::string& commandName,
                                                 const Json& commandParams ) {
    if( commandName == Resources::Fields::Command::Names::DoorLock::LockDoor ) {
        sendDoorLockClusterLockDoorCommand(
            nodeId, endpointId, commandParams.empty() ? "" : commandParams.get< std::string >() );
    }
    else if( commandName == Resources::Fields::Command::Names::DoorLock::UnlockDoor ) {
        sendDoorLockClusterUnlockDoorCommand(
            nodeId, endpointId, commandParams.empty() ? "" : commandParams.get< std::string >() );
    }
    else if( commandName == Resources::Fields::Command::Names::DoorLock::SetPinCode ) {
        sendDoorLockClusterSetPinCodeCommand(
            nodeId,
            endpointId,
            commandParams[ Resources::CommandParams::DoorLockCluster::UserId ].get< uint16_t >(),
            userStatusFromString(
                commandParams[ Resources::CommandParams::DoorLockCluster::UserStatus ].get< std::string >() ),
            commandParams[ Resources::CommandParams::DoorLockCluster::PinCode ].get< std::string >() );
    }
    else if( commandName == Resources::Fields::Command::Names::DoorLock::GetPinCode ) {
        sendDoorLockClusterGetPinCodeCommand( nodeId, endpointId, commandParams.get< uint16_t >() );
    }
    else if( commandName == Resources::Fields::Command::Names::DoorLock::ClearPinCode ) {
        sendDoorLockClusterClearPinCodeCommand( nodeId, endpointId, commandParams.get< uint16_t >() );
    }
    else if( commandName == Resources::Fields::Command::Names::DoorLock::ClearAllPinCodes ) {
        sendDoorLockClusterClearAllPinCodesCommand( nodeId, endpointId );
    }
    else {
        Log::print( Log::PriorityLevel::ERROR, "Unsupported Door Lock command name: %s", commandName.c_str() );
    }
}

void CommandsSender::sendDoorLockClusterLockDoorCommand( uint16_t nodeId,
                                                         uint8_t endpointId,
                                                         const std::string& pinCode ) {
    emberAfFillCommandDoorLockClusterLockDoor( pinCode.c_str() );
    sendCommandWithEndpoint( nodeId, endpointId );
}

void CommandsSender::sendDoorLockClusterUnlockDoorCommand( uint16_t nodeId,
                                                           uint8_t endpointId,
                                                           const std::string& pinCode ) {
    emberAfFillCommandDoorLockClusterUnlockDoor( pinCode.c_str() );
    sendCommandWithEndpoint( nodeId, endpointId );
}

void CommandsSender::sendDoorLockClusterSetPinCodeCommand( uint16_t nodeId,
                                                           uint8_t endpointId,
                                                           uint16_t userId,
                                                           UserStatus userStatus,
                                                           const std::string& pinCode ) {
    if( userStatus != UserStatus::OccupiedEnabled && userStatus != UserStatus::OccupiedDisabled ) {
        Log::debug( std::string( "Unsupported user status to set pin code: " ) + toString( userStatus ) );
        return;
    }

    std::string octet = static_cast< char >( pinCode.length() ) + pinCode;
    emberAfFillCommandDoorLockClusterSetPin( userId, userStatus, 0, octet.c_str() );
    sendCommandWithEndpoint( nodeId, endpointId );
}

void CommandsSender::sendDoorLockClusterGetPinCodeCommand( uint16_t nodeId, uint8_t endpointId, uint16_t userId ) {
    emberAfFillCommandDoorLockClusterGetPin( userId );
    sendCommandWithEndpoint( nodeId, endpointId );
}

void CommandsSender::sendDoorLockClusterClearPinCodeCommand( uint16_t nodeId, uint8_t endpointId, uint16_t userId ) {
    emberAfFillCommandDoorLockClusterClearPin( userId );
    sendCommandWithEndpoint( nodeId, endpointId );
}

void CommandsSender::sendDoorLockClusterClearAllPinCodesCommand( uint16_t nodeId, uint8_t endpointId ) {
    emberAfFillCommandDoorLockClusterClearAllPins();
    sendCommandWithEndpoint( nodeId, endpointId );
}

void CommandsSender::setOccupancySensingAttribute( uint16_t nodeId,
                                                   uint8_t endpointId,
                                                   uint16_t attributeId,
                                                   uint8_t attributeValueType,
                                                   const Json& value ) {
    uint8_t record[ 5 ];
    addDataToMessage( record, 0, attributeId );
    addDataToMessage( record, 2, attributeValueType );

    uint8_t length = 0;
    switch( static_cast< OccupancySensingClusterAttributeId >( attributeId ) ) {
    case OccupancySensingClusterAttributeId::PIRUnoccupiedToOccupiedThreshold:
    case OccupancySensingClusterAttributeId::UltrasonicUnoccupiedToOccupiedThreshold:
    case OccupancySensingClusterAttributeId::PhysicalContactUnoccupiedToOccupiedThreshold:
        length = 4;
        addDataToMessage( record, 3, value.get< int8_t >() );
        break;

    default:
        length = 5;
        addDataToMessage( record, 3, value.get< int16_t >() );
    }

    emberAfFillCommandGlobalClientToServerWriteAttributes( ZCL_OCCUPANCY_SENSING_CLUSTER_ID, record, length );

    sendCommandWithEndpoint( nodeId, endpointId );
}

template < typename DataType >
void CommandsSender::sendConfigureAttributeReportingCommandInternal( uint16_t nodeId,
                                                                     uint8_t endpointId,
                                                                     uint16_t clusterId,
                                                                     uint16_t attributeId,
                                                                     uint8_t attributeValueType,
                                                                     const uint16_t minInterval,
                                                                     const uint16_t maxInterval,
                                                                     DataType reportableChange,
                                                                     bool isMfgSpecific,
                                                                     uint16_t mfgCode ) {
    uint8_t record[ 20 ];
    size_t length = addDataToMessage< uint8_t >( record, 0, 0x00 );
    length += addDataToMessage( record, length, attributeId );
    length += addDataToMessage( record, length, attributeValueType );
    length += addDataToMessage( record, length, minInterval );
    length += addDataToMessage( record, length, maxInterval );
    length += addDataToMessage( record, length, reportableChange );

    if( isMfgSpecific ) {
        emberAfFillExternalManufacturerSpecificBuffer(
            ( ZCL_GLOBAL_COMMAND | ZCL_MANUFACTURER_SPECIFIC_MASK | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER ),
            clusterId,
            mfgCode,
            ZCL_CONFIGURE_REPORTING_COMMAND_ID,
            "b",
            record,
            length );
    }
    else {
        emberAfFillCommandGlobalClientToServerConfigureReporting( clusterId, record, length );
    }
    sendCommandWithEndpoint( nodeId, endpointId );
}

void CommandsSender::sendConfigureAttributeReportingCommandInternal( uint16_t nodeId,
                                                                     uint8_t endpointId,
                                                                     uint16_t clusterId,
                                                                     uint16_t attributeId,
                                                                     uint8_t attributeValueType,
                                                                     const uint16_t minInterval,
                                                                     const uint16_t maxInterval,
                                                                     bool isMfgSpecific,
                                                                     uint16_t mfgCode ) {
    uint8_t record[ 20 ];
    size_t length = addDataToMessage< uint8_t >( record, 0, 0x00 );
    length += addDataToMessage( record, length, attributeId );
    length += addDataToMessage( record, length, attributeValueType );
    length += addDataToMessage( record, length, minInterval );
    length += addDataToMessage( record, length, maxInterval );

    if( isMfgSpecific ) {
        emberAfFillExternalManufacturerSpecificBuffer(
            ( ZCL_GLOBAL_COMMAND | ZCL_MANUFACTURER_SPECIFIC_MASK | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER ),
            clusterId,
            mfgCode,
            ZCL_CONFIGURE_REPORTING_COMMAND_ID,
            "b",
            record,
            length );
    }
    else {
        emberAfFillCommandGlobalClientToServerConfigureReporting( clusterId, record, length );
    }
    sendCommandWithEndpoint( nodeId, endpointId );
}

template < typename DataType >
size_t CommandsSender::addDataToMessage( uint8_t* data, size_t startIndex, DataType reportableChange ) {
    assert( !"Unsupported data type" );
    return 0;
}

void CommandsSender::sendCommandWithEndpoint( uint16_t nodeId, uint8_t endpointId ) {
    emberAfSetCommandEndpoints( 1, endpointId );
    emberAfSendCommandUnicast( EMBER_OUTGOING_DIRECT, nodeId );
}

}  // namespace Zigbee
}  // namespace HubPlatform
