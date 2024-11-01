#pragma once

#include "zigbee/model/zigbee_node.h"

#include "database_client/idbclient.h"
#include "zigbee/model/data_structures/alert_structures.h"

namespace HubPlatform {
namespace Zigbee {

namespace Backup {
struct ZigbeeNetworkInfo;
enum class BackupRestoreStatus;
}  // namespace Backup

enum class ModeStatus;
enum class IasAceArmCommandMode;
enum class UserStatus;
enum class UserType;
enum class DoorLockEventSource;
enum class DoorLockOperationEvent;
enum class DoorLockProgrammingEvent;

class NodeInterviewConfigurations;
class NodeInterviewNamesExtractors;

struct ChildData;
struct NeighborData;
struct RouteData;

class ZigbeeAddonCallbackInterface {
public:
    ZigbeeAddonCallbackInterface() = default;

    virtual ~ZigbeeAddonCallbackInterface() = default;

public:
    virtual const ZigbeeNode* findNode( uint16_t nodeId ) = 0;

    virtual void stackInitialized() = 0;
    virtual void nodeAdded( std::shared_ptr< ZigbeeNode > zigbeeNode ) = 0;
    virtual void nodeRemoved( const uint16_t zigbeeNode ) = 0;
    virtual void nodeRejoined( uint16_t nodeId ) = 0;

    virtual void includeStatusChange( ModeStatus status ) = 0;

    virtual void attributeValueUpdated( uint16_t nodeId,
                                        uint16_t endpointId,
                                        uint16_t clusterId,
                                        uint16_t attributeId,
                                        const std::string value ) = 0;

    virtual void armCommandReceived( uint16_t nodeId,
                                     uint16_t endpointId,
                                     IasAceArmCommandMode mode,
                                     const std::string pinCode ) = 0;

    virtual void simpleCommandReceived( uint16_t nodeId,
                                        uint16_t endpointId,
                                        uint16_t clusterId,
                                        uint16_t commandId ) = 0;

    virtual void doorLockGetUserResponseReceived( uint16_t nodeId,
                                                  uint16_t endpointId,
                                                  uint16_t userId,
                                                  UserStatus userStatus,
                                                  UserType userType,
                                                  const std::string& pinCode ) = 0;

    virtual void doorLockOperatingEventNotificationReceived( uint16_t nodeId,
                                                             uint16_t endpointId,
                                                             DoorLockEventSource eventSource,
                                                             DoorLockOperationEvent event,
                                                             uint16_t userId,
                                                             const std::string& pinCode ) = 0;

    virtual void doorLockProgrammingEventNotificationReceived( uint16_t nodeId,
                                                               uint16_t endpointId,
                                                               DoorLockEventSource eventSource,
                                                               DoorLockProgrammingEvent event,
                                                               uint16_t userId,
                                                               const std::string& pinCode,
                                                               UserType userType,
                                                               UserStatus userStatus ) = 0;

    virtual void alertsNotificationOrResponseReceived(
        uint16_t nodeId,
        uint8_t endpointId,
        const std::vector< HubPlatform::Zigbee::AlertStructure >& alertStructures,
        HubPlatform::Zigbee::AlertSource type ) = 0;

    virtual void updateDeviceReachable( uint16_t nodeId, bool isReachable ) = 0;

    virtual void includeDeviceProgress( uint8_t progress ) = 0;
    virtual void userCodesRequestProgress( uint16_t nodeId, uint16_t endpointId, uint8_t progress ) = 0;

    virtual void networkDown() = 0;
    virtual void networkUp() = 0;
    virtual void onNetworkChannelChanged() = 0;
    virtual void onChildAndRouteInfo( uint16_t selfNodeId,
                                      std::vector< ChildData > children,
                                      std::vector< NeighborData > neighbors,
                                      std::vector< RouteData > routes ) = 0;

    // Backup
    virtual void networkInfoCollected( const Backup::ZigbeeNetworkInfo& info, bool completed ) = 0;
    virtual void restoreBackupCompleted() = 0;
    virtual void restoreBackupFailed( Backup::BackupRestoreStatus reason ) = 0;

    virtual std::unique_ptr< NodeInterviewConfigurations > provideNodeInterviewConfigurations(
        const std::string& manufacturerId,
        const std::string& modelId ) = 0;

    virtual std::unique_ptr< NodeInterviewNamesExtractors > provideNodeInterviewNamesExtractors() = 0;
};

}  // namespace Zigbee
}  // namespace HubPlatform
