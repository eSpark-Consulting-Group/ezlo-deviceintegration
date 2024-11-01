#pragma once

#include "service_interfaces/script_client.h"
#include "addon_interfaces/addon.h"
#include "base_utils/optional.h"

#include <znet/znet_provisioning_list_entry.hpp>

#include <chrono>

class IDbClient;

namespace znet {
class ZNode;
}  // namespace znet

namespace HubPlatform {
namespace Zwave {

class ZwaveClient : public ScriptClient {
public:
    ZwaveClient( Addons::Addon& parent );

    void startInclude( const std::string& deviceSettingFilePath );
    void startExclude();
    void startExclude( const uint8_t nodeId );
    void startRediscover( const uint8_t nodeId, const std::string& deviceSettingFilePath );

    void stopInclude();
    void stopExclude();

    void startLearn();

    void stopLearn();

    void stopSecurityInclude();

    void setAuthenticationMode( const std::vector< std::string >& modes );
    void setDeviceSpecificKey( uint16_t key );

    void setDevicesPollIntervals( bool useFixedWakeUpInterval,
                                  uint32_t wakeUpInterval,
                                  uint32_t alwaysOnDevicesInetrval,
                                  uint8_t attemptsCount );

    void setUnresponsiveDeviceIgnoreInterval( uint32_t interval );

    void setDefaultDeviceConfigurationsFile( const std::string& deviceSettingFilePath );

    void setDoorLockMode( uint8_t nodeId, uint8_t channelId, const std::string& doorLockMode );

    void requestDoorLockState( uint8_t nodeId, uint8_t channelId );

    using DoorLockConfigurationSeconds = uint16_t;
    using DoorLockConfigurationSecondsOpt = std::pair< DoorLockConfigurationSeconds, bool >;

    struct DoorLockConfiguration {
        uint8_t enabledInsideHandleIdsBitmask = 0;
        uint8_t enabledOutsideHandleIdsBitmask = 0;
        DoorLockConfigurationSecondsOpt lockTimeoutOpt = { 0, false };
    };

    struct DoorLockExtendedConfiguration {
        DoorLockConfigurationSeconds autoRelockTimeout = 0;
        DoorLockConfigurationSeconds holdAndReleaseTimeout = 0;
        bool isTwistAssistEnabled = false;
        bool isBlockToBlockEnabled = false;
    };

    using DoorLockExtendedConfigurationOpt = std::pair< DoorLockExtendedConfiguration, bool >;

    void setDoorLockConfiguration( uint8_t nodeId,
                                   uint8_t channelId,
                                   const DoorLockConfiguration& configuration,
                                   const DoorLockExtendedConfigurationOpt& extendedConfiguration );

    void requestDoorLockConfiguration( uint8_t nodeId, uint8_t channelId );

    Json getAllNodesIds() const;
    Json getNodeAsJson( const uint8_t nodeId ) const;
    Json getNodeInfoAsJson( uint8_t nodeId ) const;
    std::string getManufacturerName( const uint16_t manufacturerId ) const;
    Json getCommandClassAsJson( const uint8_t nodeId, const uint8_t channelId, const uint8_t ccId ) const;

    Json nodeBase64ToJson( const std::string& nodeBase64 ) const;

    void setValue( const uint8_t nodeId,
                   const uint8_t classId,
                   const uint8_t channelId,
                   const Json& value,
                   const common::optional< uint32_t >& durationOpt );
    void setValueMulticast( const std::vector< uint8_t >& nodeId,
                            const uint8_t classId,
                            const uint8_t channelId,
                            Json value,
                            const std::string& mode );
    void setTokenValue( const uint8_t nodeId,
                        const uint8_t classId,
                        const uint8_t channelId,
                        const std::string& value );
    void setTokenValueMulticast( const std::vector< uint8_t >& nodeId,
                                 const uint8_t classId,
                                 const uint8_t channelId,
                                 std::string value,
                                 const std::string& mode );
    void setRgbValue( const uint8_t nodeId, const uint8_t classId, const uint8_t channelId, const Json& value );
    void setRgbValueMulticast( const std::vector< uint8_t >& nodeId,
                               const uint8_t classId,
                               const uint8_t channelId,
                               Json value,
                               const std::string& mode );

    void setSoundSwitchConfiguration( const uint8_t nodeId,
                                      const uint8_t channelId,
                                      const uint8_t volume,
                                      const uint8_t defaultToneId );

    void requestSoundSwitchConfiguration( const uint8_t nodeId, const uint8_t channelId );

    void setSoundSwitchTonePlay( const uint8_t nodeId,
                                 const uint8_t channelId,
                                 const uint8_t toneId,
                                 const uint8_t volume );

    void requestSoundSwitchTonePlay( const uint8_t nodeId, const uint8_t channelId );

    void setSetpointValue( uint8_t nodeId,
                           uint8_t classId,
                           uint8_t channelId,
                           uint8_t setpointType,
                           const std::string& scale,
                           float value );

    void setConfigurationValue( uint8_t nodeId, uint16_t paramNumber, uint8_t paramSize, int64_t value );

    void addConfiguration( uint8_t nodeId, uint8_t paramNumber, uint8_t paramSize );

    void removeConfiguration( uint8_t nodeId, uint8_t paramNumber );

    void addNodeToConfigurationGroup( uint8_t nodeId,
                                      uint8_t channelId,
                                      uint8_t groupId,
                                      uint8_t destinationNode,
                                      uint8_t destinationEndpoint );

    void removeNodeFromConfigurationGroup( uint8_t nodeId,
                                           uint8_t channelId,
                                           uint8_t groupId,
                                           uint8_t destinationNode,
                                           uint8_t destinationEndpoint );

    void resetConfigurationValue( uint8_t nodeId, uint16_t paramNumber );

    void resetAllConfigurationValues( uint8_t nodeId );

    void requestConfigurationValue( uint8_t nodeId, uint16_t paramNumber );

    void switchMultilevelLevelChange( uint8_t nodeId,
                                      uint8_t channelId,
                                      const std::string& command,
                                      const common::optional< uint32_t >& durationOpt );

    void colorSwitchLevelChange( uint8_t nodeId,
                                 uint8_t channelId,
                                 const std::string& componentId,
                                 const std::string& command,
                                 const common::optional< uint32_t >& durationOpt );

    void requestValue( const uint8_t nodeId, const uint8_t classId, const uint8_t channelId );
    void requestSetpointValue( uint8_t nodeId, uint8_t channelId, uint8_t setpointType );
    void requestUserCode( uint8_t nodeId, uint8_t channelId, uint8_t userId );
    void requestUserCodes( uint8_t nodeId, uint8_t channelId );
    void requestUserCodesChecksum( uint8_t nodeId, uint8_t channelId );

    struct UserCodeElement {
        uint16_t id;
        std::string status;
        std::string code;
    };

    uint16_t calculateUserCodesChecksum( const std::vector< UserCodeElement >& userCodes );
    void requestMasterCode( uint8_t nodeId, uint8_t channelId );
    void requestUserCodesKeypadMode( uint8_t nodeId, uint8_t channelId );
    void requestAlarmSensorValue( uint8_t nodeId, uint8_t channelId, const std::string& sensorName );
    void requestAllNodesValues();
    void requestNodeValues( uint8_t nodeId,
                            std::chrono::milliseconds requestDelay,
                            size_t unsetPinCodesCountToStop,
                            bool force );
    void requestAssociationGroup( uint8_t nodeId, uint8_t channelId, uint8_t groupId );

    void sendIdentify( uint8_t nodeId, uint8_t channelId );

    void setUserCode( uint8_t nodeId, uint8_t channelId, const UserCodeElement& userCode );

    void setMasterCode( uint8_t nodeId, uint8_t channelId, const std::string& masterCode );

    void setUserCodesKeypadMode( uint8_t nodeId, uint8_t channelId, const std::string& mode );

    void setSilentSensors( uint8_t nodeId,
                           uint8_t channelId,
                           uint16_t delay,
                           const std::vector< std::string >& sensorNames );

    struct WeekDayEntryLockSchedule {
        std::string weekDay;
        uint8_t startHour;
        uint8_t startMin;
        uint8_t stopHour;
        uint8_t stopMin;
    };

    using WeekDayEntryLockScheduleOpt = std::pair< WeekDayEntryLockSchedule, bool >;

    void setWeekDayEntryLockSchedule( uint8_t nodeId,
                                      uint8_t channelId,
                                      uint8_t userId,
                                      uint8_t slotId,
                                      WeekDayEntryLockScheduleOpt valueOpt );

    struct YearDayEntryLockSchedule {
        uint16_t startYear;
        uint8_t startMonth;
        uint8_t startDay;
        uint8_t startHour;
        uint8_t startMin;
        uint16_t stopYear;
        uint8_t stopMonth;
        uint8_t stopDay;
        uint8_t stopHour;
        uint8_t stopMin;
    };

    using YearDayEntryLockScheduleOpt = std::pair< YearDayEntryLockSchedule, bool >;

    void setYearDayEntryLockSchedule( uint8_t nodeId,
                                      uint8_t channelId,
                                      uint8_t userId,
                                      uint8_t slotId,
                                      YearDayEntryLockScheduleOpt valueOpt );

    struct DailyRepeatingEntryLockSchedule {
        std::vector< std::string > weekDays;
        uint8_t startHour;
        uint8_t startMin;
        uint8_t durationHour;
        uint8_t durationMin;
    };

    using DailyRepeatingEntryLockScheduleOpt = std::pair< DailyRepeatingEntryLockSchedule, bool >;

    void setDailyRepeatingEntryLockSchedule( uint8_t nodeId,
                                             uint8_t channelId,
                                             uint8_t userId,
                                             uint8_t slotId,
                                             DailyRepeatingEntryLockScheduleOpt valueOpt );

    void setEntryLockScheduleEnable( uint8_t nodeId, uint8_t channelId, uint8_t userId, bool enable );

    std::pair< bool, std::string > isScheduleStructureAcceptable( uint8_t nodeId,
                                                                  uint8_t channelId,
                                                                  uint8_t scheduleId,
                                                                  const common::optional< uint8_t >& blockIdOpt,
                                                                  Json value );

    void setSchedule( uint8_t nodeId,
                      uint8_t channelId,
                      uint8_t scheduleId,
                      const common::optional< uint8_t >& blockIdOpt,
                      Json value );

    void requestSchedule( uint8_t nodeId,
                          uint8_t channelId,
                          uint8_t scheduleId,
                          const common::optional< uint8_t >& blockIdOpt,
                          const common::optional< bool >& activeIdCtlOpt );

    void setScheduleState( uint8_t nodeId,
                           uint8_t channelId,
                           uint8_t scheduleId,
                           const common::optional< uint8_t >& blockIdOpt,
                           uint8_t scheduleState );

    void requestScheduleState( uint8_t nodeId, uint8_t channelId, const common::optional< uint8_t >& blockIdOpt );

    void requestWeekDayEntryLockSchedule( uint8_t nodeId, uint8_t channelId, uint8_t userId, uint8_t slotId );

    void requestYearDayEntryLockSchedule( uint8_t nodeId, uint8_t channelId, uint8_t userId, uint8_t slotId );

    void requestDailyRepeatingEntryLockSchedule( uint8_t nodeId, uint8_t channelId, uint8_t userId, uint8_t slotId );

    void setBarrierSignalValue( uint8_t nodeId, uint8_t channelId, const std::string& signalName, uint8_t value );

    void requestBarrierSignalValue( uint8_t nodeId, uint8_t channelId, const std::string& signalName );

    using ThermostatSetbackValueOpt = const std::pair< float, bool >;

    void setThermostatSetbackValue( uint8_t nodeId,
                                    uint8_t channelId,
                                    const std::string& overrideType,
                                    const std::string& mode,
                                    const ThermostatSetbackValueOpt& valueOpt );

    void reset();

    void reconstructNetwork();

    void resetMeter( uint8_t nodeId, uint8_t channelId, int value, uint8_t size );

    void requestMeter( uint8_t nodeId, uint8_t channelId, const std::string& scale );

    struct NodeFirmwareInfo {
        uint8_t target;
        std::string image;
        std::string checksum;
    };

    void updateNodesFirmware( const std::vector< uint8_t >& nodeIds, const std::vector< NodeFirmwareInfo >& firmwares );

    void downloadNodeFirmware( uint8_t nodeId, uint8_t target, const std::string& path );

    Json getProvisioningInfoAsJson( uint8_t nodeId ) const;

    void setProvisioningInfo( uint8_t nodeId,
                              const std::unique_ptr< std::string >& name,
                              const std::unique_ptr< std::string >& room );

    Json getInfo() const;

    std::string getFirmwareRegion() const;

    void requestNotificationValue( uint8_t nodeId,
                                   uint8_t channelId,
                                   const std::string& notificationType,
                                   const std::string& eventType );

    void requestAlarmV1Value( uint8_t nodeId, uint8_t channelId, uint8_t alarmType );

    void checkReachableStatus( uint8_t nodeId );
    void checkNodeValues( uint8_t nodeId, std::chrono::milliseconds requestDelay, const Json& ccToCheck );

    void lockAntiTheft( uint8_t nodeId, uint8_t channelId, std::string magicCode, std::string hint );
    void lockAntiTheftDefault( uint8_t nodeId, uint8_t channelId );

    void unlockAntiTheft( uint8_t nodeId, uint8_t channelId, uint8_t classId, std::string magicCode );
    void unlockAntiTheftDefault( uint8_t nodeId, uint8_t channelId, uint8_t classId, std::string hint );

    void setThermostatMode( uint8_t nodeId, uint8_t channelId, uint8_t mode, std::string manufacturerSpecificData );

    void addDeviceSpecificConfiguration( const std::string& deviceTypeId, const Json& configuration );
    void addDeviceSpecificAssociation( const std::string& deviceTypeId, const Json& association );
    void synchronizeDeviceTime( uint8_t nodeId );
    void setWakeUpInterval( uint8_t nodeId, uint32_t value );

    void clearDeviceSpecific();

    bool hasNode( uint8_t nodeId ) const;

private:
    bool hasNode( uint8_t nodeId, uint8_t channelId ) const;
    bool hasNode( uint8_t nodeId, uint8_t channelId, uint8_t classId ) const;

    std::unique_ptr< znet::ZNode > getNode( uint8_t nodeId ) const;

    JsonObject getRawNode( uint8_t nodeId ) const;

    const IDbClient& getDBClient() const;

    void doSimpleRequest( MessageSubClasses requestClass,
                          uint8_t nodeId,
                          uint8_t channelId,
                          uint8_t classId,
                          std::function< void( Json& ) > paramsModifier = {} );

    znet::ProvisioningListEntry getProvisioningListEntry( uint8_t nodeId ) const;

    void doMulticastRequest( MessageSubClasses requestClass,
                             const std::vector< uint8_t >& nodeId,
                             uint8_t classId,
                             uint8_t channelId,
                             Json value,
                             const std::string& mode );
};

}  // namespace Zwave
}  // namespace HubPlatform
