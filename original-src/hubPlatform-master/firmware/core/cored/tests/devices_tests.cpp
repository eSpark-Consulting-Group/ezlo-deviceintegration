
#include "cored/core_service.h"
#include "cored/room_manager.h"
#include "cored/model/resources/cored_resources.h"

#include "cored/tests/service/test_core_service.h"

#include "database_client/dbqueryparser.h"

#include "common_utils/cpp14.h"
#include "cored/model/cored_item.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>

/*

TESTPLAN

DONE    1. Add device
DONE        1.1 Add device
DONE        1.2 Add child device
DONE        1.3 Add child device for non-existent parent
DONE        1.4 Add child device for child id
DONE    2. Rename device
DONE    3. Add device existed already
DONE    4. Add device to not empty DB already
DONE    5. Modify device
DONE        5.1 Modify device
DONE        5.2 Modify child device
DONE        5.1 Modify child device with non-existent parent
DONE        5.4 Modify child device with child id
DONE    6. Add item
DONE    7. Add user code item
DONE    8. add user code dictionary
DONE    9. Modify item
DONE    10. Modify user code item
DONE    11. Modify user code dictionary item
DONE    12. Modify gateway info field
DONE    13. Modify device setting
Done    14. Delete item value from HaTempItemsDBName after modify item

*/

namespace HubPlatform {
namespace UT {

using namespace Services;
using namespace CoreD::Model::Resources;

class DeviceManagerTestSuite : public ::testing::Test {
public:
    void SetUp() override {
        // Do not print errors and warnings on erroneous test cases
        // TODO: implement fake log and check error messages too.
        Log::setLocalLogLevel( Log::FATAL );

        m_service = std::make_unique< CoreD::Tests::TestCoreServiceWithoutHouseModes >();
        cleanDB();

        m_service->setUp();
    }

    void TearDown() override {
        cleanDB();
        m_service->tearDown();
    }

    CoreD::CoreService& getService() {
        return *m_service;
    }

    CoreD::DeviceManager& getDeviceManager() {
        return m_service->getDeviceManager();
    }

    void cleanDB() {
        getService().getDBClient().clean();
    }

    JsonObject decryptItem( const JsonObject& item ) {
        auto decryptedItem = item;
        CoreD::Model::Item::decryptItemAsJson( ProvisionDataUtils::getManager().getProvisionData(), decryptedItem );
        json_remove( decryptedItem, "_id" );
        return decryptedItem;
    }

    CoreD::CoreService::GatewayID ensureGateway( const Json& gate ) {
        auto gatewayId = getDeviceManager().ensureGateway( gate );
        return gatewayId[ "id" ];
    }

private:
    std::unique_ptr< CoreD::Tests::TestCoreServiceWithoutHouseModes > m_service;
};

TEST_F( DeviceManagerTestSuite, addDevice ) /* 1.1 */ {
    auto deviceID = getDeviceManager().addDevice( JSON_EMPTY_OBJ );

    EXPECT_EQ( deviceID.is_object(), 1 );
    EXPECT_EQ( deviceID.size(), 1 );
    EXPECT_NO_THROW( jsonCheckString( deviceID, "id" ) );
    EXPECT_EQ( getService().getDBClient().count( DBNames::DevicesDB, JSON_EMPTY_OBJ ), 1 );
    EXPECT_EQ( getService().getDBClient().count( HaTempDevicesDBName, JSON_EMPTY_OBJ ), 0 );
}

TEST_F( DeviceManagerTestSuite, addChildDevice ) /* 1.2 */ {
    auto deviceID = getDeviceManager().addDevice( JSON_EMPTY_OBJ );

    EXPECT_EQ( deviceID.is_object(), 1 );
    EXPECT_EQ( deviceID.size(), 1 );
    EXPECT_NO_THROW( jsonCheckString( deviceID, "id" ) );
    EXPECT_EQ( getService().getDBClient().count( DBNames::DevicesDB, JSON_EMPTY_OBJ ), 1 );
    EXPECT_EQ( getService().getDBClient().count( HaTempDevicesDBName, JSON_EMPTY_OBJ ), 0 );

    auto childDeviceID = getDeviceManager().addDevice( JSON( DeviceFields::ParentDeviceIdFieldName << deviceID ) );

    EXPECT_EQ( childDeviceID.is_object(), 1 );
    EXPECT_EQ( childDeviceID.size(), 1 );
    EXPECT_NO_THROW( jsonCheckString( childDeviceID, "id" ) );
    EXPECT_EQ( getService().getDBClient().count( DBNames::DevicesDB, JSON_EMPTY_OBJ ), 2 );
    EXPECT_EQ( getService().getDBClient().count( HaTempDevicesDBName, JSON_EMPTY_OBJ ), 0 );
}

TEST_F( DeviceManagerTestSuite, addChildForNonExistentParentDevice ) /* 1.3 */ {
    try {
        auto childDeviceID =
            getDeviceManager().addDevice( JSON( DeviceFields::ParentDeviceIdFieldName << "non-existent-device-id" ) );
        FAIL() << "Should throw an exception if there is no such parent device";
    }
    catch( const CoreD::DeviceManagerException& ex ) {
        SUCCEED();
    }
}

TEST_F( DeviceManagerTestSuite, addChildForExistentParentDeviceId ) /* 1.4 */ {
    auto parentDevice = getDeviceManager().addDevice( JSON_EMPTY_OBJ );

    EXPECT_EQ( parentDevice.is_object(), 1 );
    EXPECT_EQ( parentDevice.size(), 1 );
    EXPECT_NO_THROW( jsonCheckString( parentDevice, "id" ) );
    EXPECT_EQ( getService().getDBClient().count( DBNames::DevicesDB, JSON_EMPTY_OBJ ), 1 );
    EXPECT_EQ( getService().getDBClient().count( HaTempDevicesDBName, JSON_EMPTY_OBJ ), 0 );

    auto childDevice1 =
        getDeviceManager().addDevice( JSON( DeviceFields::ParentDeviceIdFieldName << parentDevice[ "id" ] ) );

    EXPECT_EQ( childDevice1.is_object(), 1 );
    EXPECT_EQ( childDevice1.size(), 1 );
    EXPECT_NO_THROW( jsonCheckString( childDevice1, "id" ) );
    EXPECT_EQ( getService().getDBClient().count( DBNames::DevicesDB, JSON_EMPTY_OBJ ), 2 );
    EXPECT_EQ( getService().getDBClient().count( HaTempDevicesDBName, JSON_EMPTY_OBJ ), 0 );

    EXPECT_THROW( getDeviceManager().addDevice( JSON( DeviceFields::ParentDeviceIdFieldName << childDevice1[ "id" ] ) ),
                  CoreD::NestedParentDevicesNotAllowedException );
}

TEST_F( DeviceManagerTestSuite, renameDeviceTest ) /* 2 */ {
    auto devices = getService().getDBClient().findAll( DBNames::DevicesDB, JSON_EMPTY_OBJ, nullptr );
    std::vector< JsonObject > ids;
    for( const auto& device : devices ) {
        auto deviceID = getDeviceManager().renameDevice(
            JSON( DeviceFields::IdFieldName << device[ "_id" ] << DeviceFields::NameFieldName << "device" ) );
        ids.push_back( deviceID );
    }
    EXPECT_EQ( getService().getDBClient().count( DBNames::DevicesDB, JSON( DeviceFields::NameFieldName << "device" ) ),
               ids.size() );

    for( const auto& device : devices ) {
        auto deviceID = getDeviceManager().renameDevice(
            JSON( DeviceFields::IdFieldName << device[ "_id" ] << DeviceFields::NameFieldName << "" ) );
    }
    EXPECT_EQ( getService().getDBClient().count( DBNames::DevicesDB, JSON( DeviceFields::NameFieldName << "" ) ), 0 );
    for( const auto& device : devices ) {
        auto deviceID = getDeviceManager().renameDevice( JSON( DeviceFields::IdFieldName
                                                               << device[ "_id" ] << DeviceFields::NameFieldName
                                                               << "zaqwsxedcrfvtgbyhnujmikolp123456789" ) );
    }
    EXPECT_EQ( getService().getDBClient().count(
                   DBNames::DevicesDB, JSON( DeviceFields::NameFieldName << "zaqwsxedcrfvtgbyhnujmikolp123456789" ) ),
               0 );
}

TEST_F( DeviceManagerTestSuite, addDeviceExistedAlreadyTest ) /* 3 */ {
    Json device = JSON_EMPTY_OBJ;
    device[ db::ID_FIELD_NAME ] = "device_id_1";

    getService().getDBClient().insert( DBNames::DevicesDB, device );

    auto deviceID = getDeviceManager().addDevice( device );

    EXPECT_EQ( deviceID.is_object(), 1 );
    EXPECT_EQ( deviceID.size(), 1 );
    ASSERT_NO_THROW( jsonCheckString( deviceID, "id" ) );

    auto string = json_get_str( deviceID, "id" );
    EXPECT_EQ( string, std::string{ "device_id_1" } );

    EXPECT_EQ( getService().getDBClient().count( DBNames::DevicesDB, JSON_EMPTY_OBJ ), 1 );
}

TEST_F( DeviceManagerTestSuite, addDeviceToNotEmptyDBAlreadyTest ) /* 4 */ {
    getService().getDBClient().insert( DBNames::DevicesDB, JSON( db::ID_FIELD_NAME << "device_id_2" ) );

    getDeviceManager().addDevice( JSON( db::ID_FIELD_NAME << "device_id_1" ) );

    EXPECT_EQ( getService().getDBClient().count( DBNames::DevicesDB, JSON_EMPTY_OBJ ), 2 );
}

TEST_F( DeviceManagerTestSuite, modifyDevice ) /* 5.1 */ {
    Json device = JSON_EMPTY_OBJ;
    device[ DeviceFields::NameFieldName ] = "test_name";
    device[ DeviceFields::SecurityFieldName ] = "test_security";
    device[ DeviceFields::CategoryFieldName ] = "test_category";
    device[ DeviceFields::SubcategoryFieldName ] = "test_subcategory";
    device[ DeviceFields::TypeFieldName ] = "test_type";
    device[ DeviceFields::ParentDeviceIdFieldName ] = "";
    device[ DeviceFields::DeviceTypeIdFieldName ] = "test_device_type_id";
    device[ DeviceFields::BatteryPoweredFieldName ] = false;

    Json deviceID;

    EXPECT_NO_THROW( deviceID = getDeviceManager().addDevice( device ) );

    std::string newName = "new_test_name";
    std::string newSecurity = "new_test_security";
    std::string newCategory = "new_test_category";
    std::string newSubcategory = "new_test_subcategory";
    std::string newType = "new_test_type";
    std::string newParentId = "";
    std::string newDeviceTypeId = "new_device_type_id";
    bool newBatteryPowered = true;

    device[ DeviceFields::NameFieldName ] = newName;
    device[ DeviceFields::SecurityFieldName ] = newSecurity;
    device[ DeviceFields::CategoryFieldName ] = newCategory;
    device[ DeviceFields::SubcategoryFieldName ] = newSubcategory;
    device[ DeviceFields::TypeFieldName ] = newType;
    device[ DeviceFields::ParentDeviceIdFieldName ] = newParentId;
    device[ DeviceFields::DeviceTypeIdFieldName ] = newDeviceTypeId;
    device[ DeviceFields::BatteryPoweredFieldName ] = newBatteryPowered;

    getDeviceManager().modifyDevice( deviceID[ "id" ], device );

    auto modifiedDevice = getService().getDBClient().findOne(
        DBNames::DevicesDB, JSON( db::ID_FIELD_NAME << deviceID[ "id" ] ), nullptr );

    EXPECT_EQ( modifiedDevice[ DeviceFields::NameFieldName ], newName );
    EXPECT_EQ( modifiedDevice[ DeviceFields::SecurityFieldName ], newSecurity );
    EXPECT_EQ( modifiedDevice[ DeviceFields::CategoryFieldName ], newCategory );
    EXPECT_EQ( modifiedDevice[ DeviceFields::SubcategoryFieldName ], newSubcategory );
    EXPECT_EQ( modifiedDevice[ DeviceFields::TypeFieldName ], newType );
    EXPECT_EQ( modifiedDevice[ DeviceFields::ParentDeviceIdFieldName ], newParentId );
    EXPECT_EQ( modifiedDevice[ DeviceFields::DeviceTypeIdFieldName ], newDeviceTypeId );
    EXPECT_EQ( modifiedDevice[ DeviceFields::BatteryPoweredFieldName ], newBatteryPowered );
}

TEST_F( DeviceManagerTestSuite, modifyChildDevice ) /* 5.2 */ {
    Json device = JSON_EMPTY_OBJ;
    device[ DeviceFields::NameFieldName ] = "test_name";
    device[ DeviceFields::SecurityFieldName ] = "test_security";
    device[ DeviceFields::CategoryFieldName ] = "test_category";
    device[ DeviceFields::SubcategoryFieldName ] = "test_subcategory";
    device[ DeviceFields::TypeFieldName ] = "test_type";
    device[ DeviceFields::DeviceTypeIdFieldName ] = "test_device_type_id";
    device[ DeviceFields::BatteryPoweredFieldName ] = false;

    Json deviceID;

    EXPECT_NO_THROW( deviceID = getDeviceManager().addDevice( device ) );

    Json childDevice = device;
    childDevice[ DeviceFields::ParentDeviceIdFieldName ] = json_get_str( deviceID, DeviceFields::IdFieldName );

    Json childDeviceID;
    EXPECT_NO_THROW( childDeviceID = getDeviceManager().addDevice( childDevice ) );

    std::string newName = "new_test_name";
    std::string newSecurity = "new_test_security";
    std::string newCategory = "new_test_category";
    std::string newSubcategory = "new_test_subcategory";
    std::string newType = "new_test_type";
    std::string newDeviceTypeId = "new_device_type_id";
    bool newBatteryPowered = true;

    childDevice[ DeviceFields::NameFieldName ] = newName;
    childDevice[ DeviceFields::SecurityFieldName ] = newSecurity;
    childDevice[ DeviceFields::CategoryFieldName ] = newCategory;
    childDevice[ DeviceFields::SubcategoryFieldName ] = newSubcategory;
    childDevice[ DeviceFields::TypeFieldName ] = newType;
    childDevice[ DeviceFields::ParentDeviceIdFieldName ] = json_get_str( deviceID, DeviceFields::IdFieldName );
    childDevice[ DeviceFields::DeviceTypeIdFieldName ] = newDeviceTypeId;
    childDevice[ DeviceFields::BatteryPoweredFieldName ] = newBatteryPowered;

    getDeviceManager().modifyDevice( childDeviceID[ "id" ], childDevice );

    auto modifiedDevice = getService().getDBClient().findOne(
        DBNames::DevicesDB, JSON( db::ID_FIELD_NAME << childDeviceID[ "id" ] ), nullptr );

    EXPECT_EQ( modifiedDevice[ DeviceFields::NameFieldName ], newName );
    EXPECT_EQ( modifiedDevice[ DeviceFields::SecurityFieldName ], newSecurity );
    EXPECT_EQ( modifiedDevice[ DeviceFields::CategoryFieldName ], newCategory );
    EXPECT_EQ( modifiedDevice[ DeviceFields::SubcategoryFieldName ], newSubcategory );
    EXPECT_EQ( modifiedDevice[ DeviceFields::TypeFieldName ], newType );
    EXPECT_EQ( modifiedDevice[ DeviceFields::ParentDeviceIdFieldName ],
               json_get_str( deviceID, DeviceFields::IdFieldName ) );
    EXPECT_EQ( modifiedDevice[ DeviceFields::DeviceTypeIdFieldName ], newDeviceTypeId );
    EXPECT_EQ( modifiedDevice[ DeviceFields::BatteryPoweredFieldName ], newBatteryPowered );
}

TEST_F( DeviceManagerTestSuite, modifyChildDeviceWithInvalidParent ) /* 5.3 */ {
    Json device = JSON_EMPTY_OBJ;
    device[ DeviceFields::NameFieldName ] = "test_name";
    device[ DeviceFields::SecurityFieldName ] = "test_security";
    device[ DeviceFields::CategoryFieldName ] = "test_category";
    device[ DeviceFields::SubcategoryFieldName ] = "test_subcategory";
    device[ DeviceFields::TypeFieldName ] = "test_type";
    device[ DeviceFields::ParentDeviceIdFieldName ] = "";
    device[ DeviceFields::DeviceTypeIdFieldName ] = "test_device_type_id";
    device[ DeviceFields::BatteryPoweredFieldName ] = false;

    Json deviceID;

    EXPECT_NO_THROW( deviceID = getDeviceManager().addDevice( device ) );

    Json childDevice = device;
    childDevice[ DeviceFields::ParentDeviceIdFieldName ] = json_get_str( deviceID, DeviceFields::IdFieldName );

    Json childDeviceID;
    EXPECT_NO_THROW( childDeviceID = getDeviceManager().addDevice( childDevice ) );

    std::string newName = "new_test_name";
    std::string newSecurity = "new_test_security";
    std::string newCategory = "new_test_category";
    std::string newSubcategory = "new_test_subcategory";
    std::string newType = "new_test_type";
    std::string newParentId = "new_test_parent_id";
    std::string newDeviceTypeId = "new_device_type_id";
    bool newBatteryPowered = true;

    childDevice[ DeviceFields::NameFieldName ] = newName;
    childDevice[ DeviceFields::SecurityFieldName ] = newSecurity;
    childDevice[ DeviceFields::CategoryFieldName ] = newCategory;
    childDevice[ DeviceFields::SubcategoryFieldName ] = newSubcategory;
    childDevice[ DeviceFields::TypeFieldName ] = newType;
    childDevice[ DeviceFields::ParentDeviceIdFieldName ] = newParentId;
    childDevice[ DeviceFields::DeviceTypeIdFieldName ] = newDeviceTypeId;
    childDevice[ DeviceFields::BatteryPoweredFieldName ] = newBatteryPowered;

    try {
        getDeviceManager().modifyDevice( childDeviceID[ "id" ], childDevice );
        FAIL() << "Should not modify a device with the invalid parentDeviceId property";
    }
    catch( const CoreD::DeviceManagerException& ex ) {
        SUCCEED();
    }
}

TEST_F( DeviceManagerTestSuite, modifyChildDeviceWithChildId ) { /* 5.4 */
    auto parentDevice = getDeviceManager().addDevice( JSON_EMPTY_OBJ );
    auto childDevice1 = getDeviceManager().addDevice( JSON_EMPTY_OBJ );
    auto childDevice2 = getDeviceManager().addDevice( JSON_EMPTY_OBJ );

    getDeviceManager().modifyDevice( childDevice1[ "id" ],
                                     JSON( DeviceFields::ParentDeviceIdFieldName << parentDevice[ "id" ] ) );
    EXPECT_THROW( getDeviceManager().modifyDevice(
                      childDevice2[ "id" ], JSON( DeviceFields::ParentDeviceIdFieldName << childDevice1[ "id" ] ) ),
                  CoreD::NestedParentDevicesNotAllowedException );
}

TEST_F( DeviceManagerTestSuite, addItem ) /* 6 */ {
    Json item = JsonObject::parse( R"(
    {
        "deviceId": "test_device_id",
        "enum": [
            "test_enum_1",
            "test_enum_2"
        ],
        "hasGetter": true,
        "hasSetter": true,
        "name": "test_name",
        "show": true,
        "type": "test_type",
        "value": 15,
        "valueMax": {
            "scale": "test_scale",
            "value": 20
        },
        "valueMin": {
            "scale": "test_scale",
            "value": 15
        },
        "info": {
            "zwave.references": [
                {
                    "channel": 0,
                    "class": 32,
                    "managing_commands": [ "BASIC_SET" ] 
                },
                {
                    "profile": "Meter:Electric",
                    "channel": 1,
                    "class": 48
                }
            ]
        }
    })" );

    auto itemId = getDeviceManager().addItem( item )[ "id" ];

    auto addedItem = getService().getDBClient().findOne( HaItemsDBName, {}, nullptr );

    auto expectedItem = item;
    expectedItem[ "_id" ] = itemId;
    EXPECT_EQ( expectedItem.dump( 4 ), addedItem.dump( 4 ) );
}

TEST_F( DeviceManagerTestSuite, addUserCodeItem ) /* 7 */ {
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

    getDeviceManager().addItem( item );

    auto addedItem = getService().getDBClient().findOne( HaItemsDBName, {}, nullptr );

    EXPECT_EQ( addedItem[ "value" ][ "code" ].get< std::string >().size(), 44 );
    EXPECT_EQ( item.dump( 4 ), decryptItem( addedItem ).dump( 4 ) );
}

TEST_F( DeviceManagerTestSuite, addUserCodeDictionaryItem ) {
    Json item = JsonObject::parse( R"(
    {
        "deviceId": "test_device_id",
        "type": "dictionary.userCode",
        "value": {
            "1": {
                "code": "9999",
                "name": "1"
            },
            "44": {
                "code": "4444",
                "name": "44"
            }
        }
    })" );

    getDeviceManager().addItem( item );

    auto addedItem = getService().getDBClient().findOne( HaItemsDBName, {}, nullptr );

    EXPECT_EQ( addedItem[ "value" ][ "1" ][ "code" ].get< std::string >().size(), 44 );
    EXPECT_EQ( addedItem[ "value" ][ "44" ][ "code" ].get< std::string >().size(), 44 );
    EXPECT_EQ( item.dump( 4 ), decryptItem( addedItem ).dump( 4 ) );
}

TEST_F( DeviceManagerTestSuite, modifyItem ) {
    Json item = JSON_EMPTY_OBJ;
    item[ ItemFields::DeviceIdFieldName ] = "test_device_id";
    item[ ItemFields::EnumFieldName ] = JSON_ARRAY( "test_enum_1"
                                                    << "test_enum_2" );
    item[ ItemFields::HasGetterFieldName ] = true;
    item[ ItemFields::HasSetterFieldName ] = true;
    item[ ItemFields::NameFieldName ] = "test_name";
    item[ ItemFields::ShowFieldName ] = true;
    item[ ItemFields::TypeFieldName ] = "test_type";
    item[ ItemFields::ValueFieldName ] = 15;
    item[ ItemFields::ValueMinFieldName ] = JSON( "value" << 15 << "scale"
                                                          << "test_scale" );
    item[ ItemFields::ValueMaxFieldName ] = JSON( "value" << 20 << "scale"
                                                          << "test_scale" );
    item[ ItemFields::InfoFieldName ] = JsonObject::parse( R"(
    {
        "zwave.references": [
            {
                "channel": 0,
                "class": 32,
                "managing_commands": [ "BASIC_SET" ] 
            },
            {
                "profile": "Meter:Electric",
                "channel": 1,
                "class": 48
            }
        ]
    })" );

    auto itemID = getDeviceManager().addItem( item );

    auto newDeviceId = "new_test_device_id";
    auto newEnum = JSON_ARRAY( "test_enum_3"
                               << "test_enum_4" );
    auto newHasGetter = false;
    auto newHasSetter = false;
    auto newName = "new_test_name";
    auto newShow = false;
    auto newType = "new_test_type";
    auto newValue = 20;
    auto newValueMin = JSON( "value" << 20 << "scale"
                                     << "test_scale" );
    auto newValueMax = JSON( "value" << 25 << "scale"
                                     << "test_scale" );
    auto newElementsMaxNumber = 250;
    auto newUserCodeRestriction = "^\\d{4,10}$";
    auto newUserCodeModes = JSON_ARRAY( "mode1"
                                        << "mode2" );
    auto newStringRestriction = "^\\d{5,10}$";

    auto newInfo = JsonObject::parse( R"(
    {
        "zwave.references": [
            {
                "channel": 0,
                "class": 67,
                "managing_commands": [ "THERMOSTAT_SETPOINT_SET" ]
            }
        ]
    })" );
    item[ ItemFields::DeviceIdFieldName ] = newDeviceId;
    item[ ItemFields::EnumFieldName ] = newEnum;
    item[ ItemFields::HasGetterFieldName ] = newHasGetter;
    item[ ItemFields::HasSetterFieldName ] = newHasSetter;
    item[ ItemFields::NameFieldName ] = newName;
    item[ ItemFields::ShowFieldName ] = newShow;
    item[ ItemFields::TypeFieldName ] = newType;
    item[ ItemFields::ValueFieldName ] = newValue;
    item[ ItemFields::ValueMinFieldName ] = newValueMin;
    item[ ItemFields::ValueMaxFieldName ] = newValueMax;
    item[ ItemFields::ElementsMaxNumberFieldName ] = newElementsMaxNumber;
    item[ ItemFields::UserCodeRestrictionFieldName ] = newUserCodeRestriction;
    item[ ItemFields::UserCodeModesFieldName ] = newUserCodeModes;
    item[ ItemFields::StringRestrictionFieldName ] = newStringRestriction;
    item[ ItemFields::InfoFieldName ] = newInfo;

    getDeviceManager().modifyItem( itemID[ "id" ], item );

    auto modifiedItem =
        getService().getDBClient().findOne( HaItemsDBName, JSON( db::ID_FIELD_NAME << itemID[ "id" ] ), nullptr );

    EXPECT_EQ( modifiedItem[ ItemFields::DeviceIdFieldName ], newDeviceId );
    EXPECT_EQ( modifiedItem[ ItemFields::EnumFieldName ], newEnum );
    EXPECT_EQ( modifiedItem[ ItemFields::HasGetterFieldName ], newHasGetter );
    EXPECT_EQ( modifiedItem[ ItemFields::HasSetterFieldName ], newHasSetter );
    EXPECT_EQ( modifiedItem[ ItemFields::NameFieldName ], newName );
    EXPECT_EQ( modifiedItem[ ItemFields::ShowFieldName ], newShow );
    EXPECT_EQ( modifiedItem[ ItemFields::TypeFieldName ], newType );
    EXPECT_EQ( modifiedItem[ ItemFields::ValueFieldName ], newValue );
    EXPECT_EQ( modifiedItem[ ItemFields::ValueMinFieldName ], newValueMin );
    EXPECT_EQ( modifiedItem[ ItemFields::ValueMaxFieldName ], newValueMax );
    EXPECT_EQ( modifiedItem[ ItemFields::ElementsMaxNumberFieldName ], newElementsMaxNumber );
    EXPECT_EQ( modifiedItem[ ItemFields::UserCodeRestrictionFieldName ], newUserCodeRestriction );
    EXPECT_EQ( modifiedItem[ ItemFields::UserCodeModesFieldName ], newUserCodeModes );
    EXPECT_EQ( modifiedItem[ ItemFields::StringRestrictionFieldName ], newStringRestriction );
    EXPECT_EQ( modifiedItem[ ItemFields::InfoFieldName ], newInfo );
}

TEST_F( DeviceManagerTestSuite, modifyUserCodeItem ) {
    Json item = JsonObject::parse( R"(
    {
        "deviceId": "test_device_id",
        "type": "userCode",
        "value": {
            "code": "1984",
            "name": "1"
        }
    })" );

    auto itemID = getDeviceManager().addItem( item );
    auto addedItem = getService().getDBClient().findOne( HaItemsDBName, {}, nullptr );

    item[ "value" ][ "code" ] = "1234";

    getDeviceManager().modifyItem( itemID[ "id" ], item );
    auto modifiedItem = getService().getDBClient().findOne( HaItemsDBName, {}, nullptr );

    EXPECT_NE( addedItem.dump( 4 ), modifiedItem.dump( 4 ) );
    EXPECT_EQ( modifiedItem[ "value" ][ "code" ].get< std::string >().size(), 44 );
    EXPECT_EQ( item.dump( 4 ), decryptItem( modifiedItem ).dump( 4 ) );
}

TEST_F( DeviceManagerTestSuite, modifyUserCodeDictionaryItem ) {
    Json item = JsonObject::parse( R"(
    {
        "deviceId": "test_device_id",
        "type": "dictionary.userCode",
        "value": {
            "1": {
                "code": "9999",
                "name": "1"
            },
            "44": {
                "code": "4444",
                "name": "44"
            }
        }
    })" );

    auto itemId = getDeviceManager().addItem( item )[ "id" ];
    auto addedItem = getService().getDBClient().findOne( HaItemsDBName, {}, nullptr );

    item[ "value" ][ "17" ] = JsonObject::parse( R"({
        "code": "4567",
        "name": "17"
    } )" );

    getDeviceManager().modifyItem( itemId, item );
    auto modifiedItem = getService().getDBClient().findOne( HaItemsDBName, {}, nullptr );

    EXPECT_NE( addedItem.dump( 4 ), modifiedItem.dump( 4 ) );
    EXPECT_EQ( modifiedItem[ "value" ][ "1" ][ "code" ].get< std::string >().size(), 44 );
    EXPECT_EQ( modifiedItem[ "value" ][ "17" ][ "code" ].get< std::string >().size(), 44 );
    EXPECT_EQ( modifiedItem[ "value" ][ "44" ][ "code" ].get< std::string >().size(), 44 );
    EXPECT_EQ( item.dump( 4 ), decryptItem( modifiedItem ).dump( 4 ) );
}

TEST_F( DeviceManagerTestSuite, modifyGatewayInfoField ) {
    const auto gatewayId = ensureGateway( JSON( GatewayFields::NameFieldName << "gateway" ) );

    auto addedGateway = getService().getDBClient().findOne( DBNames::GatewaysDB, {}, nullptr );

    auto infoField = JSON( "info" << JSON( "zwave.role"
                                           << "primary" ) );

    getDeviceManager().modifyGateway( gatewayId, infoField );
    auto modifiedGateway = getService().getDBClient().findOne( DBNames::GatewaysDB, {}, nullptr );

    EXPECT_NE( addedGateway.dump( 4 ), modifiedGateway.dump( 4 ) );
    EXPECT_NE( modifiedGateway.find( "info" ), modifiedGateway.end() );
    EXPECT_TRUE( modifiedGateway[ "info" ].is_object() );
    EXPECT_NE( modifiedGateway[ "info" ].find( "zwave.role" ), modifiedGateway[ "info" ].end() );
    EXPECT_EQ( modifiedGateway[ "info" ][ "zwave.role" ], "primary" );

    infoField[ "info" ][ "zwave.role" ] = "secondary";
    getDeviceManager().modifyGateway( gatewayId, infoField );
    modifiedGateway = getService().getDBClient().findOne( DBNames::GatewaysDB, {}, nullptr );

    EXPECT_NE( addedGateway.dump( 4 ), modifiedGateway.dump( 4 ) );
    EXPECT_NE( modifiedGateway.find( "info" ), modifiedGateway.end() );
    EXPECT_TRUE( modifiedGateway[ "info" ].is_object() );
    EXPECT_NE( modifiedGateway[ "info" ].find( "zwave.role" ), modifiedGateway[ "info" ].end() );
    EXPECT_EQ( modifiedGateway[ "info" ][ "zwave.role" ], "secondary" );
}

TEST_F( DeviceManagerTestSuite, modifyDeviceSetting ) {
    Json setting = JSON( DeviceSettingFields::DeviceId
                         << "test_device_id" << DeviceSettingFields::Label << JSON( TextElementFields::Text << "label" )
                         << DeviceSettingFields::Description << JSON( TextElementFields::Text << "description" )
                         << DeviceSettingFields::ValueType << "int" << DeviceSettingFields::Value << 5
                         << DeviceSettingFields::ValueMin << 0 << DeviceSettingFields::ValueMax << 10 );

    auto settingId = getDeviceManager().addDeviceSetting( setting )[ "id" ];

    setting[ DeviceSettingFields::DeviceId ] = "modified_device_id";
    setting[ DeviceSettingFields::ValueMax ] = 123;

    getDeviceManager().modifySetting( settingId, setting );

    auto modifiedSetting =
        getService().getDBClient().findOne( HaDeviceSettingsDBName, JSON( db::ID_FIELD_NAME << settingId ), nullptr );

    EXPECT_EQ( modifiedSetting[ DeviceSettingFields::ValueMax ], 123 );
    EXPECT_EQ( modifiedSetting[ DeviceSettingFields::DeviceId ], "modified_device_id" );
}

TEST_F( DeviceManagerTestSuite, addChildDeviceWithRoomTest ) {
    auto parentDeviceID = getDeviceManager().addDevice( JSON( "roomId"
                                                              << "TestRoomId" ) );
    auto childDeviceID = getDeviceManager().addDevice( JSON( "parentDeviceId" << parentDeviceID[ "id" ] ) );

    auto childDevice =
        getService().getDBClient().findOne( DBNames::DevicesDB, JSON( "_id" << childDeviceID[ "id" ] ), nullptr );

    ASSERT_FALSE( childDevice.empty() );
    EXPECT_EQ( childDevice[ "roomId" ], "TestRoomId" );
}

TEST_F( DeviceManagerTestSuite, deleteItemValueFromHaTempItemsDBNameAfterModifyItem ) {  // 14
    Json item = JSON_EMPTY_OBJ;
    item[ ItemFields::DeviceIdFieldName ] = "test_device_id";
    item[ ItemFields::EnumFieldName ] = JSON_ARRAY( "test_enum_1"
                                                    << "test_enum_2" );
    item[ ItemFields::HasGetterFieldName ] = true;
    item[ ItemFields::HasSetterFieldName ] = true;
    item[ ItemFields::NameFieldName ] = "test_name";
    item[ ItemFields::ShowFieldName ] = true;
    item[ ItemFields::TypeFieldName ] = "test_type";
    item[ ItemFields::ValueFieldName ] = 15;
    item[ ItemFields::ValueMinFieldName ] = JSON( "value" << 15 << "scale"
                                                          << "test_scale" );
    item[ ItemFields::ValueMaxFieldName ] = JSON( "value" << 20 << "scale"
                                                          << "test_scale" );

    auto itemID = getDeviceManager().addItem( item );

    EXPECT_TRUE( getService().getDBClient().has( HaTempItemsDBName, JSON( db::ID_FIELD_NAME << itemID[ "id" ] ) ) );

    auto newDeviceId = "new_test_device_id";
    auto newEnum = JSON_ARRAY( "test_enum_3"
                               << "test_enum_4" );
    auto newHasGetter = false;
    auto newHasSetter = false;
    auto newName = "new_test_name";
    auto newShow = false;
    auto newType = "new_test_type";
    auto newValue = 20;
    auto newValueMin = JSON( "value" << 20 << "scale"
                                     << "test_scale" );
    auto newValueMax = JSON( "value" << 25 << "scale"
                                     << "test_scale" );
    auto newElementsMaxNumber = 250;
    auto newUserCodeRestriction = "^\\d{4,10}$";
    auto newUserCodeModes = JSON_ARRAY( "mode1"
                                        << "mode2" );
    auto newStringRestriction = "^\\d{5,10}$";

    item[ ItemFields::DeviceIdFieldName ] = newDeviceId;
    item[ ItemFields::EnumFieldName ] = newEnum;
    item[ ItemFields::HasGetterFieldName ] = newHasGetter;
    item[ ItemFields::HasSetterFieldName ] = newHasSetter;
    item[ ItemFields::NameFieldName ] = newName;
    item[ ItemFields::ShowFieldName ] = newShow;
    item[ ItemFields::TypeFieldName ] = newType;
    item[ ItemFields::ValueFieldName ] = newValue;
    item[ ItemFields::ValueMinFieldName ] = newValueMin;
    item[ ItemFields::ValueMaxFieldName ] = newValueMax;
    item[ ItemFields::ElementsMaxNumberFieldName ] = newElementsMaxNumber;
    item[ ItemFields::UserCodeRestrictionFieldName ] = newUserCodeRestriction;
    item[ ItemFields::UserCodeModesFieldName ] = newUserCodeModes;
    item[ ItemFields::StringRestrictionFieldName ] = newStringRestriction;

    getDeviceManager().modifyItem( itemID[ "id" ], item );

    EXPECT_FALSE( getService().getDBClient().has( HaTempItemsDBName, JSON( db::ID_FIELD_NAME << itemID[ "id" ] ) ) );
}

}  // namespace UT
}  // namespace HubPlatform
