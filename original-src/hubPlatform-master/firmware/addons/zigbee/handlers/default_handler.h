#pragma once

#include "zigbee/handlers/handler_interface.h"

namespace HubPlatform {
namespace Zigbee {

class DefaultHandler : public HandlerInterface {
protected:
    DefaultHandler() = default;

public:
    // Commands
    void sendLeaveCommand( EmberNodeId nodeId ) override;

    void sendCommand( uint16_t nodeId,
                      uint8_t endpointId,
                      uint16_t clusterId,
                      const std::string& commandName,
                      const Json& commandParams ) override;

    void setAttributeValue( uint16_t nodeId,
                            uint8_t endpointId,
                            uint16_t clusterId,
                            uint16_t attributeId,
                            AttributeValueType attributeValueType,
                            const Json& value ) override;

    void requestAttributeValue( uint16_t nodeId,
                                uint8_t endpointId,
                                uint16_t clusterId,
                                uint16_t attributeId,
                                bool isMfgSpecific,
                                uint16_t mfgCode ) override;

    void configureAttributeReport( uint16_t nodeId,
                                   uint8_t endpointId,
                                   uint16_t clusterId,
                                   uint16_t attributeId,
                                   bool isMfgSpecific,
                                   uint16_t mfgCode ) override;

    void configureAttributeReport( uint16_t nodeId,
                                   uint8_t endpointId,
                                   uint16_t clusterId,
                                   uint16_t attributeId,
                                   AttributeValueType attributeValueType,
                                   const uint16_t minInterval,
                                   const uint16_t maxInterval,
                                   const Json& reportableChange,
                                   bool isMfgSpecific,
                                   uint16_t mfgCode ) override;

    // generic stack callback interface
    bool stackStatusCallback( EmberStatus status ) override;

    void deviceRejoined( EmberNodeId nodeId ) override;

    void trustCenterJoinCallback( EmberNodeId newNodeId,
                                  EmberEUI64 newNodeEui64,
                                  EmberNodeId parentOfNewNode,
                                  EmberDeviceUpdate status,
                                  EmberJoinDecision decision ) override;

    bool preMessageReceivedCallback( EmberAfIncomingMessage* incomingMessage ) override;

    bool messageSentCallback( EmberOutgoingMessageType type,
                              uint16_t indexOrDestination,
                              EmberApsFrame* apsFrame,
                              uint16_t msgLen,
                              uint8_t* message,
                              EmberStatus status ) override;

    bool preZDOMessageReceivedCallback( EmberNodeId emberNodeId,
                                        EmberApsFrame* apsFrame,
                                        uint8_t* message,
                                        uint16_t length ) override;

    bool readAttributesResponseCallback( EmberNodeId emberNodeId,
                                         EmberAfClusterId clusterId,
                                         uint8_t endpointId,
                                         uint8_t* buffer,
                                         uint16_t bufLen ) override;
    bool reportAttributesCallback( EmberNodeId emberNodeId,
                                   EmberAfClusterId clusterId,
                                   uint8_t endpointId,
                                   uint8_t* buffer,
                                   uint16_t bufLen ) override;

    bool attributesDiscoveryResponseCallback( EmberNodeId emberNodeId,
                                              EmberAfClusterId clusterId,
                                              uint8_t endpointId,
                                              uint8_t* buffer,
                                              uint16_t bufLen,
                                              bool extended ) override;

    bool commandsDiscoveryResponseCallback( EmberNodeId emberNodeId,
                                            EmberAfClusterId clusterId,
                                            uint8_t endpointId,
                                            CommandType commandsType,
                                            uint8_t* commandIds,
                                            uint16_t commandIdCount ) override;

    bool defaultResponseCallback( EmberNodeId emberNodeId,
                                  EmberAfClusterId clusterId,
                                  uint8_t endpointId,
                                  uint8_t commandId,
                                  EmberAfStatus status ) override;

    void childJoined( uint16_t nodeId, bool joining, NodeType childType ) override;

    void discoveryCompleteCallback( const EmberAfDeviceInfo* device ) override;

    void configureReportingResponseCallback( uint16_t nodeId,
                                             uint8_t endpointId,
                                             uint16_t clusterId,
                                             uint8_t* buffer,
                                             uint16_t bufLen ) override;

    // attribute specific stack callback interface
    void hubPlatformAttributeNotificationCallbackUint8( DeviceData devData, uint8_t value ) override;
    void hubPlatformAttributeNotificationCallbackUint16( DeviceData devData, uint16_t value ) override;

    // cluster specific stack callback interface
    void hubPlatformIasZoneClusterZoneStatusChangeNotificationCallback( DeviceData devData,
                                                                        uint16_t zoneStatus,
                                                                        uint8_t extendedStatus,
                                                                        uint8_t zoneId,
                                                                        uint16_t delay ) override;

    void pollControlCheckInCallback( EmberNodeId emberNodeId, uint8_t endpointId, EmberAfClusterId clusterId ) override;

    void iasAceGetPanelStatusReceivedCallback( EmberNodeId emberNodeId,
                                               uint8_t endpointId,
                                               EmberAfClusterId clusterId ) override;

    void iasAceArmReceivedCallback( EmberNodeId emberNodeId,
                                    uint8_t endpointId,
                                    uint8_t armMode,
                                    const std::string& armDisarmCode ) override;

    void iasAcePanicReceivedCallback( EmberNodeId emberNodeId, uint8_t endpointId ) override;

    void iasAceEmergencyReceivedCallback( EmberNodeId emberNodeId, uint8_t endpointId ) override;

    void doorLockLockDoorResponse( EmberNodeId emberNodeId, uint8_t endpointId, uint8_t status ) override;
    void doorLockUnlockDoorResponse( EmberNodeId emberNodeId, uint8_t endpointId, uint8_t status ) override;
    void doorLockSetPinResponse( EmberNodeId emberNodeId, uint8_t endpointId, uint8_t status ) override;
    void doorLockClearPinResponse( EmberNodeId emberNodeId, uint8_t endpointId, uint8_t status ) override;
    void doorLockClearAllPinsResponse( EmberNodeId emberNodeId, uint8_t endpointId, uint8_t status ) override;
    void doorLockGetPinResponse( EmberNodeId emberNodeId,
                                 uint8_t endpointId,
                                 uint16_t userId,
                                 uint8_t userStatus,
                                 uint8_t userType,
                                 const std::string& pin ) override;
    void doorLockOperatingEventNotification( EmberNodeId emberNodeId,
                                             uint8_t endpointId,
                                             uint8_t source,
                                             uint8_t eventCode,
                                             uint16_t userId,
                                             const std::string& pin,
                                             uint32_t timeStamp,
                                             const std::string& data ) override;
    void doorLockProgrammingEventNotification( EmberNodeId emberNodeId,
                                               uint8_t endpointId,
                                               uint8_t source,
                                               uint8_t eventCode,
                                               uint16_t userId,
                                               const std::string& pin,
                                               uint8_t userType,
                                               uint8_t userStatus,
                                               uint32_t timeStamp,
                                               const std::string& data ) override;

    void alertsNotificationOrResponseCallback(
        EmberNodeId emberNodeId,
        uint8_t endpointId,
        const std::vector< HubPlatform::Zigbee::AlertStructure >& alert_structures,
        HubPlatform::Zigbee::AlertSource type ) override;
};

}  // namespace Zigbee
}  // namespace HubPlatform
