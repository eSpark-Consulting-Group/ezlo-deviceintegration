#pragma once

#include <znet/znet.hpp>

namespace HubPlatform {
namespace Zwave {

class ZNet : public znet::ZNet {
public:
    explicit ZNet( znet::IProvisioningListProvider& classicInclusionProvider,
                   znet::IProvisioningListProvider& provider,
                   znet::IProvisioningListObserver& observer,
                   znet::IZNetOwner& znetOwner );

private:
    znet::ZNode* onFindNode( znet_node_id_t nodeId ) override;

    NodeIdsSet getNodeIds() const override;

    void znetLog( int lvl, const char* fmt, va_list list, void* arg ) override;

    void* znetAlloc( void* ptr, size_t size, void* arg ) override;

    uint32_t znetClock( void* arg ) override;

    bool uartInit() override;

    void uartClose() override;

    int znetUartWrite( const void* data, size_t size, size_t* ret_size ) override;

    int znetUartRead( void* data, size_t size, size_t* ret_size ) override;

    int znetStoreSave( size_t offset, const void* data, size_t size ) override;

    int znetStoreLoad( size_t offset, void* data, size_t size ) override;

    int znetStoreReset() override;

    void znetS2SelectKeys( znet_S2_keys_t keyList, void* arg ) override;

    void znetS2RequestPubKey( znet_S2_keys_t keyList, const znet_S2_pub_key_t* key, void* arg ) override;

    void znetS2ShowPubKey( znet_S2_keys_t keyList, const znet_S2_pub_key_t* key, void* arg ) override;

    std::unique_ptr< znet::INodeSetting > provideNodeSetting( uint16_t manufacturerId,
                                                              uint16_t productType,
                                                              uint16_t productId ) override;

    void zwaveModuleResetPinAssert() override;

    void zwaveModuleResetPinRelease() override;

    bool getKnownS0Key( uint8_t* buf ) override;

    void executeTask( std::function< void() > action, std::chrono::system_clock::duration delay ) override;

    void nodeCreate( const uint8_t nodeId ) override;

    void interviewDone( const uint8_t nodeId ) override;

    void nodeRemove( const uint8_t nodeId ) override;

    void onUpdateState( IZNet::Mode cmd, znet::ModeStatus cmdStatus, znet::ErrorOpt error ) override;

    void onNodeCheckStatusUpdated( uint8_t nodeId, znet::ModeStatus status ) override;

    void onDeviceIncludeProgressUpdate( uint8_t progress ) override;

    void onDeviceRediscoverProgressUpdate( znet_node_id_t nodeId,
                                           bool isOk,
                                           znet::ErrorOpt errorMessage,
                                           uint8_t progress ) override;

    void onLearnProgressUpdate( uint8_t devicesAmount, uint8_t currentDeviceIndex ) override;

    void onReconstructNetworkUpdate( const uint8_t devicesAmount, const uint8_t currentDeviceIndex ) override;

    void onReconstructNetworkError( const znet_node_id_t deviceId ) override;

    void onCheckReachable( znet_node_id_t nodeId ) override;

    void onUpdateReachable( znet_node_id_t nodeId, bool reachable ) override;

    void onUpdateValue( const uint8_t nodeId,
                        const uint8_t command,
                        const uint8_t channelId,
                        const uint8_t currentValue,
                        const znet::Uint8Opt targetValue,
                        const znet::SecondsOpt targetDuration ) override;

    void onUpdateTokenValue( const uint8_t nodeId,
                             const uint8_t command,
                             const uint8_t channelId,
                             const std::string& value ) override;

    void onUpdateValueAlarmSensor( const uint8_t nodeId,
                                   const uint8_t channelId,
                                   const znet::AlarmSensorType sensorType,
                                   const uint8_t value,
                                   const uint16_t seconds ) override;

    void onUpdateValueBinarySensor( const uint8_t nodeId,
                                    const uint8_t command,
                                    const uint8_t channelId,
                                    const uint8_t sensorType,
                                    const uint8_t value ) override;

    void onUpdateValueMultilevelSensor( const uint8_t nodeId,
                                        const uint8_t ccId,
                                        const uint8_t channelId,
                                        const znet::MultilevelSensor::Type type,
                                        const uint8_t scale,
                                        const float value ) override;

    void onUpdateValueDoorLockOperation( const uint8_t nodeId,
                                         const uint8_t channelId,
                                         const znet::DoorLock::Mode mode,
                                         const uint8_t insideHandlesMode,
                                         const uint8_t outsideHandlesMode,
                                         const znet::DoorCondition condition,
                                         const znet::SecondsOpt lockTimeout,
                                         const znet::DoorLock::ModeOpt targetMode,
                                         const znet::SecondsOpt targetModeTimeout ) override;

    void onUpdateValueDoorLockConfiguration(
        const uint8_t nodeId,
        const uint8_t channelId,
        const znet_door_lock_configuration_t& configuration,
        const znet_door_lock_extended_configuration_t* extendedConfiguration ) override;

    void onUpdateValueThermostatSetpoint( const uint8_t nodeId,
                                          const uint8_t channelId,
                                          const uint8_t type,
                                          const uint8_t scale,
                                          const float value,
                                          const bool isSetpointRequested ) override;

    void onUpdateValueThermostatOperatingState( const uint8_t nodeId,
                                                const uint8_t channelId,
                                                const uint8_t command,
                                                znet::ThermostatOperatingState state ) override;

    void onUpdateThermostatSetbackValue( const uint8_t nodeId,
                                         const uint8_t channelId,
                                         const znet::thermostat_setback::OverrideType overrideType,
                                         const znet::thermostat_setback::Mode mode,
                                         const znet::thermostat_setback::SetbackValueOpt valueOpt ) override;

    void onUpdateThermostatModeValue( const uint8_t nodeId,
                                      const uint8_t channelId,
                                      const uint8_t mode,
                                      znet::ManufacturerSpecificDataOpt manufacturerSpecificData ) override;

    void onUpdateFloatValue( const uint8_t nodeId,
                             const uint8_t command,
                             const uint8_t channelId,
                             const znet::meter::MeterType type,
                             const znet::meter::Scale scale,
                             const int32_t value,
                             const uint8_t precision ) override;

    void onUpdateSceneValue( const uint8_t nodeId,
                             const uint8_t command,
                             const uint8_t channelId,
                             const uint8_t sequenceNumber,
                             const uint8_t sceneNumber,
                             const std::string& keyAttribute ) override;

    void onSceneActivated( const uint8_t nodeId,
                           const uint8_t command,
                           const uint8_t channelId,
                           const uint8_t sceneNumber ) override;

    void onSimpleAvControlSet( const uint8_t nodeId,
                               const uint8_t channelId,
                               const uint8_t sequenceNumber,
                               const std::string& keyAtribute,
                               std::vector< uint16_t >&& commands ) override;

    void onUpdateRgbValue( const uint8_t nodeId,
                           const uint8_t command,
                           const uint8_t channelId,
                           znet::ColorComponentInfos&& components ) override;

    void onConfigurationReport( const uint8_t nodeId,
                                const uint8_t channelId,
                                const uint16_t configParamNum,
                                const uint8_t configParamSize,
                                const uint32_t configParamValue ) override;

    void onConfigurationRequestStatus( const uint8_t nodeId,
                                       const uint16_t configParamNum,
                                       const znet::RequestStatus::Enum status ) override;

    void onNotification( const uint8_t nodeId,
                         const uint8_t channelId,
                         const uint8_t v1AlarmType,
                         const uint8_t v1AlarmLevel,
                         znet::NotificationType notification,
                         znet::NotificationEvent event,
                         const uint8_t* data,
                         const uint8_t dataSize ) override;

    void onUnknownNotification( const uint8_t nodeId,
                                const uint8_t channelId,
                                const uint8_t v1AlarmType,
                                const uint8_t v1AlarmLevel,
                                znet::NotificationType notification,
                                znet::NotificationEvent event,
                                const uint8_t* data,
                                const uint8_t dataSize ) override;

    void onAssociationSupportedGroupingResult( const uint8_t nodeId, const uint8_t count ) override;

    void onAssociationResult( const uint8_t nodeId,
                              const uint8_t channelId,
                              const uint8_t groupId,
                              const std::vector< uint8_t >& nodes ) override;

    void onMultichannelAssociationResult( const uint8_t nodeId,
                                          const uint8_t channelId,
                                          const uint8_t groupId,
                                          const std::vector< std::pair< uint8_t, uint8_t > >& nodes ) override;

    void onUserCodeUpdated( const znet_node_id_t nodeId,
                            const znet_node_channel_id_t channelId,
                            znet::UserCode::UserCodeRanges&& userCodeRanges,
                            const znet_supported_users_t usersCount,
                            znet::UserCode::UserPollStatus userPollingStatus ) override;

    void onMasterCodeUpdated( const znet_node_id_t nodeId,
                              const znet_node_channel_id_t channelId,
                              const std::string& userCode ) override;

    void onUserCodesKeypadModeUpdated( const uint8_t nodeId,
                                       const uint8_t channelId,
                                       znet::UserCode::KeypadMode keypadMode ) override;

    void onUpdateUserCodesChecksum( const uint8_t nodeId, const uint8_t channelId, const uint16_t checksum ) override;

    void onUpdateScheduleEntryLockWeekDay( znet_node_id_t nodeId,
                                           znet_node_channel_id_t channelId,
                                           const znet_week_day_schedule_t& value ) override;

    void onUpdateScheduleEntryLockYearDay( znet_node_id_t nodeId,
                                           znet_node_channel_id_t channelId,
                                           const znet_year_day_schedule_t& value ) override;

    void onUpdateScheduleEntryLockDailyRepeating( znet_node_id_t nodeId,
                                                  znet_node_channel_id_t channelId,
                                                  const znet_daily_repeating_schedule_t& value ) override;

    void onNodeSchedule( znet_node_id_t nodeId,
                         znet_node_channel_id_t channelId,
                         uint8_t scheduleId,
                         const znet::ScheduleIdBlockOpt& idBlockOpt,
                         const znet::Schedule& value ) override;

    void onNodeScheduleState( znet_node_id_t nodeId,
                              znet_node_channel_id_t channelId,
                              const znet_cmd_schedule_state_report_t& value,
                              const znet::ScheduleIdBlockOpt& idBlockOpt ) override;

    void onUpdateSoundSwitchConfiguration( znet_node_id_t nodeId,
                                           znet_node_channel_id_t channelId,
                                           const znet_cmd_sound_switch_configuration_t& value ) override;

    void onUpdateSoundSwitchTonePlay( znet_node_id_t nodeId,
                                      znet_node_channel_id_t channelId,
                                      const znet_cmd_sound_switch_tone_play_t& value ) override;

    void onNodeTargetFirmwareUpdateProgress( const znet_node_id_t nodeId,
                                             const uint8_t target,
                                             znet::FirmwareUpdate::UpdatingResult status,
                                             uint8_t progress ) override;

    void onUpdateBarrierSignalValue( znet_node_id_t nodeId,
                                     znet_node_channel_id_t channelId,
                                     uint8_t signalId,
                                     uint8_t value ) override;

    void onHail( const znet_node_id_t nodeId ) override;

    void onIndicatorIdentify() override;

    void onNif( const znet_node_id_t nodeId ) override;

    void onSisUpdated( bool enabled ) override;

    DateTime getUtcDateTime() const override;

    DateTime getLocalDateTime() const override;

    TimeZoneSettings getTimeZoneSettings() const override;

    void setTimeZoneSettings( const char* zoneName, const char* zone ) override;

    TimerUPtr createTimer( TimerCallback cb ) override;

    void fillManufacturerSpecific( znet_manufacturer_specific_report_t& data ) override;

    void fillVersion( znet_version_report_t& data ) override;

    void fillZWavePlusInfo( znet_zwaveplus_info_report_t& data ) override;

    void fillFirmwareUpdateInfo( znet_firmware_md_report_v1_t& dataV1,
                                 znet_firmware_md_report_v3_t& dataV3,
                                 znet_firmware_md_report_v5_t& dataV5 ) override;

    void fillFirmwareLocale( std::string& locale ) override;

    void onWakeupIntervalUpdated( const znet_node_id_t nodeId, znet::Seconds interval, bool isChanged ) override;

    void onAntiTheftStateUpdated( znet_node_id_t nodeId,
                                  znet_node_channel_id_t channelId,
                                  znet_command_class_t commandClassId,
                                  const znet::AntiTheftInfoOpt& antiTheftInfo ) override;

    void onNodeCommandDelay( const uint16_t nodeId,
                             std::chrono::system_clock::time_point commandSent,
                             std::chrono::system_clock::duration delay ) override;
    void onNodeCommandFailed( const uint16_t nodeId, std::chrono::system_clock::time_point commandSent ) override;

private:
    znet::IZNetOwner& m_znetOwner;
};

}  // namespace Zwave
}  // namespace HubPlatform
