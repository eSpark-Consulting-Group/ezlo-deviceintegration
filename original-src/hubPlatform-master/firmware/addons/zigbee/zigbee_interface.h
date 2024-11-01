#pragma once

#include "base_utils/native_timer.h"
#include "service_interfaces/service.h"

#include "zigbee/zigbee_node_manager.h"
#include "zigbee/stack_callbacks.h"
#include "zigbee/zigbee_addon_callback.h"
#include "zigbee/handlers/handler_interview.h"
#include "zigbee/handlers/handler_monitoring.h"
#include "zigbee/handlers/handler_user_codes_request.h"
#include "zigbee/backup/backup_manager.h"
#include "zigbee/utils/commands_sender.h"

namespace HubPlatform {
namespace Zigbee {

enum class Mode;

enum class IZigbeeResult : int { SUCCESS = 0, FAILED = 1 };

class ZigbeeAddonCallbackInterface;

class ZigbeeInterface final : public ZigbeeFriendCallbackInterface, public ZigbeeAddonCallbackInterface {
public:
    using ZigbeeTask = std::function< void() >;
    using ZigbeeNodeUPtr = std::unique_ptr< ZigbeeNode >;

public:
    ZigbeeInterface( ZigbeeAddonCallbackInterface& callbackInterface );
    ~ZigbeeInterface() = default;

private:
    ZigbeeInterface() = delete;
    void addZigbeeTask( ZigbeeTask&& task );

public:
    IZigbeeResult init( const std::vector< std::string > initParameters = {} );
    void resetAndInitNCP();
    void mainLoop();

    IZigbeeResult startInclude();
    IZigbeeResult stopInclude();
    IZigbeeResult removeNode( EmberNodeId nodeId );
    IZigbeeResult resetToDefault();
    IZigbeeResult startNewNetwork();

    void collectNetworkInfo();
    void getNetworkInfo();

    void restoreBackup( const Backup::ZigbeeNetworkInfo& info );

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

    void requestUserCodes( uint16_t nodeId, uint8_t endpointId, uint16_t startUserId, uint16_t endUserId );

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

    void setNetworkRadioChannel( const uint8_t value );

    void getChildAndRouteInfo();

    // overriden stack callbacks
private:
    void prepareInitParameters( char** target, const std::vector< std::string > source ) const;

    void sendLeaveCommand( EmberNodeId nodeId ) override;

    bool stackStatusCallback( EmberStatus status ) override;

    void deviceRejoined( EmberNodeId nodeId ) override;

    void trustCenterJoinCallback( EmberNodeId nodeId,
                                  uint8_t* newNodeEui64,
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

    void alertsNotificationOrResponseCallback(
        EmberNodeId emberNodeId,
        uint8_t endpointId,
        const std::vector< HubPlatform::Zigbee::AlertStructure >& alert_structures,
        HubPlatform::Zigbee::AlertSource type ) override;

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

    const ZigbeeNode* findNode( uint16_t nodeId ) override;

    void stackInitialized();
    void nodeAdded( std::shared_ptr< ZigbeeNode > zigbeeNode );
    void nodeRemoved( const uint16_t zigbeeNode );
    void nodeRejoined( uint16_t nodeId ) override;

    void includeStatusChange( ModeStatus status );
    void attributeValueUpdated( uint16_t nodeId,
                                uint16_t endpointId,
                                uint16_t clusterId,
                                uint16_t attributeId,
                                const std::string value );

    void armCommandReceived( uint16_t nodeId,
                             uint16_t endpointId,
                             IasAceArmCommandMode mode,
                             const std::string pinCode ) override;

    void simpleCommandReceived( uint16_t nodeId, uint16_t endpointId, uint16_t clusterId, uint16_t commandId ) override;

    void doorLockGetUserResponseReceived( uint16_t nodeId,
                                          uint16_t endpointId,
                                          uint16_t userId,
                                          UserStatus userStatus,
                                          UserType userType,
                                          const std::string& pinCode ) override;

    void doorLockOperatingEventNotificationReceived( uint16_t nodeId,
                                                     uint16_t endpointId,
                                                     DoorLockEventSource eventSource,
                                                     DoorLockOperationEvent event,
                                                     uint16_t userId,
                                                     const std::string& pinCode ) override;

    void doorLockProgrammingEventNotificationReceived( uint16_t nodeId,
                                                       uint16_t endpointId,
                                                       DoorLockEventSource eventSource,
                                                       DoorLockProgrammingEvent event,
                                                       uint16_t userId,
                                                       const std::string& pinCode,
                                                       UserType userType,
                                                       UserStatus userStatus ) override;

    void updateDeviceReachable( uint16_t nodeId, bool isReachable ) override;

    void includeDeviceProgress( uint8_t progress ) override;
    void userCodesRequestProgress( uint16_t nodeId, uint16_t endpointId, uint8_t progress ) override;

    void networkDown() override;
    void networkUp() override;
    void onNetworkChannelChanged() override;
    void onChildAndRouteInfo( uint16_t selfNodeId,
                              std::vector< ChildData > children,
                              std::vector< NeighborData > neighbors,
                              std::vector< RouteData > routes ) override;

    void networkInfoCollected( const Backup::ZigbeeNetworkInfo& info, bool completed ) override;
    void restoreBackupCompleted() override;
    void restoreBackupFailed( Backup::BackupRestoreStatus reason ) override;

    std::unique_ptr< NodeInterviewConfigurations > provideNodeInterviewConfigurations(
        const std::string& manufacturerId,
        const std::string& modelId ) override;

    std::unique_ptr< NodeInterviewNamesExtractors > provideNodeInterviewNamesExtractors() override;

    HandlerInterface& initiateInterviewHandler( EmberNodeId nodeId );
    HandlerUserCodesRequest& initiateUserCodesRequestHandler( uint16_t nodeId, uint8_t endpointId );
    HandlerInterface& getHandler( EmberNodeId nodeId );

    void stopIncludeInternal();

    void changeMode( Mode mode, ModeStatus status );

    void alertsNotificationOrResponseReceived(
        uint16_t nodeId,
        uint8_t endpointId,
        const std::vector< HubPlatform::Zigbee::AlertStructure >& alert_structures,
        HubPlatform::Zigbee::AlertSource type ) override;

private:
    ZigbeeAddonCallbackInterface& m_callbackInterface;
    NativeTimer m_includeTimer;

    CommandsSender m_commandsSender;

    HandlerMonitoring m_monitoringHandler;
    std::map< EmberNodeId, std::shared_ptr< HandlerInterview > > m_interviewHandlers;
    std::map< EmberNodeId, std::shared_ptr< HandlerUserCodesRequest > > m_userCodesRequestHandlers;

    Backup::BackupManager m_backupManager;

    Mode m_mode;
    ModeStatus m_modeStatus;
};

}  // namespace Zigbee
}  // namespace HubPlatform
