#include "zigbee/handlers/default_handler.h"

#include "zigbee/zigbee_log.h"

namespace HubPlatform {
namespace Zigbee {

#define LOG_UNUSED_METHOD ZB_LOG_DEBUG( "Unused callback: %s\n", __PRETTY_FUNCTION__ )

void DefaultHandler::sendLeaveCommand( EmberNodeId nodeId ) {
    LOG_UNUSED_METHOD;
}

void DefaultHandler::sendCommand( uint16_t nodeId,
                                  uint8_t endpointId,
                                  uint16_t clusterId,
                                  const std::string& commandName,
                                  const Json& commandParams ) {
    LOG_UNUSED_METHOD;
}

void DefaultHandler::setAttributeValue( uint16_t nodeId,
                                        uint8_t endpointId,
                                        uint16_t clusterId,
                                        uint16_t attributeId,
                                        AttributeValueType attributeValueType,
                                        const Json& value ) {
    LOG_UNUSED_METHOD;
}

void DefaultHandler::requestAttributeValue( uint16_t nodeId,
                                            uint8_t endpointId,
                                            uint16_t clusterId,
                                            uint16_t attributeId,
                                            bool isMfgSpecific,
                                            uint16_t mfgCode ) {
    LOG_UNUSED_METHOD;
}

void DefaultHandler::configureAttributeReport( uint16_t nodeId,
                                               uint8_t endpointId,
                                               uint16_t clusterId,
                                               uint16_t attributeId,
                                               bool isMfgSpecific,
                                               uint16_t mfgCode ) {
    LOG_UNUSED_METHOD;
}

void DefaultHandler::configureAttributeReport( uint16_t nodeId,
                                               uint8_t endpointId,
                                               uint16_t clusterId,
                                               uint16_t attributeId,
                                               AttributeValueType attributeValueType,
                                               const uint16_t minInterval,
                                               const uint16_t maxInterval,
                                               const Json& reportableChange,
                                               bool isMfgSpecific,
                                               uint16_t mfgCode ) {
    LOG_UNUSED_METHOD;
}

bool DefaultHandler::stackStatusCallback( EmberStatus status ) {
    LOG_UNUSED_METHOD;
    return true;
}

void DefaultHandler::deviceRejoined( EmberNodeId nodeId ) {
    LOG_UNUSED_METHOD;
}

void DefaultHandler::trustCenterJoinCallback( EmberNodeId newNodeId,
                                              EmberEUI64 newNodeEui64,
                                              EmberNodeId parentOfNewNode,
                                              EmberDeviceUpdate status,
                                              EmberJoinDecision decision ) {
    LOG_UNUSED_METHOD;
}

bool DefaultHandler::preMessageReceivedCallback( EmberAfIncomingMessage* incomingMessage ) {
    LOG_UNUSED_METHOD;
    return true;
}

bool DefaultHandler::messageSentCallback( EmberOutgoingMessageType type,
                                          uint16_t indexOrDestination,
                                          EmberApsFrame* apsFrame,
                                          uint16_t msgLen,
                                          uint8_t* message,
                                          EmberStatus status ) {
    LOG_UNUSED_METHOD;
    return true;
}

bool DefaultHandler::preZDOMessageReceivedCallback( EmberNodeId emberNodeId,
                                                    EmberApsFrame* apsFrame,
                                                    uint8_t* message,
                                                    uint16_t length ) {
    LOG_UNUSED_METHOD;
    return true;
}

bool DefaultHandler::readAttributesResponseCallback( EmberNodeId emberNodeId,
                                                     EmberAfClusterId clusterId,
                                                     uint8_t endpointId,
                                                     uint8_t* buffer,
                                                     uint16_t bufLen ) {
    LOG_UNUSED_METHOD;
    return true;
}

bool DefaultHandler::reportAttributesCallback( EmberNodeId emberNodeId,
                                               EmberAfClusterId clusterId,
                                               uint8_t endpointId,
                                               uint8_t* buffer,
                                               uint16_t bufLen ) {
    LOG_UNUSED_METHOD;
    return true;
}

bool DefaultHandler::attributesDiscoveryResponseCallback( EmberNodeId emberNodeId,
                                                          EmberAfClusterId clusterId,
                                                          uint8_t endpointId,
                                                          uint8_t* buffer,
                                                          uint16_t bufLen,
                                                          bool extended ) {
    LOG_UNUSED_METHOD;
    return true;
}

bool DefaultHandler::commandsDiscoveryResponseCallback( EmberNodeId emberNodeId,
                                                        EmberAfClusterId clusterId,
                                                        uint8_t endpointId,
                                                        CommandType commandsType,
                                                        uint8_t* commandIds,
                                                        uint16_t commandIdCount ) {
    LOG_UNUSED_METHOD;
    return true;
}

bool DefaultHandler::defaultResponseCallback( EmberNodeId emberNodeId,
                                              EmberAfClusterId clusterId,
                                              uint8_t endpointId,
                                              uint8_t commandId,
                                              EmberAfStatus status ) {
    LOG_UNUSED_METHOD;
    return true;
}

void DefaultHandler::childJoined( uint16_t nodeId, bool joining, NodeType childType ) {
    LOG_UNUSED_METHOD;
}

void DefaultHandler::discoveryCompleteCallback( const EmberAfDeviceInfo* device ) {
    LOG_UNUSED_METHOD;
}

void DefaultHandler::configureReportingResponseCallback( uint16_t nodeId,
                                                         uint8_t endpointId,
                                                         uint16_t clusterId,
                                                         uint8_t* buffer,
                                                         uint16_t bufLen ) {
    LOG_UNUSED_METHOD;
}

void DefaultHandler::hubPlatformAttributeNotificationCallbackUint8( DeviceData devData, uint8_t value ) {
    LOG_UNUSED_METHOD;
}

void DefaultHandler::hubPlatformAttributeNotificationCallbackUint16( DeviceData devData, uint16_t value ) {
    LOG_UNUSED_METHOD;
}

void DefaultHandler::hubPlatformIasZoneClusterZoneStatusChangeNotificationCallback( DeviceData devData,
                                                                                    uint16_t zoneStatus,
                                                                                    uint8_t extendedStatus,
                                                                                    uint8_t zoneId,
                                                                                    uint16_t delay ) {
    LOG_UNUSED_METHOD;
}

void DefaultHandler::pollControlCheckInCallback( EmberNodeId emberNodeId,
                                                 uint8_t endpointId,
                                                 EmberAfClusterId clusterId ) {
    LOG_UNUSED_METHOD;
}

void DefaultHandler::iasAceGetPanelStatusReceivedCallback( EmberNodeId emberNodeId,
                                                           uint8_t endpointId,
                                                           EmberAfClusterId clusterId ) {
    LOG_UNUSED_METHOD;
}

void DefaultHandler::iasAceArmReceivedCallback( EmberNodeId emberNodeId,
                                                uint8_t endpointId,
                                                uint8_t armMode,
                                                const std::string& armDisarmCode ) {
    LOG_UNUSED_METHOD;
}

void DefaultHandler::iasAcePanicReceivedCallback( EmberNodeId emberNodeId, uint8_t endpointId ) {
    LOG_UNUSED_METHOD;
}

void DefaultHandler::iasAceEmergencyReceivedCallback( EmberNodeId emberNodeId, uint8_t endpointId ) {
    LOG_UNUSED_METHOD;
}

void DefaultHandler::alertsNotificationOrResponseCallback(
    EmberNodeId emberNodeId,
    uint8_t endpointId,
    const std::vector< HubPlatform::Zigbee::AlertStructure >& alert_structures,
    HubPlatform::Zigbee::AlertSource type ) {
    LOG_UNUSED_METHOD;
}

void DefaultHandler::doorLockLockDoorResponse( EmberNodeId emberNodeId, uint8_t endpointId, uint8_t status ) {
    LOG_UNUSED_METHOD;
}

void DefaultHandler::doorLockUnlockDoorResponse( EmberNodeId emberNodeId, uint8_t endpointId, uint8_t status ) {
    LOG_UNUSED_METHOD;
}

void DefaultHandler::doorLockSetPinResponse( EmberNodeId emberNodeId, uint8_t endpointId, uint8_t status ) {
    LOG_UNUSED_METHOD;
}

void DefaultHandler::doorLockClearPinResponse( EmberNodeId emberNodeId, uint8_t endpointId, uint8_t status ) {
    LOG_UNUSED_METHOD;
}

void DefaultHandler::doorLockClearAllPinsResponse( EmberNodeId emberNodeId, uint8_t endpointId, uint8_t status ) {
    LOG_UNUSED_METHOD;
}

void DefaultHandler::doorLockGetPinResponse( EmberNodeId emberNodeId,
                                             uint8_t endpointId,
                                             uint16_t userId,
                                             uint8_t userStatus,
                                             uint8_t userType,
                                             const std::string& pin ) {
    LOG_UNUSED_METHOD;
}

void DefaultHandler::doorLockOperatingEventNotification( EmberNodeId emberNodeId,
                                                         uint8_t endpointId,
                                                         uint8_t source,
                                                         uint8_t eventCode,
                                                         uint16_t userId,
                                                         const std::string& pin,
                                                         uint32_t timeStamp,
                                                         const std::string& data ) {
    LOG_UNUSED_METHOD;
}

void DefaultHandler::doorLockProgrammingEventNotification( EmberNodeId emberNodeId,
                                                           uint8_t endpointId,
                                                           uint8_t source,
                                                           uint8_t eventCode,
                                                           uint16_t userId,
                                                           const std::string& pin,
                                                           uint8_t userType,
                                                           uint8_t userStatus,
                                                           uint32_t timeStamp,
                                                           const std::string& data ) {
    LOG_UNUSED_METHOD;
}

}  // namespace Zigbee
}  // namespace HubPlatform
