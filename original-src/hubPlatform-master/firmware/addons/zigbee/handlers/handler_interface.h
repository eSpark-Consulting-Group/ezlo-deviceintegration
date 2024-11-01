#pragma once

extern "C" {
#include <Zigbee/definitions.h>
#include <protocol/zigbee/app/framework/include/af.h>
#include <protocol/zigbee/app/framework/include/af-types.h>
}

#include "json_utils/json_fwd.h"
#include <string>
#include "zigbee/model/data_structures/alert_structures.h"

namespace HubPlatform {
namespace Zigbee {

enum class CommandType;
enum class AttributeValueType;
enum class NodeType;

class HandlerInterface {
public:
    struct DeviceData {
        uint16_t nodeId;
        uint8_t endpointId;
        uint16_t clusterId;
        uint16_t attributeId;
    };

protected:
    HandlerInterface() = default;

public:
    virtual ~HandlerInterface() = default;

    // Commands
    virtual void sendLeaveCommand( EmberNodeId nodeId ) = 0;

    virtual void sendCommand( uint16_t nodeId,
                              uint8_t endpointId,
                              uint16_t clusterId,
                              const std::string& commandName,
                              const Json& commandParams ) = 0;

    virtual void setAttributeValue( uint16_t nodeId,
                                    uint8_t endpointId,
                                    uint16_t clusterId,
                                    uint16_t attributeId,
                                    AttributeValueType attributeValueType,
                                    const Json& value ) = 0;

    virtual void requestAttributeValue( uint16_t nodeId,
                                        uint8_t endpointId,
                                        uint16_t clusterId,
                                        uint16_t attributeId,
                                        bool isMfgSpecific,
                                        uint16_t mfgCode ) = 0;

    virtual void configureAttributeReport( uint16_t nodeId,
                                           uint8_t endpointId,
                                           uint16_t clusterId,
                                           uint16_t attributeId,
                                           bool isMfgSpecific,
                                           uint16_t mfgCode ) = 0;

    virtual void configureAttributeReport( uint16_t nodeId,
                                           uint8_t endpointId,
                                           uint16_t clusterId,
                                           uint16_t attributeId,
                                           AttributeValueType attributeValueType,
                                           const uint16_t minInterval,
                                           const uint16_t maxInterval,
                                           const Json& reportableChange,
                                           bool isMfgSpecific,
                                           uint16_t mfgCode ) = 0;

    // generic stack callback interface
    virtual bool stackStatusCallback( EmberStatus status ) = 0;

    virtual void deviceRejoined( EmberNodeId nodeId ) = 0;

    virtual void trustCenterJoinCallback( EmberNodeId newNodeId,
                                          EmberEUI64 newNodeEui64,
                                          EmberNodeId parentOfNewNode,
                                          EmberDeviceUpdate status,
                                          EmberJoinDecision decision ) = 0;

    virtual bool preMessageReceivedCallback( EmberAfIncomingMessage* incomingMessage ) = 0;

    virtual bool messageSentCallback( EmberOutgoingMessageType type,
                                      uint16_t indexOrDestination,
                                      EmberApsFrame* apsFrame,
                                      uint16_t msgLen,
                                      uint8_t* message,
                                      EmberStatus status ) = 0;

    virtual bool preZDOMessageReceivedCallback( EmberNodeId emberNodeId,
                                                EmberApsFrame* apsFrame,
                                                uint8_t* message,
                                                uint16_t length ) = 0;

    virtual bool readAttributesResponseCallback( EmberNodeId emberNodeId,
                                                 EmberAfClusterId clusterId,
                                                 uint8_t endpointId,
                                                 uint8_t* buffer,
                                                 uint16_t bufLen ) = 0;
    virtual bool reportAttributesCallback( EmberNodeId emberNodeId,
                                           EmberAfClusterId clusterId,
                                           uint8_t endpointId,
                                           uint8_t* buffer,
                                           uint16_t bufLen ) = 0;

    virtual bool attributesDiscoveryResponseCallback( EmberNodeId emberNodeId,
                                                      EmberAfClusterId clusterId,
                                                      uint8_t endpointId,
                                                      uint8_t* buffer,
                                                      uint16_t bufLen,
                                                      bool extended ) = 0;

    virtual bool commandsDiscoveryResponseCallback( EmberNodeId emberNodeId,
                                                    EmberAfClusterId clusterId,
                                                    uint8_t endpointId,
                                                    CommandType commandsType,
                                                    uint8_t* commandIds,
                                                    uint16_t commandIdCount ) = 0;

    virtual bool defaultResponseCallback( EmberNodeId emberNodeId,
                                          EmberAfClusterId clusterId,
                                          uint8_t endpointId,
                                          uint8_t commandId,
                                          EmberAfStatus status ) = 0;

    virtual void childJoined( uint16_t nodeId, bool joining, NodeType childType ) = 0;

    virtual void discoveryCompleteCallback( const EmberAfDeviceInfo* device ) = 0;

    virtual void configureReportingResponseCallback( uint16_t nodeId,
                                                     uint8_t endpointId,
                                                     uint16_t clusterId,
                                                     uint8_t* buffer,
                                                     uint16_t bufLen ) = 0;

    // attribute specific stack callback interface
    virtual void hubPlatformAttributeNotificationCallbackUint8( DeviceData devData, uint8_t value ) = 0;
    virtual void hubPlatformAttributeNotificationCallbackUint16( DeviceData devData, uint16_t value ) = 0;

    // cluster specific stack callback interface
    virtual void hubPlatformIasZoneClusterZoneStatusChangeNotificationCallback( DeviceData devData,
                                                                                uint16_t zoneStatus,
                                                                                uint8_t extendedStatus,
                                                                                uint8_t zoneId,
                                                                                uint16_t delay ) = 0;

    virtual void pollControlCheckInCallback( EmberNodeId emberNodeId,
                                             uint8_t endpointId,
                                             EmberAfClusterId clusterId ) = 0;

    virtual void iasAceGetPanelStatusReceivedCallback( EmberNodeId emberNodeId,
                                                       uint8_t endpointId,
                                                       EmberAfClusterId clusterId ) = 0;

    virtual void iasAceArmReceivedCallback( EmberNodeId emberNodeId,
                                            uint8_t endpointId,
                                            uint8_t armMode,
                                            const std::string& armDisarmCode ) = 0;

    virtual void iasAcePanicReceivedCallback( EmberNodeId emberNodeId, uint8_t endpointId ) = 0;

    virtual void iasAceEmergencyReceivedCallback( EmberNodeId emberNodeId, uint8_t endpointId ) = 0;

    virtual void doorLockLockDoorResponse( EmberNodeId emberNodeId, uint8_t endpointId, uint8_t status ) = 0;
    virtual void doorLockUnlockDoorResponse( EmberNodeId emberNodeId, uint8_t endpointId, uint8_t status ) = 0;
    virtual void doorLockSetPinResponse( EmberNodeId emberNodeId, uint8_t endpointId, uint8_t status ) = 0;
    virtual void doorLockClearPinResponse( EmberNodeId emberNodeId, uint8_t endpointId, uint8_t status ) = 0;
    virtual void doorLockClearAllPinsResponse( EmberNodeId emberNodeId, uint8_t endpointId, uint8_t status ) = 0;
    virtual void doorLockGetPinResponse( EmberNodeId emberNodeId,
                                         uint8_t endpointId,
                                         uint16_t userId,
                                         uint8_t userStatus,
                                         uint8_t userType,
                                         const std::string& pin ) = 0;
    virtual void doorLockOperatingEventNotification( EmberNodeId emberNodeId,
                                                     uint8_t endpointId,
                                                     uint8_t source,
                                                     uint8_t eventCode,
                                                     uint16_t userId,
                                                     const std::string& pin,
                                                     uint32_t timeStamp,
                                                     const std::string& data ) = 0;
    virtual void doorLockProgrammingEventNotification( EmberNodeId emberNodeId,
                                                       uint8_t endpointId,
                                                       uint8_t source,
                                                       uint8_t eventCode,
                                                       uint16_t userId,
                                                       const std::string& pin,
                                                       uint8_t userType,
                                                       uint8_t userStatus,
                                                       uint32_t timeStamp,
                                                       const std::string& data ) = 0;

    virtual void alertsNotificationOrResponseCallback( EmberNodeId emberNodeId,
                                                       uint8_t endpointId,
                                                       const std::vector< AlertStructure >& alert_structures,
                                                       HubPlatform::Zigbee::AlertSource type ) = 0;
};

}  // namespace Zigbee
}  // namespace HubPlatform
