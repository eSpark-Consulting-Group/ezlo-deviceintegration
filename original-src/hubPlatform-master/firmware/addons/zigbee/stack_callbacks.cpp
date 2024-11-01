#include <exception>
#include "zigbee/zigbee_log.h"
#include "zigbee/stack_callbacks.h"
#include "zigbee/stack_wrapper.h"

#include "zigbee/model/enums/command_type.h"

ZigbeeFriendCallbackInterface* ZigbeeFriendCallbackInterface::m_instance = nullptr;

ZigbeeFriendCallbackInterface::ZigbeeFriendCallbackInterface() {
    if( ZigbeeFriendCallbackInterface::m_instance != nullptr ) {
        ZB_LOG_ERR( "Application MUST have only one instance of ZigbeeFriendCallbackInterface" );
        std::exit( EXIT_FAILURE );
    }
    ZigbeeFriendCallbackInterface::m_instance = this;
    ZB_LOG_DEBUG( "ZigbeeFriendCallbackInterface instantiated at %p\n", ZigbeeFriendCallbackInterface::m_instance );
}

ZigbeeFriendCallbackInterface& ZigbeeFriendCallbackInterface::getInstance() {
    return *ZigbeeFriendCallbackInterface::m_instance;
}

// extern c code callbacks
bool emberAfStackStatusCallback( EmberStatus status ) {
    return ZigbeeFriendCallbackInterface::getInstance().stackStatusCallback( status );
}

void emberAfTrustCenterJoinCallback( EmberNodeId newNodeId,
                                     EmberEUI64 newNodeEui64,
                                     EmberNodeId parentOfNewNode,
                                     EmberDeviceUpdate status,
                                     EmberJoinDecision decision ) {
    ZigbeeFriendCallbackInterface::getInstance().trustCenterJoinCallback(
        newNodeId, newNodeEui64, parentOfNewNode, status, decision );
}

bool emberAfPreMessageReceivedCallback( EmberAfIncomingMessage* incomingMessage ) {
    return ZigbeeFriendCallbackInterface::getInstance().preMessageReceivedCallback( incomingMessage );
}

bool emberAfMessageSentCallback( EmberOutgoingMessageType type,
                                 uint16_t indexOrDestination,
                                 EmberApsFrame* apsFrame,
                                 uint16_t msgLen,
                                 uint8_t* message,
                                 EmberStatus status ) {
    return ZigbeeFriendCallbackInterface::getInstance().messageSentCallback(
        type, indexOrDestination, apsFrame, msgLen, message, status );
}

bool emberAfPreZDOMessageReceivedCallback( EmberNodeId emberNodeId,
                                           EmberApsFrame* apsFrame,
                                           uint8_t* message,
                                           uint16_t length ) {
    return ZigbeeFriendCallbackInterface::getInstance().preZDOMessageReceivedCallback(
        emberNodeId, apsFrame, message, length );
}

void emberAfPostAttributeChangeCallback( uint8_t endpoint,
                                         EmberAfClusterId clusterId,
                                         EmberAfAttributeId attributeId,
                                         uint8_t mask,
                                         uint16_t manufacturerCode,
                                         uint8_t type,
                                         uint8_t size,
                                         uint8_t* value ) {
    // useless callback - too much traffic
    // ZB_LOG_DEBUG( "emberAfPostAttributeChangeCallback\n" );
}

// original implementation is in: general-response-commands.c
bool emberAfReadAttributesResponseCallback( EmberAfClusterId clusterId, uint8_t* buffer, uint16_t bufLen ) {
    ZB_LOG_DEBUG( "emberAfReadAttributesResponseCallback(): %d / 0x%04x, ep: %d -> %d, cls: %u / 0x%04x\n",
                  emberAfCurrentCommand()->source,
                  emberAfCurrentCommand()->source,
                  emberAfCurrentCommand()->apsFrame->sourceEndpoint,
                  emberAfCurrentCommand()->apsFrame->destinationEndpoint,
                  clusterId,
                  clusterId );
    return ZigbeeFriendCallbackInterface::getInstance().readAttributesResponseCallback(
        emberAfCurrentCommand()->source, clusterId, emberAfCurrentCommand()->apsFrame->sourceEndpoint, buffer, bufLen );
}

bool emberAfReportAttributesCallback( EmberAfClusterId clusterId, uint8_t* buffer, uint16_t bufLen ) {
    ZB_LOG_DEBUG( "emberAfReportAttributesCallback(): %d / 0x%04x, ep: %d -> %d, cls: %u / 0x%04x\n",
                  emberAfCurrentCommand()->source,
                  emberAfCurrentCommand()->source,
                  emberAfCurrentCommand()->apsFrame->sourceEndpoint,
                  emberAfCurrentCommand()->apsFrame->destinationEndpoint,
                  clusterId,
                  clusterId );
    return ZigbeeFriendCallbackInterface::getInstance().reportAttributesCallback(
        emberAfCurrentCommand()->source, clusterId, emberAfCurrentCommand()->apsFrame->sourceEndpoint, buffer, bufLen );
}

bool emberAfDiscoverAttributesResponseCallback( EmberAfClusterId clusterId,
                                                bool discoveryComplete,
                                                uint8_t* buffer,
                                                uint16_t bufLen,
                                                bool extended ) {
    ZigbeeFriendCallbackInterface::getInstance().attributesDiscoveryResponseCallback(
        emberAfCurrentCommand()->source,
        clusterId,
        emberAfCurrentCommand()->apsFrame->sourceEndpoint,
        buffer,
        bufLen,
        extended );

    emberAfSendImmediateDefaultResponse( EMBER_ZCL_STATUS_SUCCESS );
    return true;
}

bool emberAfDiscoverCommandsGeneratedResponseCallback( EmberAfClusterId clusterId,
                                                       uint16_t manufacturerCode,
                                                       bool discoveryComplete,
                                                       uint8_t* commandIds,
                                                       uint16_t commandIdCount ) {
    ZigbeeFriendCallbackInterface::getInstance().commandsDiscoveryResponseCallback(
        emberAfCurrentCommand()->source,
        clusterId,
        emberAfCurrentCommand()->apsFrame->sourceEndpoint,
        HubPlatform::Zigbee::CommandType::Generated,
        commandIds,
        commandIdCount );

    return false;
}

bool emberAfDiscoverCommandsReceivedResponseCallback( EmberAfClusterId clusterId,
                                                      uint16_t manufacturerCode,
                                                      bool discoveryComplete,
                                                      uint8_t* commandIds,
                                                      uint16_t commandIdCount ) {
    ZigbeeFriendCallbackInterface::getInstance().commandsDiscoveryResponseCallback(
        emberAfCurrentCommand()->source,
        clusterId,
        emberAfCurrentCommand()->apsFrame->sourceEndpoint,
        HubPlatform::Zigbee::CommandType::Received,
        commandIds,
        commandIdCount );

    return false;
}

bool emberAfDefaultResponseCallback( EmberAfClusterId clusterId, uint8_t commandId, EmberAfStatus status ) {
    ZigbeeFriendCallbackInterface::getInstance().defaultResponseCallback(
        emberAfCurrentCommand()->source,
        clusterId,
        emberAfCurrentCommand()->apsFrame->sourceEndpoint,
        commandId,
        status );

    return true;
}

void ezspChildJoinHandler( int8u index,
                           boolean joining,
                           EmberNodeId childId,
                           EmberEUI64 childEui64,
                           EmberNodeType childType ) {
    ZigbeeFriendCallbackInterface::getInstance().childJoined(
        childId, joining, static_cast< HubPlatform::Zigbee::NodeType >( childType ) );
}

void emberChildJoinHandler( int8u index, boolean joining ) {
    ZB_LOG_DEBUG( "emberChildJoinHandler()\n" );
}

void emberAfOnOffClusterClientAttributeChangedCallback( int8u endpoint, EmberAfAttributeId attributeId ) {
    ZB_LOG_DEBUG( "emberAfOnOffClusterClientAttributeChangedCallback()\n" );
}

bool emberAfOnOffClusterOffCallback( void ) {
    ZB_LOG_DEBUG( "emberAfOnOffClusterOffCallback()\n" );
    return false;
}

void emberAfOnOffClusterServerAttributeChangedCallback( int8u endpoint, EmberAfAttributeId attributeId ) {
    ZB_LOG_DEBUG( "emberAfOnOffClusterServerAttributeChangedCallback\n" );
}

// bool emberAfIasZoneClusterZoneStatusChangeNotificationCallback(uint16_t zoneStatus,
//                                                               uint8_t extendedStatus,
//                                                               uint8_t zoneId,
//                                                               uint16_t delay) {
//    auto nodeId = emberAfCurrentCommand()->source;
//    ZB_LOG_DEBUG( "emberAfIasZoneClusterZoneStatusChangeNotificationCallback(): 0x%04x\n", nodeId );
/*
    uint8_t serverIndex = findIasZoneServerByNodeId(emberAfCurrentCommand()->source);
    uint8_t status = EMBER_ZCL_STATUS_NOT_FOUND;
    if (serverIndex != NO_INDEX) {
        status = EMBER_ZCL_STATUS_SUCCESS;
        setServerZoneStatus(serverIndex, zoneStatus);

        emberAfIasZoneClusterPrintln("Zone %d status change, 0x%2X from 0x%2X",
                                     zoneId,
                                     zoneStatus,
                                     emberAfCurrentCommand()->source);

        // The Test case calls for readding attributes after status change.
        //   that is silly for the production device.
        // readIasZoneServerAttributes(emberAfCurrentCommand()->source);
    }
*/
//    return true;
//}

void emberAfPluginDeviceDatabaseDiscoveryCompleteCallback( const EmberAfDeviceInfo* device ) {
    ZigbeeFriendCallbackInterface::getInstance().discoveryCompleteCallback( device );
}

void emberAfPluginBasicResetToFactoryDefaultsCallback( uint8_t endpoint ) {
    ZB_LOG_DEBUG( "emberAfPluginBasicResetToFactoryDefaultsCallback\n" );
}

// attribute specific callback interface
void hubPlatformAttributeNotificationCallbackUint8( uint16_t nodeId,
                                                    uint8_t endpointId,
                                                    uint16_t clusterId,
                                                    uint16_t attributeId,
                                                    uint8_t value ) {
    ZigbeeFriendCallbackInterface::DeviceData devData{ nodeId, endpointId, clusterId, attributeId };
    ZigbeeFriendCallbackInterface::getInstance().hubPlatformAttributeNotificationCallbackUint8( devData, value );
}

void hubPlatformAttributeNotificationCallbackUint16( uint16_t nodeId,
                                                     uint8_t endpointId,
                                                     uint16_t clusterId,
                                                     uint16_t attributeId,
                                                     uint16_t value ) {
    ZigbeeFriendCallbackInterface::DeviceData devData{ nodeId, endpointId, clusterId, attributeId };
    ZigbeeFriendCallbackInterface::getInstance().hubPlatformAttributeNotificationCallbackUint16( devData, value );
}

// cluster specific callback interface
void hubPlatformIasZoneClusterZoneStatusChangeNotificationCallback( uint16_t nodeId,
                                                                    uint8_t endpointId,
                                                                    uint16_t clusterId,
                                                                    uint16_t attributeId,
                                                                    uint16_t zoneStatus,
                                                                    uint8_t extendedStatus,
                                                                    uint8_t zoneId,
                                                                    uint16_t delay ) {
    ZigbeeFriendCallbackInterface::DeviceData devData{ nodeId, endpointId, clusterId, attributeId };
    ZigbeeFriendCallbackInterface::getInstance().hubPlatformIasZoneClusterZoneStatusChangeNotificationCallback(
        devData, zoneStatus, extendedStatus, zoneId, delay );
}

void hubPlatformPollControlCheckInCallback( uint16_t nodeId, uint8_t epId, uint16_t clsId ) {
    ZigbeeFriendCallbackInterface::getInstance().pollControlCheckInCallback( nodeId, epId, clsId );
}

bool emberAfIasAceClusterArmCallback( uint8_t armMode, uint8_t* armDisarmCode, uint8_t zoneId ) {
    int codeLength = emberAfStringLength( armDisarmCode );
    std::string code( (char*) ( armDisarmCode + 1 ), codeLength );

    ZigbeeFriendCallbackInterface::getInstance().iasAceArmReceivedCallback(
        emberAfCurrentCommand()->source, emberAfCurrentCommand()->apsFrame->sourceEndpoint, armMode, code );
    return true;
}

bool emberAfIasAceClusterEmergencyCallback() {
    ZigbeeFriendCallbackInterface::getInstance().iasAceEmergencyReceivedCallback(
        emberAfCurrentCommand()->source, emberAfCurrentCommand()->apsFrame->sourceEndpoint );
    return true;
}

bool emberAfIasAceClusterPanicCallback() {
    ZigbeeFriendCallbackInterface::getInstance().iasAcePanicReceivedCallback(
        emberAfCurrentCommand()->source, emberAfCurrentCommand()->apsFrame->sourceEndpoint );
    return true;
}

bool emberAfIasAceClusterGetPanelStatusCallback( void ) {
    ZigbeeFriendCallbackInterface::getInstance().iasAceGetPanelStatusReceivedCallback(
        emberAfCurrentCommand()->source,
        emberAfCurrentCommand()->apsFrame->sourceEndpoint,
        emberAfCurrentCommand()->apsFrame->clusterId );
    return true;
}

// Door Lock cluster
bool emberAfDoorLockClusterGetUserTypeResponseCallback( uint16_t userId, uint8_t userType ) {
    return false;
}

bool emberAfDoorLockClusterOperationEventNotificationCallback( uint8_t source,
                                                               uint8_t eventCode,
                                                               uint16_t userId,
                                                               uint8_t* pin,
                                                               uint32_t timeStamp,
                                                               uint8_t* data ) {
    std::string pinString( (char*) ( pin + 1 ), emberAfStringLength( pin ) );
    std::string dataString( (char*) ( data + 1 ), emberAfStringLength( data ) );

    ZigbeeFriendCallbackInterface::getInstance().doorLockOperatingEventNotification(
        emberAfCurrentCommand()->source,
        emberAfCurrentCommand()->apsFrame->sourceEndpoint,
        source,
        eventCode,
        userId,
        pinString,
        timeStamp,
        dataString );
    return true;
}

bool emberAfDoorLockClusterSetYeardayScheduleResponseCallback( uint8_t status ) {
    return false;
}

bool emberAfDoorLockClusterGetUserStatusResponseCallback( uint16_t userId, uint8_t status ) {
    return false;
}

bool emberAfDoorLockClusterGetPinResponseCallback( uint16_t userId,
                                                   uint8_t userStatus,
                                                   uint8_t userType,
                                                   uint8_t* pin ) {
    std::string pinString( (char*) ( pin + 1 ), emberAfStringLength( pin ) );

    ZigbeeFriendCallbackInterface::getInstance().doorLockGetPinResponse(
        emberAfCurrentCommand()->source,
        emberAfCurrentCommand()->apsFrame->sourceEndpoint,
        userId,
        userStatus,
        userType,
        pinString );
    return true;
}

bool emberAfDoorLockClusterClearHolidayScheduleResponseCallback( uint8_t status ) {
    return false;
}

bool emberAfDoorLockClusterUnlockDoorResponseCallback( uint8_t status ) {
    ZigbeeFriendCallbackInterface::getInstance().doorLockUnlockDoorResponse(
        emberAfCurrentCommand()->source, emberAfCurrentCommand()->apsFrame->sourceEndpoint, status );
    return true;
}

bool emberAfDoorLockClusterGetWeekdayScheduleResponseCallback( uint8_t scheduleId,
                                                               uint16_t userId,
                                                               uint8_t status,
                                                               uint8_t daysMask,
                                                               uint8_t startHour,
                                                               uint8_t startMinute,
                                                               uint8_t endHour,
                                                               uint8_t endMinute ) {
    return false;
}

bool emberAfDoorLockClusterGetHolidayScheduleResponseCallback( uint8_t scheduleId,
                                                               uint8_t status,
                                                               uint32_t localStartTime,
                                                               uint32_t localEndTime,
                                                               uint8_t operatingModeDuringHoliday ) {
    return false;
}

bool emberAfDoorLockClusterSetUserStatusResponseCallback( uint8_t status ) {
    return false;
}

bool emberAfDoorLockClusterClearRfidResponseCallback( uint8_t status ) {
    return false;
}

bool emberAfDoorLockClusterGetRfidResponseCallback( uint16_t userId,
                                                    uint8_t userStatus,
                                                    uint8_t userType,
                                                    uint8_t* rfid ) {
    return false;
}

bool emberAfDoorLockClusterClearWeekdayScheduleResponseCallback( uint8_t status ) {
    return false;
}

bool emberAfDoorLockClusterClearAllRfidsResponseCallback( uint8_t status ) {
    return false;
}

bool emberAfDoorLockClusterClearAllPinsResponseCallback( uint8_t status ) {
    ZigbeeFriendCallbackInterface::getInstance().doorLockClearAllPinsResponse(
        emberAfCurrentCommand()->source, emberAfCurrentCommand()->apsFrame->sourceEndpoint, status );
    return true;
}

bool emberAfDoorLockClusterGetLogRecordResponseCallback( uint16_t logEntryId,
                                                         uint32_t timestamp,
                                                         uint8_t eventType,
                                                         uint8_t source,
                                                         uint8_t eventIdOrAlarmCode,
                                                         uint16_t userId,
                                                         uint8_t* pin ) {
    return false;
}

bool emberAfDoorLockClusterToggleResponseCallback( uint8_t status ) {
    return false;
}

bool emberAfDoorLockClusterGetYeardayScheduleResponseCallback( uint8_t scheduleId,
                                                               uint16_t userId,
                                                               uint8_t status,
                                                               uint32_t localStartTime,
                                                               uint32_t localEndTime ) {
    return false;
}

bool emberAfDoorLockClusterSetWeekdayScheduleResponseCallback( uint8_t status ) {
    return false;
}

bool emberAfDoorLockClusterProgrammingEventNotificationCallback( uint8_t source,
                                                                 uint8_t eventCode,
                                                                 uint16_t userId,
                                                                 uint8_t* pin,
                                                                 uint8_t userType,
                                                                 uint8_t userStatus,
                                                                 uint32_t timeStamp,
                                                                 uint8_t* data ) {
    std::string pinString( (char*) ( pin + 1 ), emberAfStringLength( pin ) );
    std::string dataString( (char*) ( data + 1 ), emberAfStringLength( data ) );

    ZigbeeFriendCallbackInterface::getInstance().doorLockProgrammingEventNotification(
        emberAfCurrentCommand()->source,
        emberAfCurrentCommand()->apsFrame->sourceEndpoint,
        source,
        eventCode,
        userId,
        pinString,
        userType,
        userStatus,
        timeStamp,
        dataString );
    return true;
}

bool emberAfDoorLockClusterSetUserTypeResponseCallback( uint8_t status ) {
    return false;
}

bool emberAfDoorLockClusterLockDoorResponseCallback( uint8_t status ) {
    ZigbeeFriendCallbackInterface::getInstance().doorLockLockDoorResponse(
        emberAfCurrentCommand()->source, emberAfCurrentCommand()->apsFrame->sourceEndpoint, status );
    return true;
}

bool emberAfDoorLockClusterSetPinResponseCallback( uint8_t status ) {
    ZigbeeFriendCallbackInterface::getInstance().doorLockSetPinResponse(
        emberAfCurrentCommand()->source, emberAfCurrentCommand()->apsFrame->sourceEndpoint, status );
    return true;
}

bool emberAfDoorLockClusterSetRfidResponseCallback( uint8_t status ) {
    return false;
}

bool emberAfDoorLockClusterUnlockWithTimeoutResponseCallback( uint8_t status ) {
    return false;
}

bool emberAfDoorLockClusterClearPinResponseCallback( uint8_t status ) {
    ZigbeeFriendCallbackInterface::getInstance().doorLockClearPinResponse(
        emberAfCurrentCommand()->source, emberAfCurrentCommand()->apsFrame->sourceEndpoint, status );
    return true;
}

bool emberAfDoorLockClusterSetHolidayScheduleResponseCallback( uint8_t status ) {
    return false;
}

bool emberAfDoorLockClusterClearYeardayScheduleResponseCallback( uint8_t status ) {
    return false;
}

bool emberAfConfigureReportingResponseCallback( EmberAfClusterId clusterId, uint8_t* buffer, uint16_t bufLen ) {
    ZigbeeFriendCallbackInterface::getInstance().configureReportingResponseCallback(
        emberAfCurrentCommand()->source, emberAfCurrentCommand()->apsFrame->sourceEndpoint, clusterId, buffer, bufLen );

    emberAfSendImmediateDefaultResponse( EMBER_ZCL_STATUS_SUCCESS );
    return true;
}

void emberAfPluginDeviceTableNewDeviceCallback( EmberEUI64 eui64 ) {
    ZB_LOG_DEBUG( "%s\n", __PRETTY_FUNCTION__ );
}

void emberAfPluginDeviceTableRejoinDeviceCallback( EmberEUI64 newNodeEui64 ) {
    ZB_LOG_DEBUG( "%s\n", __PRETTY_FUNCTION__ );
}

void emberAfPluginDeviceTableDeviceLeftCallback( EmberEUI64 newNodeEui64 ) {
    ZB_LOG_DEBUG( "%s\n", __PRETTY_FUNCTION__ );
}

std::vector< HubPlatform::Zigbee::AlertStructure > parseAlertStructures( const uint8_t alertsCount,
                                                                         const uint8_t* alertStructures ) {
    const uint8_t SIZE_OF_ALERT_STRUCTURE_IN_BITS = 3;
    std::vector< HubPlatform::Zigbee::AlertStructure > alert_structures( alertsCount );

    for( uint8_t i = 0; i < alertsCount; ++i ) {
        HubPlatform::Zigbee::AlertStructure t;
        uint16_t start_position = SIZE_OF_ALERT_STRUCTURE_IN_BITS * i;

        t.alert_id = alertStructures[ start_position ];
        t.category = static_cast< HubPlatform::Zigbee::AlertNotificationCategory::Category >(
            alertStructures[ start_position + 1 ] & 0b00001111 );
        t.presence_recovery = static_cast< HubPlatform::Zigbee::AlertNotificationPresenceRecovery::PresenceRecovery >(
            ( alertStructures[ start_position + 1 ] & 0b00110000 ) >> 4 );
        t.manufacturer_specific_bits = alertStructures[ start_position + 2 ];

        alert_structures.at( i ) = t;
    }

    return alert_structures;
}

void ApplianceEventsAndAlertClusterAlertsNotificationOrResponseCallback( uint8_t alertsCount,
                                                                         uint8_t* alertStructures,
                                                                         HubPlatform::Zigbee::AlertSource type ) {
    std::vector< HubPlatform::Zigbee::AlertStructure > alert_structures =
        parseAlertStructures( alertsCount, alertStructures );

    ZigbeeFriendCallbackInterface::getInstance().alertsNotificationOrResponseCallback(
        emberAfCurrentCommand()->source, emberAfCurrentCommand()->apsFrame->sourceEndpoint, alert_structures, type );
}

bool emberAfApplianceEventsAndAlertClusterGetAlertsResponseCallback( uint8_t alertsCount, uint8_t* alertStructures ) {
    ApplianceEventsAndAlertClusterAlertsNotificationOrResponseCallback(
        alertsCount, alertStructures, HubPlatform::Zigbee::AlertSource::Response );
    return true;
}

bool emberAfApplianceEventsAndAlertClusterEventsNotificationCallback( uint8_t eventHeader, uint8_t eventId ) {
    return false;
}

bool emberAfApplianceEventsAndAlertClusterAlertsNotificationCallback( uint8_t alertsCount, uint8_t* alertStructures ) {
    ApplianceEventsAndAlertClusterAlertsNotificationOrResponseCallback(
        alertsCount, alertStructures, HubPlatform::Zigbee::AlertSource::Notification );
    return true;
}
