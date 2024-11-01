
#include "cored/core_service.h"
#include "cored/room_manager.h"
#include "cored/device_manager.h"
#include "cored/device_adding_wizard.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/db_modifier/db_modifier.h"
#include "cored/db_modifier/gateway_db_modifier.h"
#include "cored/db_modifier/device_db_modifier.h"
#include "cored/db_modifier/item_db_modifier.h"
#include "cored/exceptions.h"

#include "cored/tests/service/test_core_service.h"
#include "cored/tests/tests_resources.h"

#include "service_interfaces/groups_names.h"
#include "service_interfaces/bus_wrapper.h"
#include "service_interfaces/service.h"
#include "service_interfaces/message.h"
#include "service_interfaces/tests/fakes/fake_response_bus.h"

#include "json_utils/json_utils.h"
#include "base_utils/ezlodb.h"
#include "bus/base_bus.h"
#include "bus/messagesclasses.h"
#include "base_utils/utils.h"
#include "test_utils/temporary_root_folder_wrapper.h"
#include "test_utils/test_utils.h"

#include "common_utils/cpp14.h"
#include "system_utils/environment.h"

#include "database_client/dbqueryparser.h"
#include "database_client/idbclient.h"
#include "database_client/fake_db/fake_db_client.h"
#include "database_client/db_tables_names.h"
#include "update_manager/resources/um_resources.h"
#include "config_reader/config_keys.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>
#include <string>
#include <fstream>
#include <cstdio>
#include <functional>

/*
TESTPLAN

DONE    1. Add gateway broadcast
DONE    2. Add device broadcast
DONE    3. Add item broadcast
DONE    4. Add user code item broadcast
DONE    5. Remove gateway broadcast
DONE    6. Remove device broadcast
DONE    7. Update device broadcast
DONE        7.1 Update name
DONE        7.2 Update firmware
DONE        7.3 Update type
DONE    8. Remove item broadcast
DONE    9. Remove plugin gateways
DONE    10. Remove plugin gateways with devices
DONE    11. Remove plugin gateways with devices and items
DONE    12. Remove gateway
DONE    13. Remove gateway with devices
DONE    14. Remove gateway with devices and items
DONE    15. Remove gateway devices
DONE    16. Remove gateway devices with items
DONE    17. Remove device
DONE    18. Remove device with items
DONE    19. Remove device items
DONE    20. Remove item
DONE    21. Set item value
DONE        21.1 Singlecast
DONE        21.2 Multicast
DONE            21.2.1 One item
DONE            21.2.2 Several items
DONE        22.3 builtin scripts: cored manages hub.ite,.value.set itself, without lua daemon
DONE            22.3.1 singlecast
DONE            22.3.2 multicast, one item, one value
DONE            22.3.2 multicast, several items, own values item
DONE    22. Update item value
DONE    23. Update item user code value
DONE    24. Add item dictionary value
DONE    25. Set item dictionary value
DONE    26. Remove item dictionary value
DONE    27. Remove device force
DONE    28. Update reachable state
DONE    29. Update ready state
DONE    30. Update device name
DONE    31. Update device room id
DONE    32. Update device status
DONE    33. Send execute plugin script with parameters
DONE    34. Handle set service notifications
DONE    35. Remove gateway devices with child
DONE    36. Compare firmware version
DONE    37. Update firmware modification time
*/

namespace HubPlatform {
namespace UT {

using namespace Services;
using namespace CoreD::Model::Resources;

class CoredTestSuite : public ::testing::Test {
public:
    using KeyJsonList = std::vector< std::pair< std::string, JsonObject > >;

public:
    CoredTestSuite()
    : m_cored( std::make_unique< Services::BusWrapper >(
          Services::BusWrapper::BusTypeTag< Services::UT::FakeResponseBus >{} ) ) {
        // Do not print errors and warnings on erroneous test cases
        // TODO: implement fake log and check error messages too.
        Log::setLocalLogLevel( Log::TRACE );
    }

    void SetUp() override {
        m_cored.setUp();
        cleanDB();
    }

    void TearDown() override {
        cleanDB();
        m_cored.tearDown();
    }

    CoreD::CoreService& getCoreDService() {
        return m_cored;
    }

    IDbClient& getDBClient() {
        return getCoreDService().getDBClient();
    }

    void cleanDB() {
        getDBClient().clean();
    }

    const Services::UT::FakeResponseBus& getTestBus() {
        return *dynamic_cast< Services::UT::FakeResponseBus* >( getCoreDService().getBus() );
    }

    JsonObject decryptItem( const JsonObject& item ) {
        auto decryptedItem = item;
        CoreD::Model::Item::decryptItemAsJson( ProvisionDataUtils::getManager().getProvisionData(), decryptedItem );
        json_remove( decryptedItem, "_id" );
        return decryptedItem;
    }

protected:
    CoreD::CoreService::GatewayID ensureGateway( const Json& gate ) {
        auto gatewayId = getCoreDService().m_deviceManager->ensureGateway( gate );
        return gatewayId[ "id" ];
    }
    CoreD::CoreService::DeviceID addDevice( const Json& device ) {
        auto deviceId = getCoreDService().m_deviceManager->addDevice( device );
        return deviceId[ "id" ];
    }
    CoreD::CoreService::DeviceID renameDevice( const Json& device ) {
        auto deviceId = getCoreDService().m_deviceManager->renameDevice( device );
        return deviceId[ "id" ];
    }
    CoreD::CoreService::DeviceID updateDevice( const std::string& id, const Json& fields ) {
        auto deviceId = getCoreDService().m_deviceManager->modifyDevice( id, fields );
        return deviceId[ "id" ];
    }
    CoreD::CoreService::ItemID addItem( const Json& item ) {
        auto itemId = getCoreDService().m_deviceManager->addItem( item );
        return itemId[ "id" ];
    }
    void removePluginGateways( const CoreD::CoreService::PluginID& pluginId ) {
        getCoreDService().removePluginGateways( pluginId );
    }
    void removeGateway( const CoreD::CoreService::GatewayID& gatewayId ) {
        getCoreDService().removeGateway( gatewayId );
    }
    void removeGatewayDevices( const CoreD::CoreService::GatewayID& gatewayId ) {
        getCoreDService().removeGatewayDevices( gatewayId );
    }
    void removeDevice( const CoreD::CoreService::DeviceID& deviceId ) {
        getCoreDService().removeDevice( deviceId );
    }
    void removeDeviceItems( const CoreD::CoreService::DeviceID& deviceId ) {
        getCoreDService().removeDeviceItems( deviceId );
    }
    void removeItem( const CoreD::CoreService::ItemID& itemId ) {
        getCoreDService().removeItem( itemId );
    }
    bool setItemValue( const JsonObject& json ) {
        return getCoreDService().handleSetItemValue( json );
    }
    void updateItemValue( const JsonObject& json ) {
        getCoreDService().handleUpdateItemValue( json );
    }
    bool addItemDictionaryValue( const JsonObject& json ) {
        return getCoreDService().handleAddItemDictionaryValue( json );
    }
    bool setItemDictionaryValue( const JsonObject& json ) {
        return getCoreDService().handleSetItemDictionaryValue( json );
    }
    bool removeItemDictionaryValue( const JsonObject& json ) {
        return getCoreDService().handleRemoveItemDictionaryValue( json );
    }
    Json getItemDictionaryValue( const std::string& itemId, const std::string& dictionaryId ) {
        CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getCoreDService() );
        return coreDInterface.getItemDictionaryValue( itemId, dictionaryId );
    }

    bool clearItemDictionary( const JsonObject& json ) {
        return getCoreDService().handleClearItemDictionary( json );
    }
    void removeDeviceForce( const JsonObject& json ) {
        getCoreDService().handleRemoveDeviceForce( json );
    }
    void updateReachableState( const JsonObject& json ) {
        getCoreDService().handleUpdateReachableState( json );
    }
    void updateReadyState( const JsonObject& json ) {
        getCoreDService().handleUpdateReadyState( json );
    }
    void updateDeviceName( const JsonObject& json ) {
        getCoreDService().handleUpdateDeviceName( json );
    }
    void updateDeviceRoomId( const JsonObject& json ) {
        getCoreDService().handleUpdateDeviceRoomId( json );
    }
    void updateDeviceStatus( const JsonObject& json ) {
        getCoreDService().handleUpdateDeviceStatus( json );
    }
    void sendExecutePluginScript( const std::string& script, const Json& scriptParams ) {
        getCoreDService().sendExecutePluginScript( script, scriptParams );
    }
    JsonObject handleSetServiceNotifications( const JsonObject& request ) {
        return getCoreDService().handleSetServiceNotifications( request );
    }
    JsonObject handleUpdateDeviceFirmware( const JsonObject& request ) {
        return getCoreDService().handleUpdateDeviceFirmware( request );
    }
    void checkDeviceGroupBroadcast( MessageSubClasses subclass, const KeyJsonList& fields, int codeLine );
    void checkInternalBroadcast( MessageSubClasses subclass, const KeyJsonList& fields, int codeLine );
    void checkGroupBroadcast( const char* group, MessageSubClasses subclass, const KeyJsonList& fields, int codeLine );

protected:
    CoreD::Tests::TestCoreServiceWithoutHouseModes m_cored;
};

void CoredTestSuite::checkGroupBroadcast( const char* group,
                                          MessageSubClasses subclass,
                                          const CoredTestSuite::KeyJsonList& fields,
                                          int codeLine ) {
    const auto message = getTestBus().getMessage();
    EXPECT_EQ( message.group, group ) << "Message group differs at " __FILE__ ":" << codeLine;
    EXPECT_EQ( message.msgClass, EVENT ) << "MessageClass differs at " __FILE__ ":" << codeLine;
    EXPECT_EQ( message.msgSubClass, subclass ) << "MessageSubClass differs at " __FILE__ ":" << codeLine;

    for( auto& field : fields ) {
        Json fieldData = JSON_EMPTY_OBJ;
        EXPECT_TRUE( json_get( message.data, field.first, fieldData ) )
            << "Could not fetch field " << field.first << ", at " __FILE__ ":" << codeLine;
        EXPECT_EQ( fieldData, field.second )
            << "Field check failed: name=" << field.first << " at " __FILE__ ":" << codeLine;
    }
}

void CoredTestSuite::checkDeviceGroupBroadcast( MessageSubClasses subclass,
                                                const CoredTestSuite::KeyJsonList& fields,
                                                int codeLine ) {
    checkGroupBroadcast( GroupsNames::DEVICE_GROUP, subclass, fields, codeLine );
}

void CoredTestSuite::checkInternalBroadcast( MessageSubClasses subclass,
                                             const CoredTestSuite::KeyJsonList& fields,
                                             int codeLine ) {
    checkGroupBroadcast( GroupsNames::INTERNAL_BROADCAST_GROUP, subclass, fields, codeLine );
}

TEST_F( CoredTestSuite, addAndUpdateGatewayBroadcasts ) {
    ensureGateway( JSON( GatewayFields::NameFieldName << "gateway" ) );

    checkGroupBroadcast(
        GroupsNames::BROADCAST_GROUP, HUB_GATEWAY_ADDED, { { GatewayFields::NameFieldName, "gateway" } }, __LINE__ );

    ensureGateway( JSON( GatewayFields::NameFieldName << "gateway" << GatewayFields::StatusFieldName << "idle" ) );

    checkGroupBroadcast(
        GroupsNames::BROADCAST_GROUP, HUB_GATEWAY_UPDATED, { { GatewayFields::StatusFieldName, "idle" } }, __LINE__ );
}

TEST_F( CoredTestSuite, addDeviceBroadcast ) {
    addDevice( JSON( DeviceFields::NameFieldName << "device" ) );

    checkDeviceGroupBroadcast( HUB_DEVICE_ADDED, { { DeviceFields::NameFieldName, "device" } }, __LINE__ );
}

TEST_F( CoredTestSuite, addItemBroadcast ) {
    addItem( JSON( ItemFields::NameFieldName << "item" ) );

    checkDeviceGroupBroadcast( HUB_ITEM_ADDED, { { DeviceFields::NameFieldName, "item" } }, __LINE__ );
}

TEST_F( CoredTestSuite, addUserCodeItemBroadcast ) {
    Json item = JsonObject::parse( R"(
    {
        "deviceId": "test_device_id",
        "hasGetter": true,
        "hasSetter": true,
        "name": "test_name",
        "show": true,
        "type": "userCode",
        "value": {
            "code": "1984",
            "name": "1"
        }
    })" );
    addItem( item );

    checkDeviceGroupBroadcast( HUB_ITEM_ADDED, { { ItemFields::ValueFieldName, item[ "value" ] } }, __LINE__ );
}

TEST_F( CoredTestSuite, removeGatewayBroadcast ) {
    const auto gatewayId = ensureGateway( JSON( GatewayFields::NameFieldName << "gateway" ) );
    removeGateway( gatewayId );

    checkGroupBroadcast(
        GroupsNames::BROADCAST_GROUP, HUB_GATEWAY_REMOVED, { { GatewayFields::IdFieldName, gatewayId } }, __LINE__ );
}

TEST_F( CoredTestSuite, removeDeviceBroadcast ) {
    const auto deviceId = addDevice( JSON( DeviceFields::NameFieldName << "device" ) );
    removeDevice( deviceId );

    checkDeviceGroupBroadcast( HUB_DEVICE_REMOVED, { { DeviceFields::IdFieldName, deviceId } }, __LINE__ );
}

TEST_F( CoredTestSuite, updateDeviceNameBroadcast ) {
    const auto deviceId = addDevice( JSON( DeviceFields::NameFieldName << "device" ) );
    renameDevice( JSON( DeviceFields::IdFieldName << deviceId << DeviceFields::NameFieldName << "new_name" ) );

    checkDeviceGroupBroadcast( HUB_DEVICE_UPDATED,
                               { { DeviceFields::NameFieldName, "new_name" }, { System::SyncNotification, true } },
                               __LINE__ );
}

TEST_F( CoredTestSuite, updateDeviceFirmwareBroadcast ) {
    auto firmwareVersionInfo1 =
        JSON( DeviceFields::FirmwareIdFieldName << "123" << DeviceFields::FirmwareVersionFieldName << "1.1.0" );
    auto firmwareVersionInfo2 =
        JSON( DeviceFields::FirmwareIdFieldName << "123" << DeviceFields::FirmwareVersionFieldName << "1.2.0" );

    const auto deviceId = addDevice( JSON( DeviceFields::FirmwareFieldName << JSON_ARRAY( firmwareVersionInfo1 ) ) );
    updateDevice( deviceId, JSON( DeviceFields::FirmwareFieldName << JSON_ARRAY( firmwareVersionInfo2 ) ) );

    const auto message = getTestBus().getMessage();
    EXPECT_EQ( message.group, GroupsNames::DEVICE_GROUP );
    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HUB_DEVICE_UPDATED );

    JsonArray actualFirmwareVersion;
    json_get_arr( message.data, DeviceFields::FirmwareFieldName, actualFirmwareVersion );
    EXPECT_EQ( actualFirmwareVersion, JSON_ARRAY( firmwareVersionInfo2 ) );
    EXPECT_EQ( json_get_bool( message.data, System::SyncNotification ), true );
}

TEST_F( CoredTestSuite, updateDeviceTypeBroadcast ) {
    const auto deviceId = addDevice( JSON( DeviceFields::TypeFieldName << "deviceType" ) );
    updateDevice( deviceId, JSON( DeviceFields::TypeFieldName << "newType" ) );

    checkDeviceGroupBroadcast( HUB_DEVICE_UPDATED,
                               { { DeviceFields::TypeFieldName, "newType" }, { System::SyncNotification, false } },
                               __LINE__ );
}

TEST_F( CoredTestSuite, removeItemBroadcast ) {
    const auto itemId = addItem( JSON( ItemFields::NameFieldName << "item" ) );
    removeItem( itemId );

    checkDeviceGroupBroadcast( HUB_ITEM_REMOVED, { { DeviceFields::IdFieldName, itemId } }, __LINE__ );
}

TEST_F( CoredTestSuite, removePluginGateways ) {
    std::string pluginId = "1";

    getDBClient().insert( DBNames::GatewaysDB, JSON( GatewayFields::PluginIdFieldName << pluginId ) );
    EXPECT_EQ( getDBClient().count( DBNames::GatewaysDB, JSON( GatewayFields::PluginIdFieldName << pluginId ) ), 1 );

    removePluginGateways( pluginId );
    EXPECT_EQ( getDBClient().count( DBNames::GatewaysDB, JSON( GatewayFields::PluginIdFieldName << pluginId ) ), 0 );
}

TEST_F( CoredTestSuite, removePluginGatewaysWithDevices ) {
    std::string pluginId = "1";
    std::string gatewayId = "2";

    getDBClient().insert(
        DBNames::GatewaysDB,
        JSON( GatewayFields::IdFieldName << gatewayId << GatewayFields::PluginIdFieldName << pluginId ) );
    getDBClient().insert(
        DBNames::DevicesDB,
        JSON( DeviceFields::GatewayIdFieldName << gatewayId << DeviceFields::ParentDeviceIdFieldName << "" ) );

    removePluginGateways( pluginId );
    EXPECT_EQ( getDBClient().count( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayId ) ), 0 );
    EXPECT_EQ( getDBClient().count( DBNames::DevicesDB, JSON( DeviceFields::GatewayIdFieldName << gatewayId ) ), 0 );
}

TEST_F( CoredTestSuite, removePluginGatewaysWithDevicesAndItems ) {
    std::string pluginId = "1";
    std::string gatewayId = "2";
    std::string deviceId = "3";

    getDBClient().insert(
        DBNames::GatewaysDB,
        JSON( GatewayFields::IdFieldName << gatewayId << GatewayFields::PluginIdFieldName << pluginId ) );
    getDBClient().insert( DBNames::DevicesDB,
                          JSON( DeviceFields::IdFieldName << deviceId << DeviceFields::GatewayIdFieldName << gatewayId
                                                          << DeviceFields::ParentDeviceIdFieldName << "" ) );
    getDBClient().insert( DBNames::ItemsDB, JSON( ItemFields::DeviceIdFieldName << deviceId ) );

    removePluginGateways( pluginId );
    EXPECT_EQ( getDBClient().count( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayId ) ), 0 );
    EXPECT_EQ( getDBClient().count( DBNames::DevicesDB, JSON( DeviceFields::GatewayIdFieldName << gatewayId ) ), 0 );
    EXPECT_EQ( getDBClient().count( DBNames::ItemsDB, JSON( ItemFields::DeviceIdFieldName << deviceId ) ), 0 );
}

TEST_F( CoredTestSuite, removeGateway ) {
    std::string gatewayId = "1";

    getDBClient().insert( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayId ) );
    EXPECT_EQ( getDBClient().count( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayId ) ), 1 );

    removeGateway( gatewayId );
    EXPECT_EQ( getDBClient().count( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayId ) ), 0 );
}

TEST_F( CoredTestSuite, removeGatewayWithDevices ) {
    std::string gatewayId = "1";

    getDBClient().insert( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayId ) );
    getDBClient().insert(
        DBNames::DevicesDB,
        JSON( DeviceFields::GatewayIdFieldName << gatewayId << DeviceFields::ParentDeviceIdFieldName << "" ) );

    removeGateway( gatewayId );
    EXPECT_EQ( getDBClient().count( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayId ) ), 0 );
    EXPECT_EQ( getDBClient().count( DBNames::DevicesDB, JSON( DeviceFields::GatewayIdFieldName << gatewayId ) ), 0 );
}

TEST_F( CoredTestSuite, removeGatewayWithDevicesAndItems ) {
    std::string gatewayId = "2";
    std::string deviceId = "3";

    getDBClient().insert( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayId ) );
    getDBClient().insert( DBNames::DevicesDB,
                          JSON( DeviceFields::IdFieldName << deviceId << DeviceFields::GatewayIdFieldName << gatewayId
                                                          << DeviceFields::ParentDeviceIdFieldName << "" ) );
    getDBClient().insert( DBNames::ItemsDB, JSON( ItemFields::DeviceIdFieldName << deviceId ) );

    removeGateway( gatewayId );
    EXPECT_EQ( getDBClient().count( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayId ) ), 0 );
    EXPECT_EQ( getDBClient().count( DBNames::DevicesDB, JSON( DeviceFields::GatewayIdFieldName << gatewayId ) ), 0 );
    EXPECT_EQ( getDBClient().count( DBNames::ItemsDB, JSON( ItemFields::DeviceIdFieldName << deviceId ) ), 0 );
}

TEST_F( CoredTestSuite, removeGatewayDevices ) {
    std::string gatewayId = "1";

    getDBClient().insert( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayId ) );
    getDBClient().insert(
        DBNames::DevicesDB,
        JSON( DeviceFields::GatewayIdFieldName << gatewayId << DeviceFields::ParentDeviceIdFieldName << "" ) );

    removeGatewayDevices( gatewayId );
    EXPECT_EQ( getDBClient().count( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayId ) ), 1 );
    EXPECT_EQ( getDBClient().count( DBNames::DevicesDB, JSON( DeviceFields::GatewayIdFieldName << gatewayId ) ), 0 );
}

TEST_F( CoredTestSuite, removeGatewayDevicesWithChild ) {
    std::string gatewayId = "1";
    std::string parentDeviceId = "1";
    std::string childDeviceId = "2";

    getDBClient().insert( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayId ) );
    getDBClient().insert(
        DBNames::DevicesDB,
        JSON( DeviceFields::IdFieldName << parentDeviceId << DeviceFields::GatewayIdFieldName << gatewayId
                                        << DeviceFields::ParentDeviceIdFieldName << "" ) );

    getDBClient().insert(
        DBNames::DevicesDB,
        JSON( DeviceFields::IdFieldName << childDeviceId << DeviceFields::GatewayIdFieldName << gatewayId
                                        << DeviceFields::ParentDeviceIdFieldName << parentDeviceId ) );

    removeGatewayDevices( gatewayId );
    EXPECT_EQ( getDBClient().count( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayId ) ), 1 );
    EXPECT_EQ( getDBClient().count( DBNames::DevicesDB, JSON( DeviceFields::GatewayIdFieldName << gatewayId ) ), 0 );
}

TEST_F( CoredTestSuite, removeGatewayDevicesWithItems ) {
    std::string gatewayId = "1";
    std::string deviceId = "2";

    getDBClient().insert( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayId ) );
    getDBClient().insert( DBNames::DevicesDB,
                          JSON( DeviceFields::IdFieldName << deviceId << DeviceFields::GatewayIdFieldName << gatewayId
                                                          << DeviceFields::ParentDeviceIdFieldName << "" ) );
    getDBClient().insert( DBNames::ItemsDB, JSON( ItemFields::DeviceIdFieldName << deviceId ) );

    removeGatewayDevices( gatewayId );
    EXPECT_EQ( getDBClient().count( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayId ) ), 1 );
    EXPECT_EQ( getDBClient().count( DBNames::DevicesDB, JSON( DeviceFields::GatewayIdFieldName << gatewayId ) ), 0 );
    EXPECT_EQ( getDBClient().count( DBNames::ItemsDB, JSON( ItemFields::DeviceIdFieldName << deviceId ) ), 0 );
}

TEST_F( CoredTestSuite, removeDevice ) {
    const auto mainDeviceId = "1";
    getDBClient().insert( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << mainDeviceId ) );

    const auto childDeviceIds = { "2", "3", "3", "4", "5", "6" };
    for( const auto& id : childDeviceIds ) {
        getDBClient().insert(
            DBNames::DevicesDB,
            JSON( DeviceFields::IdFieldName << id << DeviceFields::ParentDeviceIdFieldName << mainDeviceId ) );
    }

    removeDevice( mainDeviceId );

    EXPECT_EQ( getDBClient().count( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << mainDeviceId ) ), 0 );
    for( const auto& id : childDeviceIds ) {
        EXPECT_EQ( getDBClient().count( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << id ) ), 0 );
    }
}

TEST_F( CoredTestSuite, removeDeviceWithItems ) {
    std::string deviceId = "1";

    getDBClient().insert( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << deviceId ) );
    getDBClient().insert( DBNames::ItemsDB, JSON( ItemFields::DeviceIdFieldName << deviceId ) );

    removeDevice( deviceId );
    EXPECT_EQ( getDBClient().count( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << deviceId ) ), 0 );
    EXPECT_EQ( getDBClient().count( DBNames::ItemsDB, JSON( ItemFields::DeviceIdFieldName << deviceId ) ), 0 );
}

TEST_F( CoredTestSuite, removeDeviceItems ) {
    std::string deviceId = "1";

    getDBClient().insert( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << deviceId ) );
    getDBClient().insert( DBNames::ItemsDB, JSON( ItemFields::DeviceIdFieldName << deviceId ) );

    removeDeviceItems( deviceId );
    EXPECT_EQ( getDBClient().count( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << deviceId ) ), 1 );
    EXPECT_EQ( getDBClient().count( DBNames::ItemsDB, JSON( ItemFields::DeviceIdFieldName << deviceId ) ), 0 );
}

TEST_F( CoredTestSuite, removeItem ) {
    std::string itemId = "1";

    getDBClient().insert( DBNames::ItemsDB, JSON( ItemFields::IdFieldName << itemId ) );

    removeItem( itemId );
    EXPECT_EQ( getDBClient().count( DBNames::ItemsDB, JSON( ItemFields::IdFieldName << itemId ) ), 0 );
}

TEST_F( CoredTestSuite, setItemValueSinglecast ) {
    constexpr bool value = true;
    const Json jsonValue = value;
    const std::string commandScript{ "set_item_value_script_name" };

    const auto gatewayId = ensureGateway( JSON( GatewayFields::SetItemValueCommandFieldName << commandScript ) );
    const auto deviceId = addDevice( JSON( DeviceFields::GatewayIdFieldName << gatewayId ) );
    const auto itemId = addItem( JSON( ItemFields::DeviceIdFieldName << deviceId ) );

    auto expectedParams = JSON( "script" << commandScript );
    expectedParams[ "scriptParams" ] = JSON( "item_id" << itemId << "value" << value );

    setItemValue( JSON( "_id" << itemId << "value" << jsonValue ) );

    const auto message = getTestBus().getMessage();
    const auto& data = message.data[ "params" ];
    EXPECT_EQ( data, expectedParams );
}

TEST_F( CoredTestSuite, setItemValueMulticastOneItem ) {
    constexpr bool value = true;
    const Json jsonValue = value;
    const std::string commandScript{ "set_item_value_script_name" };

    const auto gatewayId = ensureGateway( JSON( GatewayFields::SetItemValueCommandFieldName << commandScript ) );
    const auto deviceId = addDevice( JSON( DeviceFields::GatewayIdFieldName << gatewayId ) );
    const auto itemId1 = addItem( JSON( ItemFields::DeviceIdFieldName << deviceId ) );

    auto expectedParams = JSON( "script" << commandScript );
    expectedParams[ "scriptParams" ] = JSON( "item_ids" << JSON_ARRAY_( itemId1 ) << "value" << value );

    setItemValue( JSON( "ids" << JSON_ARRAY_( itemId1 ) << "value" << jsonValue ) );

    const auto message = getTestBus().getMessage();
    const auto& data = message.data[ "params" ];

    EXPECT_EQ( data, expectedParams );
}

TEST_F( CoredTestSuite, setItemValueMulticastSeveralItems ) {
    constexpr bool value = true;
    const Json jsonValue = value;
    const std::string commandScript{ "set_item_value_script_name" };

    const auto gatewayId = ensureGateway( JSON( GatewayFields::SetItemValueCommandFieldName << commandScript ) );
    const auto deviceId = addDevice( JSON( DeviceFields::GatewayIdFieldName << gatewayId ) );
    const auto itemId1 = addItem( JSON( ItemFields::DeviceIdFieldName << deviceId ) );
    const auto itemId2 = addItem( JSON( ItemFields::DeviceIdFieldName << deviceId ) );
    const auto itemId3 = addItem( JSON( ItemFields::DeviceIdFieldName << deviceId ) );

    auto expectedParams = JSON( "script" << commandScript );
    expectedParams[ "scriptParams" ] =
        JSON( "item_ids" << JSON_ARRAY_( itemId1, itemId2, itemId3 ) << "value" << value );

    setItemValue( JSON( "ids" << JSON_ARRAY_( itemId1, itemId2, itemId3 ) << "value" << jsonValue ) );

    const auto message = getTestBus().getMessage();
    const auto& data = message.data[ "params" ];

    EXPECT_EQ( data, expectedParams );
}

Json makeIntItem( const std::string& deviceId,
                  const std::string& name,
                  int value = 0,
                  int minValue = 0,
                  int maxValue = 0 ) {
    Json item;
    item[ ItemFields::DeviceIdFieldName ] = deviceId;
    item[ ItemFields::HasGetterFieldName ] = true;
    item[ ItemFields::HasSetterFieldName ] = true;
    item[ ItemFields::NameFieldName ] = name;
    item[ ItemFields::TypeFieldName ] = "int";
    item[ ItemFields::ValueFieldName ] = value;
    item[ ItemFields::ShowFieldName ] = true;
    item[ ItemFields::EnumFieldName ] = JSON_EMPTY_ARR;
    if( minValue != maxValue ) {
        item[ ItemFields::ValueMinFieldName ] = minValue;
        item[ ItemFields::ValueMaxFieldName ] = maxValue;
    }
    return item;
}

TEST_F( CoredTestSuite, builtinScripts_setItemValueSinglecast ) /* 22.3.1 */ {
    constexpr int value = 10;
    const Json jsonValue = value;
    const std::string commandScript{ CoreD::Model::Resources::GatewayBuiltInScripts::SetItemValueScript };

    const auto gatewayId = ensureGateway( JSON( GatewayFields::SetItemValueCommandFieldName << commandScript ) );
    const auto deviceId = addDevice( JSON( DeviceFields::GatewayIdFieldName << gatewayId ) );
    Json item = makeIntItem( deviceId, "counter", 15, 0, 30 );
    const auto itemId = addItem( item );
    checkDeviceGroupBroadcast( HUB_ITEM_ADDED, {}, __LINE__ );

    auto expectedParams = JSON( "script" << commandScript );
    expectedParams[ "scriptParams" ] = JSON( "item_id" << itemId << "value" << value );

    setItemValue( JSON( "_id" << itemId << "value" << jsonValue ) );
    checkDeviceGroupBroadcast( HUB_ITEM_UPDATED, { { "value", jsonValue } }, __LINE__ );
}

TEST_F( CoredTestSuite, builtinScripts_setItemValueMulticastOneItemOneValue ) /* 22.3.2 */ {
    constexpr int value = 10;
    const Json jsonValue = value;
    const std::string commandScript{ CoreD::Model::Resources::GatewayBuiltInScripts::SetItemValueScript };

    const auto gatewayId = ensureGateway( JSON( GatewayFields::SetItemValueCommandFieldName << commandScript ) );
    const auto deviceId = addDevice( JSON( DeviceFields::GatewayIdFieldName << gatewayId ) );
    Json item = makeIntItem( deviceId, "counter", 15, 0, 30 );
    const auto itemId1 = addItem( item );
    checkDeviceGroupBroadcast( HUB_ITEM_ADDED, {}, __LINE__ );

    auto expectedParams = JSON( "script" << commandScript );
    expectedParams[ "scriptParams" ] = JSON( "item_ids" << JSON_ARRAY_( itemId1 ) << "value" << value );

    Json callParams = JSON( "ids" << JSON_ARRAY_( itemId1 ) << "value" << jsonValue );
    setItemValue( callParams );
    checkDeviceGroupBroadcast( HUB_ITEM_UPDATED, { { "value", jsonValue } }, __LINE__ );
}

TEST_F( CoredTestSuite, builtinScripts_setItemValueMulticastSeveralItemsOwnValues ) /* 22.3.3 */ {
    constexpr bool value = true;
    const Json jsonValue = value;
    const std::string commandScript{ CoreD::Model::Resources::GatewayBuiltInScripts::SetItemValueScript };

    const auto gatewayId = ensureGateway( JSON( GatewayFields::SetItemValueCommandFieldName << commandScript ) );
    const auto deviceId = addDevice( JSON( DeviceFields::GatewayIdFieldName << gatewayId ) );

    Json item1 = makeIntItem( deviceId, "counter1", 15, 0, 30 );
    const auto itemId1 = addItem( item1 );
    checkDeviceGroupBroadcast( HUB_ITEM_ADDED, {}, __LINE__ );
    Json item2 = makeIntItem( deviceId, "counter2", 15, 0, 30 );
    const auto itemId2 = addItem( item2 );
    checkDeviceGroupBroadcast( HUB_ITEM_ADDED, {}, __LINE__ );
    Json item3 = makeIntItem( deviceId, "counter3", 15, 0, 30 );
    const auto itemId3 = addItem( item3 );
    checkDeviceGroupBroadcast( HUB_ITEM_ADDED, {}, __LINE__ );

    auto setItemValueParams =
        JSON( "items" << JSON( itemId1 << JSON( "value" << 10 ) << itemId2 << JSON( "value" << 20 ) << itemId3
                                       << JSON( "value" << 30 ) ) );

    setItemValue( setItemValueParams );
    // can't check for values here because of undefined order between item identifiers used
    Json itemObject = m_cored.getItem( itemId1 );
    EXPECT_EQ( json_get_int( itemObject, "value", -1 ), 10 );
    itemObject = m_cored.getItem( itemId2 );
    EXPECT_EQ( json_get_int( itemObject, "value", -1 ), 20 );
    itemObject = m_cored.getItem( itemId3 );
    EXPECT_EQ( json_get_int( itemObject, "value", -1 ), 30 );

    // Fetch broadcasts in reverse order from the back of the queue: updated, extended updated
    checkDeviceGroupBroadcast( HUB_ITEM_UPDATED, {}, __LINE__ );
    checkInternalBroadcast( EXTENDED_HUB_ITEM_UPDATED, {}, __LINE__ );
    checkDeviceGroupBroadcast( HUB_ITEM_UPDATED, {}, __LINE__ );
    checkInternalBroadcast( EXTENDED_HUB_ITEM_UPDATED, {}, __LINE__ );
    checkDeviceGroupBroadcast( HUB_ITEM_UPDATED, {}, __LINE__ );
    checkInternalBroadcast( EXTENDED_HUB_ITEM_UPDATED, {}, __LINE__ );
}

TEST_F( CoredTestSuite, updateItemValue ) {
    const auto deviceId = addDevice( JSON( DeviceFields::NameFieldName << "unique_id" ) );
    const auto itemId =
        addItem( JSON( ItemFields::DeviceIdFieldName << deviceId << ItemFields::ValueFieldName << true ) );

    constexpr bool newValue{ false };
    constexpr bool newValueMin{ false };
    constexpr bool newValueMax{ false };

    updateItemValue( JSON( RPCParams::ItemId << itemId << RPCParams::ItemValue << newValue << RPCParams::ItemValueMin
                                             << newValueMin << RPCParams::ItemValueMax << newValueMax ) );

    JsonObject item = getDBClient().findOne( DBNames::TempItemsDB, JSON( ItemFields::IdFieldName << itemId ), nullptr );
    if( item == JSON_EMPTY_OBJ ) {
        item = getDBClient().findOne( DBNames::ItemsDB, JSON( ItemFields::IdFieldName << itemId ), nullptr );
        bool result{ true };
        EXPECT_TRUE( json_get( item, ItemFields::ValueFieldName, result ) );
    }
    else {
        bool result{ false };
        EXPECT_TRUE( json_get( item, ItemFields::ValueFieldName, result ) );
        EXPECT_EQ( result, newValue );
    }
    EXPECT_EQ( json_get_str( item, ItemFields::IdFieldName ), itemId );

    item = getDBClient().findOne( DBNames::ItemsDB, JSON( ItemFields::IdFieldName << itemId ), nullptr );
    if( item == JSON_EMPTY_OBJ ) {
        item = getDBClient().findOne( DBNames::TempItemsDB, JSON( ItemFields::IdFieldName << itemId ), nullptr );
        bool result{ true };
        EXPECT_TRUE( json_get( item, ItemFields::ValueMinFieldName, result ) );
    }
    else {
        bool result{ false };
        EXPECT_TRUE( json_get( item, ItemFields::ValueMinFieldName, result ) );
        EXPECT_EQ( result, newValueMin );
    }
    EXPECT_EQ( json_get_str( item, ItemFields::IdFieldName ), itemId );

    item = getDBClient().findOne( DBNames::ItemsDB, JSON( ItemFields::IdFieldName << itemId ), nullptr );
    if( item == JSON_EMPTY_OBJ ) {
        item = getDBClient().findOne( DBNames::TempItemsDB, JSON( ItemFields::IdFieldName << itemId ), nullptr );
        bool result{ true };
        EXPECT_TRUE( json_get( item, ItemFields::ValueMaxFieldName, result ) );
    }
    else {
        bool result{ false };
        EXPECT_TRUE( json_get( item, ItemFields::ValueMaxFieldName, result ) );
        EXPECT_EQ( result, newValueMax );
    }
    // EXPECT_EQ( json_get_str( item, ItemFields::DeviceIdFieldName ), deviceId );
    EXPECT_EQ( json_get_str( item, ItemFields::IdFieldName ), itemId );
}

TEST_F( CoredTestSuite, updateItemUserCodeValue ) {
    const auto deviceId = addDevice( JSON( DeviceFields::NameFieldName << "unique_id" ) );
    Json item = JsonObject::parse( R"(
    {
        "deviceId": "test_device_id",
        "hasGetter": true,
        "hasSetter": true,
        "name": "test_name",
        "show": true,
        "type": "userCode",
        "value": {
            "code": "1984",
            "name": "1"
        }
    })" );

    const auto itemId = addItem( item );

    const auto newValue = JsonObject::parse( R"(
    {
        "code": "0000",
        "name": "1"
    })" );

    updateItemValue(
        JSON( RPCParams::ItemId << itemId << RPCParams::ItemType << "userCode" << RPCParams::ItemValue << newValue ) );

    JsonObject tempUpdatedItem =
        getDBClient().findOne( DBNames::TempItemsDB, JSON( ItemFields::IdFieldName << itemId ), nullptr );
    EXPECT_EQ( tempUpdatedItem[ "value" ][ "code" ].get< std::string >().size(), 44 );

    JsonObject updatedItem =
        getDBClient().findOne( DBNames::ItemsDB, JSON( ItemFields::IdFieldName << itemId ), nullptr );
    EXPECT_EQ( updatedItem[ "value" ][ "code" ].get< std::string >().size(), 44 );
    EXPECT_EQ( item[ "value" ].dump( 4 ), decryptItem( updatedItem )[ "value" ].dump( 4 ) );

    checkDeviceGroupBroadcast( HUB_ITEM_UPDATED, { { "value", newValue } }, __LINE__ );
}

TEST_F( CoredTestSuite, addItemDictionaryValue ) {
    constexpr bool value = true;
    const std::string valueType{ "bool" };
    const std::string commandScript{ "add_item_dictionary_value_script_name" };

    const auto gatewayId =
        ensureGateway( JSON( GatewayFields::AddItemDictionaryValueCommandFieldName << commandScript ) );
    const auto deviceId = addDevice( JSON( DeviceFields::GatewayIdFieldName << gatewayId ) );
    const auto itemId = addItem( JSON( ItemFields::DeviceIdFieldName << deviceId ) );

    auto expectedParams = JSON( "script" << commandScript );
    expectedParams[ "scriptParams" ] = JSON( "item_id" << itemId << "value_type" << valueType << "value" << value );

    auto args = JSON( "_id" << itemId );
    args[ DictionaryEditRequestFields::ElementFieldName ] =
        JSON( DictionaryEditRequestFields::ValueTypeFieldName << valueType
                                                              << DictionaryEditRequestFields::ValueFieldName << value );
    addItemDictionaryValue( args );

    const auto message = getTestBus().getMessage();
    const auto& data = message.data[ "params" ];

    EXPECT_EQ( data, expectedParams );
}

TEST_F( CoredTestSuite, setItemDictionaryValue ) {
    constexpr bool value = true;
    const Json jsonValue = value;
    const std::string key{ "some_dict_key" };
    const std::string valueType{ "bool" };
    const std::string commandScript{ "set_item_dictionary_value_script_name" };

    const auto gatewayId =
        ensureGateway( JSON( GatewayFields::SetItemDictionaryValueCommandFieldName << commandScript ) );
    const auto deviceId = addDevice( JSON( DeviceFields::GatewayIdFieldName << gatewayId ) );
    const auto itemId = addItem( JSON( ItemFields::DeviceIdFieldName << deviceId ) );

    auto expectedParams = JSON( "script" << commandScript );
    expectedParams[ "scriptParams" ] =
        JSON( "item_id" << itemId << "key" << key << "value_type" << valueType << "value" << value );

    auto args = JSON( "_id" << itemId << "key" << key );
    args[ DictionaryEditRequestFields::ElementFieldName ] =
        JSON( DictionaryEditRequestFields::ValueTypeFieldName << valueType
                                                              << DictionaryEditRequestFields::ValueFieldName << value );
    setItemDictionaryValue( args );
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getCoreDService() );
    auto item = coreDInterface.getItem( itemId );

    const auto message = getTestBus().getMessage();
    const auto& data = message.data[ "params" ];

    EXPECT_EQ( data, expectedParams );
}

TEST_F( CoredTestSuite, removeItemDictionaryValue ) {
    const std::string key{ "some_dict_key" };
    const std::string commandScript{ "remove_item_dictionary_value_script_name" };

    const auto gatewayId =
        ensureGateway( JSON( GatewayFields::RemoveItemDictionaryValueCommandFieldName << commandScript ) );
    const auto deviceId = addDevice( JSON( DeviceFields::GatewayIdFieldName << gatewayId ) );
    const auto itemId = addItem( JSON( ItemFields::DeviceIdFieldName << deviceId ) );

    auto expectedParams = JSON( "script" << commandScript );
    expectedParams[ "scriptParams" ] = JSON( "item_id" << itemId << "key" << key );

    removeItemDictionaryValue( JSON( "_id" << itemId << DictionaryEditRequestFields::KeyFieldName << key ) );

    const auto message = getTestBus().getMessage();
    const auto& data = message.data[ "params" ];

    EXPECT_EQ( data, expectedParams );
}

TEST_F( CoredTestSuite, clearItemDictionary ) {
    const std::string commandScript{ "clear_item_dictionary_script_name" };

    const auto gatewayId = ensureGateway( JSON( GatewayFields::ClearItemDictionaryCommandFieldName << commandScript ) );
    const auto deviceId = addDevice( JSON( DeviceFields::GatewayIdFieldName << gatewayId ) );
    const auto itemId = addItem( JSON( ItemFields::DeviceIdFieldName << deviceId ) );

    auto expectedParams = JSON( "script" << commandScript );
    expectedParams[ "scriptParams" ] = JSON( "item_id" << itemId );

    clearItemDictionary( JSON( "_id" << itemId ) );

    const auto message = getTestBus().getMessage();
    const auto& data = message.data[ "params" ];

    EXPECT_EQ( data, expectedParams );
}

TEST_F( CoredTestSuite, removeDeviceForce ) {
    const std::string commandScript{ "force_remove" };

    const auto gatewayId = ensureGateway( JSON( GatewayFields::ForceRemoveDeviceCommandFieldName << commandScript ) );
    const auto device1Id =
        addDevice( JSON( DeviceFields::GatewayIdFieldName << gatewayId << DeviceFields::ReachableFieldName << true ) );
    const auto device2Id =
        addDevice( JSON( DeviceFields::GatewayIdFieldName << gatewayId << DeviceFields::ReachableFieldName << false ) );
    {
        auto expectedParams = JSON( "script" << commandScript );
        expectedParams[ "scriptParams" ] = JSON( "deviceId" << device1Id );
        removeDeviceForce( JSON( RPCParams::DeviceId << device1Id ) );
        const auto message = getTestBus().getMessage();
        const auto& data = message.data[ "params" ];
        EXPECT_EQ( data, expectedParams );
    }

    {
        auto expectedParams = JSON( "script" << commandScript );
        expectedParams[ "scriptParams" ] = JSON( "deviceId" << device2Id );
        removeDeviceForce( JSON( RPCParams::DeviceId << device2Id ) );
        const auto message = getTestBus().getMessage();
        const auto& data = message.data[ "params" ];
        EXPECT_EQ( data, expectedParams );
    }
}

TEST_F( CoredTestSuite, updateReachableState ) {
    EXPECT_THROW( updateReachableState( JSON( RPCParams::ReachableState << true ) ), CoreD::EmptyParameter );
    EXPECT_THROW( updateReachableState( JSON( RPCParams::DeviceId << "deviceId" ) ), CoreD::EmptyParameter );

    constexpr bool defaultReachableState{ true };

    const auto deviceId = addDevice( JSON( DeviceFields::ReachableFieldName << defaultReachableState ) );

    constexpr bool newReachableState{ !defaultReachableState };
    updateReachableState( JSON( RPCParams::DeviceId << deviceId << RPCParams::ReachableState << newReachableState ) );

    JsonObject device =
        getDBClient().findOne( DBNames::TempDevicesDB, JSON( DeviceFields::IdFieldName << deviceId ), nullptr );
    EXPECT_FALSE( device.empty() );
    EXPECT_EQ( json_get_bool( device, DeviceFields::ReachableFieldName, true ), newReachableState );
    EXPECT_FALSE( device.empty() );
    device = getDBClient().findOne( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << deviceId ), nullptr );
    EXPECT_EQ( json_get_bool( device, DeviceFields::ReachableFieldName, false ), defaultReachableState );
}

TEST_F( CoredTestSuite, updateReadyState ) {
    EXPECT_THROW( updateReadyState( JSON( RPCParams::ReadyState << true ) ), CoreD::EmptyParameter );
    EXPECT_THROW( updateReadyState( JSON( RPCParams::DeviceId << "deviceId" ) ), CoreD::EmptyParameter );

    constexpr bool defaultReadyState{ true };

    const auto deviceId = addDevice( JSON( DeviceFields::ReadyFieldName << defaultReadyState ) );

    constexpr bool newReadyState{ !defaultReadyState };
    EXPECT_NO_THROW(
        updateReadyState( JSON( RPCParams::DeviceId << deviceId << RPCParams::ReadyState << newReadyState ) ) );

    JsonObject device =
        getDBClient().findOne( DBNames::TempDevicesDB, JSON( DeviceFields::IdFieldName << deviceId ), nullptr );
    EXPECT_FALSE( device.empty() );
    EXPECT_EQ( json_get_bool( device, DeviceFields::ReadyFieldName, true ), newReadyState );
    device = getDBClient().findOne( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << deviceId ), nullptr );
    EXPECT_FALSE( device.empty() );
    EXPECT_EQ( json_get_bool( device, DeviceFields::ReadyFieldName, false ), defaultReadyState );
}

TEST_F( CoredTestSuite, updateDeviceName ) {
    EXPECT_THROW( updateDeviceName( JSON( RPCParams::DeviceName << "name" ) ), CoreD::EmptyParameter );
    EXPECT_THROW( updateDeviceName( JSON( RPCParams::DeviceId << "deviceId" ) ), CoreD::EmptyParameter );

    const std::string oldName{ "OldName" };

    const std::string deviceId = addDevice( JSON( DeviceFields::NameFieldName << oldName ) );

    const std::string newName{ "NewName" };
    updateDeviceName( JSON( RPCParams::DeviceId << deviceId << RPCParams::DeviceName << newName ) );

    JsonObject device =
        getDBClient().findOne( DBNames::TempDevicesDB, JSON( DeviceFields::IdFieldName << deviceId ), nullptr );
    EXPECT_TRUE( device.empty() );
    device = getDBClient().findOne( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << deviceId ), nullptr );
    EXPECT_FALSE( device.empty() );
    const std::string result = json_get_str( device, DeviceFields::NameFieldName );
    EXPECT_EQ( result, newName );
}

TEST_F( CoredTestSuite, updateDeviceRoomId ) {
    EXPECT_THROW( updateDeviceRoomId( JSON( RPCParams::DeviceRoomId << "RoomId" ) ), CoreD::EmptyParameter );
    EXPECT_THROW( updateDeviceRoomId( JSON( RPCParams::DeviceId << "deviceId" ) ), CoreD::EmptyParameter );

    const std::string oldRoomId{ "10" };

    const std::string deviceId = addDevice( JSON( DeviceFields::RoomIdFieldName << oldRoomId ) );

    const std::string newRoomId{ "20" };
    updateDeviceRoomId( JSON( RPCParams::DeviceId << deviceId << RPCParams::DeviceRoomId << newRoomId ) );

    JsonObject device =
        getDBClient().findOne( DBNames::TempDevicesDB, JSON( DeviceFields::IdFieldName << deviceId ), nullptr );
    EXPECT_TRUE( device.empty() );
    device = getDBClient().findOne( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << deviceId ), nullptr );
    EXPECT_FALSE( device.empty() );
    const std::string result = json_get_str( device, DeviceFields::RoomIdFieldName );
    EXPECT_EQ( result, newRoomId );
}

TEST_F( CoredTestSuite, updateDeviceStatus ) {
    EXPECT_THROW( updateDeviceStatus( JSON( RPCParams::DeviceStatus << "devStatus" ) ), CoreD::EmptyParameter );
    EXPECT_THROW( updateDeviceStatus( JSON( RPCParams::DeviceId << "deviceId" ) ), CoreD::EmptyParameter );

    const std::string oldStatus{ "broken" };

    const std::string deviceId = addDevice( JSON( DeviceFields::StatusFieldName << oldStatus ) );

    const std::string newStatus{ "idle" };
    updateDeviceStatus( JSON( RPCParams::DeviceId << deviceId << RPCParams::DeviceStatus << newStatus ) );

    JsonObject device =
        getDBClient().findOne( DBNames::TempDevicesDB, JSON( DeviceFields::IdFieldName << deviceId ), nullptr );
    EXPECT_TRUE( device.empty() );
    device = getDBClient().findOne( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << deviceId ), nullptr );
    EXPECT_FALSE( device.empty() );
    const std::string result = json_get_str( device, DeviceFields::StatusFieldName );
    EXPECT_EQ( result, newStatus );
}

TEST_F( CoredTestSuite, sendExecutePluginScriptWithParameters ) {
    constexpr auto scriptName = "script_name";
    const auto scriptParameters = JSON( "key"
                                        << "value" );

    auto expectedData = prepareMethod( "executeScript" );
    expectedData[ "params" ][ "script" ] = scriptName;
    expectedData[ "params" ][ "scriptParams" ] = scriptParameters;

    sendExecutePluginScript( scriptName, scriptParameters );
    auto message = getTestBus().getMessage();

    EXPECT_EQ( message.group, "addon.lua" );
    EXPECT_EQ( message.msgClass, JSON_RPC );
    EXPECT_EQ( message.msgSubClass, CALL_METHOD );

    expectedData.erase( "id" );
    message.data.erase( "id" );
    EXPECT_EQ( message.data, expectedData );
}

TEST_F( CoredTestSuite, handleSetServiceNotifications ) {
    const std::size_t deviceNumber = 11;
    std::vector< CoreD::DeviceManager::DeviceID > notifiedDevice;
    std::vector< CoreD::DeviceManager::DeviceID > nonNotifiedDevice;
    for( std::size_t i = 0; i < deviceNumber; ++i ) {
        auto deviceId = addDevice( JSON( DeviceFields::ServiceNotificationFieldName << false ) );
        if( i % 2 ) {
            notifiedDevice.emplace_back( std::move( deviceId ) );
        }
        else {
            nonNotifiedDevice.emplace_back( std::move( deviceId ) );
        }
    }
    auto checkServiceNotificationFlag =
        [this]( bool flag, std::vector< CoreD::DeviceManager::DeviceID > deviceIds, int codeLine ) {
            std::string codeLineStr = "code line: " + std::to_string( codeLine );
            const auto fieldsToReturn = JSON( DeviceFields::IdFieldName << 1 );
            auto dbDeviceIds = getDBClient().findAll(
                DBNames::DevicesDB, JSON( DeviceFields::ServiceNotificationFieldName << flag ), &fieldsToReturn );
            EXPECT_EQ( dbDeviceIds.size(), deviceIds.size() ) << codeLineStr;
            for( std::size_t i = 0; i < dbDeviceIds.size(); ++i ) {
                EXPECT_EQ( dbDeviceIds[ i ][ DeviceFields::IdFieldName ].dump(), deviceIds[ i ].dump() ) << codeLineStr;
            }
        };

    EXPECT_TRUE( handleSetServiceNotifications( JSON( "ids" << notifiedDevice ) ).empty() );
    checkServiceNotificationFlag( true, notifiedDevice, __LINE__ );
    checkServiceNotificationFlag( false, nonNotifiedDevice, __LINE__ );

    notifiedDevice.emplace_back( "newID" );
    EXPECT_FALSE( handleSetServiceNotifications( JSON( "ids" << notifiedDevice ) ).empty() );
    notifiedDevice.erase( notifiedDevice.end() - 1 );
    checkServiceNotificationFlag( true, notifiedDevice, __LINE__ );
    checkServiceNotificationFlag( false, nonNotifiedDevice, __LINE__ );

    notifiedDevice.emplace_back( std::move( nonNotifiedDevice.back() ) );
    nonNotifiedDevice.pop_back();
    EXPECT_TRUE( handleSetServiceNotifications( JSON( "ids" << notifiedDevice ) ).empty() );
    checkServiceNotificationFlag( true, notifiedDevice, __LINE__ );
    checkServiceNotificationFlag( false, nonNotifiedDevice, __LINE__ );
}

TEST_F( CoredTestSuite, handleUpdateDeviceFirmware ) {
    const auto gatewayId1 =
        ensureGateway( JSON( GatewayFields::NameFieldName
                             << "gateway1" << GatewayFields::UpdateDeviceFirmwareCommandFieldName << "script1" ) );
    const auto gatewayId2 =
        ensureGateway( JSON( GatewayFields::NameFieldName
                             << "gateway2" << GatewayFields::UpdateDeviceFirmwareCommandFieldName << "script2" ) );

    std::vector< std::string > devices1;
    devices1.push_back( addDevice( JSON( DeviceFields::GatewayIdFieldName << gatewayId1 ) ) );
    devices1.push_back( addDevice( JSON( DeviceFields::GatewayIdFieldName << gatewayId1 ) ) );

    std::vector< std::string > devices2;
    devices2.push_back( addDevice( JSON( DeviceFields::GatewayIdFieldName << gatewayId2 ) ) );
    devices2.push_back( addDevice( JSON( DeviceFields::GatewayIdFieldName << gatewayId2 ) ) );

    auto update1 = JSON( "devices" << devices1 << "firmware"
                                   << "firmware1" );
    auto update2 = JSON( "devices" << devices2 << "firmware"
                                   << "firmware2" );

    std::vector< std::string > tmpDevices;
    tmpDevices.insert( tmpDevices.begin(), devices1.begin(), devices1.end() );
    tmpDevices.insert( tmpDevices.end(), devices2.begin(), devices2.end() );
    auto update3 = JSON( "devices" << tmpDevices << "firmware"
                                   << "firmware3" );

    tmpDevices.clear();
    tmpDevices.push_back( devices2[ 0 ] );
    tmpDevices.push_back( devices1[ 0 ] );
    tmpDevices.push_back( devices2[ 1 ] );
    tmpDevices.push_back( devices1[ 1 ] );
    auto update4 = JSON( "devices" << tmpDevices << "firmware"
                                   << "firmware4" );

    auto updates = JSON( "updates" << JSON_ARRAY_( update1, update2, update3, update4 ) );
    auto answer = handleUpdateDeviceFirmware( updates );
    ASSERT_EQ( answer.count( "error" ), 0 );

    auto checkMessage = [this]( const Json& expectedScript, const Json& expectedParams, int codeLine ) {
        std::string codeLineStr = "code line: " + std::to_string( codeLine );
        auto expectedData = prepareMethod( "executeScript" );
        expectedData[ "params" ][ "script" ] = expectedScript;
        expectedData[ "params" ][ "scriptParams" ] = expectedParams;

        Services::Message message;
        do {
            message = getTestBus().getFirstMessage();
            ASSERT_FALSE( message.group.empty() ) << codeLineStr;
        } while( message.group != "addon.lua" );

        EXPECT_EQ( message.msgClass, JSON_RPC ) << codeLineStr;
        EXPECT_EQ( message.msgSubClass, CALL_METHOD ) << codeLineStr;

        expectedData.erase( "id" );
        message.data.erase( "id" );
        EXPECT_EQ( message.data, expectedData ) << codeLineStr;
    };

    checkMessage( "script1",
                  JSON( "devices" << devices1 << "firmware"
                                  << "firmware1" ),
                  __LINE__ );
    checkMessage( "script2",
                  JSON( "devices" << devices2 << "firmware"
                                  << "firmware2" ),
                  __LINE__ );
    if( gatewayId1 < gatewayId2 ) {
        checkMessage( "script1",
                      JSON( "devices" << devices1 << "firmware"
                                      << "firmware3" ),
                      __LINE__ );
        checkMessage( "script2",
                      JSON( "devices" << devices2 << "firmware"
                                      << "firmware3" ),
                      __LINE__ );
        checkMessage( "script1",
                      JSON( "devices" << devices1 << "firmware"
                                      << "firmware4" ),
                      __LINE__ );
        checkMessage( "script2",
                      JSON( "devices" << devices2 << "firmware"
                                      << "firmware4" ),
                      __LINE__ );
    }
    else {
        checkMessage( "script2",
                      JSON( "devices" << devices2 << "firmware"
                                      << "firmware3" ),
                      __LINE__ );
        checkMessage( "script1",
                      JSON( "devices" << devices1 << "firmware"
                                      << "firmware3" ),
                      __LINE__ );
        checkMessage( "script2",
                      JSON( "devices" << devices2 << "firmware"
                                      << "firmware4" ),
                      __LINE__ );
        checkMessage( "script1",
                      JSON( "devices" << devices1 << "firmware"
                                      << "firmware4" ),
                      __LINE__ );
    }
}

TEST_F( CoredTestSuite, handleUpdateDeviceFirmwareErrors ) {
    auto update1 = JSON( "devices" << JSON_ARRAY_( "device1", "device2" ) << "firmware"
                                   << "firmware1" );
    auto update2 = JSON( "devices" << JSON_ARRAY_( "device3", "device4" ) << "firmware"
                                   << "firmware2" );

    auto updates = JSON( "updates" << JSON_ARRAY_( update1, update2 ) );
    auto answer = handleUpdateDeviceFirmware( updates );
    auto error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "message" ), "Devices not found!" );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.devices.notfound" );
    EXPECT_EQ( json_get_str( error, "reason" ), "device ids: device1, device2, device3, device4" );

    std::vector< std::string > devices1;
    devices1.push_back( addDevice( JSON( DeviceFields::GatewayIdFieldName << "gatewayId1" ) ) );
    devices1.push_back( addDevice( JSON( DeviceFields::GatewayIdFieldName << "gatewayId1" ) ) );

    updates[ "updates" ][ 0 ][ "devices" ] = devices1;
    answer = handleUpdateDeviceFirmware( updates );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "message" ), "No gateway found!" );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.gateway.notfound" );
    EXPECT_EQ( json_get_str( error, "reason" ), "gateway id: gatewayId1" );

    const auto gatewayId1 =
        ensureGateway( JSON( GatewayFields::NameFieldName
                             << "gateway1" << GatewayFields::UpdateDeviceFirmwareCommandFieldName << "script1" ) );

    devices1.clear();
    devices1.push_back( addDevice( JSON( DeviceFields::GatewayIdFieldName << gatewayId1 ) ) );
    devices1.push_back( addDevice( JSON( DeviceFields::GatewayIdFieldName << gatewayId1 ) ) );

    updates[ "updates" ][ 0 ][ "devices" ] = devices1;
    answer = handleUpdateDeviceFirmware( updates );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "message" ), "Devices not found!" );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.devices.notfound" );
    EXPECT_EQ( json_get_str( error, "reason" ), "device ids: device3, device4" );

    devices1.clear();
    devices1.push_back( addDevice( JSON( DeviceFields::GatewayIdFieldName << "gatewayId2" ) ) );
    devices1.push_back( addDevice( JSON( DeviceFields::GatewayIdFieldName << "gatewayId2" ) ) );

    updates[ "updates" ][ 1 ][ "devices" ] = devices1;
    answer = handleUpdateDeviceFirmware( updates );
    error = answer[ "error" ];
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "message" ), "No gateway found!" );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.gateway.notfound" );
    EXPECT_EQ( json_get_str( error, "reason" ), "gateway id: gatewayId2" );

    const auto gatewayId2 =
        ensureGateway( JSON( GatewayFields::NameFieldName
                             << "gateway1" << GatewayFields::UpdateDeviceFirmwareCommandFieldName << "script1" ) );
    devices1.clear();
    devices1.push_back( addDevice( JSON( DeviceFields::GatewayIdFieldName << gatewayId2 ) ) );
    devices1.push_back( addDevice( JSON( DeviceFields::GatewayIdFieldName << gatewayId2 ) ) );

    updates[ "updates" ][ 1 ][ "devices" ] = devices1;
    answer = handleUpdateDeviceFirmware( updates );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() );
}

TEST_F( CoredTestSuite, addItemWithInfoBroadcast ) {
    Json item = JsonObject::parse( R"(
    {
        "deviceId": "test_device_id",
        "hasGetter": true,
        "hasSetter": true,
        "name": "test_name",
        "show": true,
        "value": 0,
        "info": {
            "zwave.references": [
                {
                    "channel": 0,
                    "class": 67,
                    "managing_commands": [ "THERMOSTAT_SETPOINT_SET" ]
                }
            ]
        }
    })" );
    addItem( item );
    checkDeviceGroupBroadcast( HUB_ITEM_ADDED, { { "info", item[ "info" ] } }, __LINE__ );
}

TEST_F( CoredTestSuite, testCompareFirmwareVersion ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getCoreDService() );
    {
        std::string targetVersion = "2.0.53.0";
        std::string sourceVersion = "2.0.53.0";
        EXPECT_FALSE( coreDInterface.isFirmwareVersionOlder( targetVersion, sourceVersion ) );
    }

    {
        std::string targetVersion = "2.0.54.0";
        std::string sourceVersion = "2.0.53.0";
        EXPECT_TRUE( coreDInterface.isFirmwareVersionOlder( targetVersion, sourceVersion ) );
    }

    {
        std::string targetVersion = "2.0.53.2345";
        std::string sourceVersion = "2.0.53.2344";
        EXPECT_TRUE( coreDInterface.isFirmwareVersionOlder( targetVersion, sourceVersion ) );
    }

    {
        std::string targetVersion = "2.0.53.aaeeccdd55";
        std::string sourceVersion = "2.0.53.aaeeccdc55";
        EXPECT_TRUE( coreDInterface.isFirmwareVersionOlder( targetVersion, sourceVersion ) );
    }

    {
        std::string targetVersion = "2.0.53.EFWR-777@wrt_build.2";
        std::string sourceVersion = "2.0.53.EFWR-777@wrt_build.1";
        EXPECT_TRUE( coreDInterface.isFirmwareVersionOlder( targetVersion, sourceVersion ) );
    }

    {
        std::string targetVersion = "2.0.53.EFWR-777@wrt_build.2";
        std::string sourceVersion = "2.0.53.EFWR-777@wrt_build.3";
        EXPECT_FALSE( coreDInterface.isFirmwareVersionOlder( targetVersion, sourceVersion ) );
    }

    {
        std::string targetVersion = "2.0.53.2345";
        std::string sourceVersion = "2.0.53.0";
        EXPECT_FALSE( coreDInterface.isFirmwareVersionOlder( targetVersion, sourceVersion ) );
    }

    {
        std::string targetVersion = "2.0.53.0";
        std::string sourceVersion = "2.0.53.2345";
        EXPECT_TRUE( coreDInterface.isFirmwareVersionOlder( targetVersion, sourceVersion ) );
    }
}

TEST_F( CoredTestSuite, testUpdateFirmwareModificationTime ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getCoreDService() );

    IDbClient& dbClient = getDBClient();

    auto currentTimestamp = std::chrono::system_clock::now().time_since_epoch().count();
    dbClient.insert( HaUpdateManagerPendingUpdate,
                     JSON( db::ID_FIELD_NAME << UpdateManager::Resources::PendingUpdateFields::PendingUpdate << "value"
                                             << currentTimestamp ),
                     false );

    // CoreD::CoreService coreService;
    // bool result = coreService.initLogOptionsFromFirmwareConfig();
    // EXPECT_FALSE( result );
}

}  // namespace UT
}  // namespace HubPlatform
