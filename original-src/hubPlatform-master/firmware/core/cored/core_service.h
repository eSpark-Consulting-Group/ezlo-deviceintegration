#pragma once

#include "cored/core_messenger.h"
#include "cored/room_manager.h"
#include "cored/device_manager.h"
#include "cored/exceptions.h"
#include "cored/db_modifier/room_db_modifier.h"
#include "cored/model/cored_gateway_processes.h"
#include "cored/bus_db_manager.h"

#include "addon_interfaces/addon.h"
#include "service_interfaces/service.h"
#include "cored/house_mode_proc.h"

#include "cored/group_operations/handler/group_operations_handler.h"
#include "group_operations/device_item_group_manager.h"
#include "cored/policies/handler/policies_operations_handler.h"

#include "json_utils/json_utils.h"
#include "update_manager/remote_interface/um_remote_interface.h"

#include <memory>
#include <string>
#include <cored/group_operations/device_group_find_handler.h>

namespace HubPlatform {

namespace UT {
class DeviceManagerTestSuite;
class CoredTestSuite;
class HouseModesProcessorFixture;
class RoomManagerTestSuite;
class ItemDbModifierTest;
class GatewaySettingsTest;
class DeviceSettingsTest;
class SettingsManagerTest;
class FeatureManagerTestSuite;
class SwingerShutdownTestSuite;
class WalkTestSuite;
class ProtectTestSuite;
class ProtectionConfigTests;
class PolicyOperationsFixture;
}  // namespace UT

namespace Hub {
class SettingsManager;
}

namespace CoreD {

class WhatHappened;
class PermanentDBWriteScheduler;

class GatewayDbModifier;
class DeviceDbModifier;
class ItemDbModifier;
class PolicyDbModifier;
class GatewaySettingDbModifier;
class DeviceSettingDbModifier;
class RoomDbModifier;
class IFeaturesManager;
class HouseModeAlarms;
class SecurityItemDbModifier;
namespace SwingerShutdown {
class SwingerShutdown;
};  // namespace SwingerShutdown

namespace WalkTest {
class WalkTest;
};  // namespace WalkTest

class CoreService : public Services::Service, public Messenger {
    friend class UT::DeviceManagerTestSuite;
    friend class UT::CoredTestSuite;
    friend class UT::RoomManagerTestSuite;
    friend class UT::ItemDbModifierTest;
    friend class UT::GatewaySettingsTest;
    friend class UT::DeviceSettingsTest;
    friend class UT::SettingsManagerTest;
    friend class UT::HouseModesProcessorFixture;
    friend class UT::GroupOperationsFixture;
    friend class UT::PolicyOperationsFixture;
    friend class UT::SwingerShutdownTestSuite;
    friend class UT::FeatureManagerTestSuite;
    friend class UT::WalkTestSuite;
    friend class UT::ProtectTestSuite;
    friend class UT::ProtectionConfigTests;

public:
    CoreService();
    ~CoreService() override;
    JsonObject getHubStatus( const JsonObject& request );

    int restoreBackup( const std::string& tempBackupFolder ) override;

    GatewayDbModifier& getGatewaysDbModifier();
    DeviceDbModifier& getDevicesDbModifier();
    ItemDbModifier& getItemsDbModifier();
    PolicyDbModifier& getPoliciesDbModifier();
    GatewaySettingDbModifier& getGatewaySettingsDbModifier();
    DeviceSettingDbModifier& getDeviceSettingsDbModifier();
    RoomDbModifier& getRoomDbModifier();

    void handleHouseModeSwitched( const JsonObject& message );

    bool initLogOptionsFromFirmwareConfig( const std::string& firmwareCfgPath,
                                           const ConfigReader& configReader,
                                           const UpdateManager::RemoteInterface& umRemoteInterface );

protected:
    CoreService( const std::string& name,
                 std::unique_ptr< Services::BusWrapper > impl,
                 std::unique_ptr< IDbClient > dbClient,
                 ConfigReader&& configReader );

    void setUp() override;
    void tearDown() override;
    void handleLegacyMethodCall( const JsonObject& json, const std::string& sender ) override;
    void handleMethodCall( const JsonObject& json, MessageSubClasses requestType, const std::string& sender ) override;
    JsonObject handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) override;
    bool handleAsyncRequestFromBus( const JsonObject& request,
                                    MessageSubClasses requestType,
                                    const std::string& uuid,
                                    const std::string& sender ) override;
    void handleMessageFromBus( const Services::Message& message, const std::string& sender );
    void handleEventMessage( const int subClass, const JsonObject& message );

    void handleRemovePluginGateways( const JsonObject& json );
    void handleRemoveGatewayDevices( const JsonObject& json );
    void handleRemoveDevice( const JsonObject& json );
    void handleRenameDevice( const JsonObject& json );
    bool handleSetItemValue( const JsonObject& json, const std::string& operationId = "" );
    bool handleAddItemDictionaryValue( const JsonObject& json, const std::string& operationId = "" );
    bool handleSetItemDictionaryValue( const JsonObject& json, const std::string& operationId = "" );
    bool handleRemoveItemDictionaryValue( const JsonObject& json, const std::string& operationId = "" );
    bool handleClearItemDictionary( const JsonObject& json, const std::string& operationId = "" );
    void handleSetGatewaySettingValue( const JsonObject& json );
    void handleUpdateGatewaySettingValue( const JsonObject& json );
    void handleUpdateGatewaySettingStatus( const JsonObject& json );
    void handleStartGatewayProcess( const JsonObject& json );
    void handleStopGatewayProcess( const JsonObject& json );
    JsonObject handleSetDeviceSettingValue( const JsonObject& json );
    JsonObject handleSetDeviceSettingDictionaryValue( const JsonObject& json );
    JsonObject handleResetDeviceSetting( const JsonObject& json );
    JsonObject handleCheckDevice( const JsonObject& json );
    void handleUpdateItemField( const JsonObject& json );
    void handleUpdateItemValue( const JsonObject& json );
    void handleUpdateItemDictionaryValue( const JsonObject& json );
    void handleUpdateSettingValue( const JsonObject& json );
    void handleUpdateSettingDictionaryValue( const JsonObject& json );
    void handleUpdateSettingStatus( const JsonObject& json );
    void handleUpdateReachableState( const JsonObject& json );
    void handleUpdateDeviceStatus( const JsonObject& json );
    void handleUpdateReadyState( const JsonObject& json );
    void handleUpdateDeviceName( const JsonObject& json );
    void handleUpdateDeviceRoomId( const JsonObject& json );
    void handleRemoveDeviceForce( const JsonObject& json );
    void handleSetFeatureStatus( const JsonObject& json );
    void handleSetSettingValue( const JsonObject& json );
    void handleSettingUpdated( const JsonObject& json );
    void handleRemoveGatewaySetting( const JsonObject& json );

    using PluginID = std::string;
    using GatewayID = std::string;
    using DeviceID = std::string;
    using ItemID = std::string;
    using PolicyID = std::string;
    using DeviceSettingID = std::string;
    using GatewaySettingID = std::string;
    using RoomID = std::string;
    using ItemGroupId = std::string;
    using DeviceGroupId = std::string;

    using WalkTestStopCallback = std::function< void() >;

    JsonObject handleModifyGateway( const JsonObject& params );
    JsonObject handleAddDevice( const JsonObject& params );
    JsonObject handleModifyDevice( const JsonObject& params );
    JsonObject handleModifyItem( const JsonObject& params );
    JsonObject handleModifyDeviceSetting( const JsonObject& params );
    JsonObject handleAddItem( const JsonObject& request );
    JsonObject handleRemoveItem( const ItemID& itemId );

    JsonObject handleRemovePolicy( const PolicyID& policyId );

    JsonObject addDeviceToHouseModes( const std::string& deviceId, const Json& device );

    JsonObject handleSetHouseMode( const JsonObject& params );
    JsonObject handleSetHubCoordinates( const JsonObject& json );
    JsonObject handleSetHubTimeZone( const JsonObject& params );
    JsonObject handleCancelSwitchHouseMode( const JsonObject& params );
    JsonObject handleCancelEntryDelay( const JsonObject& params );
    JsonObject handleSkipEntryDelay( const JsonObject& params );
    JsonObject handleSetEntryDelay( const JsonObject& params );
    JsonObject handleResetEntryDelay( const JsonObject& params );
    JsonObject checkEntryDelayObject( const JsonObject& params );
    JsonObject handleAddDisarmedDevice( const JsonObject& params );
    JsonObject handleRemoveDisarmedDevice( const JsonObject& params );
    JsonObject handleSetNotificationsForHouseMode( const JsonObject& params );
    JsonObject handleAddAlarmsOffDevice( const JsonObject& params );
    JsonObject handleRemoveAlarmsOffDevice( const JsonObject& params );
    JsonObject handleSetDisarmedDefault( const JsonObject& params );
    JsonObject handleHouseModeProtectSet( const JsonObject& params );
    JsonObject handleHouseModeProtectButtonsSet( const JsonObject& params );
    JsonObject handleAddProtectDevices( const JsonObject& params );
    JsonObject handleRemoveProtectDevices( const JsonObject& params );
    JsonObject handleSetServiceNotifications( const JsonObject& params );
    JsonObject handleUpdateDeviceFirmware( const JsonObject& params );
    JsonObject handleBackupCreate( const JsonObject& params );
    void handleBackupRestore( const JsonObject& params );

    void removeDisarmedDevice( const DeviceID& deviceId );
    void removeAlarmsOffDevice( const DeviceID& deviceId );
    void eraseDisarmedDevice( const DeviceID& deviceId );
    void removeProtectButton( const DeviceID& deviceId );

    JsonObject handleAddCamerasOffDevice( const JsonObject& params );
    JsonObject handleRemoveCamerasOffDevice( const JsonObject& params );
    void removeCamerasOffDevice( const DeviceID& deviceId );
    JsonObject handleAddBypassDevices( const JsonObject& params );
    JsonObject handleRemoveBypassDevices( const JsonObject& params );
    void removeBypassDevice( const DeviceID& deviceId );
    JsonObject handleLocalAlarmOff( const JsonObject& params );
    JsonObject handleForceDisarm( const JsonObject& params );

    /* @brief remove all plugin's gateways */
    void removePluginGateways( const PluginID& pluginId );

    /* @brief remove a gateway specified by a @c gatewayId id and all devices registered for this gateway */
    void removeGateway( const GatewayID& gatewayId );

    /* @brief remove gateway settings managed by a @c gatewayId id */
    void removeGatewaySettings( const GatewayID& gatewayId );

    /* @brief stop gateway processes if any by a @c gatewayId id */
    void stopGatewayProcesses( const GatewayID& gatewayId );

    /* @brief remove a gateway setting specified by a @c settingId id */
    void removeGatewaySetting( const GatewaySettingID& settingId );

    /* @brief remove all devices registered for a gateway specified by a @c gatewayId id */
    void removeGatewayDevices( const GatewayID& gatewayId );

    /* @brief remove a device specified by a @c deviceId id and all items registered for this device */
    void removeDevice( const DeviceID& deviceId );

    /* @brief remove all items registered for a device specified by a @c deviceId id */
    void removeDeviceItems( const DeviceID& deviceId );

    /* @brief remove all device settings registered for a device specified by a @c deviceId id */
    void removeDeviceSettings( const DeviceID& deviceId );

    /* @brief remove an item specified by an @c itemId id */
    void removeItem( const ItemID& itemId );

    /* @brief remove a policy specified by an @c policyId id */
    void removePolicy( const PolicyID& policyId );

    /* @brief remove an device setting specified by an @c settingId id */
    void removeDeviceSetting( const DeviceSettingID& settingId );

    /* @brief deletes a room specified by an @c itemId id and unplaces devices from the room */
    void deleteRoom( const RoomID& roomId );

    std::vector< std::string > getChildDeviceIds( const DeviceID& parentDeviceId ) const;
    void stopWalkTest();

protected:
    JsonObject sendBackupRequest( const std::string& service,
                                  const JsonObject& request,
                                  MessageSubClasses processType ) override;

    void sendExecutePluginScript( const std::string& script, const Json& scriptParams ) override;

    virtual void setUpHouseModes();
    virtual void setUpPredefinedItemGroups();
    virtual void setUpPredefinedDeviceGroups();
    virtual void setUpPredefinedContactIds();
    virtual void setUpPredefinedSecurityConfigs();
    virtual void setUpPredefinedPolicies();
    virtual void setUpGeneratedSensorGroups();

    DeviceManager& getDeviceManager();
    JsonObject getItem( const ItemID& itemId ) const;

    void handleReloadGlobalLogOptions( const JsonObject& json );

    static JsonObject handleRequestProvisionData( const JsonObject& params );
    JsonObject handleRequestDBData( const JsonObject& params );
    void handlePerformDBOperation( const JsonObject& params );

    JsonObject handleAddVeraDeviceMapping( const JsonObject& params );
    Json makeValueForUpdate( const ItemID& itemId, const JsonObject& params );

private:
    static std::string generateOperationId( const std::string& uuid, const std::string& sender );

    bool setHubTimeZone( const std::string& zoneName );

    DbModifier::Dictionary getDictionaryOperation( const std::string& key,
                                                   const JsonObject& oldValue,
                                                   const JsonObject& newValue ) const;

    JsonObject getSetting( const DeviceSettingID& settingId ) const;

    Json getDeviceGateway( const std::string& itemId );
    bool isCustomResponse( Json& request, const std::string& operationId, const Json& gateway );
    std::string getScript( const Json& gateway );

    template < typename T >
    T getParameter( const JsonObject& json, const std::string& key ) {
        T param;
        if( !json_get( json, key, param ) ) {
            throw EmptyParameter( key );
        }
        return param;
    }

    void stopGatewayProcess( const GatewayID& gatewayId,
                             CoreD::Model::GatewayProcess process,
                             CoreD::Model::GatewayProcessResult result );

    void sendDeviceBroadcast( MessageSubClasses subClass, const Json& json );
    void sendItemBroadcast( MessageSubClasses subClass, const Json& json );
    void sendPolicyBroadcast( MessageSubClasses subClass, const Json& json );
    void sendGatewaySettingBroadcast( MessageSubClasses subClass, const Json& json );
    void sendDeviceSettingBroadcast( MessageSubClasses subClass, const Json& json );
    void sendDeviceItemGroupBroadcast( MessageSubClasses subClass, const Json& json );

    bool handleSetMultipleItemValues( const JsonArray& ids,
                                      const JsonObject& value,
                                      const std::string& operationId,
                                      const Json& source );

    bool handleSetDifferentItemValues( const JsonObject& items, const std::string& operationId, const Json& source );

    bool handleSetSingleItemValue( const JsonObject& json, const std::string& operationId, const Json& source );

    JsonObject handleBackupVeraRequest( const Json& request );
    Json readVeraNodesInformation( const std::string& pathToFile ) const;
    std::string findEzloDeviceId( int veraDeviceId ) const;
    JsonObject generateVeraBackupResponse( const std::string& status, const std::string& errorDescription ) const;

    void sendCloudVideoforwardRequest( MessageSubClasses subClass, const Json& json );
    static Json createCloudVideoforwardRequestData( const std::string& identity );
    JsonObject handlePlaceDevice( const JsonObject& request );
    void swingerShutdownDeviceRemovedHandler( const Json& json );
    void swingerShutdownItemUpdatedHandler( const Json& json );
    void swingerShutdownItemRemovedHandler( const Json& json );
    Json getSwingerShutdownList( const Json& request );
    Json swingerShutdownReset( const JsonObject& request );
    Json swingerShutdownDisableAdd( const JsonObject& request );
    Json swingerShutdownDisableRemove( const JsonObject& request );
    Json swingerShutdownLimitsSet( const JsonObject& request );
    Json swingerShutdownLimitsReset( const JsonObject& request );
    Json protectSystemTest( const JsonObject& request );
    Json protectSystemTestStop( const JsonObject& request );
    Json protectSystemTestExtend( const JsonObject& request );

    Services::PostponedTaskPtr scheduleStopWalkTest( std::unique_ptr< WalkTest::WalkTest >& walkTest,
                                                     Services::Service& service,
                                                     std::chrono::seconds interval,
                                                     const WalkTestStopCallback& callback );

private:
    std::unique_ptr< RoomManager > m_roomManager;

    std::unique_ptr< DeviceManager > m_deviceManager;
    std::unique_ptr< WhatHappened > m_whatHappened;
    std::unique_ptr< GatewayDbModifier > m_gatewaysDb;
    std::unique_ptr< DeviceDbModifier > m_devicesDb;
    std::unique_ptr< SwingerShutdown::SwingerShutdown > m_swingerShutdownDevicesManager;
    std::unique_ptr< SecurityItemDbModifier > m_securityItemsDbModifier;
    std::unique_ptr< ItemDbModifier > m_itemsDb;
    std::unique_ptr< PolicyDbModifier > m_policiesDb;
    std::unique_ptr< GatewaySettingDbModifier > m_gatewaySettingsDb;
    std::unique_ptr< DeviceSettingDbModifier > m_deviceSettingsDb;
    std::unique_ptr< RoomDbModifier > m_roomsDb;
    std::unique_ptr< HouseModeProcessor > m_houseModeProcessor;
    std::unique_ptr< ConfigReader > m_configReader;
    std::unique_ptr< PermanentDBWriteScheduler > m_permanentDBWriteScheduler;
    std::unique_ptr< IFeaturesManager > m_featuresManager;
    std::unique_ptr< Hub::SettingsManager > m_settingsManager;
    std::unique_ptr< BusDbManager > m_busDBManager;

    const std::string& serviceName = getQualifiedName();

    std::unique_ptr< HouseModeAlarms > m_houseModeAlarms;

    CoreDRemoteInterface::CoreDInterfaceUPtr m_core;
    std::unique_ptr< HubPlatform::CoreD::DeviceItemGroupManager > m_deviceItemGroupManager;
    std::unique_ptr< HubPlatform::CoreD::GroupOperationsHandler > m_deviceGroupHandler;
    std::unique_ptr< HubPlatform::CoreD::PolicyOperationsHandler > m_policyHandler;
    std::unique_ptr< WalkTest::WalkTest > m_walkTest;
    Services::PostponedTaskPtr m_walkTestStopTask;

    void cleanWalkTestStopTask();
    void stopWalkTestIfNeeded();
};

}  // namespace CoreD
}  // namespace HubPlatform
