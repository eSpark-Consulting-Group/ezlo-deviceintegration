#pragma once

#include "zigbee/handlers/handler_interface.h"

#include <memory>

extern "C" {
#include <Zigbee/definitions.h>
#include <protocol/zigbee/app/framework/include/af.h>
#include <protocol/zigbee/app/framework/include/af-types.h>
#include "app/builder/zigbeegw/hubplatform_callbacks.h"
}

class ZigbeeFriendCallbackInterface : public HubPlatform::Zigbee::HandlerInterface {
public:
    ZigbeeFriendCallbackInterface();

    virtual ~ZigbeeFriendCallbackInterface() = default;

public:
protected:
    static ZigbeeFriendCallbackInterface& getInstance();

public:
    // friend c functions that override EmberStack callbacks ( marked as weak )
    friend bool emberAfStackStatusCallback( EmberStatus status );

    friend void emberAfTrustCenterJoinCallback( EmberNodeId newNodeId,
                                                EmberEUI64 newNodeEui64,
                                                EmberNodeId parentOfNewNode,
                                                EmberDeviceUpdate status,
                                                EmberJoinDecision decision );

    friend bool emberAfPreMessageReceivedCallback( EmberAfIncomingMessage* incomingMessage );

    friend bool emberAfMessageSentCallback( EmberOutgoingMessageType type,
                                            uint16_t indexOrDestination,
                                            EmberApsFrame* apsFrame,
                                            uint16_t msgLen,
                                            uint8_t* message,
                                            EmberStatus status );

    friend bool emberAfPreZDOMessageReceivedCallback( EmberNodeId emberNodeId,
                                                      EmberApsFrame* apsFrame,
                                                      uint8_t* message,
                                                      uint16_t length );

    friend bool emberAfReadAttributesResponseCallback( EmberAfClusterId clusterId, uint8_t* buffer, uint16_t bufLen );
    friend bool emberAfReportAttributesCallback( EmberAfClusterId clusterId, uint8_t* buffer, uint16_t bufLen );
    friend void emberAfPluginDeviceDatabaseDiscoveryCompleteCallback( const EmberAfDeviceInfo* device );

    friend bool emberAfDiscoverAttributesResponseCallback( EmberAfClusterId clusterId,
                                                           bool discoveryComplete,
                                                           uint8_t* buffer,
                                                           uint16_t bufLen,
                                                           bool extended );

    friend bool emberAfDiscoverCommandsGeneratedResponseCallback( EmberAfClusterId clusterId,
                                                                  uint16_t manufacturerCode,
                                                                  bool discoveryComplete,
                                                                  uint8_t* commandIds,
                                                                  uint16_t commandIdCount );

    friend bool emberAfDiscoverCommandsReceivedResponseCallback( EmberAfClusterId clusterId,
                                                                 uint16_t manufacturerCode,
                                                                 bool discoveryComplete,
                                                                 uint8_t* commandIds,
                                                                 uint16_t commandIdCount );

    friend bool emberAfDefaultResponseCallback( EmberAfClusterId clusterId, uint8_t commandId, EmberAfStatus status );

    friend void ezspChildJoinHandler( int8u index,
                                      boolean joining,
                                      EmberNodeId childId,
                                      EmberEUI64 childEui64,
                                      EmberNodeType childType );

    friend bool emberAfConfigureReportingResponseCallback( EmberAfClusterId clusterId,
                                                           uint8_t* buffer,
                                                           uint16_t bufLen );

    // attribute specific callbacks
    friend void hubPlatformAttributeNotificationCallbackUint8( uint16_t nodeId,
                                                               uint8_t endpointId,
                                                               uint16_t clusterId,
                                                               uint16_t attributeId,
                                                               uint8_t value );

    friend void hubPlatformAttributeNotificationCallbackUint16( uint16_t nodeId,
                                                                uint8_t endpointId,
                                                                uint16_t clusterId,
                                                                uint16_t attributeId,
                                                                uint16_t value );

    // cluster specific callbacks
    friend void hubPlatformIasZoneClusterZoneStatusChangeNotificationCallback( uint16_t nodeId,
                                                                               uint8_t endpointId,
                                                                               uint16_t clusterId,
                                                                               uint16_t attributeId,
                                                                               uint16_t zoneStatus,
                                                                               uint8_t extendedStatus,
                                                                               uint8_t zoneId,
                                                                               uint16_t delay );

    friend void hubPlatformPollControlCheckInCallback( uint16_t nodeId, uint8_t epId, uint16_t clsId );

    friend bool emberAfIasAceClusterArmCallback( uint8_t armMode, uint8_t* armDisarmCode, uint8_t zoneId );

    friend bool emberAfIasAceClusterGetPanelStatusCallback();

    friend bool emberAfIasAceClusterEmergencyCallback();
    friend bool emberAfIasAceClusterPanicCallback();

    friend bool emberAfDoorLockClusterLockDoorResponseCallback( uint8_t status );
    friend bool emberAfDoorLockClusterUnlockDoorResponseCallback( uint8_t status );
    friend bool emberAfDoorLockClusterSetPinResponseCallback( uint8_t status );
    friend bool emberAfDoorLockClusterClearPinResponseCallback( uint8_t status );
    friend bool emberAfDoorLockClusterClearAllPinsResponseCallback( uint8_t status );
    friend bool emberAfDoorLockClusterGetPinResponseCallback( uint16_t userId,
                                                              uint8_t userStatus,
                                                              uint8_t userType,
                                                              uint8_t* pin );
    friend bool emberAfDoorLockClusterOperationEventNotificationCallback( uint8_t source,
                                                                          uint8_t eventCode,
                                                                          uint16_t userId,
                                                                          uint8_t* pin,
                                                                          uint32_t timeStamp,
                                                                          uint8_t* data );
    friend bool emberAfDoorLockClusterProgrammingEventNotificationCallback( uint8_t source,
                                                                            uint8_t eventCode,
                                                                            uint16_t userId,
                                                                            uint8_t* pin,
                                                                            uint8_t userType,
                                                                            uint8_t userStatus,
                                                                            uint32_t timeStamp,
                                                                            uint8_t* data );
    friend bool emberAfApplianceEventsAndAlertClusterAlertsNotificationCallback( uint8_t alertsCount,
                                                                                 uint8_t* alertStructures );

    friend bool emberAfApplianceEventsAndAlertClusterGetAlertsResponseCallback( uint8_t alertsCount,
                                                                                uint8_t* alertStructures );

    friend void ApplianceEventsAndAlertClusterAlertsNotificationOrResponseCallback(
        uint8_t alertsCount,
        uint8_t* alertStructures,
        HubPlatform::Zigbee::AlertSource type );

private:
    static ZigbeeFriendCallbackInterface* m_instance;
};
