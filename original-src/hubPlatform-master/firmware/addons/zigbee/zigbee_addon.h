#pragma once

#include "system_utils/thread_unique_safe_wrapper.h"
#include "config_reader/config_reader.h"
#include "addon_interfaces/addon.h"
#include "zigbee/zigbee_addon_callback.h"
#include "zigbee/zigbee_interface.h"
#include "zigbee/model/zigbee_node.h"
#include "zigbee/zigbee_node_manager.h"
#include "zigbee/zigbee_reporter.h"

#include <vector>

namespace HubPlatform {
namespace Zigbee {

/*
 * TODO review test suite
namespace UT {
class ZwaveAddonTestSuite;
}
*/

class ZigbeeAddon final : public Addons::Addon, public ZigbeeAddonCallbackInterface {
    //    friend class UT::ZwaveAddonTestSuite;
public:
    static const char* const Name;

public:
    ZigbeeAddon();

    CommonUtils::AddonVersion getVersion() const override;

    int restoreBackup( const std::string& tempBackupFolder ) override;

public:
    // Zigbee ( addon ) public interface
    void startInclude( const Json& params );
    void stopInclude();
    void removeNode( const uint16_t nodeId );
    void reset();
    void setAttributeValue( const Json& params );
    void sendCommand( const Json& params );
    void requestAttributeValue( const Json& params );
    void requestUserCodes( const Json& params );
    void requestMfgSpecificAttributeValue( const Json& params );
    void configureAttributeReport( const Json& params );
    void configureMfgSpecificAttributeReport( const Json& params );
    void setNetworkRadioChannel( const Json& params );
    void getChildAndRouteInfo( const Json& /*params*/ );

    int restoreBackupTmp();

private:
    ZigbeeAddon( const std::string& name,
                 std::unique_ptr< Services::BusWrapper > impl,
                 std::unique_ptr< IDbClient > dbClient );

    void setUp() override;
    void tearDown() override;

    void restartAddon();

    enum class SetUpMode { Default, BackupRestore };
    void setUpInternal( SetUpMode mode );

    void initReachableStateRecheck();

    void handleStartInclude( const JsonObject& params );
    void handleStopInclude( const JsonObject& params );
    void handleRemoveNode( const JsonObject& params );
    void handleReset( const Json& params );

    void handleMessageFromBus( const Services::Message& message, const std::string& /*sender*/ ) override;
    JsonObject handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) override;
    bool handleAsyncRequestFromBus( const JsonObject& request,
                                    MessageSubClasses requestType,
                                    const std::string& uuid,
                                    const std::string& sender ) override;

    void handleLegacyMethodCall( const JsonObject& json, const std::string& /*sender*/ ) override;
    void handleMethodCall( const JsonObject& json, MessageSubClasses requestType, const std::string& sender ) override;

    void moduleResetStatusChanged( ModeStatus status );

    // addon callback interface
    const ZigbeeNode* findNode( uint16_t nodeId ) override;

    void stackInitialized() override;
    void nodeAdded( std::shared_ptr< ZigbeeNode > zigbeeNode ) override;
    void nodeRemoved( const uint16_t nodeId ) override;
    void nodeRejoined( uint16_t nodeId ) override;

    void includeStatusChange( ModeStatus status ) override;

    void attributeValueUpdated( uint16_t nodeId,
                                uint16_t endpointId,
                                uint16_t clusterId,
                                uint16_t attributeId,
                                const std::string value ) override;

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

    void networkInfoCollected( const Backup::ZigbeeNetworkInfo& info, bool completed );
    void restoreBackupCompleted() override;
    void restoreBackupFailed( Backup::BackupRestoreStatus reason ) override;

    std::unique_ptr< NodeInterviewConfigurations > provideNodeInterviewConfigurations(
        const std::string& manufacturerId,
        const std::string& modelId ) override;

    std::unique_ptr< NodeInterviewNamesExtractors > provideNodeInterviewNamesExtractors() override;

    void alertsNotificationOrResponseReceived(
        uint16_t nodeId,
        uint8_t endpointId,
        const std::vector< HubPlatform::Zigbee::AlertStructure >& alert_structures,
        HubPlatform::Zigbee::AlertSource type ) override;

    void requestAttributeValueInternal( uint16_t nodeId,
                                        uint8_t endpointId,
                                        uint16_t clusterId,
                                        uint16_t attributeId,
                                        bool isMfgSpecific );

    void configureAttributeReportInternal( uint16_t nodeId,
                                           uint8_t endpointId,
                                           uint16_t clusterId,
                                           uint16_t attributeId,
                                           const Json& params,
                                           bool isMfgSpecific );

private:
    std::atomic_bool m_isZigbeeInitialized;
    std::mutex m_mutex;
    std::condition_variable m_initCondVar;

    std::atomic_int m_backupRestoreStatus;
    std::mutex m_backupRestoreMutex;
    std::condition_variable m_backupRestoreCondVar;

    std::unique_ptr< ZigbeeInterface > m_iZigbee;
    std::shared_ptr< NodeManager > m_nodeManager;
    std::unique_ptr< ConfigReader > m_configReader;

    struct AsyncRequestInfo {
        AsyncRequestInfo( JsonObject request, std::string uuid, std::string sender )
        : m_request( request )
        , m_uuid( uuid )
        , m_sender( sender ) {}

        JsonObject m_request;
        std::string m_uuid;
        std::string m_sender;
    };

    std::unique_ptr< AsyncRequestInfo > m_backupRequestInfo;
    std::unique_ptr< AsyncRequestInfo > m_networkRequestInfo;

    std::string m_interviewConfigurationsFilePatsh;

    std::unique_ptr< Backup::ZigbeeNetworkInfo > m_networkInfoCache;
};

}  // namespace Zigbee
}  // namespace HubPlatform
