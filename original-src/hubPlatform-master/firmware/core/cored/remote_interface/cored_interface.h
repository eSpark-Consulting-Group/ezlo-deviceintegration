#pragma once

#include "json_utils/json_fwd.h"

#include "cored/model/cored_gateway.h"
#include "cored/model/cored_device.h"
#include "cored/model/cored_item.h"
#include "cored/model/cored_gateway_setting.h"
#include "cored/model/values/value.h"
#include "cored/model/configuration/configuration.h"
#include "cored/model/setting/synchronization_status.h"
#include "cored/house_modes/house_mode_entry.h"
#include "cored/house_modes/protect_service_enum.h"
#include "cored/house_modes/protect_button.h"
#include "hub_settings/hub_setting.h"
#include "cored/remote_interface/system_attribute_types.h"
#include "cored/group_operations/remote_interface/group_operation_interface.h"
#include "cored/swinger_shutdown/remote_interface/swinger_shutdown_interface.h"
#include "cored/walk_test/remote_interface/walk_test_interface.h"
#include "cored/protect/remote_interface/contact_id_interface.h"
#include "cored/protect/remote_interface/security_config_interface.h"
#include "cored/policies/remote_interface/policies_operation_interface.h"

#include <string>
#include <unordered_map>

namespace rpc {
class Message;
}

namespace HubPlatform {

namespace CoreD {
namespace Model {

class Value;
class DeviceSetting;
class ZwaveConfigurationValue;

enum class GatewayProcess;
enum class GatewayProcessResult;

}  // namespace Model
}  // namespace CoreD

namespace CoreDRemoteInterface {

using ValueUPtr = std::unique_ptr< CoreD::Model::Value >;
using ValueLoader = std::function< ValueUPtr( CoreD::Model::ValueType ) >;
class TemperatureValue;

class CoreDInterface {
public:
    using DeviceID = std::string;
    using ItemID = std::string;
    using PolicyID = std::string;
    using ItemDictionaryNumberID = std::uint32_t;
    using DictionaryId = std::string;
    using GatewayID = std::string;
    using PluginId = std::string;
    using HouseModeId = std::string;
    using UserId = std::string;
    using RoomId = std::string;
    using DeviceSettingId = std::string;
    using GatewaySettingId = std::string;
    using ItemGroupId = std::string;
    using DeviceGroupId = std::string;

    using DeviceIDs = std::vector< std::string >;
    using SettingIDs = std::vector< std::string >;

    using DeviceAddResult = std::pair< DeviceID, bool >;
    using DevicesGetResult = std::pair< JsonObject, bool >;
    using RawDevices = std::vector< JsonObject >;
    using RawGatewaysSettings = std::vector< JsonObject >;
    using GatewaySettingAddResult = std::pair< GatewaySettingId, bool >;

    using ItemAddResult = std::pair< ItemID, bool >;
    using ItemsGetResult = std::pair< JsonObject, bool >;
    using RawItems = std::vector< JsonObject >;

    using RawPolicies = std::vector< JsonObject >;

    using DeviceSettingAddResult = std::pair< DeviceSettingId, bool >;

    using GatewayEnsureResult = std::pair< GatewayID, bool >;
    using GatewaysGetResult = std::pair< JsonObject, bool >;
    using RawGateways = std::vector< JsonObject >;

    using DeviceWizardCategories = std::pair< JsonObject, bool >;
    using DeviceWizardDevices = std::pair< JsonObject, bool >;

    using HouseModesGetResult = std::vector< CoreDHouseModes::HouseModeEntry >;

    using DeviceSettings = std::unordered_map< DeviceSettingId, CoreD::Model::DeviceSetting >;

    using SendResponseCallback = std::function< void( const JsonObject& response ) >;

    virtual ~CoreDInterface() = default;

    virtual CoreD::Model::Configuration& getConfiguration() = 0;

    virtual GatewayEnsureResult ensureGateway( const CoreD::Model::Gateway& gateway ) = 0;
    virtual void updateGateway( const GatewayID& gatewayId, const Json& gateway ) = 0;
    virtual void setGatewayStatus( const PluginId& pluginId, CoreD::Model::GatewayStatus status ) = 0;
    virtual RawGateways getGateways() = 0;
    virtual GatewaysGetResult getGateways( const rpc::Message& request ) = 0;
    virtual Json getGatewayByPluginId( const PluginId& pluginId ) = 0;
    virtual Json getGatewayById( const GatewayID& pluginId ) = 0;

    virtual GatewaySettingAddResult addGatewaySetting( const CoreD::Model::GatewaySetting& setting ) = 0;
    virtual void removeGatewaySetting( const GatewaySettingId& settingId ) = 0;
    virtual Json getGatewaySetting( const GatewaySettingId& gatewaySettingId ) = 0;
    virtual RawGatewaysSettings getGatewaySettings( const GatewayID& gatewayId ) = 0;
    virtual RawGatewaysSettings getGatewaysSettings() = 0;
    virtual bool isGatewaySettingReadOnly( const GatewaySettingId& gatewaySettingId ) = 0;
    virtual void setGatewaySettingValue( const JsonObject& data ) = 0;
    virtual void updateGatewaySettingValue( const GatewaySettingId& settingId, ValueLoader valueLoader ) = 0;
    virtual void updateGatewaySettingStatus( const GatewaySettingId& settingId,
                                             CoreD::Model::SynchronizationStatus::Enum status ) = 0;

    virtual void startGatewayProcess( const GatewayID& gatewayId, CoreD::Model::GatewayProcess process ) = 0;
    virtual void stopGatewayProcess( const GatewayID& gatewayId,
                                     CoreD::Model::GatewayProcess process,
                                     CoreD::Model::GatewayProcessResult result ) = 0;
    virtual CoreD::Model::GatewayProcess getGatewayCurrentProcess( const GatewayID& gatewayId ) const = 0;

    virtual bool isProcessRunning( CoreD::Model::GatewayProcess process ) const = 0;

    virtual DeviceAddResult addDevice( const CoreD::Model::Device& device ) = 0;
    virtual void modifyDevice( const DeviceID& deviceId, const CoreD::Model::Device& device ) = 0;
    virtual ItemAddResult addItem( const CoreD::Model::Item& item ) = 0;
    virtual bool modifyItem( const ItemID& itemId, const CoreD::Model::Item& item ) = 0;
    virtual DeviceSettingAddResult addDeviceSetting( const CoreD::Model::DeviceSetting& setting ) = 0;
    virtual bool modifyDeviceSetting( const DeviceSettingId& id, const CoreD::Model::DeviceSetting& setting ) = 0;
    virtual RawDevices getDevices() const = 0;
    virtual RawDevices getDevicesBy( const JsonObject& condition ) = 0;
    virtual JsonObject getSingleDevice( const DeviceID& deviceId ) = 0;
    virtual DeviceID getRootDeviceId( const DeviceID& deviceId ) = 0;
    virtual RawItems getItems() = 0;
    virtual RawItems getItemsBy( const JsonObject& condition ) = 0;
    virtual JsonObject getItem( const ItemID& itemId ) const = 0;
    virtual RawItems getItemsByDeviceId( const DeviceID& deviceId ) const = 0;
    virtual SettingIDs getDeviceSettingIdsByDeviceId( const DeviceID& deviceId ) const = 0;
    virtual CoreD::Model::DeviceSetting getRawDeviceSetting( const DeviceSettingId& settingId ) const = 0;
    virtual Json getDeviceSetting( const DeviceSettingId& settingId ) const = 0;
    virtual DeviceSettings getDeviceSettings() const = 0;
    virtual DeviceSettings getDeviceSettingsBy( const JsonObject& condition ) const = 0;
    virtual DeviceWizardCategories getDeviceWizardCategories() const = 0;
    virtual DeviceWizardDevices getDeviceWizardDevices( const std::string& categoryFilter ) const = 0;
    virtual JsonObject getHubStatus( const rpc::Message& request ) = 0;
    virtual JsonObject updateDeviceFirmware( const JsonObject& request ) = 0;
    virtual void removePluginGateways( const PluginId& pluginId ) = 0;
    virtual void removeGatewayDevices( const GatewayID& gatewayId ) = 0;
    virtual void removeDevice( const DeviceID& deviceId ) = 0;
    virtual void removeDeviceSync( const DeviceID& deviceId ) = 0;
    virtual void removeItem( const ItemID& itemId ) = 0;
    virtual void removeDeviceSetting( const DeviceSettingId& settingId ) = 0;
    virtual void removeDeviceForce( const DeviceID& deviceId ) = 0;
    virtual void removePolicy( const PolicyID& policyId ) = 0;
    virtual JsonObject setDeviceArmed( const DeviceID& deviceId, bool armed ) = 0;
    virtual JsonObject setDeviceProtectConfig( const DeviceID& deviceId, std::string& protectConfig ) = 0;

    virtual void setItemValue( const JsonObject& data,
                               SendResponseCallback sendResponseCallback = []( const JsonObject& ) {} ) = 0;
    virtual JsonObject setSettingValue( const JsonObject& data ) = 0;
    virtual JsonObject resetSetting( const JsonObject& data ) = 0;
    virtual bool isDeviceSettingReadOnly( const DeviceSettingId& deviceSettingId ) = 0;
    virtual JsonObject checkDevice( const JsonObject& data ) = 0;
    virtual ValueUPtr getItemValueInMinMaxRange( const std::string& itemId, int percent ) = 0;
    virtual void addItemDictionaryValue( const JsonObject& data,
                                         SendResponseCallback sendResponseCallback = []( const JsonObject& ) {} ) = 0;
    virtual void setItemDictionaryValue( const JsonObject& data,
                                         SendResponseCallback sendResponseCallback = []( const JsonObject& ) {} ) = 0;
    virtual void clearItemDictionary( const JsonObject& data,
                                      SendResponseCallback sendResponseCallback = []( const JsonObject& ) {} ) = 0;
    virtual void removeItemDictionaryValue( const JsonObject& data,
                                            SendResponseCallback sendResponseCallback = []( const JsonObject& ) {
                                            } ) = 0;
    virtual JsonObject getItemDictionaryValue( const ItemID& itemId, const std::string& dictionaryId ) = 0;

    virtual JsonObject setSettingDictionaryValue( const JsonObject& data ) = 0;

    virtual void updateUserCodeRestriction( const ItemID& itemId, const std::string& restriction ) = 0;

    virtual void updateItemValue( const ItemID& itemId, ValueLoader valueLoader ) = 0;
    virtual void updateItemValueWithMinMax( const ItemID& itemId,
                                            ValueLoader valueLoader,
                                            ValueLoader valueMinLoader,
                                            ValueLoader valueMaxLoader ) = 0;
    virtual ItemDictionaryNumberID generateItemDictionaryNumberId( const ItemID& itemId,
                                                                   ItemDictionaryNumberID minId,
                                                                   ItemDictionaryNumberID maxId ) = 0;
    virtual void updateItemDictionaryValue( const ItemID& itemId,
                                            const DictionaryId& itemDictionaryId,
                                            ValueLoader valueLoader ) = 0;
    virtual void updateSettingValue( const DeviceSettingId& settingId,
                                     ValueLoader valueLoader,
                                     CoreD::Model::SynchronizationStatus::Enum status ) = 0;
    virtual void updateSettingDictionaryValue( const DeviceSettingId& settingId,
                                               const DictionaryId& key,
                                               ValueLoader valueLoader ) = 0;
    virtual void updateSettingStatus( const DeviceSettingId& settingId,
                                      CoreD::Model::SynchronizationStatus::Enum status ) = 0;
    virtual void updateSettingDictionaryStatus( const DeviceSettingId& settingId,
                                                const DictionaryId& key,
                                                CoreD::Model::SynchronizationStatus::Enum status ) = 0;
    virtual void updateReachableState( const DeviceID& deviceId, bool reachable ) = 0;
    virtual void updateDeviceStatus( const DeviceID& deviceId, const std::string& status ) = 0;
    virtual void updateReadyState( const DeviceID& deviceId, bool ready ) = 0;
    virtual void updateDeviceName( const DeviceID& deviceId, const std::string& deviceName ) = 0;
    virtual void updateDeviceRoomId( const DeviceID& deviceId, const std::string& deviceRoomId ) = 0;
    virtual JsonObject setHouseMode( const HouseModeId& houseModeId ) = 0;
    virtual HouseModeId cancelHouseModeSwitching() = 0;
    virtual JsonObject cancelEntryDelay() = 0;
    virtual JsonObject skipEntryDelay() = 0;
    virtual JsonObject setEntryDelay( const JsonObject& params ) = 0;
    virtual JsonObject resetEntryDelay() = 0;
    virtual HouseModesGetResult getHouseModes() const = 0;
    virtual DeviceIDs getArmedDeviceIDsAffectedByHouseModes() const = 0;
    virtual DeviceIDs getAlarmDeviceIDsAffectedByHouseModes() const = 0;

    virtual HouseModeId getCurrentHouseModeId() const = 0;
    virtual HouseModeId getHouseModeIdSwitchTo() const = 0;
    virtual void setSwitchDelay( int delay ) = 0;
    virtual void setAlarmDelay( int delay ) = 0;
    virtual int getSwitchDelayLeft() = 0;
    virtual void setAlarmPhase( const std::string& phase, const std::string& status ) = 0;
    virtual std::tuple< std::string, std::string > getAlarmPhase() = 0;
    virtual std::string getSwitchToPhase() = 0;
    virtual JsonObject setDisarmedDefault( const HouseModeId& houseModeId, bool disarmedDefault ) = 0;
    virtual JsonObject houseModeProtectSet( const HouseModeId& houseModeId, bool protect ) = 0;
    virtual JsonObject addDisarmedDevice( const HouseModeId& houseModeId, const DeviceID& deviceId ) = 0;
    virtual JsonObject removeDisarmedDevice( const HouseModeId& houseModeId, const DeviceID& deviceId ) = 0;
    virtual JsonObject setNotificationsForHouseMode( const HouseModeId& houseModeId,
                                                     bool notifyAll,
                                                     const std::vector< UserId >& userIds ) = 0;
    virtual JsonObject addAlarmsOffDevice( const HouseModeId& houseModeId, const DeviceID& deviceId ) = 0;
    virtual JsonObject removeAlarmsOffDevice( const HouseModeId& houseModeId, const DeviceID& deviceId ) = 0;
    virtual JsonObject addCamerasOffDevice( const HouseModeId& houseModeId, const DeviceID& deviceId ) = 0;
    virtual JsonObject removeCamerasOffDevice( const HouseModeId& houseModeId, const DeviceID& deviceId ) = 0;
    virtual JsonObject addBypassDevices( const HouseModeId& houseModeId, const std::vector< DeviceID >& deviceIds ) = 0;
    virtual JsonObject removeBypassDevices( const HouseModeId& houseModeId,
                                            const std::vector< DeviceID >& deviceIds ) = 0;

    virtual JsonObject houseModesProtectButtonsSet( const DeviceID& deviceId,
                                                    const CoreDHouseModes::ProtectServiceEnum& protectService ) = 0;

    virtual std::vector< Hub::Setting > getSettings() const = 0;
    virtual std::unique_ptr< Hub::Setting > findSetting( const std::string& setting ) const = 0;
    virtual void saveSetting( const Hub::Setting& setting ) = 0;

    // Position, is default
    using GeoPositionState = std::pair< GeoPosition, bool >;

    virtual GeoPositionState getGeoPosition() const = 0;
    /* @brief rooms*/
    virtual JsonObject getAllRooms() = 0;
    virtual JsonObject createRoom( const std::string& name ) = 0;
    virtual JsonObject setRoomName( const RoomId& roomId, const std::string& newName ) = 0;
    virtual JsonObject setRoomsOrder( const std::vector< RoomId >& orderedRooms ) = 0;
    virtual JsonObject getRoom( const RoomId& roomId ) = 0;
    virtual JsonObject deleteRoom( const RoomId& roomId ) = 0;
    virtual JsonObject deleteAllRooms() = 0;

    virtual std::unique_ptr< CoreD::GroupOperationInterface > groupOperations() = 0;
    virtual std::unique_ptr< CoreD::PolicyOperationInterface > policyOperations() = 0;
    virtual std::unique_ptr< CoreD::SwingerShutdown::SwingerShutdownInterface > swingerShutdown() = 0;
    virtual std::unique_ptr< CoreD::WalkTest::WalkTestInterface > walkTest() = 0;
    virtual JsonObject turnOffLocalAlarm( const JsonObject& params ) = 0;
    virtual JsonObject forceDisarm( const JsonObject& params ) = 0;
    virtual std::unique_ptr< CoreD::Protect::ProtectContactIdInterface > contactIdOperations() = 0;
    virtual std::unique_ptr< CoreD::Protect::ProtectSecurityConfigInterface > securityConfigOperations() = 0;
};

using CoreDInterfaceUPtr = std::unique_ptr< CoreDInterface >;

}  // namespace CoreDRemoteInterface
}  // namespace HubPlatform
