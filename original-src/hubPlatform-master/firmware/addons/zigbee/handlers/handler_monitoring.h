#pragma once

#include "zigbee/zigbee_addon_callback.h"
#include "zigbee/handlers/default_handler.h"
#include "zigbee/utils/commands_sender_interface.h"

#include "base_utils/native_timer.h"

namespace HubPlatform {
namespace Zigbee {

class HandlerMonitoring : public DefaultHandler {
public:
    HandlerMonitoring( ZigbeeAddonCallbackInterface& callbackInterface,
                       CommandsSenderInterface& commandsSender,
                       std::chrono::system_clock::duration reachableStateTimeout = std::chrono::seconds{ 30 } );

    ~HandlerMonitoring();

protected:
    ZigbeeAddonCallbackInterface& getCallbackInterface();
    CommandsSenderInterface& getCommandsSender();

    // Callbacks
    void doorLockGetPinResponse( EmberNodeId emberNodeId,
                                 uint8_t endpointId,
                                 uint16_t userId,
                                 uint8_t userStatus,
                                 uint8_t userType,
                                 const std::string& pin ) override;

private:
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

    // Callbacks
    void deviceRejoined( EmberNodeId nodeId ) override;

    void trustCenterJoinCallback( EmberNodeId nodeId,
                                  uint8_t* newNodeEui64,
                                  EmberNodeId parentOfNewNode,
                                  EmberDeviceUpdate status,
                                  EmberJoinDecision decision ) override;

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

    void configureReportingResponseCallback( uint16_t nodeId,
                                             uint8_t endpointId,
                                             uint16_t clusterId,
                                             uint8_t* buffer,
                                             uint16_t bufLen ) override;

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

    bool defaultResponseCallback( EmberNodeId emberNodeId,
                                  EmberAfClusterId clusterId,
                                  uint8_t endpointId,
                                  uint8_t commandId,
                                  EmberAfStatus status ) override;

    void childJoined( uint16_t nodeId, bool joining, NodeType childType ) override;

    void alertsNotificationOrResponseCallback(
        EmberNodeId emberNodeId,
        uint8_t endpointId,
        const std::vector< HubPlatform::Zigbee::AlertStructure >& alert_structures,
        HubPlatform::Zigbee::AlertSource type ) override;

    // Timers helpers
    void initReachableTimer( uint16_t nodeId );
    void resetReachableTimer( uint16_t nodeId );
    void deviceIsReachable( uint16_t nodeId );

private:
    ZigbeeAddonCallbackInterface& m_callbackInterface;
    CommandsSenderInterface& m_commandsSender;

    std::chrono::system_clock::duration m_reachableStateTimeout;

    uint16_t m_forceRemoveNodeId;
    NativeTimer m_sendLeaveTimer;  // force remove timer

    std::map< uint16_t, std::shared_ptr< NativeTimer > > m_reachableTimers;
};

}  // namespace Zigbee
}  // namespace HubPlatform
