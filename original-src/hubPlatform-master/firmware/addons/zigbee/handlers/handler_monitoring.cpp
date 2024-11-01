#include "zigbee/handlers/handler_monitoring.h"
#include "zigbee/model/zigbee_endpoint.h"
#include "zigbee/utils/attributes_parser.h"

#include "zigbee/model/enums/node_type.h"

#include "zigbee/zigbee_task_loop.h"
#include "zigbee/zigbee_log.h"

#include "database_client/idbclient.h"
#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"

#include <tiny/tiny_consolecolors.h>

extern "C" {
#include "app/framework/plugin/device-table/device-table.h"
#include "app/framework/plugin/device-table/device-table-internal.h"
#include "app/framework/plugin/device-database/device-database.h"
#include "protocol/zigbee/stack/include/child.h"
}

namespace HubPlatform {
namespace Zigbee {
namespace {
static const std::chrono::seconds ForceRemoveTimeoutSeconds( 8 );
}  // namespace

HandlerMonitoring::HandlerMonitoring( ZigbeeAddonCallbackInterface& callbackInterface,
                                      CommandsSenderInterface& commandsSender,
                                      std::chrono::system_clock::duration reachableStateTimeout )
: m_callbackInterface( callbackInterface )
, m_commandsSender( commandsSender )
, m_reachableStateTimeout( reachableStateTimeout )
, m_forceRemoveNodeId( EMBER_BROADCAST_ADDRESS )
, m_sendLeaveTimer( [this]() {
    ZigbeeTaskLoop::getInstance().addTask( [this]() {
        Log::debug( "Device force remove: NOT reachable (send timeout ) - " + std::to_string( m_forceRemoveNodeId ) );
        if( m_forceRemoveNodeId == EMBER_BROADCAST_ADDRESS ) {
            return;
        }

        // wipe device table
        auto index = emberAfDeviceTableGetIndexFromNodeId( m_forceRemoveNodeId );
        assert( index != EMBER_AF_PLUGIN_DEVICE_TABLE_NULL_INDEX );

        auto deviceInfo = emberAfDeviceTableFindDeviceTableEntry( index );
        emberAfPluginDeviceDatabaseEraseDevice( deviceInfo->eui64 );
        while( index != EMBER_AF_PLUGIN_DEVICE_TABLE_NULL_INDEX ) {
            emAfPluginDeviceTableDeleteEntry( index );
            index = emAfDeviceTableFindNextEndpoint( index );
        }
        emAfDeviceTableSave();
        m_callbackInterface.nodeRemoved( m_forceRemoveNodeId );
        m_forceRemoveNodeId = EMBER_BROADCAST_ADDRESS;
    } );
} ) {}

HandlerMonitoring::~HandlerMonitoring() {
    m_sendLeaveTimer.stop();
    for( const auto& timerData : m_reachableTimers ) {
        timerData.second->stop();
    }
}

ZigbeeAddonCallbackInterface& HandlerMonitoring::getCallbackInterface() {
    return m_callbackInterface;
}

CommandsSenderInterface& HandlerMonitoring::getCommandsSender() {
    return m_commandsSender;
}

void HandlerMonitoring::sendLeaveCommand( EmberNodeId nodeId ) {
    auto tableIndex = emberAfDeviceTableGetIndexFromNodeId( nodeId );
    if( tableIndex != EMBER_AF_PLUGIN_DEVICE_TABLE_NULL_INDEX ) {
        emberAfPluginDeviceTableSendLeave( tableIndex );
        m_forceRemoveNodeId = nodeId;
        m_sendLeaveTimer.start( ForceRemoveTimeoutSeconds, true );
    }
    else {
        m_callbackInterface.nodeRemoved( nodeId );
    }
}

void HandlerMonitoring::sendCommand( uint16_t nodeId,
                                     uint8_t endpointId,
                                     uint16_t clusterId,
                                     const std::string& commandName,
                                     const Json& commandParams ) {
    Log::print(
        Log::PriorityLevel::DEBUG,
        "Try to send command: nodeId: 0x%04x, endpointId: 0x%02x, clusterName: %s, commandName: %s, commandParams: %s",
        nodeId,
        endpointId,
        toString( static_cast< ClusterId >( clusterId ) ),
        commandName.c_str(),
        commandParams.dump().c_str() );

    m_commandsSender.sendCommand( nodeId, endpointId, clusterId, commandName, commandParams );

    if( clusterId != ZCL_IAS_ACE_CLUSTER_ID && clusterId != ZCL_POLL_CONTROL_CLUSTER_ID ) {
        initReachableTimer( nodeId );
    }
}

void HandlerMonitoring::setAttributeValue( uint16_t nodeId,
                                           uint8_t endpointId,
                                           uint16_t clusterId,
                                           uint16_t attributeId,
                                           AttributeValueType attributeValueType,
                                           const Json& value ) {
    Log::print(
        Log::PriorityLevel::DEBUG,
        "Try to set attribute value: nodeId: 0x%04x, endpointId: 0x%02x, clusterName: %s, attributeName: %s, value: %s",
        nodeId,
        endpointId,
        toString( static_cast< ClusterId >( clusterId ) ),
        toString( static_cast< ClusterId >( clusterId ), attributeId ),
        value.dump().c_str() );

    m_commandsSender.setAttributeValue(
        nodeId, endpointId, clusterId, attributeId, static_cast< uint8_t >( attributeValueType ), value );
    initReachableTimer( nodeId );
}

void HandlerMonitoring::requestAttributeValue( uint16_t nodeId,
                                               uint8_t endpointId,
                                               uint16_t clusterId,
                                               uint16_t attributeId,
                                               bool isMfgSpecific,
                                               uint16_t mfgCode ) {
    Log::print(
        Log::PriorityLevel::DEBUG,
        "Try to request attribute value: nodeId: 0x%04x, endpointId: 0x%02x, clusterName: %s, attributeName: %s",
        nodeId,
        endpointId,
        toString( static_cast< ClusterId >( clusterId ) ),
        toString( static_cast< ClusterId >( clusterId ), attributeId ) );

    m_commandsSender.requestAttribute( nodeId, endpointId, clusterId, attributeId, isMfgSpecific, mfgCode );
    initReachableTimer( nodeId );
}

void HandlerMonitoring::configureAttributeReport( uint16_t nodeId,
                                                  uint8_t endpointId,
                                                  uint16_t clusterId,
                                                  uint16_t attributeId,
                                                  bool isMfgSpecific,
                                                  uint16_t mfgCode ) {
    Log::print( Log::PriorityLevel::DEBUG,
                "Try to configure attribute: nodeId: 0x%04x, endpointId: 0x%02x, clusterName: %s, attributeName: %s",
                nodeId,
                endpointId,
                toString( static_cast< ClusterId >( clusterId ) ),
                toString( static_cast< ClusterId >( clusterId ), attributeId ) );

    m_commandsSender.sendConfigureAttributeReportingCommand(
        nodeId, endpointId, clusterId, attributeId, isMfgSpecific, mfgCode );
    initReachableTimer( nodeId );
}

void HandlerMonitoring::configureAttributeReport( uint16_t nodeId,
                                                  uint8_t endpointId,
                                                  uint16_t clusterId,
                                                  uint16_t attributeId,
                                                  AttributeValueType attributeValueType,
                                                  const uint16_t minInterval,
                                                  const uint16_t maxInterval,
                                                  const Json& reportableChange,
                                                  bool isMfgSpecific,
                                                  uint16_t mfgCode ) {
    Log::print( Log::PriorityLevel::DEBUG,
                "Try to configure attribute: nodeId: 0x%04x, endpointId: 0x%02x, clusterName: %s, attributeName: %s, "
                "attributeValueType: %s, minInterval: %d, maxInterval: %d, reportableChange: %s",
                nodeId,
                endpointId,
                toString( static_cast< ClusterId >( clusterId ) ),
                toString( static_cast< ClusterId >( clusterId ), attributeId ),
                toString( attributeValueType ),
                minInterval,
                maxInterval,
                reportableChange.dump().c_str() );

    m_commandsSender.sendConfigureAttributeReportingCommand( nodeId,
                                                             endpointId,
                                                             clusterId,
                                                             attributeId,
                                                             attributeValueType,
                                                             minInterval,
                                                             maxInterval,
                                                             reportableChange,
                                                             isMfgSpecific,
                                                             mfgCode );
    initReachableTimer( nodeId );
}

void HandlerMonitoring::deviceRejoined( EmberNodeId nodeId ) {
    deviceIsReachable( nodeId );
    m_callbackInterface.nodeRejoined( nodeId );
}

void HandlerMonitoring::trustCenterJoinCallback( EmberNodeId nodeId,
                                                 uint8_t* newNodeEui64,
                                                 EmberNodeId parentOfNewNode,
                                                 EmberDeviceUpdate status,
                                                 EmberJoinDecision decision ) {
    if( EMBER_DEVICE_LEFT == status ) {
        Log::print( Log::PriorityLevel::DEBUG, "Device left with id: 0x%04x", nodeId );

        if( nodeId == m_forceRemoveNodeId ) {
            m_sendLeaveTimer.stop();
            m_forceRemoveNodeId = EMBER_BROADCAST_ADDRESS;
        }

        auto index = emberAfDeviceTableGetFirstIndexFromEui64( newNodeEui64 );
        emberAfPluginDeviceDatabaseEraseDevice( newNodeEui64 );
        while( index != EMBER_AF_PLUGIN_DEVICE_TABLE_NULL_INDEX ) {
            emAfPluginDeviceTableDeleteEntry( index );
            index = emAfDeviceTableFindNextEndpoint( index );
        }
        emAfDeviceTableSave();
        m_callbackInterface.nodeRemoved( nodeId );
    }
}

bool HandlerMonitoring::readAttributesResponseCallback( EmberNodeId emberNodeId,
                                                        EmberAfClusterId clusterId,
                                                        uint8_t endpointId,
                                                        uint8_t* buffer,
                                                        uint16_t bufLen ) {
    AttributesParser::parseAttributesResponse(
        buffer,
        bufLen,
        [this, emberNodeId, clusterId, endpointId](
            uint16_t attributeId, AttributeValueType valueType, std::string value, bool actualValue ) {
            Log::print( Log::PriorityLevel::DEBUG,
                        "Receive attribute response from node_id: 0x%04x, actual_value: %s",
                        emberNodeId,
                        actualValue ? "true" : "false" );
            if( actualValue ) {
                m_callbackInterface.attributeValueUpdated( emberNodeId, endpointId, clusterId, attributeId, value );
            }
        } );
    deviceIsReachable( emberNodeId );
    return true;
}

bool HandlerMonitoring::reportAttributesCallback( EmberNodeId emberNodeId,
                                                  EmberAfClusterId clusterId,
                                                  uint8_t endpointId,
                                                  uint8_t* buffer,
                                                  uint16_t bufLen ) {
    AttributesParser::parseAttributesReport(
        buffer,
        bufLen,
        [this, emberNodeId, clusterId, endpointId](
            uint16_t attributeId, AttributeValueType valueType, std::string value ) {
            Log::debug( "Receive attribute report from node_id: " + std::to_string( emberNodeId ) );
            m_callbackInterface.attributeValueUpdated( emberNodeId, endpointId, clusterId, attributeId, value );
        } );
    deviceIsReachable( emberNodeId );
    return true;
}

void HandlerMonitoring::configureReportingResponseCallback( uint16_t nodeId,
                                                            uint8_t endpointId,
                                                            uint16_t clusterId,
                                                            uint8_t* buffer,
                                                            uint16_t bufLen ) {
    {
        uint16_t bufIndex = 0;

        // Each record in the response has a one-byte status.  If the status is not
        // SUCCESS, the record will also contain a one-byte direction and a two-byte
        // attribute id.
        while( bufIndex + 1 <= bufLen ) {
            EmberAfStatus status = (EmberAfStatus) emberAfGetInt8u( buffer, bufIndex, bufLen );
            std::string info;
            bufIndex++;
            info += "status: " + std::to_string( status );
            if( status != EMBER_ZCL_STATUS_SUCCESS ) {
                EmberAfReportingDirection direction;
                EmberAfAttributeId attributeId;
                direction = (EmberAfReportingDirection) emberAfGetInt8u( buffer, bufIndex, bufLen );
                bufIndex++;
                attributeId = (EmberAfAttributeId) emberAfGetInt16u( buffer, bufIndex, bufLen );
                bufIndex += 2;
                info += ", direction: " + std::to_string( direction ) + ", attr: " + std::to_string( attributeId );
            }
            Log::print( Log::PriorityLevel::DEBUG,
                        "Attribute configure response received: nodeId 0x%04x, endpointId 0x%02x, clusterId 0x%04x: %s",
                        nodeId,
                        endpointId,
                        clusterId,
                        info.c_str() );
        }
    }
    deviceIsReachable( nodeId );
}

// cluster specific stack callback interface
void HandlerMonitoring::hubPlatformIasZoneClusterZoneStatusChangeNotificationCallback( DeviceData devData,
                                                                                       uint16_t zoneStatus,
                                                                                       uint8_t extendedStatus,
                                                                                       uint8_t zoneId,
                                                                                       uint16_t delay ) {
    m_callbackInterface.attributeValueUpdated(
        devData.nodeId, devData.endpointId, devData.clusterId, devData.attributeId, std::to_string( zoneStatus ) );
    deviceIsReachable( devData.nodeId );
}

void HandlerMonitoring::pollControlCheckInCallback( EmberNodeId emberNodeId,
                                                    uint8_t endpointId,
                                                    EmberAfClusterId clusterId ) {
    m_callbackInterface.simpleCommandReceived(
        emberNodeId, endpointId, ZCL_POLL_CONTROL_CLUSTER_ID, ZCL_CHECK_IN_COMMAND_ID );
    deviceIsReachable( emberNodeId );
}

void HandlerMonitoring::iasAceGetPanelStatusReceivedCallback( EmberNodeId emberNodeId,
                                                              uint8_t endpointId,
                                                              EmberAfClusterId clusterId ) {
    m_callbackInterface.simpleCommandReceived(
        emberNodeId, endpointId, ZCL_IAS_ACE_CLUSTER_ID, ZCL_GET_PANEL_STATUS_COMMAND_ID );
    deviceIsReachable( emberNodeId );
}

void HandlerMonitoring::iasAceArmReceivedCallback( EmberNodeId emberNodeId,
                                                   uint8_t endpointId,
                                                   uint8_t armMode,
                                                   const std::string& armDisarmCode ) {
    m_callbackInterface.armCommandReceived(
        emberNodeId, endpointId, static_cast< IasAceArmCommandMode >( armMode ), armDisarmCode );
    deviceIsReachable( emberNodeId );
}

void HandlerMonitoring::iasAcePanicReceivedCallback( EmberNodeId emberNodeId, uint8_t endpointId ) {
    m_callbackInterface.simpleCommandReceived( emberNodeId, endpointId, ZCL_IAS_ACE_CLUSTER_ID, ZCL_PANIC_COMMAND_ID );
    deviceIsReachable( emberNodeId );
}

void HandlerMonitoring::iasAceEmergencyReceivedCallback( EmberNodeId emberNodeId, uint8_t endpointId ) {
    m_callbackInterface.simpleCommandReceived(
        emberNodeId, endpointId, ZCL_IAS_ACE_CLUSTER_ID, ZCL_EMERGENCY_COMMAND_ID );
    deviceIsReachable( emberNodeId );
}

void HandlerMonitoring::doorLockLockDoorResponse( EmberNodeId emberNodeId, uint8_t endpointId, uint8_t status ) {
    deviceIsReachable( emberNodeId );
}

void HandlerMonitoring::doorLockUnlockDoorResponse( EmberNodeId emberNodeId, uint8_t endpointId, uint8_t status ) {
    deviceIsReachable( emberNodeId );
}

void HandlerMonitoring::doorLockSetPinResponse( EmberNodeId emberNodeId, uint8_t endpointId, uint8_t status ) {
    deviceIsReachable( emberNodeId );
}

void HandlerMonitoring::doorLockClearPinResponse( EmberNodeId emberNodeId, uint8_t endpointId, uint8_t status ) {
    deviceIsReachable( emberNodeId );
}

void HandlerMonitoring::doorLockClearAllPinsResponse( EmberNodeId emberNodeId, uint8_t endpointId, uint8_t status ) {
    deviceIsReachable( emberNodeId );
}

void HandlerMonitoring::doorLockGetPinResponse( EmberNodeId emberNodeId,
                                                uint8_t endpointId,
                                                uint16_t userId,
                                                uint8_t userStatus,
                                                uint8_t userType,
                                                const std::string& pin ) {
    m_callbackInterface.doorLockGetUserResponseReceived( emberNodeId,
                                                         endpointId,
                                                         userId,
                                                         static_cast< UserStatus >( userStatus ),
                                                         static_cast< UserType >( userType ),
                                                         pin );

    deviceIsReachable( emberNodeId );
}

void HandlerMonitoring::doorLockOperatingEventNotification( EmberNodeId emberNodeId,
                                                            uint8_t endpointId,
                                                            uint8_t source,
                                                            uint8_t eventCode,
                                                            uint16_t userId,
                                                            const std::string& pin,
                                                            uint32_t timeStamp,
                                                            const std::string& data ) {
    m_callbackInterface.doorLockOperatingEventNotificationReceived( emberNodeId,
                                                                    endpointId,
                                                                    static_cast< DoorLockEventSource >( source ),
                                                                    static_cast< DoorLockOperationEvent >( eventCode ),
                                                                    userId,
                                                                    pin );

    deviceIsReachable( emberNodeId );
}

void HandlerMonitoring::doorLockProgrammingEventNotification( EmberNodeId emberNodeId,
                                                              uint8_t endpointId,
                                                              uint8_t source,
                                                              uint8_t eventCode,
                                                              uint16_t userId,
                                                              const std::string& pin,
                                                              uint8_t userType,
                                                              uint8_t userStatus,
                                                              uint32_t timeStamp,
                                                              const std::string& data ) {
    m_callbackInterface.doorLockProgrammingEventNotificationReceived(
        emberNodeId,
        endpointId,
        static_cast< DoorLockEventSource >( source ),
        static_cast< DoorLockProgrammingEvent >( eventCode ),
        userId,
        pin,
        static_cast< UserType >( userType ),
        static_cast< UserStatus >( userStatus ) );

    deviceIsReachable( emberNodeId );
}

bool HandlerMonitoring::defaultResponseCallback( EmberNodeId emberNodeId,
                                                 EmberAfClusterId clusterId,
                                                 uint8_t endpointId,
                                                 uint8_t commandId,
                                                 EmberAfStatus status ) {
    deviceIsReachable( emberNodeId );
    return true;
}

void HandlerMonitoring::childJoined( uint16_t nodeId, bool joining, NodeType childType ) {
    LOGGER_DEBUG_PRINT( cc::debug( "Child node joined: nodeId: 0x%04x, joining: %s, childType: %s" ).c_str(),
                        nodeId,
                        joining ? "true" : "false",
                        toString( childType ) );

    if( joining ) {
        deviceIsReachable( nodeId );
    }
}

void HandlerMonitoring::initReachableTimer( uint16_t nodeId ) {
    resetReachableTimer( nodeId );
    m_reachableTimers.emplace( nodeId, std::make_shared< NativeTimer >( [this, nodeId]() {
                                   ZigbeeTaskLoop::getInstance().addTask( [this, nodeId]() {
                                       const auto* node = m_callbackInterface.findNode( nodeId );
                                       if( node && !node->isSleepy() ) {
                                           Log::debug( "Device is not longer reachable " + std::to_string( nodeId ) );
                                           m_callbackInterface.updateDeviceReachable( nodeId, false );
                                       }
                                   } );
                               } ) );
    m_reachableTimers[ nodeId ]->start( m_reachableStateTimeout, true );
}

void HandlerMonitoring::resetReachableTimer( uint16_t nodeId ) {
    if( m_reachableTimers.find( nodeId ) != m_reachableTimers.end() ) {
        m_reachableTimers[ nodeId ]->stop();
        m_reachableTimers.erase( nodeId );
    }
}

void HandlerMonitoring::deviceIsReachable( uint16_t nodeId ) {
    resetReachableTimer( nodeId );
    m_callbackInterface.updateDeviceReachable( nodeId, true );
}

void HandlerMonitoring::alertsNotificationOrResponseCallback(
    EmberNodeId emberNodeId,
    uint8_t endpointId,
    const std::vector< HubPlatform::Zigbee::AlertStructure >& alert_structures,
    HubPlatform::Zigbee::AlertSource type ) {
    m_callbackInterface.alertsNotificationOrResponseReceived( emberNodeId, endpointId, alert_structures, type );
    deviceIsReachable( emberNodeId );
}

}  // namespace Zigbee
}  // namespace HubPlatform
