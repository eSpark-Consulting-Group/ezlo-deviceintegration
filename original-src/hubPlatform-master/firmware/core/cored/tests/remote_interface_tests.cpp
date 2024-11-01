
#include "cored/core_service.h"
#include "cored/room_manager.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/values/values.h"
#include "cored/remote_interface/cored_remote_interface.h"

#include "cored/tests/service/test_core_service.h"
#include "cored/tests/tests_resources.h"

#include "database_client/dbqueryparser.h"
#include "database_client/db_tables_names.h"

#include "service_interfaces/tests/fixtures/fake_service_fixture.h"

#include "jsonrpc/message.h"
#include "common_utils/cpp14.h"
#include "base_utils/utils.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>

/*

TESTPLAN

        1. Add gateway
DONE    2. Gateway to json
        3. Get gateway by plugin id
DONE    4. Get device wizard categories
DONE    5. Process cmd
DONE    6. Get item
DONE    7. Get items by device id
DONE    8. Default geo position
DONE    9. Geo position
DONE    10. Geo position is not default when modified
DONE    11. Encrypted items
DONE        11.1 Get user code item
DONE        11.2 Get user code items
DONE        11.3 Get user code items by
DONE        11.4 Get user code items by device id
DONE    12 Update item dictionary valueWhen elements max number specified to non-zero
DONE    13 Update item dictionary valueWhen elements max number specified to zero
DONE    14 Is inside week interval
DONE    15 Get next week event
DONE    16 Get time with delay
*/

namespace HubPlatform {
namespace UT {

using namespace CoreD::Model;

const static std::string fileDeviceAddingWizard = "/tmp/DeviceAddingWizard.json";

class CoredRemoteInterfaceTestSuite : public Services::UT::FakeServiceTestFixture {
public:
    CoredRemoteInterfaceTestSuite()
    : m_localTime( "2021-06-22T18:27:15+0300" )
    , m_coredRemoteInterface( getHostService(),
                              [this]() { return GregorianCalendar::Date( m_localTime ); },
                              m_provisionData ) {
        m_provisionData.setControllerKey( "\x38\xD3\xEB\x18\x59\x8D\x44\x4E\xEE\x41\x0B\x3E\x70\xC9\xFA\xB7\xA2\xF8\x3C"
                                          "\x90\x4E\x2C\x16\xFA\xC1\xF1\xE5\xD4\x86\xEF\x8B" );
    }

    static JsonObject convertGatewayToJson( const Gateway& gateway ) {
        auto result = gateway.toJson();
        Gateway::checkGatewayAsJson( result );
        return result;
    }

    void SetUp() override {
        Services::UT::FakeServiceTestFixture::SetUp();
    }

    JsonObject createItem( const std::string& itemId,
                           ValueType valueType,
                           const std::string& dev_id = "dev_id",
                           const std::string& item_type = "item_type",
                           bool hasGetter = false,
                           bool hasSetter = false ) const {
        auto value = ValueFactory::createDefaultItemValue( valueType );

        Item item( dev_id, item_type, valueType, hasGetter, hasSetter, std::move( value ) );
        auto joItem = item.toJson();
        joItem[ Resources::ItemFields::IdFieldName ] = itemId;

        return joItem;
    }

    JsonObject createSimpleItem( const std::string& itemId ) const {
        return createItem( itemId, ValueType::Enum::Int );
    }
    JsonObject createSimpleDevice( const std::string& deviceId ) const {
        using namespace Resources;
        return JSON( DeviceFields::IdFieldName << deviceId << DeviceFields::ReadyFieldName << true );
    }

    const Services::UT::FakeResponseBus& getFakeBus() {
        return getHostService().getFakeResponseBus();
    }

    CoreDRemoteInterface::CoreDRemoteInterface& getRemoteInterface() {
        return m_coredRemoteInterface;
    }

    JsonObject createDecryptedUserCodeItem( const std::string& itemId, const std::string& dev_id = "dev_id" ) const {
        Json item = JsonObject::parse( R"(
        {
            "_id": ")" + itemId + R"(",
            "deviceId": ")" + dev_id + R"(",
            "enum": [],
            "hasGetter": true,
            "hasSetter": true,
            "name": "test_name",
            "show": true,
            "type": "userCode",
            "value": {
                "code": "9999",
                "name": "1"
            }
        })" );
        return item;
    }

    JsonObject createEncryptedUserCodeItem( const std::string& itemId, const std::string& dev_id = "dev_id" ) const {
        Json item = JsonObject::parse( R"(
        {
            "_id": ")" + itemId + R"(",
            "deviceId": ")" + dev_id + R"(",
            "enum": [],
            "hasGetter": true,
            "hasSetter": true,
            "name": "test_name",
            "show": true,
            "type": "userCode",
            "value": {
                "code": "j7NV2lXIhTGj+ZB8Yu0R/ilRqeFzu5bXOO2M/tk/HV0=",
                "name": "1"
            }
        })" );
        return item;
    }

    void setLocalTime( const std::string& time ) {
        m_localTime = time;
    }

private:
    std::string m_localTime;

    ProvisionDataUtils::ProvisionData m_provisionData;
    CoreDRemoteInterface::CoreDRemoteInterface m_coredRemoteInterface;
};

class TestMessage : public rpc::Message {
public:
    GenericSenderPtr getNetworkObject() const override {
        return pClient_.get();
    }

private:
    using ClientType = tiny::ws::client::client;
    using ClientTypePtr = tiny::retain_release_ptr< ClientType, rpc::LegacySynchronizationTraits< ClientType > >;
    mutable ClientTypePtr pClient_;
};

// TODO: implement TestBus and enable the test
TEST_F( CoredRemoteInterfaceTestSuite, DISABLED_addGateway ) {
    using namespace CoreDRemoteInterface;

    auto& coreDRemoteInterface = getRemoteInterface();

    Gateway::GatewayUnreachableReasons unreachableReasons = { std::make_pair( "1", "2" ) };
    Gateway::GatewayUnreachableActions unreachableActions = { std::make_pair( "3", "4" ) };
    Gateway::Operations operations = { { "add_device", "myplug:add_device.template" },
                                       { "remove_device", "myplug:remove_device.template" } };
    Gateway gateway{ "myplug", unreachableReasons, unreachableActions, "myplug", operations, "", "", "command" };

    coreDRemoteInterface.ensureGateway( gateway );

    const auto rawGateways = coreDRemoteInterface.getGateways();
    ASSERT_EQ( rawGateways.size(), 1u );
    EXPECT_EQ( rawGateways.front(), convertGatewayToJson( gateway ) );
}

TEST_F( CoredRemoteInterfaceTestSuite, gatewayToJson ) {
    using namespace CoreDRemoteInterface;

    Gateway::GatewayUnreachableReasons unreachableReasons = { std::make_pair( "1", "2" ) };
    Gateway::GatewayUnreachableActions unreachableActions = { std::make_pair( "3", "4" ) };
    Gateway::Operations operations = { { "add_device", "myplug:add_device.template" },
                                       { "remove_device", "myplug:remove_device.template" } };

    const char* const PluginName = "my_plugin_name";
    const std::string SetItemValueCommand = "HUB:my_plugin_name/scripts/set_item_value";
    const char* const AddItemDictionaryValueCommand = "HUB:my_plugin_name/scripts/add_item_dictionary_value";
    const char* const SetItemDictionaryValueCommand = "HUB:my_plugin_name/scripts/set_item_dictionary_value";
    const char* const RemoveItemDictionaryValueCommand = "HUB:my_plugin_name/scripts/remove_item_dictionary_value";
    const char* const ClearItemDictionaryValueCommand = "HUB:my_plugin_name/scripts/clear_item_dictionary_value";
    const char* const ForceRemoveDeviceCommand = "HUB:my_plugin_name/scripts/force_remove_device";
    const char* const UpdateDeviceFirmwareCommand = "HUB:my_plugin_name/scripts/update_device_firmware";
    const char* const CheckDeviceCommand = "HUB:my_plugin_name/scripts/check_device";

    const char* const CreateAbstractSubscriptionCommand = "HUB:my_plugin_name/scripts/create_abstract_subscription";
    const char* const UpdateAbstractSubscriptionCommand = "HUB:my_plugin_name/scripts/update_abstract_subscription";
    const char* const DeleteAbstractSubscriptionCommand = "HUB:my_plugin_name/scripts/delete_abstract_subscription";
    const char* const DispatchAbstractSubscriptionCommand = "HUB:my_plugin_name/scripts/dispatch_abstract_subscription";

    Gateway gateway{ PluginName,
                     unreachableReasons,
                     unreachableActions,
                     "myplug",
                     operations,
                     "",
                     "",
                     SetItemValueCommand,
                     "auto",
                     AddItemDictionaryValueCommand,
                     SetItemDictionaryValueCommand,
                     RemoveItemDictionaryValueCommand,
                     ClearItemDictionaryValueCommand,
                     "",
                     "",
                     "",
                     "",
                     ForceRemoveDeviceCommand,
                     UpdateDeviceFirmwareCommand,
                     CheckDeviceCommand,
                     CreateAbstractSubscriptionCommand,
                     UpdateAbstractSubscriptionCommand,
                     DeleteAbstractSubscriptionCommand,
                     DispatchAbstractSubscriptionCommand };

    const auto rawGateway = convertGatewayToJson( gateway );

    EXPECT_EQ( rawGateway[ "name" ], PluginName );
    EXPECT_EQ( rawGateway[ "operations" ],
               JSON( "add_device"
                     << "myplug:add_device.template"
                     << "remove_device"
                     << "myplug:remove_device.template" ) );

    using namespace CoreD::Model::Resources;
    EXPECT_EQ( json_get_str( rawGateway, GatewayFields::SetItemValueCommandFieldName ), SetItemValueCommand );
    EXPECT_EQ( json_get_str( rawGateway, GatewayFields::AddItemDictionaryValueCommandFieldName ),
               AddItemDictionaryValueCommand );
    EXPECT_EQ( json_get_str( rawGateway, GatewayFields::SetItemDictionaryValueCommandFieldName ),
               SetItemDictionaryValueCommand );
    EXPECT_EQ( json_get_str( rawGateway, GatewayFields::RemoveItemDictionaryValueCommandFieldName ),
               RemoveItemDictionaryValueCommand );
    EXPECT_EQ( json_get_str( rawGateway, GatewayFields::ClearItemDictionaryCommandFieldName ),
               ClearItemDictionaryValueCommand );
    EXPECT_EQ( json_get_str( rawGateway, GatewayFields::ForceRemoveDeviceCommandFieldName ), ForceRemoveDeviceCommand );
    EXPECT_EQ( json_get_str( rawGateway, GatewayFields::UpdateDeviceFirmwareCommandFieldName ),
               UpdateDeviceFirmwareCommand );
    EXPECT_EQ( json_get_str( rawGateway, GatewayFields::CheckDeviceCommandFieldName ), CheckDeviceCommand );

    EXPECT_EQ( json_get_str( rawGateway, GatewayFields::CreateAbstractSubscriptionCommandFieldName ),
               CreateAbstractSubscriptionCommand );
    EXPECT_EQ( json_get_str( rawGateway, GatewayFields::UpdateAbstractSubscriptionCommandFieldName ),
               UpdateAbstractSubscriptionCommand );
    EXPECT_EQ( json_get_str( rawGateway, GatewayFields::DeleteAbstractSubscriptionCommandFieldName ),
               DeleteAbstractSubscriptionCommand );
    EXPECT_EQ( json_get_str( rawGateway, GatewayFields::DispatchAbstractSubscriptionCommandFieldName ),
               DispatchAbstractSubscriptionCommand );
}

TEST_F( CoredRemoteInterfaceTestSuite, DISABLED_getGatewayByPluginId ) {
    using namespace CoreDRemoteInterface;
    using namespace Resources;

    auto& coreDRemoteInterface = getRemoteInterface();
    const std::string pluginId = "zwave_plugin";

    Gateway::GatewayUnreachableReasons unreachableReasons = { std::make_pair( "1", "2" ) };
    Gateway::GatewayUnreachableActions unreachableActions = { std::make_pair( "3", "4" ) };
    Gateway gateway{ "zwave", unreachableReasons, unreachableActions, pluginId, {}, "", "", "command" };

    coreDRemoteInterface.ensureGateway( gateway );

    auto gate = coreDRemoteInterface.getGatewayByPluginId( pluginId );
    ASSERT_TRUE( gate.is_object() && !gate.empty() );

    auto realPluginId = json_get_str( gate, GatewayFields::PluginIdFieldName );
    ASSERT_TRUE( !realPluginId.empty() );

    EXPECT_EQ( realPluginId, pluginId );
}

TEST_F( CoredRemoteInterfaceTestSuite, getDeviceWizardCategories ) {
    using namespace CoreDRemoteInterface;
    auto& coreDRemoteInterface = getRemoteInterface();

    CoreDInterface::DeviceWizardCategories categories = coreDRemoteInterface.getDeviceWizardCategories();
    EXPECT_EQ( categories.second, false );

    utils::writeToFile( fileDeviceAddingWizard, resources::daw::SourceJson.dump() );
    coreDRemoteInterface.setDeviceAddingWizard( std::make_unique< DeviceAddingWizard >( fileDeviceAddingWizard ) );
    categories = coreDRemoteInterface.getDeviceWizardCategories();
    remove( fileDeviceAddingWizard.c_str() );
    EXPECT_EQ( categories.second, true );
    EXPECT_EQ( categories.first, resources::daw::CategoriesList );
}

TEST_F( CoredRemoteInterfaceTestSuite, processCmd ) {
    using namespace CoreDRemoteInterface;
    auto& coreDRemoteInterface = getRemoteInterface();
    TestMessage message;
    message.parse( "{\
                      \"method\": \"devices_wizard.categories.list\",\
                      \"id\": \"_ID_\",\
                      \"params\": {}\
                  }" );
    coreDRemoteInterface.processCmd( "categories.list", message );
    JsonArray shouldBe = JSON( "error" << JSON( "message"
                                                << "source json file isn't opened" ) );
    EXPECT_EQ( message.getAnswer(), shouldBe );

    message.getAnswer().clear();
    utils::writeToFile( fileDeviceAddingWizard, resources::daw::SourceJson.dump() );
    coreDRemoteInterface.setDeviceAddingWizard( std::make_unique< DeviceAddingWizard >( fileDeviceAddingWizard ) );
    coreDRemoteInterface.processCmd( "categories.list", message );
    EXPECT_EQ( message.getAnswer(), resources::daw::CategoriesList );

    message.parse( "{\
                      \"method\": \"devices_wizard.devices.list\",\
                      \"id\": \"_ID_\",\
                      \"params\": {}\
                  }" );
    coreDRemoteInterface.processCmd( "devices.list", message );
    EXPECT_EQ( message.getAnswer(), resources::daw::DeviceList );

    message.parse( "{\
                      \"method\": \"devices_wizard.devices.list\",\
                      \"id\": \"_ID_\",\
                      \"params\": { \"wizardCategory\": \"ALARMS\" }\
                  }" );
    coreDRemoteInterface.processCmd( "devices.list", message );
    EXPECT_EQ( message.getAnswer(), resources::daw::AlarmDeviceList );
    remove( fileDeviceAddingWizard.c_str() );
}

TEST_F( CoredRemoteInterfaceTestSuite, getItem ) {
    using namespace CoreDRemoteInterface;
    using namespace Resources;

    const std::string itemId{ "item_id" };
    const auto joItem = createSimpleItem( itemId );
    getHostService().getDBClient().insert( DBNames::ItemsDB, joItem );

    auto& coreDRemoteInterface = getRemoteInterface();

    EXPECT_EQ( joItem, coreDRemoteInterface.getItem( itemId ) );
}

TEST_F( CoredRemoteInterfaceTestSuite, getItemsByDeviceId ) {
    using namespace CoreDRemoteInterface;
    using namespace Resources;

    const std::string dev_id{ "dev_id" };
    const std::string itemId1{ "item_id_1" };
    const std::string itemId2{ "item_id_2" };
    const auto joItem1 = createSimpleItem( itemId1 );
    const auto joItem2 = createSimpleItem( itemId2 );
    getHostService().getDBClient().insert( DBNames::ItemsDB, joItem1 );
    getHostService().getDBClient().insert( DBNames::ItemsDB, joItem2 );

    auto& coreDRemoteInterface = getRemoteInterface();

    const auto items = coreDRemoteInterface.getItemsByDeviceId( dev_id );
    EXPECT_EQ( joItem1, items[ 0 ] );
    EXPECT_EQ( joItem2, items[ 1 ] );
}

TEST_F( CoredRemoteInterfaceTestSuite, defaultGeoPosition ) {
    using namespace CoreDRemoteInterface;
    auto& coreDRemoteInterface = getRemoteInterface();
    GeoPosition resPos = coreDRemoteInterface.getGeoPosition().first;
    EXPECT_EQ( 39.8338509f, resPos.latitude );
    EXPECT_EQ( -74.8718262f, resPos.longitude );
}

TEST_F( CoredRemoteInterfaceTestSuite, geoPosition ) {
    using namespace CoreDRemoteInterface;

    GeoPosition pos{ 49.9808100, 36.2527200 };
    getHostService().getDBClient().insert(
        HUBSettings, JSON( "_id" << SettingsFields::LAT_NAME << SettingsFields::LAT_NAME << pos.latitude ) );
    getHostService().getDBClient().insert(
        HUBSettings, JSON( "_id" << SettingsFields::LNG_NAME << SettingsFields::LNG_NAME << pos.longitude ) );

    auto& coreDRemoteInterface = getRemoteInterface();
    GeoPosition resPos = coreDRemoteInterface.getGeoPosition().first;
    EXPECT_EQ( pos.latitude, resPos.latitude );
    EXPECT_EQ( pos.longitude, resPos.longitude );
}

TEST_F( CoredRemoteInterfaceTestSuite, geoPositionIsNotDefaultWhenModified ) {
    using namespace CoreDRemoteInterface;

    GeoPosition defaultGeoposition{ 39.8338509f, -74.8718262f };

    auto& coreDRemoteInterface = getRemoteInterface();

    auto geopositionPair = coreDRemoteInterface.getGeoPosition();
    GeoPosition resultPos = geopositionPair.first;
    bool geopositionIsDefault = geopositionPair.second;

    EXPECT_EQ( geopositionIsDefault, true );
    EXPECT_EQ( defaultGeoposition.latitude, resultPos.latitude );
    EXPECT_EQ( defaultGeoposition.longitude, resultPos.longitude );

    getHostService().getDBClient().insert(
        HUBSettings,
        JSON( "_id" << SettingsFields::LAT_NAME << SettingsFields::LAT_NAME << defaultGeoposition.latitude ) );
    getHostService().getDBClient().insert(
        HUBSettings,
        JSON( "_id" << SettingsFields::LNG_NAME << SettingsFields::LNG_NAME << defaultGeoposition.longitude ) );

    geopositionPair = coreDRemoteInterface.getGeoPosition();
    resultPos = geopositionPair.first;
    geopositionIsDefault = geopositionPair.second;

    EXPECT_EQ( geopositionIsDefault, false );
    EXPECT_EQ( defaultGeoposition.latitude, resultPos.latitude );
    EXPECT_EQ( defaultGeoposition.longitude, resultPos.longitude );
}
TEST_F( CoredRemoteInterfaceTestSuite, getUserCodeItem /*11.1*/ ) {
    const std::string itemId{ "item_id" };
    const auto encryptedItem = createEncryptedUserCodeItem( itemId );
    const auto decryptedItem = createDecryptedUserCodeItem( itemId );
    getHostService().getDBClient().insert( Resources::DBNames::ItemsDB, encryptedItem );

    EXPECT_EQ( decryptedItem.dump( 4 ), getRemoteInterface().getItem( itemId ).dump( 4 ) );
}

TEST_F( CoredRemoteInterfaceTestSuite, getUserCodeItems /*11.2*/ ) {
    using namespace CoreDRemoteInterface;
    using namespace Resources;

    const std::string itemId1{ "item_id_1" };
    const std::string itemId2{ "item_id_2" };
    const auto encryptedItem1 = createEncryptedUserCodeItem( itemId1 );
    const auto decryptedItem1 = createDecryptedUserCodeItem( itemId1 );
    const auto encryptedItem2 = createEncryptedUserCodeItem( itemId2 );
    const auto decryptedItem2 = createDecryptedUserCodeItem( itemId2 );
    getHostService().getDBClient().insert( DBNames::ItemsDB, encryptedItem1 );
    getHostService().getDBClient().insert( DBNames::ItemsDB, encryptedItem2 );

    const auto items = getRemoteInterface().getItems();
    EXPECT_EQ( decryptedItem1.dump( 4 ), items[ 0 ].dump( 4 ) );
    EXPECT_EQ( decryptedItem2.dump( 4 ), items[ 1 ].dump( 4 ) );
}

TEST_F( CoredRemoteInterfaceTestSuite, getUserCodeItemsBy /*11.3*/ ) {
    using namespace CoreDRemoteInterface;
    using namespace Resources;

    const std::string dev_id{ "dev_id" };
    const std::string itemId1{ "item_id_1" };
    const std::string itemId2{ "item_id_2" };
    const auto encryptedItem1 = createEncryptedUserCodeItem( itemId1, dev_id );
    const auto decryptedItem1 = createDecryptedUserCodeItem( itemId1, dev_id );
    const auto encryptedItem2 = createEncryptedUserCodeItem( itemId2, dev_id );
    const auto decryptedItem2 = createDecryptedUserCodeItem( itemId2, dev_id );
    getHostService().getDBClient().insert( DBNames::ItemsDB, encryptedItem1 );
    getHostService().getDBClient().insert( DBNames::ItemsDB, encryptedItem2 );

    const auto items =
        getRemoteInterface().getItemsBy( JSON( Model::Resources::ItemFields::DeviceIdFieldName << dev_id ) );
    EXPECT_EQ( decryptedItem1.dump( 4 ), items[ 0 ].dump( 4 ) );
    EXPECT_EQ( decryptedItem2.dump( 4 ), items[ 1 ].dump( 4 ) );
}

TEST_F( CoredRemoteInterfaceTestSuite, getUserCodeItemsByDeviceId /*11.4*/ ) {
    using namespace CoreDRemoteInterface;
    using namespace Resources;

    const std::string dev_id{ "dev_id" };
    const std::string itemId1{ "item_id_1" };
    const std::string itemId2{ "item_id_2" };
    const auto encryptedItem1 = createEncryptedUserCodeItem( itemId1, dev_id );
    const auto decryptedItem1 = createDecryptedUserCodeItem( itemId1, dev_id );
    const auto encryptedItem2 = createEncryptedUserCodeItem( itemId2, dev_id );
    const auto decryptedItem2 = createDecryptedUserCodeItem( itemId2, dev_id );
    getHostService().getDBClient().insert( DBNames::ItemsDB, encryptedItem1 );
    getHostService().getDBClient().insert( DBNames::ItemsDB, encryptedItem2 );

    const auto items = getRemoteInterface().getItemsByDeviceId( dev_id );
    EXPECT_EQ( decryptedItem1.dump( 4 ), items[ 0 ].dump( 4 ) );
    EXPECT_EQ( decryptedItem2.dump( 4 ), items[ 1 ].dump( 4 ) );
}

TEST_F( CoredRemoteInterfaceTestSuite, testUpdateItemDictionaryValueWhenElementsMaxNumberSpecifiedToNonZero ) {
    using namespace CoreDRemoteInterface;
    using namespace Resources;
    using namespace Model;

    const std::string itemId = "item_id";
    const ValueType elementType = ValueType::Enum::Int;

    auto& coreDRemoteInterface = getRemoteInterface();

    auto elementLoader = []( ValueType elementType ) { return ValueFactory::createDefaultItemValue( elementType ); };

    auto generateElementId = []() {
        static int i = 0;
        return std::to_string( i++ );
    };

    auto joItem = createItem( itemId, ValueType::makeDictionaryType( elementType ) );
    joItem[ ItemFields::ElementsMaxNumberFieldName ] = 1;
    getHostService().getDBClient().insert( DBNames::ItemsDB, joItem );

    coreDRemoteInterface.updateItemDictionaryValue( itemId, generateElementId(), elementLoader );

    auto requestToUpdate = getFakeBus().getMessage().data;
    EXPECT_EQ( json_get_str( requestToUpdate, "method" ), std::string{ "updateItemDictionaryValue" } );
}

TEST_F( CoredRemoteInterfaceTestSuite, testUpdateItemDictionaryValueWhenElementsMaxNumberSpecifiedToZero ) {
    using namespace CoreDRemoteInterface;
    using namespace Resources;
    using namespace Model;

    const std::string itemId = "item_id";
    const ValueType elementType = ValueType::Enum::Int;

    auto& coreDRemoteInterface = getRemoteInterface();

    auto elementLoader = []( ValueType elementType ) { return ValueFactory::createDefaultItemValue( elementType ); };

    auto generateElementId = []() {
        static int i = 0;
        return std::to_string( i++ );
    };

    auto joItem = createItem( itemId, ValueType::makeDictionaryType( elementType ) );
    joItem[ ItemFields::ElementsMaxNumberFieldName ] = 0;
    getHostService().getDBClient().insert( DBNames::ItemsDB, joItem );

    coreDRemoteInterface.updateItemDictionaryValue( itemId, generateElementId(), elementLoader );

    auto requestToUpdate = getFakeBus().getMessage().data;
    EXPECT_NE( json_get_str( requestToUpdate, "method" ), std::string{ "updateItemValue" } );
}

TEST_F( CoredRemoteInterfaceTestSuite, testIsInsideWeekInterval ) {
    // Wrong weekday
    EXPECT_EQ( getRemoteInterface().isInsideWeekInterval( { "monday", "wednesday" }, "18:27:15", "19:27:15" ), false );

    // Local time lower than start time
    EXPECT_EQ( getRemoteInterface().isInsideWeekInterval( { "monday", "tuesday" }, "18:27:16", "19:27:15" ), false );

    // Local time higher than end time
    EXPECT_EQ( getRemoteInterface().isInsideWeekInterval( { "monday", "tuesday" }, "18:15:16", "18:20:15" ), false );

    // Local time equal to start time
    EXPECT_EQ( getRemoteInterface().isInsideWeekInterval( { "monday", "tuesday" }, "18:27:15", "18:28:46" ), true );

    // Local time equal to end time
    EXPECT_EQ( getRemoteInterface().isInsideWeekInterval( { "monday", "tuesday" }, "18:26:38", "18:27:15" ), true );

    // Local time inside bounds
    EXPECT_EQ( getRemoteInterface().isInsideWeekInterval( { "monday", "tuesday" }, "18:26:38", "18:28:15" ), true );

    // Weekdays in non-sorted order
    EXPECT_EQ( getRemoteInterface().isInsideWeekInterval(
                   { "friday", "monday", "tuesday", "saturday" }, "18:26:38", "18:28:15" ),
               true );
}

TEST_F( CoredRemoteInterfaceTestSuite, testNextWeekEvent ) {
    // Same day event
    EXPECT_EQ( getRemoteInterface().getNextWeekEvent( { "tuesday" }, "19:46:38" ), "2021-06-22T19:46:38" );

    // Next week event
    EXPECT_EQ( getRemoteInterface().getNextWeekEvent( { "tuesday" }, "17:46:38" ), "2021-06-29T17:46:38" );

    // Next day event
    EXPECT_EQ( getRemoteInterface().getNextWeekEvent( { "tuesday", "wednesday" }, "05:13:56" ), "2021-06-23T05:13:56" );

    // Time is equal to local
    EXPECT_EQ( getRemoteInterface().getNextWeekEvent( { "tuesday", "wednesday" }, "18:27:15" ), "2021-06-23T18:27:15" );

    // Weekdays in wrong sequence
    EXPECT_EQ(
        getRemoteInterface().getNextWeekEvent( { "sunday", "friday", "saturday", "wednesday", "tuesday" }, "12:13:56" ),
        "2021-06-23T12:13:56" );
}

TEST_F( CoredRemoteInterfaceTestSuite, testGetTimeWithDelay ) {
    // Delay in seconds
    EXPECT_EQ( getRemoteInterface().calculateTimeWithDelay( "00:00:55" ), "2021-06-22T18:28:10" );

    // Delay in minutes
    EXPECT_EQ( getRemoteInterface().calculateTimeWithDelay( "00:45:00" ), "2021-06-22T19:12:15" );

    // Delay in hours
    EXPECT_EQ( getRemoteInterface().calculateTimeWithDelay( "12:00:00" ), "2021-06-23T06:27:15" );

    // combined
    EXPECT_EQ( getRemoteInterface().calculateTimeWithDelay( "12:45:55" ), "2021-06-23T07:13:10" );
}

TEST_F( CoredRemoteInterfaceTestSuite, setItemValueSourceErrors ) {
    using namespace CoreDRemoteInterface;
    using namespace Resources;
    using namespace CoreD;

    const std::string deviceId{ "dev_id" };
    const std::string itemId1{ "item_id_1" };
    const auto device1 = createSimpleDevice( deviceId );
    const auto joItem1 = createSimpleItem( itemId1 );
    getHostService().getDBClient().insert( DBNames::DevicesDB, device1 );
    getHostService().getDBClient().insert( DBNames::ItemsDB, joItem1 );
    int value = 111;
    Json source = JSON_EMPTY_OBJ;

    auto& remote = getRemoteInterface();

    JsonObject data = JSON( "_id" << itemId1 << "value" << value << "source" << source );
    EXPECT_NO_THROW( remote.setItemValue( data ) );

    data[ "source" ] = { { "someKey", "someValue" } };
    EXPECT_THROW( remote.setItemValue( data ), InvalidValueSourceTypeError ) << "source.type field not passed";

    data[ "source" ] = { { "type", 100 } };
    EXPECT_THROW( remote.setItemValue( data ), InvalidValueSourceTypeError ) << "source.type field is not a string";

    data[ "source" ] = { { "type", "someValue" } };
    EXPECT_NO_THROW( remote.setItemValue( data ) );
}

TEST_F( CoredRemoteInterfaceTestSuite, contactIdsList ) {
    using namespace CoreDRemoteInterface;
    using namespace Resources;
    using namespace CoreD;

    JsonObject contactId_1 = JSON( "text"
                                   << "110 Fire Generic"
                                   << "code"
                                   << "110"
                                   << "on"
                                   << "ALERT_FIRE"
                                   << "off"
                                   << "ALERT_FIRE_OFF" );
    JsonObject contactId_2 = JSON( "text"
                                   << "122 Silent Duress"
                                   << "code"
                                   << "122"
                                   << "on"
                                   << "ALERT_POLICE_SILENT"
                                   << "off"
                                   << "ALERT_POLICE_SILENT_OFF" );
    getHostService().getDBClient().insert( DBNames::ContactIdsDB, contactId_1 );
    getHostService().getDBClient().insert( DBNames::ContactIdsDB, contactId_2 );

    auto contactIdsNumbers = getHostService().getDBClient().count( DBNames::ContactIdsDB, JSON_EMPTY_OBJ );

    auto& remote = getRemoteInterface();

    auto answer = remote.contactIdOperations()->list( JSON_EMPTY_OBJ );
    EXPECT_TRUE( json_contains( answer, "aliases" ) );
    EXPECT_TRUE( json_contains( answer, "version" ) );
    JsonObject aliases;
    json_get_arr( answer, "aliases", aliases );
    EXPECT_EQ( aliases.size(), (int) contactIdsNumbers );
}

TEST_F( CoredRemoteInterfaceTestSuite, devicesListNoException ) {
    auto deviceJson = JSON_EMPTY_OBJ;
    deviceJson[ "_id" ] = "test_id";
    deviceJson[ "gatewayId" ] = "test_gateway";
    deviceJson[ "name" ] = "smoke";
    deviceJson[ "type" ] = "134_3_24";
    deviceJson[ "deviceTypeId" ] = "security_sensor";
    deviceJson[ "category" ] = "smoke";
    deviceJson[ "subcategory" ] = "co2";
    deviceJson[ "batteryPowered" ] = false;
    deviceJson[ "roomId" ] = "";
    deviceJson[ "parentDeviceId" ] = "";
    deviceJson[ "parentRoom" ] = true;
    deviceJson[ "reachable" ] = true;
    deviceJson[ "status" ] = true;
    deviceJson[ "info" ] = JSON_EMPTY_OBJ;

    getHostService().getDBClient().insert( HaDevicesDBName, deviceJson );

    auto& remote = getRemoteInterface();
    std::vector< Json > devices;

    EXPECT_NO_THROW( devices = remote.getDevices() );

    EXPECT_EQ( devices.size(), 1 );
    EXPECT_EQ( devices[ 0 ], deviceJson );
}

}  // namespace UT
}  // namespace HubPlatform
