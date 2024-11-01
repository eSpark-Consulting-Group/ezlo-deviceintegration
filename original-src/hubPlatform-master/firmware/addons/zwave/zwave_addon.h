#pragma once

#include "uart.h"
#include "znet_store.h"
#include "zwave/zwave_node_manager.h"
#include "zwave/zwave_node_statistics_collector.h"
#include "zwave/zwave_addon_commands_handler.h"

#include "addon_interfaces/addon.h"
#include "service_interfaces/postponed_task.h"
#include "base_utils/zwave/zwave_stick.h"
#include "device_firmware_update_manager/default_device_firmware_update_delegate.h"
#include "zwave/zwave_backup_helper.h"
#include "zwave/zwave_vera_migration_helper.h"

#include <znet/znet.hpp>
#include <znet/znet_node.hpp>
#include <znet/znet_requests_manager.hpp>
#include <znet/znet_default_date_time_provider.hpp>
#include <znet/iznet_provisioning_list_observer.hpp>
#include <znet/znet_provisioning_list_controller.hpp>
#include <znet/cc_0x70_configuration.hpp>

#include <vector>
#include <memory>
#include <fstream>

namespace znet {
class ZNode;
}

namespace HubPlatform {
namespace Zwave {
namespace UT {
class ZwaveAddonTestSuite;
class ZwaveBackupTestSuite;
class TestZwaveAddon;
}  // namespace UT

class ZNetFactory;

class ZwaveAddon : public Addons::Addon,
                   public DefaultDeviceFirmwareUpdateDelegate,
                   public znet::IZNetOwner,
                   public znet::DefaultDateTimeProvider,
                   public znet::IProvisioningListObserver {
    friend class UT::ZwaveAddonTestSuite;
    friend class UT::ZwaveBackupTestSuite;
    friend class UT::TestZwaveAddon;

public:
    static const char* const Name;
    static const char* const Version;

    using ZWaveTask = std::function< void() >;

public:
    ZwaveAddon( const std::string& ttyPath, const std::string& znetStoreFilePath );
    ~ZwaveAddon() override;

    CommonUtils::AddonVersion getVersion() const override;

    int restoreBackup( const std::string& tempBackupFolder ) override;

public:
    // Zwave public interface
    void startInclude( const Json& params );
    void startExclude();

    void stopInclude();
    void stopExclude();

    void startExclude( const uint8_t nodeId );

    void startLearn();
    void stopLearn();

    void stopSecurityInclude( const Json& /*params*/ );

    void reset();
    void setAuthenticationMode( const Json& params );
    void setDeviceSpecificKey( const Json& params );
    void setUserCode( const Json& params );
    void requestUserCode( const Json& params );
    void requestUserCodes( const Json& params );
    void requestAssociationGroup( const Json& params );
    void setValue( const Json& params );
    void setValueMulticast( const Json& params );
    void setTokenValue( const Json& params );
    void setTokenValueMulticast( const Json& params );
    void setRgbValue( const Json& params );
    void setRgbValueMulticast( const Json& params );
    void setSoundSwitchConfiguration( const Json& params );
    void requestSoundSwitchConfiguration( const Json& params );
    void setSoundSwitchTonePlay( const Json& params );
    void requestSoundSwitchTonePlay( const Json& params );
    void setSetpointValue( const Json& params );
    void setConfigurationValue( const Json& params );
    void addConfiguration( const Json& params );
    void removeConfiguration( const Json& params );
    void resetConfigurationValue( const Json& params );
    void resetAllConfigurationValues( const Json& params );
    void requestConfigurationValue( const Json& params );
    void switchMultilevelLevelChange( const Json& params );
    void colorSwitchLevelChange( const Json& params );
    void requestValue( const Json& params );
    void requestSetpointValue( const Json& params );
    void requestAllNodesValues( const Json& );
    void requestNodeValues( const Json& params );
    void setSilentSensors( const Json& params );
    void requestSensorAlarmValue( const Json& params );
    void addNodeToAssociationGroup( const Json& params );
    void removeNodeFromAssociationGroup( const Json& params );
    void setWeekDayEntryLockSchedule( const Json& params );
    void setYearDayEntryLockSchedule( const Json& params );
    void setDailyRepeatingEntryLockSchedule( const Json& params );
    void setEntryLockScheduleEnable( const Json& params );
    void requestWeekDayEntryLockSchedule( const Json& params );
    void requestYearDayEntryLockSchedule( const Json& params );
    void requestDailyRepeatingEntryLockSchedule( const Json& params );
    void setSchedule( const Json& params );
    void requestSchedule( const Json& params );
    void setScheduleState( const Json& params );
    void requestScheduleState( const Json& params );
    void reconstructNetwork( const Json& params );
    void requestUserCodesChecksum( const Json& params );
    void requestMasterCode( const Json& params );
    void setMasterCode( const Json& params );
    void requestUserCodesKeypadMode( const Json& params );
    void setUserCodesKeypadMode( const Json& params );
    void setDoorLockMode( const Json& params );
    void requestDoorLockState( const Json& params );
    void setDoorLockConfiguration( const Json& params );
    void requestDoorLockConfiguration( const Json& params );
    void resetMeter( const Json& params );
    void requestMeter( const Json& params );
    void updateNodesFirmware( const Json& params );
    void downloadNodeFirmware( const Json& params );
    void startRediscover( const Json& params );
    void sendIdentify( const Json& params );
    void requestNotificationValue( const Json& params );
    void requestAlarmV1Value( const Json& params );
    void setBarrierSignalValue( const Json& params );
    void requestBarrierSignalValue( const Json& params );
    void setThermostatSetbackValue( const Json& params );
    void setDevicesPollIntervals( const Json& params );
    void setUnresponsiveDeviceIgnoreInterval( const Json& params );
    void setDefaultDeviceConfigurationsFile( const Json& params );
    void checkReachableStatus( const Json& params );
    void checkNodeValues( const Json& params );
    void lockAntiTheft( const Json& params );
    void lockAntiTheftDefault( const Json& params );
    void unlockAntiTheft( const Json& params );
    void unlockAntiTheftDefault( const Json& params );
    void addDeviceSpecificConfiguration( const Json& params );
    void setThermostatMode( const Json& params );
    void addDeviceSpecificAssociation( const Json& params );
    void clearDeviceSpecific( const Json& params );
    void synchronizeDeviceTime( const Json& params );
    Json getZwaveInfo() const;
    void setWakeUpInterval( const Json& params );

    void setTestMode();

private:
    void addZWaveTask( ZWaveTask&& task );

    template < typename Type, typename MethodSet, typename MethodRemove, typename Getter >
    void setScheduleEntryLockValue( const Json& params,
                                    MethodSet methodSet,
                                    MethodRemove methodRemove,
                                    Getter&& getter,
                                    const char* name );

    template < typename Method >
    void requestScheduleEntryLockValue( const Json& params, Method method );

private:
    ZwaveAddon( const std::string& name,
                const std::string& ttyPath,
                const std::string& znetStoreFilePath,
                std::unique_ptr< Services::BusWrapper > impl,
                std::unique_ptr< IDbClient > dbClient,
                std::unique_ptr< ZWaveStick > zwaveStick,
                std::unique_ptr< ZNetFactory > znetFactory );

    void setUp() override;

    void tearDown() override;

    JsonObject handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) override;
    bool handleAsyncRequestFromBus( const JsonObject& request,
                                    MessageSubClasses requestType,
                                    const std::string& uuid,
                                    const std::string& sender ) override;
    void handleMessageFromBus( const Services::Message& message, const std::string& sender ) override;

    void handleLegacyMethodCall( const JsonObject& json, const std::string& sender ) override;
    void handleMethodCall( const JsonObject& json, MessageSubClasses requestType, const std::string& sender ) override;

    void updateLeds();

public:
    void defineDaemonParameters( tiny::command_line::parser& parser ) override;

    // DeviceFirmwareUpdateManager::delegate interface
    bool startFirmwareUpdate( const DeviceFirmwareUpdateManager::Device& device,
                              const DeviceFirmwareUpdateManager::Target& target,
                              const DeviceFirmwareUpdateManager::Path& destination,
                              std::function< void( bool ) > resultCallback ) override;

    void onFirmwareDownloadStatus( const DeviceFirmwareUpdateManager::ExtendedLink& link,
                                   bool isOk,
                                   uint8_t progress ) override;

    void addFirmwareUpdateTask( std::function< void() > task ) override;

    void onFirmwareUpdateFinished( const DeviceFirmwareUpdateManager::Device& device,
                                   const DeviceFirmwareUpdateManager::TargetsUpdateResults& targetsResults ) override;

    void onFirmwareUpdateStarted( const DeviceFirmwareUpdateManager::Device& device ) override;

    // IZnet interface
    znet::ZNode* onFindNode( znet_node_id_t nodeId ) final;

    NodeIdsSet getNodeIds() const final;

    void znetLog( int lvl, const char* fmt, va_list list, void* arg ) final;
    void* znetAlloc( void* ptr, size_t size, void* arg ) final;
    uint32_t znetClock( void* arg ) final;

    bool uartInit() final;
    void uartClose() final;
    int znetUartWrite( const void* data, size_t size, size_t* retSize ) final;
    int znetUartRead( void* data, size_t size, size_t* ret_size ) final;
    int znetStoreSave( size_t offset, const void* data, size_t size ) final;
    int znetStoreLoad( size_t offset, void* data, size_t size ) final;
    int znetStoreReset() final;

    void znetS2SelectKeys( znet_S2_keys_t keyList, void* arg ) final;
    void znetS2RequestPubKey( znet_S2_keys_t keyList, const znet_S2_pub_key_t* key, void* arg ) final;
    void znetS2ShowPubKey( znet_S2_keys_t keyList, const znet_S2_pub_key_t* key, void* arg ) final;

    std::unique_ptr< znet::INodeSetting > provideNodeSetting( uint16_t manufacturerId,
                                                              uint16_t productType,
                                                              uint16_t productId ) final;

    void zwaveModuleResetPinAssert() final;
    void zwaveModuleResetPinRelease() final;

    bool getKnownS0Key( uint8_t* buf ) final;

    void executeTask( std::function< void() > action, std::chrono::system_clock::duration delay ) final;

    void nodeCreate( const uint8_t nodeId ) final;

    void interviewDone( const uint8_t nodeId ) final;

    void nodeRemove( const uint8_t nodeId ) final;

    void onUpdateState( znet::IZNet::Mode cmd, znet::ModeStatus cmdStatus, znet::ErrorOpt error ) final;

    void onNodeCheckStatusUpdated( uint8_t nodeId, znet::ModeStatus status ) final;

    void onDeviceIncludeProgressUpdate( uint8_t progress ) final;

    void onDeviceRediscoverProgressUpdate( znet_node_id_t nodeId,
                                           bool isOk,
                                           znet::ErrorOpt errorMessage,
                                           uint8_t progress ) final;

    void onLearnProgressUpdate( uint8_t devicesAmount, uint8_t currentDeviceIndex ) final;

    void onReconstructNetworkUpdate( const uint8_t devicesAmount, const uint8_t currentDeviceIndex ) final;

    void onReconstructNetworkError( const znet_node_id_t deviceId ) final;

    void onCheckReachable( uint8_t nodeId ) final;

    void onUpdateReachable( uint8_t nodeId, bool reachable ) final;

    void onUpdateValue( const uint8_t nodeId,
                        const uint8_t command,
                        const uint8_t channelId,
                        const uint8_t currentValue,
                        const znet::Uint8Opt targetValue,
                        const znet::SecondsOpt targetDuration ) final;

    void onUpdateTokenValue( const uint8_t nodeId,
                             const uint8_t command,
                             const uint8_t channelId,
                             const std::string& value ) final;

    void onUpdateValueAlarmSensor( const uint8_t nodeId,
                                   const uint8_t channelId,
                                   const znet::AlarmSensorType sensorType,
                                   const uint8_t value,
                                   const uint16_t seconds ) final;

    void onUpdateValueBinarySensor( const uint8_t nodeId,
                                    const uint8_t command,
                                    const uint8_t channelId,
                                    const uint8_t sensorType,
                                    const uint8_t value ) final;

    void onUpdateValueMultilevelSensor( const uint8_t nodeId,
                                        const uint8_t ccId,
                                        const uint8_t channelId,
                                        const znet::MultilevelSensor::Type type,
                                        const uint8_t scale,
                                        const float value ) final;

    void onUpdateValueDoorLockOperation( const uint8_t nodeId,
                                         const uint8_t channelId,
                                         const znet::DoorLock::Mode mode,
                                         const uint8_t insideHandlesMode,
                                         const uint8_t outsideHandlesMode,
                                         const znet::DoorCondition condition,
                                         const znet::SecondsOpt lockTimeout,
                                         const znet::DoorLock::ModeOpt targetMode,
                                         const znet::SecondsOpt targetModeTimeout ) final;

    void onUpdateValueDoorLockConfiguration(
        const uint8_t nodeId,
        const uint8_t channelId,
        const znet_door_lock_configuration_t& configuration,
        const znet_door_lock_extended_configuration_t* extendedConfiguration ) final;

    void onUpdateValueThermostatSetpoint( const uint8_t nodeId,
                                          const uint8_t channelId,
                                          const uint8_t type,
                                          const uint8_t scale,
                                          const float value,
                                          const bool isSetpointRequested ) final;

    void onUpdateThermostatModeValue( const uint8_t nodeId,
                                      const uint8_t channelId,
                                      const uint8_t mode,
                                      znet::ManufacturerSpecificDataOpt manufacturerSpecificData ) override;

    void onUpdateValueThermostatOperatingState( const uint8_t nodeId,
                                                const uint8_t channelId,
                                                const uint8_t command,
                                                znet::ThermostatOperatingState state ) final;

    void onUpdateThermostatSetbackValue( const uint8_t nodeId,
                                         const uint8_t channelId,
                                         const znet::thermostat_setback::OverrideType overrideType,
                                         const znet::thermostat_setback::Mode mode,
                                         const znet::thermostat_setback::SetbackValueOpt valueOpt ) final;

    void onUpdateFloatValue( const uint8_t nodeId,
                             const uint8_t command,
                             const uint8_t channelId,
                             const znet::meter::MeterType type,
                             const znet::meter::Scale scale,
                             const int32_t value,
                             const uint8_t precision ) final;

    void onUpdateSceneValue( const uint8_t nodeId,
                             const uint8_t command,
                             const uint8_t channelId,
                             const uint8_t sequenceNumber,
                             const uint8_t sceneNumber,
                             const std::string& keyAttribute ) final;

    void onSceneActivated( const uint8_t nodeId,
                           const uint8_t command,
                           const uint8_t channelId,
                           const uint8_t sceneNumber ) final;

    void onSimpleAvControlSet( const uint8_t nodeId,
                               const uint8_t channelId,
                               const uint8_t sequenceNumber,
                               const std::string& keyAtribute,
                               std::vector< uint16_t >&& commands ) final;

    void onUpdateRgbValue( const uint8_t nodeId,
                           const uint8_t command,
                           const uint8_t channelId,
                           znet::ColorComponentInfos&& components ) final;

    void onConfigurationReport( const uint8_t nodeId,
                                const uint8_t channelId,
                                const uint16_t configParamNum,
                                const uint8_t configParamSize,
                                const uint32_t configParamValue ) final;

    void onConfigurationRequestStatus( const uint8_t nodeId,
                                       const uint16_t configParamNum,
                                       const znet::RequestStatus::Enum status ) final;

    void onUnknownNotification( const uint8_t nodeId,
                                const uint8_t channelId,
                                const uint8_t v1AlarmType,
                                const uint8_t v1AlarmLevel,
                                znet::NotificationType notification,
                                znet::NotificationEvent event,
                                const uint8_t* data,
                                const uint8_t dataSize ) final;

    void onNotification( const uint8_t nodeId,
                         const uint8_t channelId,
                         const uint8_t v1AlarmType,
                         const uint8_t v1AlarmLevel,
                         znet::NotificationType notification,
                         znet::NotificationEvent event,
                         const uint8_t* data,
                         const uint8_t dataSize ) final;

    void onAssociationSupportedGroupingResult( const uint8_t nodeId, const uint8_t count ) final;

    void onAssociationResult( const uint8_t nodeId,
                              const uint8_t channelId,
                              const uint8_t groupId,
                              const std::vector< uint8_t >& nodes ) final;

    void onMultichannelAssociationResult( const uint8_t nodeId,
                                          const uint8_t channelId,
                                          const uint8_t groupId,
                                          const std::vector< std::pair< uint8_t, uint8_t > >& nodes ) final;

    void onUserCodeUpdated( const znet_node_id_t nodeId,
                            const znet_node_channel_id_t channelId,
                            znet::UserCode::UserCodeRanges&& userCodeRanges,
                            znet_supported_users_t usersCount,
                            znet::UserCode::UserPollStatus userPollingStatus ) final;

    void onMasterCodeUpdated( const znet_node_id_t nodeId,
                              const znet_node_channel_id_t channelId,
                              const std::string& masterCode ) final;

    void onUserCodesKeypadModeUpdated( const uint8_t nodeId,
                                       const uint8_t channelId,
                                       znet::UserCode::KeypadMode keypadMode ) final;

    void onUpdateUserCodesChecksum( const uint8_t nodeId, const uint8_t channelId, const uint16_t checksum ) final;

    void onUpdateScheduleEntryLockWeekDay( znet_node_id_t nodeId,
                                           znet_node_channel_id_t channelId,
                                           const znet_week_day_schedule_t& value ) final;

    void onUpdateScheduleEntryLockYearDay( znet_node_id_t nodeId,
                                           znet_node_channel_id_t channelId,
                                           const znet_year_day_schedule_t& value ) final;

    void onUpdateScheduleEntryLockDailyRepeating( znet_node_id_t nodeId,
                                                  znet_node_channel_id_t channelId,
                                                  const znet_daily_repeating_schedule_t& value ) final;

    void onNodeSchedule( znet_node_id_t nodeId,
                         znet_node_channel_id_t channelId,
                         uint8_t scheduleId,
                         const znet::ScheduleIdBlockOpt& idBlockOpt,
                         const znet::Schedule& value ) final;

    void onNodeScheduleState( znet_node_id_t nodeId,
                              znet_node_channel_id_t channelId,
                              const znet_cmd_schedule_state_report_t& value,
                              const znet::ScheduleIdBlockOpt& idBlockOpt ) final;

    void onUpdateSoundSwitchConfiguration( znet_node_id_t nodeId,
                                           znet_node_channel_id_t channelId,
                                           const znet_cmd_sound_switch_configuration_t& value ) final;

    void onUpdateSoundSwitchTonePlay( znet_node_id_t nodeId,
                                      znet_node_channel_id_t channelId,
                                      const znet_cmd_sound_switch_tone_play_t& value ) final;

    void onNodeTargetFirmwareUpdateProgress( const znet_node_id_t nodeId,
                                             const uint8_t target,
                                             znet::FirmwareUpdate::UpdatingResult status,
                                             uint8_t progress ) final;

    void onUpdateBarrierSignalValue( znet_node_id_t nodeId,
                                     znet_node_channel_id_t channelId,
                                     uint8_t signalId,
                                     uint8_t value ) final;

    void onHail( const znet_node_id_t nodeId ) final;

    void onIndicatorIdentify() final;

    void onNif( const znet_node_id_t nodeId ) final;

    void onSisUpdated( bool enabled ) final;

    TimerUPtr createTimer( TimerCallback ) final;

    void fillManufacturerSpecific( znet_manufacturer_specific_report_t& data ) final;

    void fillVersion( znet_version_report_t& data ) final;

    void fillZWavePlusInfo( znet_zwaveplus_info_report_t& data ) final;

    void fillFirmwareUpdateInfo( znet_firmware_md_report_v1_t& dataV1,
                                 znet_firmware_md_report_v3_t& dataV3,
                                 znet_firmware_md_report_v5_t& dataV5 ) final;

    void fillFirmwareLocale( std::string& locale ) final;

    void setTimeZoneSettings( const char* zoneName, const char* zone ) final;

    void onWakeupIntervalUpdated( const znet_node_id_t nodeId, znet::Seconds interval, bool isChanged ) override;

    void onAntiTheftStateUpdated( znet_node_id_t nodeId,
                                  znet_node_channel_id_t channelId,
                                  znet_command_class_t commandClassId,
                                  const znet::AntiTheftInfoOpt& antiTheftInfo ) override;

    void onNodeCommandDelay( const uint16_t nodeId,
                             std::chrono::system_clock::time_point commandSent,
                             std::chrono::system_clock::duration delay ) override;
    void onNodeCommandFailed( const uint16_t nodeId, std::chrono::system_clock::time_point commandSent ) override;

public:
    void onProvisioningListEntryAdded( const znet::ProvisioningListEntry& entry ) override;
    void onProvisioningListEntryUpdated( const znet::ProvisioningListEntry& oldEntry,
                                         const znet::ProvisioningListEntry& newEntry ) override;
    void onProvisioningListEntryRemoved( uint32_t provisioningId ) override;

private:
    using DSKStrings = std::vector< std::string >;

    static bool isSupportedEventStatus( znet::ModeStatus cmdStatus );

    template < typename ZClassType >
    std::shared_ptr< ZClassType > findCommandClassSafe( znet_node_id_t nodeId,
                                                        znet_node_channel_id_t channelId,
                                                        const char* methodName );

    std::shared_ptr< znet::ZClass > findCommandClassSafe( znet_node_id_t nodeId,
                                                          znet_node_channel_id_t channelId,
                                                          znet_command_class_t classId,
                                                          const char* methodName );

    void startChipBackup( const std::string& folder, bool backupCreation );

    std::string doChipBackupSync( const std::string& folder, bool backupCreation );

    // vera backup prototype
    void onVeraBackupMigrationRequested( const std::string& folder );

    void rediscoverNextVeraDevice();

    bool initZwaveSync();

    void onVeraBackupStatus( const std::string& status, const std::string& errorDescription );

private:
    std::string m_ttyPath;
    std::string m_znetStoreFilePath;
    std::string m_deviceSettingPath;

    std::unique_ptr< ZNetFactory > m_znetFactory;

    std::unique_ptr< Uart > m_uart;
    std::unique_ptr< ZNetStore > m_store;
    const std::string& m_serviceName = getQualifiedName();

    std::shared_ptr< NodeManager > m_nodeManager;
    std::shared_ptr< NodeStatisticsCollector > m_nodeStatisticsCollector;
    std::unique_ptr< znet::IZNet > m_znet;
    std::unique_ptr< ZWaveStick > m_zwaveStick;
    std::unique_ptr< znet::IProvisioningListProvider > m_provisioningListProvider;
    std::unique_ptr< znet::IProvisioningListProvider > m_classicInclusionSecurityProvider;
    std::unique_ptr< ZwaveAddonCommandsHandler > m_zwaveAddonCommandsHandler;
    std::unique_ptr< DeviceFirmwareUpdateManager > m_deviceFirmwareUpdateManager;

    std::thread m_zwaveThread;
    TaskLoop m_zwaveTaskLoop;
    std::atomic_bool m_wasZwaveIndicatorInitialized{ false };
    std::atomic_bool m_wasZwaveInitialized{ false };
    bool m_isTestMode = false;
    bool m_isVeraBackupMode = false;
    std::mutex m_mutex;
    std::condition_variable m_initCondVar;

    Services::PostponedTaskPtr m_zwaveLoopTask;

    std::unique_ptr< BackupHelper > m_backupHelper;
    std::unique_ptr< VeraMigrationHelper > m_veraMigrationHelper;

    struct LastZwaveState {
        LastZwaveState( znet::IZNet::Mode cmd = znet::IZNet::Mode::Initializing,
                        znet::ModeStatus cmdStatus = znet::ModeStatus::Undefined )
        : cmd( cmd )
        , cmdStatus( cmdStatus ) {}

        znet::IZNet::Mode cmd;
        znet::ModeStatus cmdStatus;
    };
    threads::UniqueSafeWrapper< LastZwaveState > m_lastZwaveState;

    const static std::string EnvmFileName;

    struct AsyncRequestInfo {
        AsyncRequestInfo( JsonObject request, std::string uuid, std::string sender )
        : m_request( request )
        , m_uuid( uuid )
        , m_sender( sender ) {}

        JsonObject m_request;
        std::string m_uuid;
        std::string m_sender;
    };

    std::unique_ptr< AsyncRequestInfo > m_veraBackupRequestInfo;
};

}  // namespace Zwave
}  // namespace HubPlatform
