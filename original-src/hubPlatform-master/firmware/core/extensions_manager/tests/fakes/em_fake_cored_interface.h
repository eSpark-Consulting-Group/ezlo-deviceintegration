#pragma once

#include <cored/core_service.h>
#include "extensions_manager/tests/fakes/em_fake_cored_configuration.h"

#include "cored/model/cored_device_setting.h"
#include "cored/remote_interface/cored_interface.h"
#include "json_utils/json_wrapper.h"

namespace HubPlatform {
namespace CoreDRemoteInterface {
namespace UT {

class FakeCoreDInterface final : public CoreDInterface {
public:
    CoreD::Model::Configuration& getConfiguration() override {
        static FakeConfiguration s_configuration;
        return s_configuration;
    }
    GatewayEnsureResult ensureGateway( const CoreD::Model::Gateway& /*gateway*/ ) override {
        return {};
    }
    void updateGateway( const GatewayID& gatewayId, const Json& gateway ) override {}
    void setGatewayStatus( const PluginId& pluginId, CoreD::Model::GatewayStatus status ) override {}
    RawGateways getGateways() override {
        return {};
    }
    GatewaysGetResult getGateways( const rpc::Message& /*request*/ ) override {
        return {};
    }
    Json getGatewayByPluginId( const std::string& /*pluginId*/ ) override {
        return JSON_EMPTY_OBJ;
    }
    Json getGatewayById( const std::string& /*gatewayId*/ ) override {
        return JSON_EMPTY_OBJ;
    }
    GatewaySettingAddResult addGatewaySetting( const CoreD::Model::GatewaySetting& setting ) override {
        return {};
    }
    void removeGatewaySetting( const GatewaySettingId& settingId ) override {}
    Json getGatewaySetting( const GatewaySettingId& gatewaySettingId ) override {
        return JSON_EMPTY_OBJ;
    }
    RawGatewaysSettings getGatewaySettings( const GatewayID& gatewayId ) override {
        return RawGatewaysSettings();
    }
    RawGatewaysSettings getGatewaysSettings() override {
        return RawGatewaysSettings();
    }
    bool isGatewaySettingReadOnly( const GatewaySettingId& gatewaySettingId ) override {
        return true;
    }
    void setGatewaySettingValue( const JsonObject& data ) override {}
    void updateGatewaySettingValue( const GatewaySettingId& settingId, ValueLoader valueLoader ) override {}
    void updateGatewaySettingStatus( const GatewaySettingId& settingId,
                                     CoreD::Model::SynchronizationStatus::Enum status ) override {}

    void startGatewayProcess( const GatewayID& gatewayId, CoreD::Model::GatewayProcess process ) override {}
    void stopGatewayProcess( const GatewayID& gatewayId,
                             CoreD::Model::GatewayProcess process,
                             CoreD::Model::GatewayProcessResult result ) override {}
    CoreD::Model::GatewayProcess getGatewayCurrentProcess( const GatewayID& gatewayId ) const override {
        return CoreD::Model::GatewayProcess::Unknown;
    }

    bool isProcessRunning( CoreD::Model::GatewayProcess process ) const override {
        return false;
    }

    DeviceAddResult addDevice( const CoreD::Model::Device& /*device*/ ) override {
        return {};
    }

    int getSwitchDelayLeft() {
        return {};
    }

    void setAlarmPhase( const std::string& phase, const std::string& status ) {}

    std::tuple< std::string, std::string > getAlarmPhase() {
        return {};
    }

    std::string getSwitchToPhase() {
        return {};
    }

    void modifyDevice( const DeviceID& deviceId, const CoreD::Model::Device& device ) override {}
    ItemAddResult addItem( const CoreD::Model::Item& /*item*/ ) override {
        return {};
    }
    bool modifyItem( const ItemID& itemId, const CoreD::Model::Item& item ) override {
        return true;
    }
    DeviceSettingAddResult addDeviceSetting( const CoreD::Model::DeviceSetting& setting ) override {
        return {};
    }
    bool modifyDeviceSetting( const DeviceSettingId& settingId, const CoreD::Model::DeviceSetting& setting ) override {
        return true;
    }
    RawDevices getDevices() const override {
        return {};
    }
    RawDevices getDevicesBy( const JsonObject& /*request*/ ) override {
        return {};
    }
    JsonObject getSingleDevice( const std::string& /*deviceId*/ ) override {
        return JSON_EMPTY_OBJ;
    }
    DeviceID getRootDeviceId( const DeviceID& deviceId ) override {
        return {};
    }

    RawItems getItemsBy( const JsonObject& /*request*/ ) override {
        return {};
    }
    JsonObject getItem( const ItemID& /*itemId*/ ) const override {
        return JSON_EMPTY_OBJ;
    }
    RawItems getItems() override {
        return {};
    }
    RawItems getItemsByDeviceId( const DeviceID& /*deviceId*/ ) const override {
        return {};
    }

    SettingIDs getDeviceSettingIdsByDeviceId( const DeviceID& /*deviceId*/ ) const override {
        return {};
    }
    CoreD::Model::DeviceSetting getRawDeviceSetting( const DeviceSettingId& /*settingId*/ ) const override {
        return CoreD::Model::DeviceSetting{ {}, {}, {} };
    }
    JsonObject getDeviceSetting( const DeviceSettingId& /*settingId*/ ) const override {
        return {};
    }
    DeviceSettings getDeviceSettings() const override {
        return DeviceSettings();
    }
    DeviceSettings getDeviceSettingsBy( const JsonObject& condition ) const override {
        return DeviceSettings();
    }
    DeviceWizardCategories getDeviceWizardCategories() const override {
        return {};
    }
    DeviceWizardDevices getDeviceWizardDevices( const std::string& /*categoryFilter*/ ) const override {
        return {};
    }
    JsonObject getHubStatus( const rpc::Message& /*request*/ ) override {
        return JSON_EMPTY_OBJ;
    }
    JsonObject updateDeviceFirmware( const JsonObject& request ) override {
        return JSON_EMPTY_OBJ;
    }
    void removePluginGateways( const PluginId& /*pluginId*/ ) override {}
    void removeGatewayDevices( const GatewayID& /*gatewayId*/ ) override {}
    void removeDevice( const DeviceID& /*deviceId*/ ) override {}
    void removeDeviceSync( const DeviceID& /*deviceId*/ ) override {}
    void removeItem( const ItemID& /*itemId*/ ) override {}
    void removeDeviceSetting( const DeviceSettingId& /*settingId*/ ) override {}
    void removePolicy( const PolicyID& /*policyId*/ ) override {}
    JsonObject setDeviceArmed( const DeviceID& /*deviceId*/, bool /*armed*/ ) override {
        return JSON_EMPTY_OBJ;
    }
    JsonObject setDeviceProtectConfig( const DeviceID& /*deviceId*/, std::string& /*protectConfig*/ ) override {
        return JSON_EMPTY_OBJ;
    }
    void setItemValue( const JsonObject& data,
                       SendResponseCallback sendResponseCallback = []( const JsonObject& ) {} ) override {}
    JsonObject setSettingValue( const JsonObject& data ) override {
        return JSON_EMPTY_OBJ;
    }
    bool isDeviceSettingReadOnly( const DeviceSettingId& deviceSettingId ) override {
        return true;
    }
    JsonObject resetSetting( const JsonObject& data ) override {
        return JSON_EMPTY_OBJ;
    }
    JsonObject checkDevice( const JsonObject& data ) override {
        return JSON_EMPTY_OBJ;
    }
    ValueUPtr getItemValueInMinMaxRange( const std::string& itemId, int percent ) override {
        return nullptr;
    }

    void addItemDictionaryValue( const JsonObject& data,
                                 SendResponseCallback sendResponseCallback = []( const JsonObject& ) {} ) override {}
    void setItemDictionaryValue( const JsonObject& data,
                                 SendResponseCallback sendResponseCallback = []( const JsonObject& ) {} ) override {}
    void removeItemDictionaryValue( const JsonObject& data,
                                    SendResponseCallback sendResponseCallback = []( const JsonObject& ) {} ) override {}
    JsonObject getItemDictionaryValue( const ItemID& itemId, const std::string& dictionaryId ) override {
        return {};
    }
    void clearItemDictionary( const JsonObject& data,
                              SendResponseCallback sendResponseCallback = []( const JsonObject& ) {} ) override {}

    JsonObject setSettingDictionaryValue( const JsonObject& data ) override {
        return JSON_EMPTY_OBJ;
    }

    ItemDictionaryNumberID generateItemDictionaryNumberId( const ItemID& itemId,
                                                           ItemDictionaryNumberID minId,
                                                           ItemDictionaryNumberID maxId ) override {
        return minId;
    }
    void updateItemDictionaryValue( const ItemID& itemId,
                                    const DictionaryId& itemDictionaryId,
                                    ValueLoader valueLoader ) override {}
    void updateUserCodeRestriction( const ItemID& itemId, const std::string& restriction ) override {}
    void updateItemValue( const ItemID& itemId, ValueLoader valueLoader ) override {}
    void updateItemValueWithMinMax( const ItemID& itemId,
                                    ValueLoader valueLoader,
                                    ValueLoader valueMinLoader,
                                    ValueLoader valueMaxLoader ) override {}
    void updateSettingValue( const DeviceSettingId& settingId,
                             ValueLoader valueLoader,
                             CoreD::Model::SynchronizationStatus::Enum status ) override {}
    void updateSettingDictionaryValue( const DeviceSettingId& settingId,
                                       const DictionaryId& key,
                                       ValueLoader valueLoader ) override {}
    void updateSettingStatus( const DeviceSettingId& settingId,
                              CoreD::Model::SynchronizationStatus::Enum status ) override{};
    void updateSettingDictionaryStatus( const DeviceSettingId& settingId,
                                        const DictionaryId& key,
                                        CoreD::Model::SynchronizationStatus::Enum status ) override{};
    void updateReachableState( const DeviceID& deviceId, bool reachable ) override {}
    void updateDeviceStatus( const CoreDInterface::DeviceID& deviceId, const std::string& status ) override {}
    void updateReadyState( const DeviceID& deviceId, bool ready ) override {}
    void updateDeviceName( const DeviceID& deviceId, const std::string& deviceName ) override {}
    void updateDeviceRoomId( const DeviceID& deviceId, const std::string& deviceRoomId ) override {}
    JsonObject setHouseMode( const HouseModeId& houseModeId ) override {
        return JSON_EMPTY_OBJ;
    }
    HouseModeId cancelHouseModeSwitching() override {
        return {};
    }
    HouseModesGetResult getHouseModes() const override {
        return {};
    }
    DeviceIDs getArmedDeviceIDsAffectedByHouseModes() const override {
        return {};
    }
    DeviceIDs getAlarmDeviceIDsAffectedByHouseModes() const override {
        return {};
    }
    HouseModeId getCurrentHouseModeId() const override {
        return {};
    }
    HouseModeId getHouseModeIdSwitchTo() const override {
        return {};
    }

    void setSwitchDelay( int /*delay*/ ) override {}
    void setAlarmDelay( int /*delay*/ ) override {}
    JsonObject setDisarmedDefault( const HouseModeId& /*houseModeId*/, bool /*disarmedDefault*/ ) override {
        return JSON_EMPTY_OBJ;
    }
    JsonObject houseModeProtectSet( const HouseModeId& houseModeId, bool protect ) override {
        return JSON_EMPTY_OBJ;
    }
    JsonObject addDisarmedDevice( const HouseModeId& /*houseModeId*/, const DeviceID& /*deviceId*/ ) override {
        return JSON_EMPTY_OBJ;
    }
    JsonObject removeDisarmedDevice( const HouseModeId& /*houseModeId*/, const DeviceID& /*deviceId*/ ) override {
        return JSON_EMPTY_OBJ;
    }
    JsonObject setNotificationsForHouseMode( const HouseModeId& houseModeId,
                                             bool notifyAll,
                                             const std::vector< UserId >& userIds ) override {
        return JSON_EMPTY_OBJ;
    }

    JsonObject addAlarmsOffDevice( const HouseModeId& /*houseModeId*/, const DeviceID& /*deviceId*/ ) override {
        return JSON_EMPTY_OBJ;
    }
    JsonObject removeAlarmsOffDevice( const HouseModeId& /*houseModeId*/, const DeviceID& /*deviceId*/ ) override {
        return JSON_EMPTY_OBJ;
    }
    JsonObject addCamerasOffDevice( const HouseModeId& houseModeId, const DeviceID& deviceId ) override {
        return JSON_EMPTY_OBJ;
    }
    JsonObject removeCamerasOffDevice( const HouseModeId& houseModeId, const DeviceID& deviceId ) override {
        return JSON_EMPTY_OBJ;
    }
    JsonObject addBypassDevices( const HouseModeId& houseModeId, const std::vector< DeviceID >& deviceIds ) override {
        return JSON_EMPTY_OBJ;
    }
    JsonObject removeBypassDevices( const HouseModeId& houseModeId,
                                    const std::vector< DeviceID >& deviceIds ) override {
        return JSON_EMPTY_OBJ;
    }
    JsonObject houseModesProtectButtonsSet( const DeviceID& deviceId,
                                            const CoreDHouseModes::ProtectServiceEnum& protectService ) override {
        return JSON_EMPTY_OBJ;
    }

    void removeDeviceForce( const DeviceID& /*deviceId*/ ) override {}

    std::vector< Hub::Setting > getSettings() const override {
        return {};
    }
    std::unique_ptr< Hub::Setting > findSetting( const std::string& setting ) const override {
        return nullptr;
    }
    void saveSetting( const Hub::Setting& setting ) override {}
    GeoPositionState getGeoPosition() const override {
        return {};
    }
    JsonObject getAllRooms() override {
        return {};
    };
    JsonObject createRoom( const std::string& name ) override {
        return {};
    };
    JsonObject setRoomName( const RoomId& roomId, const std::string& newName ) override {
        return {};
    };
    JsonObject setRoomsOrder( const std::vector< RoomId >& orderedRooms ) override {
        return {};
    };
    JsonObject getRoom( const RoomId& roomId ) override {
        return {};
    };
    JsonObject deleteRoom( const RoomId& roomId ) override {
        return {};
    };
    JsonObject deleteAllRooms() override {
        return {};
    };
    JsonObject cancelEntryDelay() override {
        return {};
    };
    JsonObject skipEntryDelay() override {
        return {};
    };

    JsonObject setEntryDelay( const JsonObject& params ) override {
        return JSON_EMPTY_OBJ;
    };

    JsonObject resetEntryDelay() override {
        return JSON_EMPTY_OBJ;
    };

    std::unique_ptr< CoreD::GroupOperationInterface > groupOperations() {
        return {};
    }

    std::unique_ptr< CoreD::PolicyOperationInterface > policyOperations() {
        return {};
    }

    std::unique_ptr< CoreD::SwingerShutdown::SwingerShutdownInterface > swingerShutdown() {
        return {};
    }

    std::unique_ptr< CoreD::WalkTest::WalkTestInterface > walkTest() {
        return {};
    }

    JsonObject turnOffLocalAlarm( const JsonObject& params ) override {
        return JSON_EMPTY_OBJ;
    };
    JsonObject forceDisarm( const JsonObject& params ) override {
        return JSON_EMPTY_OBJ;
    };

    std::unique_ptr< CoreD::Protect::ProtectContactIdInterface > contactIdOperations() override {
        return {};
    }
    std::unique_ptr< CoreD::Protect::ProtectSecurityConfigInterface > securityConfigOperations() override {
        return {};
    }
};

}  // namespace UT
}  // namespace CoreDRemoteInterface
}  // namespace HubPlatform
