#pragma once

#include "cored/remote_interface/cored_interface.h"
#include "json_utils/json_fwd.h"
#include "cored/device_adding_wizard.h"

#include "cored/model/cored_device.h"
#include "cored/model/cored_item.h"
#include "cored/model/cored_policy.h"
#include "cored/model/cored_feature.h"
#include "hub_settings/hub_setting.h"
#include "cored/model/cored_gateway_processes.h"

#include "service_interfaces/groups_names.h"
#include "bus/messagesclasses.h"
#include "provision_data_utils/provision_data_manager.h"

#include "base_utils/gregorian_calendar.h"
#include "cored/group_operations/remote_interface/group_operation_interface.h"
#include "cored/swinger_shutdown/remote_interface/swinger_shutdown_interface.h"
#include "cored/walk_test/remote_interface/walk_test_interface.h"

#include <string>
#include <vector>
#include <utility>
#include <functional>
#include <chrono>

namespace rpc {
class Message;
}

class IDbClient;

namespace HubPlatform {

namespace Services {
class Service;
}  // namespace Services

namespace CoreD {
class WhatHappened;
}  // namespace CoreD

namespace CoreDRemoteInterface {
using namespace HubPlatform::CoreD;

class DictionaryValueOverflow : public std::runtime_error {
public:
    explicit DictionaryValueOverflow()
    : runtime_error{ "adding new element would cause the dictionary to exceed max allowed elements" } {}
};

class CoreDRemoteInterface : public CoreDInterface {
public:
    using LocalTimeGetter = std::function< GregorianCalendar::Date() >;

public:
    CoreDRemoteInterface(
        Services::Service& hostService,
        const ProvisionDataUtils::ProvisionData& provisionData = ProvisionDataUtils::getManager().getProvisionData() );

    CoreDRemoteInterface(
        Services::Service& hostService,
        LocalTimeGetter localTimeGetter,
        const ProvisionDataUtils::ProvisionData& provisionData = ProvisionDataUtils::getManager().getProvisionData() );
    ~CoreDRemoteInterface() override;

    CoreD::Model::Configuration& getConfiguration() override;

    /* @brief Adds a gateway to the db or update it if it already exists, make corresponding a broadcast message */
    GatewayEnsureResult ensureGateway( const CoreD::Model::Gateway& gateway ) override;

    /* @brief modify gateway fields in the db
     *
     * @param[in] gatewayId - gateway id
     * @param[in] gateway - new gateway info */
    void updateGateway( const GatewayID& gatewayId, const Json& gateway ) override;

    void setGatewayStatus( const PluginId& pluginId, CoreD::Model::GatewayStatus status ) override;

    /* @brief get raw gatewayS from the db */
    RawGateways getGateways() override;

    /* @brief get gatewayS from the db
     *
     * @return pair where a second elm represents a success of an operation
     * @note perform some transformations (specific for Hub API) over raw gateways from the db */
    GatewaysGetResult getGateways( const rpc::Message& request ) override;

    Json getGatewayByPluginId( const PluginId& pluginId ) override;
    Json getGatewayById( const GatewayID& pluginId ) override;

    GatewaySettingAddResult addGatewaySetting( const CoreD::Model::GatewaySetting& setting ) override;
    void removeGatewaySetting( const GatewaySettingId& settingId ) override;
    Json getGatewaySetting( const GatewaySettingId& gatewaySettingId ) override;
    RawGatewaysSettings getGatewaySettings( const GatewayID& gatewayId ) override;
    RawGatewaysSettings getGatewaysSettings() override;
    bool isGatewaySettingReadOnly( const GatewaySettingId& gatewaySettingId ) override;
    void setGatewaySettingValue( const JsonObject& data ) override;
    void updateGatewaySettingValue( const GatewaySettingId& settingId, ValueLoader valueLoader ) override;
    void updateGatewaySettingStatus( const GatewaySettingId& settingId,
                                     CoreD::Model::SynchronizationStatus::Enum status ) override;

    void startGatewayProcess( const GatewayID& gatewayId, CoreD::Model::GatewayProcess process ) override;
    void stopGatewayProcess( const GatewayID& gatewayId,
                             CoreD::Model::GatewayProcess process,
                             CoreD::Model::GatewayProcessResult result ) override;
    CoreD::Model::GatewayProcess getGatewayCurrentProcess( const GatewayID& gatewayId ) const override;

    bool isProcessRunning( CoreD::Model::GatewayProcess process ) const override;

    /* @brief add a device to the db and make a broadcast message
     *
     * @param[in] device - describe a device to be added
     * @return pair where a second elm represents a success of an operation */
    DeviceAddResult addDevice( const CoreD::Model::Device& device ) override;

    /* @brief modify device fields in the db
     *
     * @param[in] deviceId - device id
     * @param[in] device - new device info */
    void modifyDevice( const DeviceID& deviceId, const CoreD::Model::Device& device ) override;

    /* @brief add an item to the db and make a broadcast message
     *
     * @param[in] item - describe an item to be added
     * @return pair where a second elm represents a success of an operation */
    ItemAddResult addItem( const CoreD::Model::Item& item ) override;

    /* @brief modify item fields in the db
     *
     * @param[in] itemId - item id
     * @param[in] item - new item info */
    bool modifyItem( const ItemID& itemId, const CoreD::Model::Item& item ) override;

    /* @brief Add a setting to the db and make a broadcast message
     *
     * @param[in] item - describe n setting to be added
     * @return pair where a second elm represents a success of an operation */
    DeviceSettingAddResult addDeviceSetting( const CoreD::Model::DeviceSetting& setting ) override;

    /* @brief modify device setting fields in the db
     *
     * @param[in] settingId - device setting id
     * @param[in] setting - new device setting info */
    bool modifyDeviceSetting( const DeviceSettingId& settingId, const CoreD::Model::DeviceSetting& setting ) override;

    /*  @brief get raw devices from the db */
    RawDevices getDevices() const override;

    /*  @brief get raw devices from the db by condition */
    RawDevices getDevicesBy( const JsonObject& condition ) override;

    /* brief get single device by id from the db */
    JsonObject getSingleDevice( const DeviceID& deviceId ) override;

    /* brief get root device id by device id from the db */
    DeviceID getRootDeviceId( const DeviceID& deviceId ) override;

    /*  @brief get raw item from the db by specified id*/
    JsonObject getItem( const ItemID& itemId ) const override;

    /*  @brief get raw items from the db */
    RawItems getItems() override;

    /*  @brief get raw device from the db */
    RawItems getItemsByDeviceId( const DeviceID& deviceId ) const override;

    /*  @brief get raw device settings from the db */
    SettingIDs getDeviceSettingIdsByDeviceId( const DeviceID& deviceId ) const override;

    /*  @brief get device setting from the db */
    CoreD::Model::DeviceSetting getRawDeviceSetting( const DeviceSettingId& settingId ) const override;

    /*  @brief get device setting from the db */
    JsonObject getDeviceSetting( const DeviceSettingId& settingId ) const override;

    /* @brief get items from the db by condition */
    RawItems getItemsBy( const JsonObject& condition ) override;

    /*  @brief get device settings from the db */
    DeviceSettings getDeviceSettings() const override;

    /*  @brief get device settings from the db by a condition */
    DeviceSettings getDeviceSettingsBy( const JsonObject& condition ) const override;

    /* @brief get Device Categories for Device Adding Wizard
     *
     * @return pair where a second elm represents a success of an operation
     */
    DeviceWizardCategories getDeviceWizardCategories() const override;

    /* @brief get Device List for Device Adding Wizard
     *
     * @return pair where a second elm represents a success of an operation
     */
    DeviceWizardDevices getDeviceWizardDevices( const std::string& categoryFilter ) const override;

    /* @brief set DeviceAddingWizard
     */
    void setDeviceAddingWizard( std::unique_ptr< DeviceAddingWizard > wizard );

    void processCmd( const std::string& method, rpc::Message& rpcMessage );

    JsonObject getHubStatus( const rpc::Message& request ) override;
    JsonObject updateDeviceFirmware( const JsonObject& request ) override;

    /* @brief remove all plugin's gateways
     * @note it's an async call */
    void removePluginGateways( const PluginId& pluginId ) override;

    /* @brief remove all devices registered for a gateway specified by a @c gatewayId id
     * @note it's an async call */
    void removeGatewayDevices( const GatewayID& gatewayId ) override;

    /* @brief remove a device specified by a @c deviceId id and all items registered for this device
     * @note it's an async call */
    void removeDevice( const DeviceID& deviceId ) override;

    /* @brief remove a device specified by a @c deviceId id and all items registered for this device
     * @note it's an async call */
    void removeDeviceSync( const DeviceID& deviceId ) override;

    /* @brief remove an item specified by a @c itemId id
     * @note it's an async call */
    void removeItem( const ItemID& itemId ) override;

    /* @brief Remove a setting specified by a @c settingId id
     * @note it's an async call */
    void removeDeviceSetting( const DeviceSettingId& settingId ) override;

    /* @brief remove a policy specified by a @c policyId id
     * @note it's an async call */
    void removePolicy( const PolicyID& policyId ) override;

    /* @brief force remove a device specified by a @c deviceId id and all items registered for this device
     * @note it's an async call */
    void removeDeviceForce( const DeviceID& deviceId ) override;
    Json renameDevice( const JsonObject& device );
    JsonObject placeDevice( const std::string& deviceId,
                            const std::string& roomId,
                            const JsonArray& separateChildDevices );

    JsonObject setDeviceArmed( const DeviceID& deviceId, bool armed ) override;
    JsonObject setDeviceProtectConfig( const DeviceID& deviceId, std::string& protectConfig ) override;

    using SendResponseCallback = std::function< void( const JsonObject& response ) >;

    void setItemValue( const JsonObject& data,
                       SendResponseCallback sendResponseCallback = []( const JsonObject& ) {} ) override;

    JsonObject setSettingValue( const JsonObject& data ) override;

    JsonObject resetSetting( const JsonObject& data ) override;

    bool isDeviceSettingReadOnly( const DeviceSettingId& deviceSettingId ) override;

    JsonObject checkDevice( const JsonObject& data ) override;

    /* @brief getItemValueInMinMaxRange calculates new value for item if item contains min- and maxValue
     * @param percent - percent of min max range that will be added to current value
     * @return result of calculation, can be an int, float or scalable value.
     * */
    ValueUPtr getItemValueInMinMaxRange( const std::string& itemId, int percent ) override;

    /* These methods ask corresponding plugin to modify value on an actual device,
     * then plugin somehow will modify value in firmware calling other methods */
    void addItemDictionaryValue( const JsonObject& data,
                                 SendResponseCallback sendResponseCallback = []( const JsonObject& ) {} ) override;
    void setItemDictionaryValue( const JsonObject& data,
                                 SendResponseCallback sendResponseCallback = []( const JsonObject& ) {} ) override;
    void removeItemDictionaryValue( const JsonObject& data,
                                    SendResponseCallback sendResponseCallback = []( const JsonObject& ) {} ) override;

    void clearItemDictionary( const JsonObject& data,
                              SendResponseCallback sendResponseCallback = []( const JsonObject& ) {} ) override;

    JsonObject setSettingDictionaryValue( const JsonObject& data ) override;

    JsonObject getItemDictionaryValue( const ItemID& itemId, const std::string& dictionaryId ) override;

    void updateUserCodeRestriction( const ItemID& itemId, const std::string& restriction ) override;

    /* @brief update item value by specified id
     * @note it's an async call */
    void updateItemValue( const ItemID& itemId, ValueLoader valueLoader ) override;

    void updateItemValueWithMinMax( const ItemID& itemId,
                                    ValueLoader valueLoader,
                                    ValueLoader valueMinLoader,
                                    ValueLoader valueMaxLoader ) override;

    ItemDictionaryNumberID generateItemDictionaryNumberId( const ItemID& itemId,
                                                           ItemDictionaryNumberID minId,
                                                           ItemDictionaryNumberID maxId ) override;
    void updateItemDictionaryValue( const ItemID& itemId,
                                    const DictionaryId& itemDictionaryId,
                                    ValueLoader valueLoader ) override;

    void updateSettingValue( const DeviceSettingId& settingId,
                             ValueLoader valueLoader,
                             CoreD::Model::SynchronizationStatus::Enum status ) override;

    void updateSettingDictionaryValue( const DeviceSettingId& settingId,
                                       const DictionaryId& key,
                                       ValueLoader valueLoader ) override;

    void updateSettingStatus( const DeviceSettingId& settingId,
                              CoreD::Model::SynchronizationStatus::Enum status ) override;

    void updateSettingDictionaryStatus( const DeviceSettingId& settingId,
                                        const DictionaryId& key,
                                        CoreD::Model::SynchronizationStatus::Enum status ) override;

    /* @brief wrapper for sending data
     * to core service for saving action */
    JsonObject makeWhatHappenedRequest( const JsonObject& data, const MessageSubClasses subclass );

    /* @brief update reachable state specified by a @c deviceId id and all items registered for this device
     * @note it's an async call */
    void updateReachableState( const DeviceID& deviceId, bool reachable ) override;

    /* @brief update status state specified by a @c deviceId id
     * @note it's an async call */
    void updateDeviceStatus( const DeviceID& deviceId, const std::string& status ) override;

    /* @brief update ready state specified by a @c deviceId id and all items registered for this device
     * @note it's an async call */
    void updateReadyState( const DeviceID& deviceId, bool ready ) override;

    /* @brief update device name specified by a @c deviceId id and all items registered for this device
     * @note it's an async call */
    void updateDeviceName( const DeviceID& deviceId, const std::string& deviceName ) override;

    /* @brief update device room Id specified by a @c deviceId id and all items registered for this device
     * @note it's an async call */
    void updateDeviceRoomId( const DeviceID& deviceId, const std::string& deviceRoomId ) override;

    /* @brief send ui broadcast with user defined data @broadcastData from plugin with id @pluginId
     * @note it's an async call */
    void sendPluginUIBroadcast( std::string pluginId, JsonObject broadcastData );

    JsonObject setHouseMode( const HouseModeId& houseModeId ) override;
    CoreDInterface::HouseModeId cancelHouseModeSwitching() override;
    JsonObject cancelEntryDelay() override;
    JsonObject skipEntryDelay() override;
    JsonObject setEntryDelay( const JsonObject& params ) override;
    JsonObject resetEntryDelay() override;

    /* @brief get house modes */
    HouseModesGetResult getHouseModes() const override;
    DeviceIDs getArmedDeviceIDsAffectedByHouseModes() const override;
    DeviceIDs getAlarmDeviceIDsAffectedByHouseModes() const override;

    std::unique_ptr< CoreDHouseModes::HouseModeEntry > getHouseModeById( const HouseModeId& id ) const;
    std::unique_ptr< CoreDHouseModes::HouseModeEntry > getHouseModeByName( const std::string& name ) const;

    HouseModeId getCurrentHouseModeId() const override;
    HouseModeId getHouseModeIdSwitchTo() const override;

    void setSwitchDelay( int delay ) override;
    void setAlarmDelay( int delay ) override;

    int getSwitchDelay();
    int getAlarmDelay();
    Json getEntryDelay();
    int getSwitchDelayLeft();
    void setAlarmPhase( const std::string& phase, const std::string& status );
    std::tuple< std::string, std::string > getAlarmPhase();
    std::string getSwitchToPhase();
    Json getAbortWindow();

    JsonObject setDisarmedDefault( const HouseModeId& houseModeId, bool disarmedDefault ) override;
    JsonObject houseModeProtectSet( const HouseModeId& houseModeId, bool protect ) override;
    JsonObject addDisarmedDevice( const HouseModeId& houseModeId, const DeviceID& deviceId ) override;
    JsonObject removeDisarmedDevice( const HouseModeId& houseModeId, const DeviceID& deviceId ) override;
    JsonObject setNotificationsForHouseMode( const HouseModeId& houseModeId,
                                             bool notifyAll,
                                             const std::vector< UserId >& userIds ) override;
    JsonObject addAlarmsOffDevice( const HouseModeId& houseModeId, const DeviceID& deviceId ) override;
    JsonObject removeAlarmsOffDevice( const HouseModeId& houseModeId, const DeviceID& deviceId ) override;
    JsonObject addCamerasOffDevice( const HouseModeId& houseModeId, const DeviceID& deviceId ) override;
    JsonObject removeCamerasOffDevice( const HouseModeId& houseModeId, const DeviceID& deviceId ) override;
    JsonObject addBypassDevices( const HouseModeId& houseModeId, const std::vector< DeviceID >& deviceIds ) override;
    JsonObject removeBypassDevices( const HouseModeId& houseModeId, const std::vector< DeviceID >& deviceIds ) override;
    DeviceIDs getCameraDeviceIDs();

    JsonObject houseModesProtectButtonsSet( const DeviceID& deviceId,
                                            const CoreDHouseModes::ProtectServiceEnum& protectService ) override;
    std::vector< CoreDHouseModes::ProtectButton > getHouseModesProtectButtons();

    JsonObject houseModesProtectDevicesAdd( const CoreDInterface::DeviceIDs& deviceIds );
    JsonObject houseModesProtectDevicesRemove( const CoreDInterface::DeviceIDs& deviceIds );
    DeviceIDs getHouseModesProtectDevicesIDs();

    /* @brief Returns pair where first is geo-position itself
     * and second is flag that tells whether geo-position is default */
    GeoPositionState getGeoPosition() const override;
    JsonObject setGeoPosition( const GeoPosition& pos );

    JsonObject setHubTimeZone( const JsonObject& params );
    JsonObject getAllSupportedTimeZones() const;

    /* @brief rooms*/
    JsonObject getAllRooms();
    JsonObject createRoom( const std::string& name );
    JsonObject setRoomName( const RoomId& roomId, const std::string& newName );
    JsonObject setRoomsOrder( const std::vector< RoomId >& orderedRooms );
    JsonObject getRoom( const RoomId& roomId ) override;
    JsonObject deleteRoom( const RoomId& roomId );
    JsonObject deleteAllRooms();

    JsonObject setServiceNotifications( const JsonArray& ids );

    std::string getHubTimeZone();

    bool isInsideWeekInterval( const std::vector< std::string >& weekdays,
                               const std::string& startTime,
                               const std::string& endTime );
    std::string getNextWeekEvent( const std::vector< std::string >& weekdays, const std::string& time );
    std::string calculateTimeWithDelay( const std::string& time );

    // TODO: make interface out of this
    std::vector< Model::Feature > getFeatures() const;
    std::unique_ptr< Model::Feature > findFeature( const std::string& feature ) const;
    bool checkFeatureStatus( const std::string& feature ) const;
    void setFeatureStatus( const Model::Feature& feature );

    std::vector< Hub::Setting > getSettings() const override;
    std::unique_ptr< Hub::Setting > findSetting( const std::string& setting ) const override;
    void saveSetting( const Hub::Setting& setting ) override;
    void reloadGlobalLogOptions();

    void addVeraDeviceMapping( const int veraDeviceId, const Json& deviceData );
    Json fetchGateway( const Json& gatewayDbQuery );

    std::unique_ptr< GroupOperationInterface > groupOperations();
    std::unique_ptr< PolicyOperationInterface > policyOperations();
    std::unique_ptr< SwingerShutdown::SwingerShutdownInterface > swingerShutdown();
    std::unique_ptr< WalkTest::WalkTestInterface > walkTest();
    bool isFirmwareVersionLess( const std::string& targetVersion );
    bool isFirmwareVersionOlder( const std::string& targetVersion, const std::string& sourceVersion );
    JsonObject turnOffLocalAlarm( const JsonObject& params ) override;
    JsonObject forceDisarm( const JsonObject& params ) override;
    std::unique_ptr< Protect::ProtectContactIdInterface > contactIdOperations() override;
    std::unique_ptr< Protect::ProtectSecurityConfigInterface > securityConfigOperations() override;

public:
    static std::string getHubLocationState( const GeoPositionState& locationState );
    static bool isHubTimeZoneDefault();

    void setBusTimeout( std::chrono::seconds timeoutValue );
    std::chrono::seconds getBusTimeout() const;

private:
    void sendMethodCall( MessageSubClasses methodType, const JsonObject& data );
    Json prepareSettingValue( const Json& requestData, const Json& setting );

    // NOTE: deprecated method, instead of it use sendMethodCall
    void makeRPC( const Json& rpcMessage ) const;

    JsonObject findItem( const Json& filter ) const;
    RawItems findItems( const Json& filter ) const;

    JsonObject findDevice( const Json& filter ) const;
    RawDevices findDevices( const Json& filter ) const;

    JsonObject findPolicy( const Json& filter ) const;
    RawPolicies findPolicies( const Json& filter ) const;

    JsonObject findOne( const Json& filter, const char* dbName, const char* tempDbName ) const;
    using JsonObjects = std::vector< JsonObject >;
    JsonObjects findAll( const Json& filter,
                         const char* dbName,
                         const char* tempDbName,
                         const char* idFieldName ) const;

    using CheckPred = std::function< bool( CoreD::Model::ValueType ) >;
    JsonObject checkValueType( JsonObject object, const std::string& typeStr, CheckPred checkPred ) const;
    JsonObject checkItemValueType( const ItemID& itemId, CheckPred checkPred ) const;
    JsonObject checkSettingValueType( const DeviceSettingId& settingId, CheckPred checkPred ) const;
    bool canAddElementToDictionary( const ItemID& itemId );
    void checkBusyDevice( const DeviceID& deviceId ) const;
    JsonObject checkValueSource( JsonObject object ) const;

    std::function< bool( CoreD::Model::ValueType valueType ) > getValueChecker( const JsonObject& data );

    std::set< uint8_t > convertWeekdays( const std::vector< std::string >& weekDays ) const;
    GregorianCalendar::Date getDateWithModifiedTime( const GregorianCalendar::Date& date,
                                                     const std::string& time ) const;

private:
    virtual IDbClient& getDBClient();
    virtual const IDbClient& getDBClient() const;

    void handleGetDeviceWizardCategories( rpc::Message& rpcMessage );
    void handleGetDeviceWizardDevices( rpc::Message& rpcMessage );

    Json handleSetMultipleItemValues( JsonObject data );
    Json handleSetDifferentItemValues( JsonObject data );
    Json handleSetSingleItemValue( JsonObject data );

private:
    static constexpr float DefaultLatitude = 39.8338509f;
    static constexpr float DefaultLongitude = -74.8718262f;

private:
    Services::Service& m_hostService;
    std::unique_ptr< CoreD::Model::Configuration > m_configuration;
    std::unique_ptr< DeviceAddingWizard > m_deviceAddingWizard;
    const ProvisionDataUtils::ProvisionData& m_provisionData;

    LocalTimeGetter m_localTimeGetter;

    std::chrono::seconds m_busTimeout{ 5 };
};

}  // namespace CoreDRemoteInterface
}  // namespace HubPlatform
