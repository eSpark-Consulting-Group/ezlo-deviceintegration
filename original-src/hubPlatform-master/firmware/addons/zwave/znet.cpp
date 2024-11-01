#include "zwave/znet.h"

#include <znet/iznet_node_setting.hpp>

namespace HubPlatform {
namespace Zwave {

ZNet::ZNet( znet::IProvisioningListProvider& classicInclusionProvider,
            znet::IProvisioningListProvider& provider,
            znet::IProvisioningListObserver& observer,
            znet::IZNetOwner& znetOwner )
: znet::ZNet( classicInclusionProvider, provider, observer )
, m_znetOwner( znetOwner ) {}

znet::ZNode* ZNet::onFindNode( znet_node_id_t nodeId ) {
    return m_znetOwner.onFindNode( nodeId );
}

ZNet::NodeIdsSet ZNet::getNodeIds() const {
    return m_znetOwner.getNodeIds();
}

void ZNet::znetLog( int lvl, const char* fmt, va_list list, void* arg ) {
    m_znetOwner.znetLog( lvl, fmt, list, arg );
}

void* ZNet::znetAlloc( void* ptr, size_t size, void* arg ) {
    return m_znetOwner.znetAlloc( ptr, size, arg );
}

uint32_t ZNet::znetClock( void* arg ) {
    return m_znetOwner.znetClock( arg );
}

bool ZNet::uartInit() {
    return m_znetOwner.uartInit();
}

void ZNet::uartClose() {
    m_znetOwner.uartClose();
}

int ZNet::znetUartWrite( const void* data, size_t size, size_t* ret_size ) {
    return m_znetOwner.znetUartWrite( data, size, ret_size );
}

int ZNet::znetUartRead( void* data, size_t size, size_t* ret_size ) {
    return m_znetOwner.znetUartRead( data, size, ret_size );
}

int ZNet::znetStoreSave( size_t offset, const void* data, size_t size ) {
    return m_znetOwner.znetStoreSave( offset, data, size );
}

int ZNet::znetStoreLoad( size_t offset, void* data, size_t size ) {
    return m_znetOwner.znetStoreLoad( offset, data, size );
}

int ZNet::znetStoreReset() {
    return m_znetOwner.znetStoreReset();
}

void ZNet::znetS2SelectKeys( znet_S2_keys_t keyList, void* arg ) {
    m_znetOwner.znetS2SelectKeys( keyList, arg );
}

void ZNet::znetS2RequestPubKey( znet_S2_keys_t keyList, const znet_S2_pub_key_t* key, void* arg ) {
    m_znetOwner.znetS2RequestPubKey( keyList, key, arg );
}

void ZNet::znetS2ShowPubKey( znet_S2_keys_t keyList, const znet_S2_pub_key_t* key, void* arg ) {
    m_znetOwner.znetS2ShowPubKey( keyList, key, arg );
}

std::unique_ptr< znet::INodeSetting > ZNet::provideNodeSetting( uint16_t manufacturerId,
                                                                uint16_t productType,
                                                                uint16_t productId ) {
    return m_znetOwner.provideNodeSetting( manufacturerId, productType, productId );
}

void ZNet::zwaveModuleResetPinAssert() {
    m_znetOwner.zwaveModuleResetPinAssert();
}

void ZNet::zwaveModuleResetPinRelease() {
    m_znetOwner.zwaveModuleResetPinRelease();
}

bool ZNet::getKnownS0Key( uint8_t* buf ) {
    return m_znetOwner.getKnownS0Key( buf );
}

void ZNet::executeTask( std::function< void() > action, std::chrono::system_clock::duration delay ) {
    m_znetOwner.executeTask( action, delay );
}

void ZNet::nodeCreate( const uint8_t nodeId ) {
    m_znetOwner.nodeCreate( nodeId );
}

void ZNet::interviewDone( const uint8_t nodeId ) {
    m_znetOwner.interviewDone( nodeId );
}

void ZNet::nodeRemove( const uint8_t nodeId ) {
    m_znetOwner.nodeRemove( nodeId );
}

void ZNet::onUpdateState( IZNet::Mode cmd, znet::ModeStatus cmdStatus, znet::ErrorOpt error ) {
    m_znetOwner.onUpdateState( cmd, cmdStatus, std::move( error ) );
}

void ZNet::onNodeCheckStatusUpdated( uint8_t nodeId, znet::ModeStatus status ) {
    m_znetOwner.onNodeCheckStatusUpdated( nodeId, status );
}

void ZNet::onDeviceIncludeProgressUpdate( uint8_t progress ) {
    m_znetOwner.onDeviceIncludeProgressUpdate( progress );
}

void ZNet::onDeviceRediscoverProgressUpdate( znet_node_id_t nodeId,
                                             bool isOk,
                                             znet::ErrorOpt errorMessage,
                                             uint8_t progress ) {
    m_znetOwner.onDeviceRediscoverProgressUpdate( nodeId, isOk, errorMessage, progress );
}

void ZNet::onLearnProgressUpdate( uint8_t devicesAmount, uint8_t currentDeviceIndex ) {
    m_znetOwner.onLearnProgressUpdate( devicesAmount, currentDeviceIndex );
}

void ZNet::onReconstructNetworkUpdate( const uint8_t devicesAmount, const uint8_t currentDeviceIndex ) {
    m_znetOwner.onReconstructNetworkUpdate( devicesAmount, currentDeviceIndex );
}

void ZNet::onReconstructNetworkError( const znet_node_id_t deviceId ) {
    m_znetOwner.onReconstructNetworkError( deviceId );
}

void ZNet::onCheckReachable( znet_node_id_t nodeId ) {
    m_znetOwner.onCheckReachable( nodeId );
}

void ZNet::onUpdateReachable( znet_node_id_t nodeId, bool reachable ) {
    m_znetOwner.onUpdateReachable( nodeId, reachable );
}

void ZNet::onUpdateValue( const uint8_t nodeId,
                          const uint8_t command,
                          const uint8_t channelId,
                          const uint8_t currentValue,
                          const znet::Uint8Opt targetValue,
                          const znet::SecondsOpt targetDuration ) {
    m_znetOwner.onUpdateValue( nodeId, command, channelId, currentValue, targetValue, targetDuration );
}

void ZNet::onUpdateTokenValue( const uint8_t nodeId,
                               const uint8_t command,
                               const uint8_t channelId,
                               const std::string& value ) {
    m_znetOwner.onUpdateTokenValue( nodeId, command, channelId, value );
}

void ZNet::onUpdateValueAlarmSensor( const uint8_t nodeId,
                                     const uint8_t channelId,
                                     const znet::AlarmSensorType sensorType,
                                     const uint8_t value,
                                     const uint16_t seconds ) {
    m_znetOwner.onUpdateValueAlarmSensor( nodeId, channelId, sensorType, value, seconds );
}

void ZNet::onUpdateValueBinarySensor( const uint8_t nodeId,
                                      const uint8_t command,
                                      const uint8_t channelId,
                                      const uint8_t sensorType,
                                      const uint8_t value ) {
    m_znetOwner.onUpdateValueBinarySensor( nodeId, command, channelId, sensorType, value );
}

void ZNet::onUpdateValueMultilevelSensor( const uint8_t nodeId,
                                          const uint8_t ccId,
                                          const uint8_t channelId,
                                          const znet::MultilevelSensor::Type type,
                                          const uint8_t scale,
                                          const float value ) {
    m_znetOwner.onUpdateValueMultilevelSensor( nodeId, ccId, channelId, type, scale, value );
}

void ZNet::onUpdateValueDoorLockOperation( const uint8_t nodeId,
                                           const uint8_t channelId,
                                           const znet::DoorLock::Mode mode,
                                           const uint8_t insideHandlesMode,
                                           const uint8_t outsideHandlesMode,
                                           const znet::DoorCondition condition,
                                           const znet::SecondsOpt lockTimeout,
                                           const znet::DoorLock::ModeOpt targetMode,
                                           const znet::SecondsOpt targetModeTimeout ) {
    m_znetOwner.onUpdateValueDoorLockOperation( nodeId,
                                                channelId,
                                                mode,
                                                insideHandlesMode,
                                                outsideHandlesMode,
                                                condition,
                                                lockTimeout,
                                                targetMode,
                                                targetModeTimeout );
}

void ZNet::onUpdateValueDoorLockConfiguration( const uint8_t nodeId,
                                               const uint8_t channelId,
                                               const znet_door_lock_configuration_t& configuration,
                                               const znet_door_lock_extended_configuration_t* extendedConfiguration ) {
    m_znetOwner.onUpdateValueDoorLockConfiguration( nodeId, channelId, configuration, extendedConfiguration );
}

void ZNet::onUpdateValueThermostatSetpoint( const uint8_t nodeId,
                                            const uint8_t channelId,
                                            const uint8_t type,
                                            const uint8_t scale,
                                            const float value,
                                            const bool isSetpointRequested ) {
    m_znetOwner.onUpdateValueThermostatSetpoint( nodeId, channelId, type, scale, value, isSetpointRequested );
}

void ZNet::onUpdateValueThermostatOperatingState( const uint8_t nodeId,
                                                  const uint8_t channelId,
                                                  const uint8_t command,
                                                  znet::ThermostatOperatingState state ) {
    m_znetOwner.onUpdateValueThermostatOperatingState( nodeId, channelId, command, state );
}

void ZNet::onUpdateThermostatSetbackValue( const uint8_t nodeId,
                                           const uint8_t channelId,
                                           const znet::thermostat_setback::OverrideType overrideType,
                                           const znet::thermostat_setback::Mode mode,
                                           const znet::thermostat_setback::SetbackValueOpt valueOpt ) {
    m_znetOwner.onUpdateThermostatSetbackValue( nodeId, channelId, overrideType, mode, valueOpt );
}

void ZNet::onUpdateThermostatModeValue( const uint8_t nodeId,
                                        const uint8_t channelId,
                                        const uint8_t mode,
                                        znet::ManufacturerSpecificDataOpt manufacturerSpecificData ) {
    m_znetOwner.onUpdateThermostatModeValue( nodeId, channelId, mode, std::move( manufacturerSpecificData ) );
}

void ZNet::onUpdateFloatValue( const uint8_t nodeId,
                               const uint8_t command,
                               const uint8_t channelId,
                               const znet::meter::MeterType type,
                               const znet::meter::Scale scale,
                               const int32_t value,
                               const uint8_t precision ) {
    m_znetOwner.onUpdateFloatValue( nodeId, command, channelId, type, scale, value, precision );
}

void ZNet::onUpdateSceneValue( const uint8_t nodeId,
                               const uint8_t command,
                               const uint8_t channelId,
                               const uint8_t sequenceNumber,
                               const uint8_t sceneNumber,
                               const std::string& keyAttribute ) {
    m_znetOwner.onUpdateSceneValue( nodeId, command, channelId, sequenceNumber, sceneNumber, keyAttribute );
}

void ZNet::onSceneActivated( const uint8_t nodeId,
                             const uint8_t command,
                             const uint8_t channelId,
                             const uint8_t sceneNumber ) {
    m_znetOwner.onSceneActivated( nodeId, command, channelId, sceneNumber );
}

void ZNet::onSimpleAvControlSet( const uint8_t nodeId,
                                 const uint8_t channelId,
                                 const uint8_t sequenceNumber,
                                 const std::string& keyAtribute,
                                 std::vector< uint16_t >&& commands ) {
    m_znetOwner.onSimpleAvControlSet( nodeId, channelId, sequenceNumber, keyAtribute, std::move( commands ) );
}

void ZNet::onUpdateRgbValue( const uint8_t nodeId,
                             const uint8_t command,
                             const uint8_t channelId,
                             znet::ColorComponentInfos&& components ) {
    m_znetOwner.onUpdateRgbValue( nodeId, command, channelId, std::move( components ) );
}

void ZNet::onConfigurationReport( const uint8_t nodeId,
                                  const uint8_t channelId,
                                  const uint16_t configParamNum,
                                  const uint8_t configParamSize,
                                  const uint32_t configParamValue ) {
    m_znetOwner.onConfigurationReport( nodeId, channelId, configParamNum, configParamSize, configParamValue );
}

void ZNet::onConfigurationRequestStatus( const uint8_t nodeId,
                                         const uint16_t configParamNum,
                                         const znet::RequestStatus::Enum status ) {
    m_znetOwner.onConfigurationRequestStatus( nodeId, configParamNum, status );
}

void ZNet::onNotification( const uint8_t nodeId,
                           const uint8_t channelId,
                           const uint8_t v1AlarmType,
                           const uint8_t v1AlarmLevel,
                           znet::NotificationType notification,
                           znet::NotificationEvent event,
                           const uint8_t* data,
                           const uint8_t dataSize ) {
    m_znetOwner.onNotification( nodeId, channelId, v1AlarmType, v1AlarmLevel, notification, event, data, dataSize );
}

void ZNet::onUnknownNotification( const uint8_t nodeId,
                                  const uint8_t channelId,
                                  const uint8_t v1AlarmType,
                                  const uint8_t v1AlarmLevel,
                                  znet::NotificationType notification,
                                  znet::NotificationEvent event,
                                  const uint8_t* data,
                                  const uint8_t dataSize ) {
    m_znetOwner.onUnknownNotification(
        nodeId, channelId, v1AlarmType, v1AlarmLevel, notification, event, data, dataSize );
}

void ZNet::onAssociationSupportedGroupingResult( const uint8_t nodeId, const uint8_t count ) {
    m_znetOwner.onAssociationSupportedGroupingResult( nodeId, count );
}

void ZNet::onAssociationResult( const uint8_t nodeId,
                                const uint8_t channelId,
                                const uint8_t groupId,
                                const std::vector< uint8_t >& nodes ) {
    m_znetOwner.onAssociationResult( nodeId, channelId, groupId, nodes );
}

void ZNet::onMultichannelAssociationResult( const uint8_t nodeId,
                                            const uint8_t channelId,
                                            const uint8_t groupId,
                                            const std::vector< std::pair< uint8_t, uint8_t > >& nodes ) {
    m_znetOwner.onMultichannelAssociationResult( nodeId, channelId, groupId, nodes );
}

void ZNet::onUserCodeUpdated( const znet_node_id_t nodeId,
                              const znet_node_channel_id_t channelId,
                              znet::UserCode::UserCodeRanges&& userCodeRanges,
                              const znet_supported_users_t usersCount,
                              znet::UserCode::UserPollStatus userPollingStatus ) {
    m_znetOwner.onUserCodeUpdated( nodeId, channelId, std::move( userCodeRanges ), usersCount, userPollingStatus );
}

void ZNet::onMasterCodeUpdated( const uint8_t nodeId, const uint8_t channelId, const std::string& userCode ) {
    m_znetOwner.onMasterCodeUpdated( nodeId, channelId, userCode );
}

void ZNet::onUserCodesKeypadModeUpdated( const uint8_t nodeId,
                                         const uint8_t channelId,
                                         znet::UserCode::KeypadMode keypadMode ) {
    m_znetOwner.onUserCodesKeypadModeUpdated( nodeId, channelId, keypadMode );
}

void ZNet::onUpdateUserCodesChecksum( const uint8_t nodeId, const uint8_t channelId, const uint16_t checksum ) {
    m_znetOwner.onUpdateUserCodesChecksum( nodeId, channelId, checksum );
}

void ZNet::onUpdateScheduleEntryLockWeekDay( znet_node_id_t nodeId,
                                             znet_node_channel_id_t channelId,
                                             const znet_week_day_schedule_t& value ) {
    m_znetOwner.onUpdateScheduleEntryLockWeekDay( nodeId, channelId, value );
}

void ZNet::onUpdateScheduleEntryLockYearDay( znet_node_id_t nodeId,
                                             znet_node_channel_id_t channelId,
                                             const znet_year_day_schedule_t& value ) {
    m_znetOwner.onUpdateScheduleEntryLockYearDay( nodeId, channelId, value );
}

void ZNet::onUpdateScheduleEntryLockDailyRepeating( znet_node_id_t nodeId,
                                                    znet_node_channel_id_t channelId,
                                                    const znet_daily_repeating_schedule_t& value ) {
    m_znetOwner.onUpdateScheduleEntryLockDailyRepeating( nodeId, channelId, value );
}

void ZNet::onNodeSchedule( znet_node_id_t nodeId,
                           znet_node_channel_id_t channelId,
                           uint8_t scheduleId,
                           const znet::ScheduleIdBlockOpt& idBlockOpt,
                           const znet::Schedule& value ) {
    m_znetOwner.onNodeSchedule( nodeId, channelId, scheduleId, idBlockOpt, value );
}

void ZNet::onNodeScheduleState( znet_node_id_t nodeId,
                                znet_node_channel_id_t channelId,
                                const znet_cmd_schedule_state_report_t& value,
                                const znet::ScheduleIdBlockOpt& idBlockOpt ) {
    m_znetOwner.onNodeScheduleState( nodeId, channelId, value, idBlockOpt );
}

void ZNet::onUpdateSoundSwitchConfiguration( znet_node_id_t nodeId,
                                             znet_node_channel_id_t channelId,
                                             const znet_cmd_sound_switch_configuration_t& value ) {
    m_znetOwner.onUpdateSoundSwitchConfiguration( nodeId, channelId, value );
}

void ZNet::onUpdateSoundSwitchTonePlay( znet_node_id_t nodeId,
                                        znet_node_channel_id_t channelId,
                                        const znet_cmd_sound_switch_tone_play_t& value ) {
    m_znetOwner.onUpdateSoundSwitchTonePlay( nodeId, channelId, value );
}

void ZNet::onNodeTargetFirmwareUpdateProgress( const znet_node_id_t nodeId,
                                               const uint8_t target,
                                               znet::FirmwareUpdate::UpdatingResult status,
                                               uint8_t progress ) {
    m_znetOwner.onNodeTargetFirmwareUpdateProgress( nodeId, target, status, progress );
}

void ZNet::onUpdateBarrierSignalValue( znet_node_id_t nodeId,
                                       znet_node_channel_id_t channelId,
                                       uint8_t signalId,
                                       uint8_t value ) {
    m_znetOwner.onUpdateBarrierSignalValue( nodeId, channelId, signalId, value );
}

void ZNet::onHail( const uint8_t nodeId ) {
    m_znetOwner.onHail( nodeId );
}

void ZNet::onIndicatorIdentify() {
    m_znetOwner.onIndicatorIdentify();
}

void ZNet::onNif( const znet_node_id_t nodeId ) {
    m_znetOwner.onNif( nodeId );
}

void ZNet::onSisUpdated( bool enabled ) {
    m_znetOwner.onSisUpdated( enabled );
}

ZNet::DateTime ZNet::getUtcDateTime() const {
    return m_znetOwner.getUtcDateTime();
}

ZNet::DateTime ZNet::getLocalDateTime() const {
    return m_znetOwner.getLocalDateTime();
}

ZNet::TimeZoneSettings ZNet::getTimeZoneSettings() const {
    return m_znetOwner.getTimeZoneSettings();
}

void ZNet::setTimeZoneSettings( const char* zoneName, const char* zone ) {
    m_znetOwner.setTimeZoneSettings( zoneName, zone );
}

ZNet::TimerUPtr ZNet::createTimer( TimerCallback cb ) {
    return m_znetOwner.createTimer( std::move( cb ) );
}

void ZNet::fillManufacturerSpecific( znet_manufacturer_specific_report_t& data ) {
    m_znetOwner.fillManufacturerSpecific( data );
}

void ZNet::fillVersion( znet_version_report_t& data ) {
    m_znetOwner.fillVersion( data );
}

void ZNet::fillZWavePlusInfo( znet_zwaveplus_info_report_t& data ) {
    m_znetOwner.fillZWavePlusInfo( data );
}

void ZNet::fillFirmwareUpdateInfo( znet_firmware_md_report_v1_t& dataV1,
                                   znet_firmware_md_report_v3_t& dataV3,
                                   znet_firmware_md_report_v5_t& dataV5 ) {
    m_znetOwner.fillFirmwareUpdateInfo( dataV1, dataV3, dataV5 );
}

void ZNet::fillFirmwareLocale( std::string& locale ) {
    m_znetOwner.fillFirmwareLocale( locale );
}

void ZNet::onWakeupIntervalUpdated( const znet_node_id_t nodeId, znet::Seconds interval, bool isChanged ) {
    m_znetOwner.onWakeupIntervalUpdated( nodeId, interval, isChanged );
}

void ZNet::onAntiTheftStateUpdated( znet_node_id_t nodeId,
                                    znet_node_channel_id_t channelId,
                                    znet_command_class_t commandClassId,
                                    const znet::AntiTheftInfoOpt& antiTheftInfo ) {
    m_znetOwner.onAntiTheftStateUpdated( nodeId, channelId, commandClassId, antiTheftInfo );
}

void ZNet::onNodeCommandDelay( const uint16_t nodeId,
                               std::chrono::system_clock::time_point commandSent,
                               std::chrono::system_clock::duration delay ) {
    m_znetOwner.onNodeCommandDelay( nodeId, commandSent, delay );
}

void ZNet::onNodeCommandFailed( const uint16_t nodeId, std::chrono::system_clock::time_point commandSent ) {
    m_znetOwner.onNodeCommandFailed( nodeId, commandSent );
}

}  // namespace Zwave
}  // namespace HubPlatform
