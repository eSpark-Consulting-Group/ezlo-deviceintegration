#pragma once

#include "test_utils/component_tests/common_nma_test_fixture.h"
#include "cored/remote_interface/cored_interface.h"
#include "cored/model/cored_item.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/cored_policy.h"

#include "json_utils/json_utils.h"
#include "jsonrpc/error_codes.h"
#include <type_traits>

namespace HubPlatform {
namespace Tests {

class CoreNmaTest : public CommonNmaTests {
public:
    void testHouseModeAlarmedBroadcast( const std::string& mode,
                                        const std::string& deviceId,
                                        int codeLine,
                                        std::chrono::seconds timeoutSeconds = progressBroadcastTimeout );
    void testHouseModeAlarmedPhaseStatusBroadcast( const std::string& mode,
                                                   const std::string& deviceId,
                                                   const std::string& phase,
                                                   const std::string& status,
                                                   int codeLine,
                                                   bool silent = false,
                                                   std::chrono::seconds timeoutSeconds = progressBroadcastTimeout );

    void testHouseModeAlarmedPhaseStatusMainBroadcast( const std::string& mode,
                                                       const std::string& deviceId,
                                                       const std::string& status,
                                                       int codeLine,
                                                       std::chrono::seconds timeoutSeconds = progressBroadcastTimeout );

    void testHouseModesSwitchedBroadcast( const std::string& mode,
                                          const std::string& status,
                                          int codeLine,
                                          std::chrono::seconds timeoutSeconds = progressBroadcastTimeout );
    void testHouseModesSwitchedBroadcast( const JsonObject& broadcast,
                                          const std::string& mode,
                                          const std::string& status,
                                          int codeLine );

    CoreDRemoteInterface::CoreDInterface::DeviceID addDevice( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                                                              const std::string& name,
                                                              const std::string& type,
                                                              const std::string& deviceTypeId,
                                                              const std::string& category,
                                                              const std::string& subcategory,
                                                              bool batteryPowered,
                                                              int codeLine );

    CoreDRemoteInterface::CoreDInterface::DeviceID addDevice( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                                                              const CoreD::Model::Device& device,
                                                              int codeLine );

    CoreDRemoteInterface::CoreDInterface::DeviceID addDevice( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                                                              const std::string& name,
                                                              const std::string& type,
                                                              const std::string& deviceTypeId,
                                                              const std::string& category,
                                                              const std::string& subcategory,
                                                              bool batteryPowered,
                                                              const std::string& parentId,
                                                              int codeLine );

    CoreDRemoteInterface::CoreDInterface::DeviceID addParentDeviceWithRoom(
        CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
        const std::string& name,
        const std::string& type,
        const std::string& deviceTypeId,
        const std::string& category,
        const std::string& subcategory,
        bool batteryPowered,
        const std::string& roomId,
        int codeLine );

    CoreDRemoteInterface::CoreDInterface::DeviceID addDeviceWithRoom(
        CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
        const std::string& name,
        const std::string& type,
        const std::string& deviceTypeId,
        const std::string& category,
        const std::string& subcategory,
        bool batteryPowered,
        const std::string& parentId,
        const std::string& roomId,
        bool parenRoom,
        int codeLine );

    void removeDevice( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                       const std::string& deviceId,
                       size_t itemNumber,
                       size_t settingNumber,
                       int codeLine );

    void removeItem( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                     const std::string& itemId,
                     int codeLine );

    void removePolicy( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                       const std::string& policyId,
                       int codeLine );

    void removeDeviceGroup( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                            const std::string& deviceGroupId,
                            int codeLine );

    JsonObject getDeviceGroup( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                               const std::string& deviceGroupId,
                               int codeLine );

    void addDeviceGroupDevices( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                                const std::string& deviceGroupId,
                                const std::vector< std::string >& devices,
                                int codeLine );

    void removeDeviceGroupDevices( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                                   const std::string& deviceGroupId,
                                   const std::vector< std::string >& devices,
                                   int codeLine );

    CoreD::Model::Device createDevice( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                                       const std::string& name,
                                       const std::string& type,
                                       const std::string& deviceTypeId,
                                       const std::string& category,
                                       const std::string& subcategory,
                                       bool batteryPowered,
                                       int codeLine );

    std::string getMainGateway( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface, int codeLine );

    JsonObject fetchExistingDevices();
    size_t clearExistingDevices( int codeLine );
    void dumpDevices( const JsonObject& devicesArray );

    CoreDRemoteInterface::CoreDInterface::PolicyID addPolicy( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                                                              const std::string& name,
                                                              const std::string& protectConfig,
                                                              const std::string& deviceGroup,
                                                              const std::vector< std::string >& meshbots,
                                                              bool persistent,
                                                              int codeLine );
    JsonObject fetchExistingPolicies();
    size_t clearExistingPolicies( int codeLine );
    void dumpPolicies( const JsonObject& policiesArray );

    CoreDRemoteInterface::CoreDInterface::ItemID addItem( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                                                          const std::string& deviceId,
                                                          const std::string& name,
                                                          CoreD::Model::ValueType valueType,
                                                          bool hasGetter,
                                                          bool hasSetter,
                                                          int codeLine );

    CoreDRemoteInterface::CoreDInterface::ItemID addItem( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                                                          const std::string& deviceId,
                                                          const std::string& name,
                                                          const CoreD::Model::ValueType& valueType,
                                                          bool hasGetter,
                                                          bool hasSetter,
                                                          const CoreD::Model::Item::Enum& enumList,
                                                          int codeLine );

    using BroadcastAssert = std::function< void( JsonObject& broadcast, const CoreD::Model::Item& item ) >;

    CoreDRemoteInterface::CoreDInterface::ItemID addItem( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                                                          const CoreD::Model::Item& item,
                                                          int codeLine,
                                                          BroadcastAssert assertPred );

    CoreDRemoteInterface::CoreDInterface::ItemID addDictionary(
        CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
        const std::string& deviceId,
        const std::string& name,
        CoreD::Model::ValueType valueType,
        bool hasGetter,
        bool hasSetter,
        int codeLine );

    CoreDRemoteInterface::CoreDInterface::DeviceSettingId addDeviceSetting(
        CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
        const std::string& deviceId,
        CoreD::Model::ValueType valueType,
        int codeLine );

    CoreDRemoteInterface::CoreDInterface::DeviceGroupId addDeviceGroup(
        CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
        const std::string& name,
        const bool persistent,
        const std::string& entryDelay,
        const bool followEntry,
        const std::string& role,
        const std::string& packageId,
        const std::vector< std::string >& devices,
        const std::vector< std::string >& exceptions,
        int codeLine );

    void updateDeviceGroup( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                            const std::string& id,
                            const std::string& name,
                            const bool persistent,
                            const std::string& entryDelay,
                            const bool followEntry,
                            const std::vector< std::string >& devices,
                            const std::vector< std::string >& exceptions,
                            int codeLine );

    struct DeviceWithOneItemData {
        std::string m_deviceId;
        std::string m_itemId;
    };

    DeviceWithOneItemData addDeviceWithOneItem( CoreD::Model::ValueType::Enum valueType, int codeLine = -1 );
    DeviceWithOneItemData addDeviceWithOneItemAndRoom( CoreD::Model::ValueType::Enum valueType,
                                                       const std::string& roomId,
                                                       int codeLine );

    DeviceWithOneItemData addChildDeviceWithOneItem( CoreD::Model::ValueType::Enum valueType,
                                                     const std::string& parentId,
                                                     int codeLine = -1 );
    DeviceWithOneItemData addChildDeviceWithOneItemAndParentRoom( CoreD::Model::ValueType::Enum valueType,
                                                                  const std::string& parentId,
                                                                  bool parentRoom,
                                                                  const std::string roomId,
                                                                  int codeLine );

    DeviceWithOneItemData addDeviceWithMotionSensorItem( int codeLine );

    DeviceWithOneItemData addDeviceWithSecurityThreatItem( int codeLine );

    DeviceWithOneItemData addDeviceWithProtectAlarmItem( int codeLine );

    DeviceWithOneItemData addDeviceWithSmokeSensorItem( int codeLine );

    void setItemValue( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                       const std::string& itemId,
                       const JsonObject& value,
                       int codeLine,
                       JsonObject scale = nullptr );

    void setItemValueWithoutCheck( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                                   const std::string& itemId,
                                   const JsonObject& value,
                                   int codeLine,
                                   JsonObject scale = nullptr );

    void runScript( const std::string& scriptPath, const JsonObject& params, int codeLine );
    void runFailedScript( const std::string& scriptPath, int codeLine );
    void runFailedScript( const std::string& scriptPath, const JsonObject& params, int codeLine );
    void runScript( const std::string& scriptPath, const std::string& result, const JsonObject& params, int codeLine );
    void runScriptWithoutChecks( const std::string& scriptPath, const JsonObject& params, int codeLine );
    void runScriptCheckReason( const std::string& scriptPath,
                               const JsonObject& params,
                               const std::string& reason,
                               int codeLine );

    template < typename ValueType >
    void checkItemUpdateBroadcast( CoreDRemoteInterface::CoreDInterface::ItemID itemId,
                                   ValueType value,
                                   int codeLine ) {
        checkItemUpdateBroadcast( itemId, value, false, codeLine );
    }

    template < typename ValueType >
    void checkItemUpdateBroadcast( CoreDRemoteInterface::CoreDInterface::ItemID itemId,
                                   ValueType value,
                                   bool serviceNotification,
                                   int codeLine ) {
        JsonObject broadcast;
        std::string codeLineStr = getCodeLineString( codeLine );
        broadcast = waitForBroadcast( "hub.item.updated", codeLineStr );
        checkItemUpdateBroadcast( broadcast, itemId, value, serviceNotification, codeLine );
    }

    template < typename ValueType >
    void checkItemUpdateBroadcast( const JsonObject& broadcast,
                                   CoreDRemoteInterface::CoreDInterface::ItemID itemId,
                                   ValueType value,
                                   int codeLine ) {
        checkItemUpdateBroadcast( broadcast, itemId, value, false, codeLine );
    }

    template < typename ValueType >
    void checkDeviceSettingUpdateBroadcast( CoreDRemoteInterface::CoreDInterface::DeviceSettingId settingId,
                                            ValueType value,
                                            int codeLine ) {
        JsonObject broadcast;
        std::string codeLineStr = getCodeLineString( codeLine );
        broadcast = waitForBroadcast( "hub.setting.updated", codeLineStr );
        checkDeviceSettingUpdateBroadcast( broadcast, settingId, value, codeLine );
    }

    template < typename ValueType, typename Json >
    void compareValueEqual( ValueType const& source, Json const& resultValue, std::string const& codeLineStr ) {
        JsonObject jsSource( source );
        if( jsSource.is_number() ) {
            EXPECT_FLOAT_EQ( resultValue, jsSource.get< float >() ) << codeLineStr;
        }
        else {
            EXPECT_EQ( resultValue.dump(), jsSource.dump() ) << codeLineStr;
        }
    }

    template < typename ValueType, typename Json >
    bool isValueEqual( ValueType const& source, Json const& resultValue, std::string const& codeLineStr ) {
        JsonObject jsSource( source );
        if( jsSource.is_number() ) {
            return resultValue == jsSource.get< float >();
        }
        else {
            resultValue.dump() == jsSource.dump();
        }

        return false;
    }

    template < typename Json >
    void compareValueEqual( float const& source, Json const& resultValue, std::string const& codeLineStr ) {
        EXPECT_FLOAT_EQ( resultValue, JsonObject( source ) ) << codeLineStr;
    }

    template < typename Json >
    void compareValueEqual( double const& source, Json const& resultValue, std::string const& codeLineStr ) {
        EXPECT_FLOAT_EQ( resultValue, JsonObject( source ) ) << codeLineStr;
    }

    template < typename ValueType >
    void checkItemUpdateBroadcast( const JsonObject& broadcast,
                                   CoreDRemoteInterface::CoreDInterface::ItemID itemId,
                                   ValueType value,
                                   bool serviceNotification,
                                   int codeLine ) {
        std::string codeLineStr = getCodeLineString( codeLine );
        JsonObject result;
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" )
            << "broadcast: " << broadcast << ", at: " << codeLineStr;
        EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << codeLineStr;
        EXPECT_EQ( json_get_str( result, "_id" ), itemId ) << "broadcast: " << broadcast << ", at: " << codeLineStr;
        EXPECT_EQ( json_get_bool( result, "serviceNotification", !serviceNotification ), serviceNotification )
            << codeLineStr;

#ifdef _DEBUG
        auto resultValueDump = result[ "value" ].dump();
#endif
        compareValueEqual( value, result[ "value" ], codeLineStr );
    }

    template < typename ValueType >
    void checkDeviceSettingBroadcast( const JsonObject& broadcast,
                                      CoreDRemoteInterface::CoreDInterface::DeviceSettingId settingId,
                                      ValueType value,
                                      int codeLine ) {
        std::string codeLineStr = getCodeLineString( codeLine );
        JsonObject result;
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.setting.updated" ) << codeLineStr;
        EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << codeLineStr;
        EXPECT_EQ( json_get_str( result, "_id" ), settingId ) << codeLineStr;

#ifdef _DEBUG
        auto resultValueDump = result[ "value" ].dump();
#endif
        compareValueEqual( value, result[ "value" ], codeLineStr );
    }

    void checkDeviceUpdateBroadcast( CoreDRemoteInterface::CoreDInterface::DeviceID deviceId,
                                     bool isReady,
                                     int codeLine );

    void checkDeviceUpdateBroadcast( CoreDRemoteInterface::CoreDInterface::DeviceID deviceId,
                                     const KeyJsonList& fields,
                                     int codeLine );

    void checkItemDictionaryUpdatedBroadcast( CoreDRemoteInterface::CoreDInterface::ItemID itemId,
                                              const KeyJsonList& fields,
                                              int codeLine );

    void checkUserNotificationBroadcast( const std::string& deviceName,
                                         const std::string& roomName,
                                         const std::string& user,
                                         int codeLine );

    void checkUserNotificationBroadcast( const JsonObject& broadcast,
                                         const std::string& deviceName,
                                         const std::string& roomName,
                                         const std::string& user,
                                         int codeLine );

    void checkExtensionsPluginRunProgressBroadcast( const std::string& status, int codeLine );
    void checkExtensionsPluginRunProgressBroadcast( const JsonObject& broadcast,
                                                    const std::string& status,
                                                    int codeLine );

    void checkExtensionsPluginRunProgressBroadcastByReason( const std::string& status,
                                                            const std::string& reason,
                                                            int codeLine );

    static double toFahrenheit( double inCelsius );
    static double toCelsius( double inFahrenheit );
    static double convertToScale( double inScale, const char* fromScale, const char* toScale );

    static void compareTemperatureValues( const Json& itemInfo,
                                          const Json& response,
                                          const std::string& responseKey,
                                          const std::string& message = std::string{} );

    void setDefaultTemperatureScaleSetting( const std::string& scale, int codeLine );

    void checkResult( const Json& request, const std::string& expectedValue, int line );

    void checkFailure( const Json& request, const std::string& errorMessage, int line );

    void checkUiBroadcast( const std::string& expectedValue, int line );

    template < typename ValueType >
    bool isItemUpdateBroadcast( const JsonObject& broadcast,
                                CoreDRemoteInterface::CoreDInterface::ItemID itemId,
                                ValueType value ) {
        JsonObject result;

        return json_get_str( broadcast, "msg_subclass" ) == "hub.item.updated" &&
               json_get_obj( broadcast, "result", result ) && json_get_str( result, "_id" ) == itemId &&
               std::abs( value - result[ "value" ].get< ValueType >() ) <= std::numeric_limits< ValueType >::epsilon();
    }

    bool isDeviceUpdateBroadcast( const JsonObject& broadcast,
                                  CoreDRemoteInterface::CoreDInterface::DeviceID deviceId,
                                  bool flag ) {
        JsonObject result;

        return json_get_str( broadcast, "msg_subclass" ) == "hub.device.updated" &&
               json_get_obj( broadcast, "result", result ) && json_get_str( result, "_id" ) == deviceId &&
               json_get_bool( result, "armed" ) == flag;
    }

    bool isHouseModeSwitchedBroadcast( const JsonObject& broadcast,
                                       const std::string& toMode,
                                       const std::string& status );

    bool isUserNotificationBroadcast( const JsonObject& broadcast,
                                      const std::string& deviceName,
                                      const std::string& roomName,
                                      const std::string& user ) const;

    static CoreDRemoteInterface::CoreDInterface& makeRemote( Services::Service& hostService );
    static CoreDRemoteInterface::CoreDInterface& makeRemote();

    JsonObject getLastDeviceGroupBroadcast();

private:
    JsonObject m_lastDeviceGroupBroadcast = {};
};

}  // namespace Tests
}  // namespace HubPlatform
