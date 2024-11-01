/** hub.data.list command related tests.
 */

#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/visitors/item_value_float_visitor.h"
#include "cored/model/exceptions.h"
#include "cored/remote_interface/cored_interface.h"
#include "cored/model/values/values.h"
#include "jsonrpc/jsonrpc.h"

#include "common_test_resources/fixtures/core_test_fixture.h"
#include "common_test_resources/resources/cored_resources.h"
#include "common_test_resources/resources/uid_resources.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"

#include <gtest/gtest.h>
#include "database_client/dbqueryparser.h"

namespace HubPlatform {
namespace Tests {

/*
Suite Setup:
    - Creates a device, three items, two rooms and two scenes.

TESTPLAN:
    0. DataListEmptyParams: send hub.data.list with no params, WRONG_PARAMS expected;
    1. DataListDevicesAll: fetch all devices, one response;
    2. DataListDevicesFilterById: fetch devices filtered by device id, one response;
    3. DataListDevicesIdDoesNotExist: response should contain value null for request.devices.ids[i] if
            it doesn't exist;
    3.1 DataListDevices_IdInvalidType: invalid params, error.data = rpc.params.invalid.devices;
    3.2 DataListDevices_FieldInvalidType: invalid params, error.data = rpc.params.invalid.devices;
    4. DataListItemsAll: fetch all items, one response, 3 items in it;
    5. DataListRoomsAll: fetch all rooms, one response, 2 items in it;
    6. DataListScenesAll: fetch all scenes, one response, 2 items in it;
    7. DataListAllFour: fetch all four collections, four independent responses expected.

*/

using namespace CoreD;
using namespace Services;

using MakeRequest =
    std::function< JsonObject( std::string const& itemId, JsonObject const& value, JsonObject const& scale ) >;
using AssertionPred = std::function< void( std::string itemId, JsonObject const& value, int codeLine ) >;

JsonObject makeRequestDefault( std::string const& itemId, JsonObject const& value, JsonObject const& scale );

using StringList = std::vector< std::string >;

class HubDataListFixture : public CoreNmaTest {
public:
    HubDataListFixture();
    static void SetUpTestCase();
    static void TearDownTestCase();

    static StringList createDevicesAndItems( CoreNmaTest& coreTest,
                                             CoreDRemoteInterface::CoreDInterface& remoteInterface );

    static StringList createRooms( CoreNmaTest& coreTest, CoreDRemoteInterface::CoreDInterface& remoteInterface );
    static void deleteRooms( CoreNmaTest& coreTest, CoreDRemoteInterface::CoreDInterface& remoteInterface );

    static StringList createScenes( CoreNmaTest& coreTest );
    static std::string createScene( CoreNmaTest& coreTest, const JsonObject& sceneJson );
    static void deleteScenes( CoreNmaTest& coreTest );

    void SetUp() override;
    void TearDown() override;

    void TestBody() override{};

public:
    struct DeviceInfo {
        std::string name;
        std::string type;
        std::string typeId;
        std::string category;
        std::string subcategory;
        bool batteryPowered;
        bool persistent;
    };

    struct ItemInfo {
        std::string deviceId;
        std::string itemId;
        std::string itemName;
        CoreD::Model::ValueType::Enum valueType;
        bool hasGetter;
        bool hasSetter;
        JsonObject valueRequest;
        JsonObject valueMin;
        JsonObject valueMax;
    };

    using ItemsNameToItemInfoMap = std::map< std::string, ItemInfo >;
    struct DeviceItem {
        DeviceInfo info;
        ItemsNameToItemInfoMap idMap;
        std::map< std::string, std::string > idToName;

        bool itemInfoByName( const std::string& name, ItemInfo& foundInfo ) {
            auto foundIt = idMap.find( name );
            if( foundIt != idMap.end() ) {
                foundInfo = foundIt->second;
                return true;
            }
            return false;
        }

        bool itemInfoById( const std::string& itemId, ItemInfo& foundInfo ) {
            auto found = idToName.find( itemId );
            if( found != idToName.end() ) {
                return itemInfoByName( found->second, foundInfo );
            }
            return false;
        }
    };
    using DeviceItemsMap = std::map< std::string, DeviceItem >;

protected:
    CoreDRemoteInterface::CoreDInterface& remoteInterface;

    static StringList& deviceIdList() {
        static StringList deviceIds;
        return deviceIds;
    }

    static DeviceItemsMap& deviceItems() {
        static DeviceItemsMap dict;
        return dict;
    }

    static StringList& sceneIdList() {
        static StringList scenes;
        return scenes;
    }

    static StringList& roomIdList() {
        static StringList rooms;
        return rooms;
    }
};

HubDataListFixture::HubDataListFixture()
: remoteInterface( makeRemote( nma() ) ) {}

StringList HubDataListFixture::createDevicesAndItems( CoreNmaTest& coreTest,
                                                      CoreDRemoteInterface::CoreDInterface& remoteInterface ) {
    DeviceInfo info = { "testDevice", "smoke", "134_3_24", "security_sensor", "smoke", false, true };
    CoreD::Model::Device device( "1", info.name, info.type, info.typeId, info.category, info.batteryPowered );
    device.setSubcategory( info.subcategory ).setPersistent( info.persistent );

    std::string deviceId = coreTest.addDevice( remoteInterface, device, __LINE__ );

    auto& mapDeviceItems = deviceItems();
    auto& deviceItem = mapDeviceItems[ deviceId ];
    ItemsNameToItemInfoMap& deviceItems = deviceItem.idMap;
    deviceItem.info = info;

    // create items for test
    using namespace Model::Resources;
    using namespace CoreD::Model;

    {
        std::string itemName = "item_int";
        int value = 1;

        ItemInfo info = { deviceId, "", itemName, ValueType::Enum::Int, true, true };

        ValueUPtr valuePtr = ValueFactory::createItemValue( info.valueType, Json( value ) );
        Item item(
            info.deviceId, info.itemName, info.valueType, info.hasGetter, info.hasSetter, std::move( valuePtr ) );
        auto itemId = coreTest.addItem( remoteInterface, item, __LINE__, nullptr );
        info.itemId = itemId;
        info.valueRequest = JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << value );

        deviceItems[ itemName ] = info;
        deviceItem.idToName[ itemId ] = info.itemName;
    }

    {
        std::string itemName = "item_float";
        float value = 1.5;
        ItemInfo info = { deviceId, "", itemName, CoreD::Model::ValueType::Enum::Float, true, true };
        ValueUPtr valuePtr = ValueFactory::createItemValue( info.valueType, Json( value ) );
        Item item(
            info.deviceId, info.itemName, info.valueType, info.hasGetter, info.hasSetter, std::move( valuePtr ) );

        auto itemId = coreTest.addItem( remoteInterface, item, __LINE__, nullptr );
        info.itemId = itemId;
        info.valueRequest = JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << value );
        deviceItems[ itemName ] = info;
        deviceItem.idToName[ itemId ] = info.itemName;
    }

    {
        std::string itemName = "item_temperature";
        ItemInfo info = { deviceId, "", itemName, CoreD::Model::ValueType::Enum::Temperature, true, true };
        float value = 10.0f, minValue = 0.0f, maxValue = 40.0f;

        using namespace CoreD::Model;

        auto valueType = ValueType::Enum::Temperature;
        auto joValue = JSON( "scale"
                             << "celsius" );
        joValue[ "value" ] = value;

        CoreD::Model::Item item( info.deviceId,
                                 info.itemName,
                                 info.valueType,
                                 info.hasGetter,
                                 info.hasSetter,
                                 ValueFactory::createItemValue( valueType, joValue ) );

        // Append min/max values

        joValue[ "value" ] = minValue;
        info.valueMin = joValue;
        item.setLowerLimitValue( ValueFactory::createItemValue( valueType, info.valueMin ) );

        joValue[ "value" ] = maxValue;
        info.valueMax = joValue;
        item.setUpperLimitValue( ValueFactory::createItemValue( valueType, joValue ) );

        auto itemInsertedBroadcastAssert = []( JsonObject& broadcast, const Model::Item& item ) -> void {
            // EFWR-1975: check validity of a broadcast here
            JsonObject result;
            auto dump = broadcast.dump();
            auto joItem = item.toJson();
            auto joDump = joItem.dump();
            ASSERT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.added" ) << dump;
            EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );

            ASSERT_EQ( result.count( ItemFields::ValueMinFieldName ), 0 )
                << "should have no valueMin field in broadcast: ";
            ASSERT_EQ( result.count( ItemFields::ValueMaxFieldName ), 0 )
                << "should have no valueMax field in broadcast: ";

            using namespace ItemFields;

            if( result.count( Response::MinValueFieldName ) && result.count( Response::MaxValueFieldName ) ) {
                // EFWR-1975: new value for min/max; should be int/float
                auto itemValueMin = json_get_float( joItem, Response::MinValueFieldName );
                auto brValueMin = json_get_float( broadcast, Response::MinValueFieldName );
                ASSERT_FLOAT_EQ( brValueMin, itemValueMin )
                    << "Item and broadcast values do not match: " << itemValueMin << " != " << brValueMin;

                auto itemValueMax = json_get_float( joItem, Response::MaxValueFieldName );
                auto brValueMax = json_get_float( broadcast, Response::MaxValueFieldName );
                ASSERT_FLOAT_EQ( brValueMax, itemValueMax )
                    << "Item and broadcast values do not match: " << itemValueMax << " != " << brValueMax;
            }
        };
        auto itemId = coreTest.addItem( remoteInterface, item, __LINE__, itemInsertedBroadcastAssert );
        info.itemId = itemId;
        info.valueRequest = JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << value << "scale"
                                                          << "celsius" );
        deviceItems[ itemName ] = info;
        deviceItem.idToName[ itemId ] = info.itemName;
    }

    StringList devices;
    devices.emplace_back( deviceId );
    return std::move( devices );
}

StringList HubDataListFixture::createRooms( CoreNmaTest& coreTest,
                                            CoreDRemoteInterface::CoreDInterface& remoteInterface ) {
    JsonObject bathroom = remoteInterface.createRoom( "Bathroom" );
    auto brBathroom = coreTest.waitForBroadcast( "hub.room.created", NMA_TEST_MAKE_MESSAGE( "room created br" ) );
    JsonObject kitchen = remoteInterface.createRoom( "Kitchen" );
    auto brKitchen = coreTest.waitForBroadcast( "hub.room.created", NMA_TEST_MAKE_MESSAGE( "room created br" ) );

    std::string dumps[] = { bathroom.dump(), kitchen.dump(), brBathroom.dump(), brKitchen.dump() };
    LOGGER_DEBUG_RAW( "Rooms created: Bathroom: " + cc::j( bathroom ) + "kitchen: " + cc::j( kitchen ) );

    StringList& ids = roomIdList();
    ids.push_back( bathroom[ "_id" ].get< std::string >() );
    ids.push_back( kitchen[ "_id" ].get< std::string >() );
    return ids;
}

std::string HubDataListFixture::createScene( CoreNmaTest& coreTest, const JsonObject& sceneJson ) {
    auto answer = coreTest.nma().sendtoHubAndGetAnswer( sceneJson );

    const auto& error = answer[ "error" ];
    EXPECT_TRUE( error.empty() ) << error.dump();
    if( !error.empty() ) {
        throw std::exception();
    }
    JsonObject broadcast = coreTest.waitForBroadcast( "hub.scene.added", NMA_TEST_MAKE_MESSAGE( "createScene" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.added" );

    JsonObject result;
    JsonObject params;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    EXPECT_TRUE( json_get_obj( sceneJson, "params", params ) );
    EXPECT_EQ( json_get_bool( result, "enabled" ), json_get_bool( params, "enabled" ) );
    EXPECT_EQ( json_get_str( result, "group_id" ), json_get_str( params, "group_id" ) );
    EXPECT_EQ( json_get_bool( result, "is_group" ), json_get_bool( params, "is_group" ) );
    EXPECT_EQ( json_get_str( result, "name" ), json_get_str( params, "name" ) );
    EXPECT_EQ( json_get_str( result, "parent_id" ), json_get_str( params, "parent_id" ) );
    auto id = json_get_str( result, "_id" );
    EXPECT_TRUE( !id.empty() );
    sceneIdList().push_back( id );
    return id;
}

StringList HubDataListFixture::createScenes( CoreNmaTest& coreTest ) {
    auto sceneId0 = createScene( coreTest, createHouseModeScene );
    auto sceneId1 = createScene( coreTest, createMonthlyScene );

    return StringList{ sceneId0, sceneId1 };
}

class ScenesHelper : public SceneManagerTestsFixture {
public:
    void TestBody() override {}
};

void HubDataListFixture::deleteScenes( CoreNmaTest& coreTest ) {
    auto& ids = sceneIdList();
    ScenesHelper sceneTests;
    sceneTests.SetUp();  // set up broadcast listener

    for( auto id : ids ) {
        sceneTests.deleteScene( id );
    }

    sceneTests.TearDown();

    ids.clear();
}

void HubDataListFixture::deleteRooms( CoreNmaTest& coreTest, CoreDRemoteInterface::CoreDInterface& remoteInterface ) {
    auto& ids = roomIdList();

    auto response = remoteInterface.deleteAllRooms();

    coreTest.ignoreBroadcasts(
        []( const JsonObject& broadcast ) { return json_get_str( broadcast, "msg_subclass" ) == "hub.room.deleted"; } );

    ids.clear();
}

void HubDataListFixture::SetUpTestCase() {
    Log::trace( "testSuite: SetUp" );

    auto& remoteInterface = makeRemote( nma() );
    HubDataListFixture coreTest;
    coreTest.CommonNmaTests::SetUp();

    // create device
    deviceIdList() = createDevicesAndItems( coreTest, remoteInterface );

    createRooms( coreTest, remoteInterface );

    createScenes( coreTest );

    coreTest.CommonNmaTests::TearDown();
}

void HubDataListFixture::TearDownTestCase() {
    Log::trace( "testSuite: TearDown" );
    HubDataListFixture coreTest;

    // Call it to wait of all broadcasts passed after the tests done
    coreTest.CommonNmaTests::SetUp();

    coreTest.clearExistingDevices( __LINE__ );
    auto& currentMapping = deviceItems();
    currentMapping.clear();

    deleteScenes( coreTest );
    deleteRooms( coreTest, makeRemote() );
    deviceIdList().clear();
}

void HubDataListFixture::SetUp() {
    CoreNmaTest::SetUp();
}

void HubDataListFixture::TearDown() {
    CoreNmaTest::TearDown();
}

class HubDataListTests : public HubDataListFixture {};

TEST_F( HubDataListTests, DataListEmptyParams ) {
    // Make empty request without any filtering, get 1 responses filled with WRONG_PARAMS error from a server.
    JsonObject request = JsonObject::parse( hubDataList );
    JsonObject error;
    JsonObject answer = nma().sendtoHubAndGetAnswer( request );

    ASSERT_TRUE( json_get_obj( answer, "error", error ) );
    auto dumpError = error.dump();

    auto code = json_get_int( error, "code" );
    ASSERT_TRUE( code < 0 ) << dumpError;
    ASSERT_EQ( static_cast< int >( Errors::Codes::INVALID_PARAMS ), code ) << dumpError;
}

void assertDeviceFields( const HubDataListFixture::DeviceInfo& info,
                         const JsonObject& jDevice,
                         const std::string& deviceId = "'not set'" ) {
    auto deviceDump = jDevice.dump();

    ASSERT_EQ( info.type, json_get_str( jDevice, "type" ) ) << "deviceId: " << deviceId << ", device: " << deviceDump;
    ASSERT_EQ( info.typeId, json_get_str( jDevice, "deviceTypeId" ) )
        << "deviceId: " << deviceId << ", device: " << deviceDump;
    ASSERT_EQ( info.category, json_get_str( jDevice, "category" ) )
        << "deviceId: " << deviceId << ", device: " << deviceDump;
    ASSERT_EQ( info.subcategory, json_get_str( jDevice, "subcategory" ) )
        << "deviceId: " << deviceId << ", device: " << deviceDump;
    ASSERT_EQ( info.name, json_get_str( jDevice, "name" ) ) << "deviceId: " << deviceId << ", device: " << deviceDump;
    ASSERT_EQ( info.batteryPowered, json_get_bool( jDevice, "batteryPowered", false ) )
        << "deviceId: " << deviceId << ", device: " << deviceDump;
    ASSERT_EQ( info.persistent, json_get_bool( jDevice, "persistent", false ) )
        << "deviceId: " << deviceId << ", device: " << deviceDump;
}

void assertItemFields( const HubDataListFixture::ItemInfo& info,
                       const JsonObject& jItem,
                       const std::string& itemId = "'not set'" ) {
    using namespace CoreD::Model::Resources;
    auto itemDump = jItem.dump();

    EXPECT_EQ( info.deviceId, json_get_str( jItem, "deviceId" ) );
    EXPECT_EQ( info.itemName, json_get_str( jItem, "name" ) );
    //    EXPECT_EQ( info.valueRequest, json_get_str( jItem, "valueFormatted" ) );
    EXPECT_EQ( info.deviceId, json_get_str( jItem, "deviceId" ) );
    ASSERT_EQ( info.hasGetter, json_get_bool( jItem, "hasGetter" ) );
    ASSERT_EQ( info.hasSetter, json_get_bool( jItem, "hasSetter" ) );

    // EFWR-1975; new min/maxValue field format
    std::string responseMinValue = ItemFields::Response::MinValueFieldName;
    std::string responseMaxValue = ItemFields::Response::MaxValueFieldName;

    EXPECT_EQ( jItem.count( ItemFields::ValueMinFieldName ), 0 );
    EXPECT_EQ( jItem.count( ItemFields::ValueMaxFieldName ), 0 );

    if( info.valueMin != nullptr ) {
        CoreNmaTest::compareTemperatureValues( info.valueMin,
                                               jItem,
                                               responseMinValue,
                                               std::string{ __FILE__ }.append( " : " ) + std::to_string( __LINE__ ) );
    }
    if( info.valueMax != nullptr ) {
        CoreNmaTest::compareTemperatureValues( info.valueMax,
                                               jItem,
                                               responseMaxValue,
                                               std::string{ __FILE__ }.append( " : " ) + std::to_string( __LINE__ ) );
    }
}

TEST_F( HubDataListTests, DataListDevicesAll ) {
    // get all devices here
    JsonObject request = JsonObject::parse( hubDataList );
    request[ "params" ][ "devices" ] = nullptr;
    auto pdump = request[ "params" ][ "devices" ].dump();

    JsonObject error, result;
    JsonObject answer = nma().sendtoHubAndGetAnswer( request );
    auto dump = answer.dump();

    ASSERT_FALSE( json_get_obj( answer, "error", error ) );
    ASSERT_TRUE( json_get_obj( answer, "result", result ) ) << "Answer dump: " << dump;

    JsonObject devices;
    ASSERT_TRUE( result.count( "devices" ) );
    ASSERT_TRUE( json_get_obj( result, "devices", devices ) ) << "'devices': not an object! Dump: " << devices.dump();
    ASSERT_TRUE( devices.is_null() || devices.is_object() )
        << "'devices' should be null or an object: " << devices.dump();

    if( !devices.is_null() ) {
        auto const& mapDeviceItems = deviceItems();
        ASSERT_FALSE( mapDeviceItems.empty() );

        for( auto it = begin( mapDeviceItems ), eit = end( mapDeviceItems ); it != eit; ++it ) {
            const std::string& deviceId = it->first;
            const auto& deviceItem = it->second;

            // assert response fields with created device info:
            const auto& info = deviceItem.info;
            JsonObject jDevice;
            ASSERT_TRUE( json_get_obj( devices, deviceId, jDevice ) );

            assertDeviceFields( info, jDevice, deviceId );
        }
    }
}

TEST_F( HubDataListTests, DataListDevicesFilterById ) {
    // get all devices here
    JsonObject request = JsonObject::parse( hubDataList );

    auto const& mapDeviceItems = deviceItems();
    ASSERT_FALSE( mapDeviceItems.empty() );

    const std::string& deviceId = mapDeviceItems.begin()->first;

    request[ "params" ][ "devices" ][ "ids" ] = JSON_ARRAY( deviceId );
    auto pdump = request[ "params" ][ "devices" ].dump();

    JsonObject error, result;
    JsonObject answer = nma().sendtoHubAndGetAnswer( request );
    auto dump = answer.dump();

    ASSERT_FALSE( json_get_obj( answer, "error", error ) );
    ASSERT_TRUE( json_get_obj( answer, "result", result ) ) << "Answer dump: " << dump;

    JsonObject devices;
    ASSERT_TRUE( result.count( "devices" ) );
    ASSERT_TRUE( json_get_obj( result, "devices", devices ) ) << "'devices': not an object! Dump: " << devices.dump();
    ASSERT_TRUE( devices.is_object() ) << "'devices' should or an object: " << devices.dump();

    ASSERT_EQ( json_object_keys( devices ).size(), 1 ) << "should have only 1 device in response";
    {
        auto deviceIt = mapDeviceItems.find( deviceId );
        ASSERT_TRUE( deviceIt != mapDeviceItems.end() );
        const auto& deviceItem = deviceIt->second;

        // assert response fields with created device info:
        const auto& info = deviceItem.info;
        JsonObject jDevice;
        ASSERT_TRUE( json_get_obj( devices, deviceId, jDevice ) );

        assertDeviceFields( info, jDevice, deviceId );
    }
}

TEST_F( HubDataListTests, DataListDevicesIdDoesNotExist ) {
    // get all devices here
    JsonObject request = JsonObject::parse( hubDataList );

    auto const& mapDeviceItems = deviceItems();
    ASSERT_FALSE( mapDeviceItems.empty() );

    const std::string& deviceId = "non-existent-device-id";

    request[ "params" ][ "devices" ][ "ids" ] = JSON_ARRAY( deviceId );
    auto pdump = request[ "params" ][ "devices" ].dump();

    JsonObject error, result;
    JsonObject answer = nma().sendtoHubAndGetAnswer( request );
    auto dump = answer.dump();

    ASSERT_FALSE( json_get_obj( answer, "error", error ) );
    ASSERT_TRUE( json_get_obj( answer, "result", result ) ) << "Answer dump: " << dump;

    JsonObject devices;
    ASSERT_TRUE( result.count( "devices" ) );
    ASSERT_TRUE( json_get_obj( result, "devices", devices ) ) << "'devices': not an object! Dump: " << devices.dump();
    ASSERT_TRUE( devices.is_null() || devices.is_object() )
        << "'devices' should be null or an object: " << devices.dump();

    ASSERT_TRUE( devices.is_object() );
    auto responseDeviceIds = json_object_keys( devices );
    ASSERT_EQ( responseDeviceIds.size(), 1 ) << "should have only 1 device in response";
    auto respId = json_get_str( responseDeviceIds[ 0 ] );
    ASSERT_EQ( respId, deviceId ) << "should have key: " << deviceId;

    JsonObject deviceData;
    ASSERT_TRUE( devices[ respId ].is_null() ) << "deviceId: value should be null:" << devices.dump();
}

TEST_F( HubDataListTests, DataListDevices_IdInvalidType ) {
    JsonObject request = JsonObject::parse( hubDataList );

    auto const& mapDeviceItems = deviceItems();
    ASSERT_FALSE( mapDeviceItems.empty() );

    const std::string& deviceId = "non-existent-device-id";

    auto invalidType = 11111;
    request[ "params" ][ "devices" ][ "ids" ] = JSON_ARRAY( invalidType );
    auto pdump = request[ "params" ][ "devices" ].dump();

    JsonObject error, result;
    JsonObject answer = nma().sendtoHubAndGetAnswer( request );
    auto dump = answer.dump();

    ASSERT_TRUE( json_get_obj( answer, "error", error ) ) << pdump;
    ASSERT_EQ( json_get_int( error, "code" ), static_cast< int >( Errors::Codes::INVALID_PARAMS ) ) << dump;
    ASSERT_EQ( json_get_str( error, "data" ), "rpc.params.invalid.devices" ) << dump;
}

TEST_F( HubDataListTests, DataListDevices_FieldInvalidType ) {
    JsonObject request = JsonObject::parse( hubDataList );

    auto const& mapDeviceItems = deviceItems();
    ASSERT_FALSE( mapDeviceItems.empty() );

    const std::string& deviceId = "non-existent-device-id";

    auto invalidType = 11111;
    request[ "params" ][ "devices" ][ "fields" ][ "include" ] = JSON_ARRAY( invalidType );
    auto pdump = request[ "params" ][ "devices" ].dump();

    JsonObject error, result;
    JsonObject answer = nma().sendtoHubAndGetAnswer( request );
    auto dump = answer.dump();

    ASSERT_TRUE( json_get_obj( answer, "error", error ) ) << pdump;
    ASSERT_EQ( json_get_int( error, "code" ), static_cast< int >( Errors::Codes::INVALID_PARAMS ) ) << dump;
    ASSERT_EQ( json_get_str( error, "data" ), "rpc.params.invalid.devices" ) << dump;
}

TEST_F( HubDataListTests, DataListItemsAll ) {
    // get all items here
    JsonObject request = JsonObject::parse( hubDataList );
    // add request.items = null to get only items
    request[ "params" ][ "items" ] = JsonObject( nullptr );

    JsonObject error, result;
    JsonObject answer = nma().sendtoHubAndGetAnswer( request );
    auto dump = answer.dump();

    ASSERT_FALSE( json_get_obj( answer, "error", error ) ) << "Answer: " << dump;
    ASSERT_TRUE( json_get_obj( answer, "result", result ) ) << "Answer dump: " << dump;

    JsonObject items;
    ASSERT_TRUE( result.count( "items" ) );
    ASSERT_TRUE( json_get_obj( result, "items", items ) ) << "'items': not an object! Dump: " << items.dump();
    ASSERT_TRUE( items.is_object() ) << "'items' should be an object: " << items.dump();

    auto& mapDeviceItems = deviceItems();

    for( auto joIt = items.begin(); joIt != items.end(); joIt++ ) {
        const std::string& itemId = joIt.key();
        const JsonObject& fields = joIt.value();

        ASSERT_TRUE( fields.is_object() );
        auto deviceId = json_get_str( fields, "deviceId" );
        ASSERT_FALSE( deviceId.empty() );

        if( std::find( begin( deviceIdList() ), end( deviceIdList() ), deviceId ) == end( deviceIdList() ) ) {
            LOGGER_TRACE_RAW( "The deviceId does not belong to that test: " + deviceId );
            continue;
        }
        ASSERT_TRUE( mapDeviceItems.find( deviceId ) != mapDeviceItems.end() );
        auto deviceItem = mapDeviceItems[ deviceId ];

        // find item.info
        ItemInfo itemInfo;
        ASSERT_TRUE( deviceItem.itemInfoById( itemId, itemInfo ) ) << itemId;

        ASSERT_EQ( itemInfo.deviceId, json_get_str( fields, "deviceId" ) );
        ASSERT_EQ( itemInfo.hasGetter, json_get_bool( fields, "hasGetter" ) );
        ASSERT_EQ( itemInfo.hasSetter, json_get_bool( fields, "hasSetter" ) );
        ASSERT_EQ( itemInfo.itemName, json_get_str( fields, "name" ) );

        assertItemFields( itemInfo, fields, itemId );
    }
}

TEST_F( HubDataListTests, DataListItemsByField ) {
    // get all items here
    JsonObject request = JsonObject::parse( hubDataList );
    // add request.items = null to get only items
    std::vector< std::string > fieldsReq = { "value", "name" };
    request[ "params" ][ "items" ] = JSON( "fields" << JSON( "include" << JsonObject( fieldsReq ) ) );

    JsonObject error, result;
    JsonObject answer = nma().sendtoHubAndGetAnswer( request );
    auto dump = answer.dump();

    ASSERT_FALSE( json_get_obj( answer, "error", error ) );
    ASSERT_TRUE( json_get_obj( answer, "result", result ) ) << "Answer dump: " << dump;

    JsonObject items;
    ASSERT_TRUE( result.count( "items" ) );
    ASSERT_TRUE( json_get_obj( result, "items", items ) ) << "'items': not an object! Dump: " << items.dump();
    ASSERT_TRUE( items.is_object() ) << "'items' should be an object: " << items.dump();

    auto& mapDeviceItems = deviceItems();

    for( auto joIt = items.begin(); joIt != items.end(); joIt++ ) {
        const std::string& itemId = joIt.key();
        const JsonObject& fields = joIt.value();

        // has only "name" field
        ASSERT_TRUE( fields.is_object() );
        auto deviceId = json_get_str( fields, "deviceId" );
        ASSERT_TRUE( deviceId.empty() ) << fields.dump();
        ASSERT_FALSE( json_get_str( fields, "name" ).empty() ) << fields.dump();
        ASSERT_EQ( fields.count( "value" ), 1 ) << fields.dump();
        auto fieldsArr = json_object_keys( fields );
        ASSERT_TRUE( fieldsArr.is_array() ) << fields.dump();
        ASSERT_EQ( fieldsArr.size(), fieldsReq.size() ) << fields.dump();
    }
}

TEST_F( HubDataListTests, DataListRoomsAll ) {
    JsonObject request = JsonObject::parse( hubDataList );
    request[ "params" ][ "rooms" ] = JsonObject( nullptr );

    JsonObject error, result;
    JsonObject answer = nma().sendtoHubAndGetAnswer( request );
    auto dump = answer.dump();

    // check error
    ASSERT_FALSE( json_get_obj( answer, "error", error ) );

    // check result
    ASSERT_TRUE( json_get_obj( answer, "result", result ) ) << "Answer dump: " << dump;

    JsonObject rooms;
    ASSERT_TRUE( result.count( "rooms" ) );
    ASSERT_TRUE( json_get_obj( result, "rooms", rooms ) ) << "'rooms': not an object! Dump: " << rooms.dump();
    ASSERT_TRUE( rooms.is_object() ) << "'rooms' should be an object: " << rooms.dump();
}

TEST_F( HubDataListTests, DataListScenesAll ) {
    // get all devices here
    JsonObject request = JsonObject::parse( hubDataList );
    // params: { scenes: null }
    request[ "params" ][ "scenes" ] = JsonObject( nullptr );

    JsonObject error, result;
    JsonObject answer = nma().sendtoHubAndGetAnswer( request );
    auto dump = answer.dump();

    // check error
    ASSERT_FALSE( json_get_obj( answer, "error", error ) );

    // check result
    ASSERT_TRUE( json_get_obj( answer, "result", result ) ) << "Answer dump: " << dump;

    // check scenes
    JsonObject scenes;
    ASSERT_TRUE( result.count( "scenes" ) );
    ASSERT_TRUE( json_get_obj( result, "scenes", scenes ) ) << "'scenes': not an object! Dump: " << scenes.dump();
    ASSERT_TRUE( scenes.is_object() ) << "'scenes' should be an object: " << scenes.dump();
}

TEST_F( HubDataListTests, DataListAllFour ) {
    JsonObject request = JsonObject::parse( hubDataList );

    std::vector< std::string > collections{ "devices", "items", "rooms", "scenes" };

    for( auto colName : collections ) {
        request[ "params" ][ colName ] = JsonObject( nullptr );
    }

    JsonObject answer = nma().sendtoHubAndGetAnswer( request );

    auto checkCollection = []( const std::string& key, JsonObject& answer ) {
        auto dump = answer.dump();

        JsonObject error, result;
        // check error
        ASSERT_FALSE( json_get_obj( answer, "error", error ) ) << dump;

        // check result
        ASSERT_TRUE( json_get_obj( answer, "result", result ) ) << "Answer dump: " << dump;

        // check scenes
        JsonObject collection;
        ASSERT_TRUE( result.count( key ) );
        ASSERT_TRUE( json_get_obj( result, key, collection ) )
            << "'" << key << "': not an object! Dump: " << collection.dump();
        ASSERT_TRUE( collection.is_object() ) << "'" << key << "' should be an object: " << collection.dump();
        ASSERT_TRUE( json_object_keys( collection ).size() > 0 )
            << "Collection should have some records " << collection.dump();
    };

    for( size_t index = 0; index < collections.size(); index++ ) {
        const auto& name = collections[ index ];
        checkCollection( name, answer );
    }
}

}  // namespace Tests
}  // namespace HubPlatform
