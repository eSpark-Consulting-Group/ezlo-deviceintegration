#include "nma_api_tests/application.h"
#include "cored/model/cored_device.h"
#include "cored/model/cored_gateway.h"
#include "cored/model/cored_item.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/value_factory.h"
#include "cored/house_modes/house_mode_utils.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/remote_interface/cored_bus_remote_interface.h"
#include "cored/exceptions.h"
#include "cored/model/values/values.h"
#include "jsonrpc/jsonrpc.h"
#include "common_utils/hub_info_helper.h"
#include "bus/base_bus.h"

#include "common_test_resources/fixtures/core_test_fixture.h"
#include "common_test_resources/resources/cored_resources.h"
#include "hub_settings/hub_settings_reader.h"

#include <gtest/gtest.h>
#include <database_client/dbqueryparser.h>

/*

TESTPLAN

DONE    1. Core
DONE        1.1 EFWR_1446_switch_on_all_sirens_on_security_threat
DONE        1.2 EFWR_1481_Broadcast_for_changed_Thermostat_Setpoint_are_not_sent
DONE        1.3 EFWR_1600_Device_ready
DONE        1.4 setServiceNotifications
DONE        1.5 hubUserNotificationPinCodeBroadcast
DONE            1.5.1 Default core remote interface
DONE            1.5.2 Bus core remote interface
DONE        1.6 hubUserNotificationPinCodeActionBroadcast
DONE        1.7 hubItemDictionaryUpdated
DONE        1.8 Events Binding
DONE        1.9 Item update by using lua bindings
DONE            1.9.1 Item update by using lua bindings
DONE            1.9.2 Item update by using lua bindings with the source field
DONE            1.9.3 Item update in batch by using lua bindings
DONE            1.9.4 Item update in batch by using lua bindings with the source field
DONE        1.10 Get Local Time by using lua bindings
DONE        1.11 Get Location by using lua bindings
DONE        1.12 Get Timezone by using lua bindings
DONE        1.13 Get Hub info by using lua bindings
DONE        1.14 Add a "deviceId" field to item.removed broadcast
DONE        1.15 Device::persistent: boolean field, create device from remoteInterface
DONE        1.16 Device::persistent: boolean field, create device from lua script
DONE        1.17 device::persistent: throw error in cored on forceRemove operation.
DONE        1.18 Item adding with value indent option
DONE            1.18.1 Item adding success case
DONE            1.18.2 Empty indented item
DONE            1.18.3 Empty min indent
DONE            1.18.4 Wrong indented item
DONE        1.19 Item modification with value indent option
DONE            1.19.1 Item modification success case
DONE            1.19.2 Empty indented item
DONE            1.19.3 Empty min indent
DONE            1.19.4 Wrong indented item
DONE        1.20 Switch on all cameras on security_threat
DONE        1.21 Remove security_threat item, remove device from house modes
DONE        1.22 Remove security_threat item from armed device, device.armed should become false
DONE    2 Rooms lua bindings
DONE        2.1 core.get_rooms lua binding
DONE        2.2 core.get_room lua binding
DONE        3.1 core.testItemValue lua binding
DONE    4. Parent room property bindings
DONE        4.1 Check in core.get_devices (parent devices) binding
DONE        4.2 Check in core.get_devices (child devices) binding
DONE        4.3 Check in core.get_device (parent device) binding
DONE        4.4 Check in core.get_device (child device) binding

*/

namespace HubPlatform {
namespace Tests {

using namespace CoreD;
using namespace CoreDHouseModes;
using namespace Services;

class CoredServiceTests : public CoreNmaTest {
public:
    void SetUp() override;
    void TearDown() override;
    static void SetUpTestCase();
};

void CoredServiceTests::SetUp() {
    CoreNmaTest::SetUp();
    clearExistingDevices( __LINE__ );
    callingFile( __FILE__ );
}

void CoredServiceTests::TearDown() {
    clearExistingDevices( __LINE__ );
    CoreNmaTest::TearDown();
}

void CoredServiceTests::SetUpTestCase() {
    CoreDHouseModes::setAlarmPhase( nma().getDBClient(), "idle", "" );
}

TEST_F( CoredServiceTests, EFWR_1446_switch_on_all_sirens_on_security_threat ) {  // 1.1
    JsonObject broadcast;
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );
    auto& dbClient = nma().getDBClient();
    CoreDHouseModes::setAlarmDelay( dbClient, 1 );

    const std::string gatewayId = getMainGateway( remoteInterface, __LINE__ );

    Model::Device device1( gatewayId, "Device 1", "sensor", "0_0_7", "security_sensor", false );
    device1.setSubcategory( "co" );
    auto deviceAddResult1 = remoteInterface.addDevice( device1 );
    EXPECT_FALSE( deviceAddResult1.first.empty() );
    EXPECT_TRUE( deviceAddResult1.second );
    broadcast = waitForBroadcast( "hub.device.added", NMA_TEST_MAKE_MESSAGE( "hub.device.added" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.added" );

    Model::Item item1( deviceAddResult1.first,
                       "security_threat",
                       Model::ValueType::Enum::Bool,
                       true,
                       false,
                       Model::ValueFactory::createDefaultItemValue( Model::ValueType::Enum::Bool ) );
    auto itemAddResult1 = remoteInterface.addItem( item1 );
    EXPECT_FALSE( itemAddResult1.first.empty() );
    EXPECT_TRUE( itemAddResult1.second );
    broadcast = waitForBroadcast( "hub.item.added", NMA_TEST_MAKE_MESSAGE( "no broadcast" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.added" );
    broadcast = waitForBroadcast( "hub.device.updated", NMA_TEST_MAKE_MESSAGE( "device.updated" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );

    Model::Device device2( gatewayId, "Device 2", "siren", "0_7_0", "siren", false );
    auto deviceAddResult2 = remoteInterface.addDevice( device2 );
    EXPECT_FALSE( deviceAddResult2.first.empty() );
    EXPECT_TRUE( deviceAddResult2.second );
    const auto& sirenDeviceId = deviceAddResult2.first;
    broadcast = waitForBroadcast( "hub.device.added", NMA_TEST_MAKE_MESSAGE( "no broadcast" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.added" );

    Model::Item item2( sirenDeviceId,
                       "siren",
                       Model::ValueType::Enum::Bool,
                       true,
                       true,
                       Model::ValueFactory::createDefaultItemValue( Model::ValueType::Enum::Bool ) );
    auto itemAddResult2 = remoteInterface.addItem( item2 );
    EXPECT_FALSE( itemAddResult2.first.empty() );
    EXPECT_TRUE( itemAddResult2.second );
    broadcast = waitForBroadcast( "hub.item.added", NMA_TEST_MAKE_MESSAGE( "no broadcast" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.added" );

    auto houseModes = remoteInterface.getHouseModes();
    auto currentModeId = remoteInterface.getCurrentHouseModeId();

    auto sirenItemBefore =
        dbClient.findOne( Model::Resources::DBNames::TempItemsDB, JSON( "_id" << itemAddResult2.first ), nullptr );
    bool valueBefore;
    EXPECT_TRUE( json_get< bool >( sirenItemBefore, Model::Resources::ItemFields::ValueFieldName, valueBefore ) );
    EXPECT_FALSE( valueBefore );

    LOGGER_TRACE( "House modes after removing siren from alarmsOffDevices:" );
    houseModes = remoteInterface.getHouseModes();
    for( const auto& houseMode : houseModes ) {
        if( houseMode.m_id == currentModeId ) {
            LOGGER_TRACE( cc::trace( "House mode: " ).append( cc::j( houseMode.toJson() ) ) );
            break;
        }
    }

    remoteInterface.updateItemValue( itemAddResult1.first, []( CoreD::Model::ValueType valueType ) {
        return CoreD::Model::ValueFactory::createItemValue( valueType, Json( true ) );
    } );
    broadcast = waitForBroadcast( "hub.item.updated", NMA_TEST_MAKE_MESSAGE( "no broadcast" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" );
    EXPECT_EQ( json_get_str( broadcast[ "result" ], "_id" ), itemAddResult1.first );
    EXPECT_EQ( json_get_str( broadcast[ "result" ], "name" ), "security_threat" );
    EXPECT_EQ( json_get_bool( broadcast[ "result" ], "value" ), true );

    testHouseModeAlarmedPhaseStatusBroadcast( currentModeId, deviceAddResult1.first, "entryDelay", "begin", __LINE__ );
    testHouseModeAlarmedPhaseStatusBroadcast( currentModeId, deviceAddResult1.first, "entryDelay", "done", __LINE__ );
    testHouseModeAlarmedPhaseStatusBroadcast( currentModeId, deviceAddResult1.first, "main", "begin", __LINE__ );

    broadcast = waitForBroadcast( "hub.item.updated", NMA_TEST_MAKE_MESSAGE( "no broadcast" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" );
    EXPECT_EQ( json_get_str( broadcast[ "result" ], "_id" ), itemAddResult2.first );
    EXPECT_EQ( json_get_str( broadcast[ "result" ], "name" ), "siren" );
    EXPECT_EQ( json_get_bool( broadcast[ "result" ], "value" ), true );

    auto sirenItemAfter =
        dbClient.findOne( Model::Resources::DBNames::TempItemsDB, JSON( "_id" << itemAddResult2.first ), nullptr );
    bool valueAfter;
    EXPECT_TRUE( json_get< bool >( sirenItemAfter, Model::Resources::ItemFields::ValueFieldName, valueAfter ) );
    EXPECT_TRUE( valueAfter );

    remoteInterface.removeDevice( deviceAddResult1.first );
    broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", NMA_TEST_MAKE_MESSAGE( "no broadcast" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );

    remoteInterface.removeDevice( deviceAddResult2.first );
    broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", NMA_TEST_MAKE_MESSAGE( "no broadcast" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
}

TEST_F( CoredServiceTests, EFWR_1481_Broadcast_for_changed_Thermostat_Setpoint_are_not_sent ) {  // 1.2
    JsonObject broadcast;
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );

    const std::string gatewayId = getMainGateway( remoteInterface, __LINE__ );

    Model::Device thermostat( gatewayId, "Z-Wave Thermostat", "thermostat", "139_21586_21570", "hvac", false );
    thermostat.setSubcategory( "hvac" );
    auto thermostatAddResult = remoteInterface.addDevice( thermostat );
    EXPECT_FALSE( thermostatAddResult.first.empty() );
    EXPECT_TRUE( thermostatAddResult.second );
    broadcast = waitForBroadcast( "hub.device.added",
                                  std::string( __FILE__ ) + ":" + std::to_string( __LINE__ ) + " - no broadcast" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.added" );

    auto makeValueLoader = []( const Json& source ) -> CoreDRemoteInterface::ValueLoader {
        return [&source]( CoreD::Model::ValueType valueType ) {
            return CoreD::Model::ValueFactory::createItemValue( valueType, source );
        };
    };

    const std::string scale = Model::ValueScales::Celsius;
    double valueInScale = 18;
    auto value = Model::ValueFactory::createItemValue( Model::ValueType::Enum::Temperature,
                                                       JSON( "value" << valueInScale << "scale" << scale ) );

    Model::Item setpointItem( thermostatAddResult.first,
                              "thermostat_setpoint",
                              Model::ValueType::Enum::Temperature,
                              true,
                              true,
                              std::move( value ) );

    auto setpointAddResult = remoteInterface.addItem( setpointItem );
    EXPECT_FALSE( setpointAddResult.first.empty() );
    EXPECT_TRUE( setpointAddResult.second );
    broadcast = waitForBroadcast( "hub.item.added",
                                  std::string( __FILE__ ) + ":" + std::to_string( __LINE__ ) + " - no broadcast" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.added" );

    compareTemperatureValues( JSON( "value" << valueInScale << "scale" << scale ),
                              broadcast[ "result" ],
                              "value",
                              std::string{ __FILE__ } + " " + std::to_string( __LINE__ ) );

    const int newSetpointValue = 27;
    Json joNewVal = JSON( "value" << newSetpointValue << "scale" << scale );
    Json joMinVal = JSON( "value" << 0 << "scale" << scale );
    Json joMaxVal = JSON( "value" << 40 << "scale" << scale );

    remoteInterface.updateItemValueWithMinMax( setpointAddResult.first,
                                               makeValueLoader( joNewVal ),
                                               makeValueLoader( joMinVal ),
                                               makeValueLoader( joMaxVal ) );

    auto theFile = std::string{ __FILE__ } + ": line ";
    broadcast = waitForBroadcast( "hub.item.updated", theFile + std::to_string( __LINE__ ) + " - no broadcast" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" );

    Json result;
    ASSERT_TRUE( json_get_obj( broadcast, "result", result ) );

    EXPECT_EQ( json_get_str( result, "name" ), "thermostat_setpoint" );
    EXPECT_EQ( json_get_str( result, "_id" ), setpointAddResult.first );

    compareTemperatureValues( joNewVal, result, "value", theFile + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast[ "result" ], "valueType" ), "temperature" );

    // TODO: minValue, maxValue
    using namespace HubPlatform::CoreD::Model::Resources::ItemFields::Response;
    compareTemperatureValues( joMinVal, result, MinValueFieldName, theFile + std::to_string( __LINE__ ) );
    compareTemperatureValues( joMaxVal, result, MaxValueFieldName, theFile + std::to_string( __LINE__ ) );

    remoteInterface.removeDevice( thermostatAddResult.first );
    broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", theFile + std::to_string( __LINE__ ) + " - no broadcast" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
}

TEST_F( CoredServiceTests, EFWR_1600_Device_ready ) {  // 1.3
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );
    auto itemId =
        addItem( coreRemoteInterface, deviceId, "testItem", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    auto setItemValueRequest = JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << 1 );
    EXPECT_NO_THROW( coreRemoteInterface.setItemValue( setItemValueRequest ) );
    checkItemUpdateBroadcast( itemId, 1, __LINE__ );

    coreRemoteInterface.updateReadyState( deviceId, false );
    checkDeviceUpdateBroadcast( deviceId, false, __LINE__ );
    setItemValueRequest[ ItemFields::ValueFieldName ] = 2;
    EXPECT_THROW( coreRemoteInterface.setItemValue( setItemValueRequest ), CoreD::DeviceBusy );

    coreRemoteInterface.updateReadyState( deviceId, true );
    checkDeviceUpdateBroadcast( deviceId, true, __LINE__ );
    EXPECT_NO_THROW( coreRemoteInterface.setItemValue( setItemValueRequest ) );
    checkItemUpdateBroadcast( itemId, 2, __LINE__ );

    coreRemoteInterface.removeDevice( deviceId );
    JsonObject broadcast;
    broadcast = waitForBroadcast( "hub.item.removed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
}

TEST_F( CoredServiceTests, setServiceNotifications ) {  // 1.4
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );
    auto itemId =
        addItem( coreRemoteInterface, deviceId, "testItem", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    auto setItemValueRequest = JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << 1 );
    EXPECT_NO_THROW( coreRemoteInterface.setItemValue( setItemValueRequest ) );
    checkItemUpdateBroadcast( itemId, 1, false, __LINE__ );
    const JsonObject setServiceNotifications = R"(
    {
        "method": "hub.devices.service.notifications.set",
        "id": "_ID_",
        "params": {
            "ids" : [ "__DEVICE_ID__" ]
        }
    }
    )"_json;

    auto answer =
        nma().sendtoHubAndGetAnswer( replaceTokens( setServiceNotifications, { { "__DEVICE_ID__", deviceId } } ) );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ServiceNotificationFieldName, true } }, __LINE__ );
    setItemValueRequest[ ItemFields::ValueFieldName ] = 2;
    EXPECT_NO_THROW( coreRemoteInterface.setItemValue( setItemValueRequest ) );
    checkItemUpdateBroadcast( itemId, 2, true, __LINE__ );

    answer =
        nma().sendtoHubAndGetAnswer( replaceTokens( setServiceNotifications, { { "__DEVICE_ID__", "deviceId" } } ) );
    error = answer[ "error" ];
    EXPECT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.device.notfound" );
    EXPECT_EQ( json_get_str( error, "message" ), "No device found!" );
    checkNoBroadcastReceived( "", __LINE__, std::chrono::seconds( 5 ) );

    answer = nma().sendtoHubAndGetAnswer( replaceTokens( setServiceNotifications, { { "__DEVICE_ID__", deviceId } } ) );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    checkNoBroadcastReceived( "", __LINE__, std::chrono::seconds( 5 ) );
    setItemValueRequest[ ItemFields::ValueFieldName ] = 3;
    EXPECT_NO_THROW( coreRemoteInterface.setItemValue( setItemValueRequest ) );
    checkItemUpdateBroadcast( itemId, 3, true, __LINE__ );

    auto request = replaceTokens( setServiceNotifications, { { "__DEVICE_ID__", "" } } );
    request[ "params" ][ "ids" ] = JSON_EMPTY_ARR;
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ServiceNotificationFieldName, false } }, __LINE__ );
    setItemValueRequest[ ItemFields::ValueFieldName ] = 4;
    EXPECT_NO_THROW( coreRemoteInterface.setItemValue( setItemValueRequest ) );
    checkItemUpdateBroadcast( itemId, 4, false, __LINE__ );

    coreRemoteInterface.removeDevice( deviceId );
    JsonObject broadcast, result;
    broadcast = waitForBroadcast( "hub.item.removed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    EXPECT_EQ( json_get_bool( result, "serviceNotification" ), false );
}

TEST_F( CoredServiceTests, hubUserNotificationPinCodeBroadcastDefaultCoreRemoteInterface ) {  // 1.5.1
    using namespace CoreD::Model;
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "Touchscreen Deadbolt",
                               "doorlock",
                               "297_32770_1536",
                               "door_lock",
                               "",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto userLockOperationId = addItem( coreRemoteInterface,
                                        deviceId,
                                        "user_lock_operation",
                                        ValueType::Enum::UserLockOperation,
                                        true,
                                        true,
                                        __LINE__ );

    auto userCodesId = addItem( coreRemoteInterface,
                                deviceId,
                                "user_codes",
                                ValueType::makeDictionaryType( ValueType::Enum::UserCode ),
                                true,
                                true,
                                __LINE__ );

    EXPECT_EQ( coreRemoteInterface.getItemDictionaryValue( userCodesId, "1" ), Json() );

    const std::string newCode = R"( { "code": "2812", "name": "One" } )";
    const auto addCodesRequest = replaceTokens(
        hubItemDictionaryValueAdd,
        { { "__ITEM_ID__", userCodesId }, { "__TYPE__", "object" }, { "__VALUE__", newCode } } )[ "params" ];

    EXPECT_NO_THROW( coreRemoteInterface.addItemDictionaryValue( addCodesRequest ) );

    JsonObject element = JSON( "1" << JSON( "value" << JsonObject::parse( newCode ) ) );
    checkItemDictionaryUpdatedBroadcast( userCodesId, { { "_id", userCodesId }, { "element", element } }, __LINE__ );

    EXPECT_EQ( coreRemoteInterface.getItemDictionaryValue( userCodesId, "1" ), JsonObject::parse( newCode ) );

    const JsonObject lockDoorLock = R"( { "action": "lock", "userId": 1 } )"_json;
    auto setItemValueRequest =
        JSON( ItemFields::IdFieldName << userLockOperationId << ItemFields::ValueFieldName << lockDoorLock );
    EXPECT_NO_THROW( coreRemoteInterface.setItemValue( setItemValueRequest ) );
    checkItemUpdateBroadcast( userLockOperationId, lockDoorLock, __LINE__ );
    checkUserNotificationBroadcast( "Touchscreen Deadbolt", "", "One", __LINE__ );

    removeDevice( coreRemoteInterface, deviceId, 2, 0, __LINE__ );
}

TEST_F( CoredServiceTests, hubUserNotificationPinCodeBroadcastBusCoreRemoteInterface ) {  // 1.5.2
    using namespace CoreD::Model;
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDBusRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "Touchscreen Deadbolt",
                               "doorlock",
                               "297_32770_1536",
                               "door_lock",
                               "",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto userLockOperationId = addItem( coreRemoteInterface,
                                        deviceId,
                                        "user_lock_operation",
                                        ValueType::Enum::UserLockOperation,
                                        true,
                                        true,
                                        __LINE__ );

    auto userCodesId = addItem( coreRemoteInterface,
                                deviceId,
                                "user_codes",
                                ValueType::makeDictionaryType( ValueType::Enum::UserCode ),
                                true,
                                true,
                                __LINE__ );

    EXPECT_EQ( coreRemoteInterface.getItemDictionaryValue( userCodesId, "1" ), Json() );

    const std::string newCode = R"( { "code": "2812", "name": "One" } )";
    const auto addCodesRequest = replaceTokens(
        hubItemDictionaryValueAdd,
        { { "__ITEM_ID__", userCodesId }, { "__TYPE__", "object" }, { "__VALUE__", newCode } } )[ "params" ];

    EXPECT_NO_THROW( coreRemoteInterface.addItemDictionaryValue( addCodesRequest ) );

    JsonObject element = JSON( "1" << JSON( "value" << JsonObject::parse( newCode ) ) );
    checkItemDictionaryUpdatedBroadcast( userCodesId, { { "_id", userCodesId }, { "element", element } }, __LINE__ );

    EXPECT_EQ( coreRemoteInterface.getItemDictionaryValue( userCodesId, "1" ), JsonObject::parse( newCode ) );

    const JsonObject lockDoorLock = R"( { "action": "lock", "userId": 1 } )"_json;
    auto setItemValueRequest =
        JSON( ItemFields::IdFieldName << userLockOperationId << ItemFields::ValueFieldName << lockDoorLock );
    EXPECT_NO_THROW( coreRemoteInterface.setItemValue( setItemValueRequest ) );
    checkItemUpdateBroadcast( userLockOperationId, lockDoorLock, __LINE__ );
    checkUserNotificationBroadcast( "Touchscreen Deadbolt", "", "One", __LINE__ );

    removeDevice( coreRemoteInterface, deviceId, 2, 0, __LINE__ );
}

TEST_F( CoredServiceTests, hubUserNotificationPinCodeActionBroadcast ) {  // 1.6
    using namespace CoreD::Model;
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "Touchscreen Deadbolt",
                               "doorlock",
                               "297_32770_1536",
                               "door_lock",
                               "",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto userCodeActionId = addItem(
        coreRemoteInterface, deviceId, "user_code_action", ValueType::Enum::UserCodeAction, true, true, __LINE__ );

    auto userCodesId = addItem( coreRemoteInterface,
                                deviceId,
                                "user_codes",
                                ValueType::makeDictionaryType( ValueType::Enum::UserCode ),
                                true,
                                true,
                                __LINE__ );

    const std::string newCode = R"( { "code": "2812", "name": "One" } )";
    const auto addCodesRequest = replaceTokens(
        hubItemDictionaryValueAdd,
        { { "__ITEM_ID__", userCodesId }, { "__TYPE__", "object" }, { "__VALUE__", newCode } } )[ "params" ];

    EXPECT_NO_THROW( coreRemoteInterface.addItemDictionaryValue( addCodesRequest ) );
    JsonObject element = JSON( "1" << JSON( "value" << JsonObject::parse( newCode ) ) );
    checkItemDictionaryUpdatedBroadcast( userCodesId, { { "_id", userCodesId }, { "element", element } }, __LINE__ );
    EXPECT_EQ( coreRemoteInterface.getItemDictionaryValue( userCodesId, "1" ), JsonObject::parse( newCode ) );

    const JsonObject pinCodeEntered = R"( { "action": "user_code_entered", "userId": 1 } )"_json;
    auto setItemValueRequest =
        JSON( ItemFields::IdFieldName << userCodeActionId << ItemFields::ValueFieldName << pinCodeEntered );
    EXPECT_NO_THROW( coreRemoteInterface.setItemValue( setItemValueRequest ) );
    checkItemUpdateBroadcast( userCodeActionId, pinCodeEntered, __LINE__ );
    checkUserNotificationBroadcast( "Touchscreen Deadbolt", "", "One", __LINE__ );

    removeDevice( coreRemoteInterface, deviceId, 2, 0, __LINE__ );
}

TEST_F( CoredServiceTests, hubItemDictionaryUpdated ) {  // 1.7
    using namespace CoreD::Model;
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "Touchscreen Deadbolt",
                               "doorlock",
                               "297_32770_1536",
                               "door_lock",
                               "",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto userCodesId = addItem( coreRemoteInterface,
                                deviceId,
                                "user_codes",
                                ValueType::makeDictionaryType( ValueType::Enum::UserCode ),
                                true,
                                true,
                                __LINE__ );

    // add dictionary element
    std::string newCode = R"( { "code": "2812", "name": "One" } )";
    auto addCodesRequest = replaceTokens(
        hubItemDictionaryValueAdd,
        { { "__ITEM_ID__", userCodesId }, { "__TYPE__", "object" }, { "__VALUE__", newCode } } )[ "params" ];

    JsonObject element = JSON( "1" << JSON( "value" << JsonObject::parse( newCode ) ) );
    KeyJsonList fields = {
        { "operation", "added" },
        { "element", element },
        { "_id", userCodesId },
        { "deviceId", deviceId },
        { "deviceName", "Touchscreen Deadbolt" },
        { "deviceCategory", "door_lock" },
        { "deviceId", deviceId },
        { "deviceSubcategory", "" },
        { "roomName", "" },
        { "name", "user_codes" },
    };
    EXPECT_NO_THROW( coreRemoteInterface.addItemDictionaryValue( addCodesRequest ) );
    checkItemDictionaryUpdatedBroadcast( userCodesId, fields, __LINE__ );
    EXPECT_EQ( coreRemoteInterface.getItemDictionaryValue( userCodesId, "1" ),
               addCodesRequest[ "element" ][ "value" ] );

    JsonObject value = R"( { "code": "2222", "name": "Two" } )"_json;
    addCodesRequest[ "element" ][ "value" ] = value;
    element = JSON( "2" << JSON( "value" << value ) );
    fields[ 1 ].second = element;
    EXPECT_NO_THROW( coreRemoteInterface.addItemDictionaryValue( addCodesRequest ) );
    checkItemDictionaryUpdatedBroadcast( userCodesId, fields, __LINE__ );
    EXPECT_EQ( coreRemoteInterface.getItemDictionaryValue( userCodesId, "2" ),
               addCodesRequest[ "element" ][ "value" ] );

    value = R"( { "code": "3333", "name": "Three" } )"_json;
    addCodesRequest[ "element" ][ "value" ] = value;
    element = JSON( "3" << JSON( "value" << value ) );
    fields[ 1 ].second = element;
    EXPECT_NO_THROW( coreRemoteInterface.addItemDictionaryValue( addCodesRequest ) );
    checkItemDictionaryUpdatedBroadcast( userCodesId, fields, __LINE__ );
    EXPECT_EQ( coreRemoteInterface.getItemDictionaryValue( userCodesId, "3" ),
               addCodesRequest[ "element" ][ "value" ] );

    // update dictionary element
    newCode = R"( { "code": "7895", "name": "One" } )";
    auto setCodesRequest = replaceTokens( hubItemDictionaryValueSet,
                                          { { "__ITEM_ID__", userCodesId },
                                            { "__KEY__", "1" },
                                            { "__TYPE__", "object" },
                                            { "__VALUE__", newCode } } )[ "params" ];
    fields[ 0 ].second = "updated";

    element = JSON( "1" << JSON( "value" << JsonObject::parse( newCode ) ) );
    fields[ 1 ].second = element;
    EXPECT_NO_THROW( coreRemoteInterface.setItemDictionaryValue( setCodesRequest ) );
    checkItemDictionaryUpdatedBroadcast( userCodesId, fields, __LINE__ );

    value = R"( { "code": "6582", "name": "TwoMod" } )"_json;
    setCodesRequest[ "element" ][ "value" ] = value;
    setCodesRequest[ "key" ] = "2";
    element = JSON( "2" << JSON( "value" << value ) );
    fields[ 1 ].second = element;
    EXPECT_NO_THROW( coreRemoteInterface.setItemDictionaryValue( setCodesRequest ) );
    checkItemDictionaryUpdatedBroadcast( userCodesId, fields, __LINE__ );

    // remove dictionary element
    auto removeCodesRequest = replaceTokens( hubItemDictionaryValueRemove,
                                             { { "__ITEM_ID__", userCodesId }, { "__KEY__", "2" } } )[ "params" ];
    fields[ 0 ].second = "removed";

    EXPECT_NO_THROW( coreRemoteInterface.removeItemDictionaryValue( removeCodesRequest ) );
    checkItemDictionaryUpdatedBroadcast( userCodesId, fields, __LINE__ );

    removeCodesRequest[ "key" ] = "1";
    value = R"( { "code": "7895", "name": "One" } )"_json;
    element = JSON( "1" << JSON( "value" << value ) );
    fields[ 1 ].second = element;
    EXPECT_NO_THROW( coreRemoteInterface.removeItemDictionaryValue( removeCodesRequest ) );
    checkItemDictionaryUpdatedBroadcast( userCodesId, fields, __LINE__ );

    // clear dictionary
    auto clearCodesRequest = replaceTokens( hubItemDictionaryClear, { { "__ITEM_ID__", userCodesId } } )[ "params" ];
    EXPECT_NO_THROW( coreRemoteInterface.clearItemDictionary( clearCodesRequest ) );
    checkItemUpdateBroadcast( userCodesId, Json::object(), __LINE__ );

    JsonObject broadcast;
    coreRemoteInterface.removeDevice( deviceId );
    broadcast = waitForBroadcast( "hub.item.removed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
}

TEST_F( CoredServiceTests, eventsBinding ) {  // 1.8
    using namespace CoreD::Model::Resources;
    auto subscribeOnCoreEvents =
        replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", "HUB:model_factory/scripts/core_event_subscribe" } } );
    auto answer = nma().sendtoHubAndGetAnswer( subscribeOnCoreEvents );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    class FunctionExitExecuter {
    public:
        using Executer = std::function< void() >;
        FunctionExitExecuter( Executer executer )
        : m_executer( std::move( executer ) ) {}
        ~FunctionExitExecuter() {
            m_executer();
        }

    private:
        Executer m_executer;
    };

    FunctionExitExecuter exitExecuter( [this]() {
        auto unsubscribeFromCoreEvents = replaceTokens(
            hubExtensionsPluginRun, { { "__SCRIPT__", "HUB:model_factory/scripts/core_event_unsubscribe" } } );
        auto answer = nma().sendtoHubAndGetAnswer( unsubscribeFromCoreEvents );
        auto error = answer[ "error" ];
        EXPECT_TRUE( error.empty() ) << error.dump( 4 );

        checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
        checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
    } );

    auto checkEventBroadcast = [this]( const std::string& expectedBroadcast,
                                       const std::string& expectedEvent,
                                       int codeLine ) {
        const std::set< std::string >& expectedBroadcasts = { "hub.extensions.plugin.ui_broadcast", expectedBroadcast };
        std::vector< JsonObject > broadcasts;
        auto codeLineStr = getCodeLineString( codeLine );
        broadcasts.push_back( waitForAnyOfBroadcasts( expectedBroadcasts, getCodeLineString( codeLine ) ) );
        broadcasts.push_back( waitForAnyOfBroadcasts( expectedBroadcasts, getCodeLineString( codeLine ) ) );
        for( auto& broadcast : broadcasts ) {
            auto name = json_get_str( broadcast, "msg_subclass" );
            EXPECT_TRUE( expectedBroadcasts.find( name ) != expectedBroadcasts.end() ) << codeLineStr;
            auto& result = broadcast[ "result" ];
            if( name == "hub.extensions.plugin.ui_broadcast" ) {
                EXPECT_EQ( json_get_str( result, "event" ), expectedEvent );
                result.erase( "event" );
                result.erase( "plugin" );
            }
            result.erase( "notifications" );  // lua does not add null variable to table via c++ API
            result.erase( "syncNotification" );
            result.erase( "userNotification" );
            result.erase( "serviceNotification" );
            result.erase( System::FirmwareTimestampMs );
        }
        EXPECT_EQ( broadcasts[ 0 ][ "result" ].dump(), broadcasts[ 1 ][ "result" ].dump() ) << codeLineStr;
    };

    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );

    auto device = createDevice( coreRemoteInterface,
                                "testDevice1",
                                "smoke",
                                "134_3_24",
                                "security_sensor",
                                "smoke",
                                false /*batteryPowered*/,
                                __LINE__ );
    std::string deviceId, itemId;
    bool isSuccess;
    std::tie( deviceId, isSuccess ) = coreRemoteInterface.addDevice( device );
    ASSERT_TRUE( isSuccess );
    checkEventBroadcast( "hub.device.added", "device_added", __LINE__ );

    coreRemoteInterface.updateDeviceName( deviceId, "room" );
    checkEventBroadcast( "hub.device.updated", "device_updated", __LINE__ );

    using namespace CoreD::Model;
    Item item( deviceId,
               "testItem1",
               ValueType::Enum::Int,
               true,
               true,
               ValueFactory::createDefaultItemValue( ValueType::Enum::Int ) );
    std::tie( itemId, isSuccess ) = coreRemoteInterface.addItem( item );
    ASSERT_TRUE( isSuccess );
    checkEventBroadcast( "hub.item.added", "item_added", __LINE__ );

    coreRemoteInterface.setItemValue( JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << 60 ) );
    checkEventBroadcast( "hub.item.updated", "item_updated", __LINE__ );

    coreRemoteInterface.removeItem( itemId );
    checkEventBroadcast( "hub.item.removed", "item_removed", __LINE__ );

    auto dictionaryType = ValueType::makeDictionaryType( ValueType::Enum::Token );
    Item dictionaryItem( deviceId,
                         "barrier_problem_sensors",
                         dictionaryType,
                         true,
                         true,
                         ValueFactory::createDefaultItemValue( dictionaryType ) );
    std::tie( itemId, isSuccess ) = coreRemoteInterface.addItem( dictionaryItem );
    ASSERT_TRUE( isSuccess );
    checkEventBroadcast( "hub.item.added", "item_added", __LINE__ );

    auto addDictionaryValue = replaceTokens(
        hubItemDictionaryValueAdd,
        { { "__ITEM_ID__", itemId }, { "__TYPE__", "token" }, { "__VALUE__", "\"low_battery\"" } } )[ "params" ];
    EXPECT_NO_THROW( coreRemoteInterface.addItemDictionaryValue( addDictionaryValue ) );
    checkEventBroadcast( "hub.item.dictionary.updated", "item_dictionary_updated", __LINE__ );

    coreRemoteInterface.removeItem( itemId );
    checkEventBroadcast( "hub.item.removed", "item_removed", __LINE__ );

    coreRemoteInterface.removeDevice( deviceId );
    checkEventBroadcast( "hub.device.removed", "device_removed", __LINE__ );
}

TEST_F( CoredServiceTests, luaItemUpdate ) {  // 1.9.1
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );

    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 = addItem(
        coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId2 =
        addItem( coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    auto itemId3 = addItem(
        coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Token, true, true, __LINE__ );
    auto itemId4 = addDictionary(
        coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    auto params = JSON( "testSelector"
                        << "set_item_value"
                        << "itemId" << itemId1 << "itemValue" << true );
    runScript( "HUB:item_control/scripts/item_update_handler", params, __LINE__ );
    checkItemUpdateBroadcast( itemId1, true, __LINE__ );

    params = JSON( "testSelector"
                   << "set_item_value"
                   << "itemId" << itemId2 << "itemValue" << 2 );
    runScript( "HUB:item_control/scripts/item_update_handler", params, __LINE__ );
    checkItemUpdateBroadcast( itemId2, 2, __LINE__ );

    params = JSON( "testSelector"
                   << "set_item_value"
                   << "itemId" << itemId3 << "itemValue"
                   << "token1" );
    runScript( "HUB:item_control/scripts/item_update_handler", params, __LINE__ );
    checkItemUpdateBroadcast( itemId3, "token1", __LINE__ );

    params = JSON( "testSelector"
                   << "add_item_dictionary_value"
                   << "itemId" << itemId4 << "itemValue" << 1 );
    runScript( "HUB:item_control/scripts/item_update_handler", params, __LINE__ );
    checkItemDictionaryUpdatedBroadcast(
        itemId4, { { "operation", "added" }, { "element", JSON( "1" << JSON( "value" << 1 ) ) } }, __LINE__ );

    params = JSON( "testSelector"
                   << "add_item_dictionary_value"
                   << "itemId" << itemId4 << "itemValue" << 1 );
    runScript( "HUB:item_control/scripts/item_update_handler", params, __LINE__ );
    checkItemDictionaryUpdatedBroadcast(
        itemId4, { { "operation", "added" }, { "element", JSON( "2" << JSON( "value" << 1 ) ) } }, __LINE__ );

    params = JSON( "testSelector"
                   << "set_item_dictionary_value"
                   << "itemId" << itemId4 << "dictionaryKey"
                   << "1"
                   << "itemValue" << 2 );
    runScript( "HUB:item_control/scripts/item_update_handler", params, __LINE__ );
    checkItemDictionaryUpdatedBroadcast(
        itemId4, { { "operation", "updated" }, { "element", JSON( "1" << JSON( "value" << 2 ) ) } }, __LINE__ );

    params = JSON( "testSelector"
                   << "delete_item_dictionary_value"
                   << "itemId" << itemId4 << "dictionaryKey"
                   << "1" );
    runScript( "HUB:item_control/scripts/item_update_handler", params, __LINE__ );
    checkItemDictionaryUpdatedBroadcast(
        itemId4, { { "operation", "removed" }, { "element", JSON( "1" << JSON( "value" << 2 ) ) } }, __LINE__ );

    runFailedScript( "HUB:item_control/scripts/restricted_methods_handler",
                     JSON( "testSelector"
                           << "add_device" ),
                     __LINE__ );
    runFailedScript( "HUB:item_control/scripts/restricted_methods_handler",
                     JSON( "testSelector"
                           << "add_item" ),
                     __LINE__ );
    runFailedScript( "HUB:item_control/scripts/restricted_methods_handler",
                     JSON( "testSelector"
                           << "modify_device" ),
                     __LINE__ );
    runFailedScript( "HUB:item_control/scripts/restricted_methods_handler",
                     JSON( "testSelector"
                           << "modify_item" ),
                     __LINE__ );
    runFailedScript( "HUB:item_control/scripts/restricted_methods_handler",
                     JSON( "testSelector"
                           << "remove_device_sync" ),
                     __LINE__ );
    runFailedScript( "HUB:item_control/scripts/restricted_methods_handler",
                     JSON( "testSelector"
                           << "remove_device" ),
                     __LINE__ );
    runFailedScript( "HUB:item_control/scripts/restricted_methods_handler",
                     JSON( "testSelector"
                           << "remove_gateway_devices" ),
                     __LINE__ );
    runFailedScript( "HUB:item_control/scripts/restricted_methods_handler",
                     JSON( "testSelector"
                           << "update_device_status" ),
                     __LINE__ );
    runFailedScript( "HUB:item_control/scripts/restricted_methods_handler",
                     JSON( "testSelector"
                           << "update_item_value_with_min_max" ),
                     __LINE__ );
    runFailedScript( "HUB:item_control/scripts/restricted_methods_handler",
                     JSON( "testSelector"
                           << "update_reachable_state" ),
                     __LINE__ );
    runFailedScript( "HUB:item_control/scripts/restricted_methods_handler",
                     JSON( "testSelector"
                           << "update_ready_state" ),
                     __LINE__ );
    runFailedScript( "HUB:item_control/scripts/restricted_methods_handler",
                     JSON( "testSelector"
                           << "update_item_value" ),
                     __LINE__ );
    runFailedScript( "HUB:item_control/scripts/restricted_methods_handler",
                     JSON( "testSelector"
                           << "update_item_dictionary_value" ),
                     __LINE__ );
    runFailedScript( "HUB:item_control/scripts/restricted_methods_handler",
                     JSON( "testSelector"
                           << "remove_item_dictionary_value" ),
                     __LINE__ );

    JsonObject broadcast;
    coreRemoteInterface.removeDevice( deviceId );
    broadcast = waitForBroadcast( "hub.item.removed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.item.removed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.item.removed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.item.removed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
}

TEST_F( CoredServiceTests, luaItemUpdateWithSource ) {  // 1.9.2
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );

    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 = addItem(
        coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId4 = addDictionary(
        coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    bool boolValue = true;
    auto params = JSON( "testSelector"
                        << "set_item_value"
                        << "itemId" << itemId1 << "itemValue" << boolValue );
    Json goodSource = JSON( "type"
                            << "nma_api_tests" );
    params = JSON( "testSelector"
                   << "set_item_value"
                   << "itemId" << itemId1 << "itemValue" << boolValue << "source" << goodSource );
    runScript( "HUB:item_control/scripts/item_update_handler", params, __LINE__ );
    checkItemUpdateBroadcast( itemId1, boolValue, __LINE__ );

    boolValue = true;
    Json badSource = JSON( "no_type_field" << true );
    params = JSON( "testSelector"
                   << "set_item_value"
                   << "itemId" << itemId1 << "itemValue" << boolValue << "source" << badSource );
    runFailedScript( "HUB:item_control/scripts/item_update_handler", params, __LINE__ );

    // dictionaries
    params = JSON( "testSelector"
                   << "add_item_dictionary_value"
                   << "itemId" << itemId4 << "itemValue" << 1 << "source" << goodSource );
    runScript( "HUB:item_control/scripts/item_update_handler", params, __LINE__ );
    checkItemDictionaryUpdatedBroadcast(
        itemId4, { { "operation", "added" }, { "element", JSON( "1" << JSON( "value" << 1 ) ) } }, __LINE__ );

    params[ "source" ] = badSource;
    runFailedScript( "HUB:item_control/scripts/item_update_handler", params, __LINE__ );

    params = JSON( "testSelector"
                   << "add_item_dictionary_value"
                   << "itemId" << itemId4 << "itemValue" << 1 << "source" << goodSource );
    runScript( "HUB:item_control/scripts/item_update_handler", params, __LINE__ );
    checkItemDictionaryUpdatedBroadcast(
        itemId4, { { "operation", "added" }, { "element", JSON( "2" << JSON( "value" << 1 ) ) } }, __LINE__ );

    params[ "source" ] = badSource;
    runFailedScript( "HUB:item_control/scripts/item_update_handler", params, __LINE__ );

    params = JSON( "testSelector"
                   << "set_item_dictionary_value"
                   << "itemId" << itemId4 << "dictionaryKey"
                   << "1"
                   << "itemValue" << 2 << "source" << goodSource );
    runScript( "HUB:item_control/scripts/item_update_handler", params, __LINE__ );
    checkItemDictionaryUpdatedBroadcast(
        itemId4, { { "operation", "updated" }, { "element", JSON( "1" << JSON( "value" << 2 ) ) } }, __LINE__ );

    params[ "source" ] = badSource;
    runFailedScript( "HUB:item_control/scripts/item_update_handler", params, __LINE__ );

    params = JSON( "testSelector"
                   << "delete_item_dictionary_value"
                   << "itemId" << itemId4 << "dictionaryKey"
                   << "1"
                   << "source" << goodSource );
    runScript( "HUB:item_control/scripts/item_update_handler", params, __LINE__ );
    checkItemDictionaryUpdatedBroadcast(
        itemId4, { { "operation", "removed" }, { "element", JSON( "1" << JSON( "value" << 2 ) ) } }, __LINE__ );

    params[ "source" ] = badSource;
    runFailedScript( "HUB:item_control/scripts/item_update_handler", params, __LINE__ );

    // cleanup
    JsonObject broadcast;
    coreRemoteInterface.removeDevice( deviceId );
    broadcast = waitForBroadcast( "hub.item.removed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.item.removed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
}

TEST_F( CoredServiceTests, luaItemUpdateInBatch ) {  // 1.9.3
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );

    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 = addItem(
        coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId2 = addItem(
        coreRemoteInterface, deviceId, "testItem2", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId3 = addItem(
        coreRemoteInterface, deviceId, "testItem3", CoreD::Model::ValueType::Enum::Token, true, true, __LINE__ );

    auto params = JSON( "testSelector"
                        << "set_item_value_in_batch"
                        << "itemIds" << JSON_ARRAY( itemId1 << itemId2 ) << "itemValue" << true );
    runScript( "HUB:item_control/scripts/item_update_handler", params, __LINE__ );
    checkItemUpdateBroadcast( itemId1, true, __LINE__ );
    checkItemUpdateBroadcast( itemId2, true, __LINE__ );

    params = JSON( "testSelector"
                   << "set_item_value_in_batch"
                   << "itemIds" << JSON_ARRAY( itemId1 << "bbb" << itemId2 ) << "itemValue" << true );
    runFailedScript( "HUB:item_control/scripts/item_update_handler", params, __LINE__ );

    params = JSON( "testSelector"
                   << "set_item_value_in_batch"
                   << "itemIds" << JSON_ARRAY( itemId1 << itemId3 << itemId2 ) << "itemValue"
                   << "abc" );
    runFailedScript( "HUB:item_control/scripts/item_update_handler", params, __LINE__ );

    // cleanup
    JsonObject broadcast;
    coreRemoteInterface.removeDevice( deviceId );
    broadcast = waitForBroadcast( "hub.item.removed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.item.removed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.item.removed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
}

TEST_F( CoredServiceTests, luaItemUpdateInBatchWithSource ) {  // 1.9.4
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );

    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 = addItem(
        coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId2 = addItem(
        coreRemoteInterface, deviceId, "testItem2", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );

    Json goodSource = JSON( "type"
                            << "nma_api_tests" );
    auto params =
        JSON( "testSelector"
              << "set_item_value_in_batch"
              << "itemIds" << JSON_ARRAY( itemId1 << itemId2 ) << "itemValue" << true << "source" << goodSource );
    runScript( "HUB:item_control/scripts/item_update_handler", params, __LINE__ );
    checkItemUpdateBroadcast( itemId1, true, __LINE__ );
    checkItemUpdateBroadcast( itemId2, true, __LINE__ );

    Json badSource = JSON( "no_type_field" << true );
    params = JSON( "testSelector"
                   << "set_item_value_in_batch"
                   << "itemIds" << JSON_ARRAY( itemId1 << itemId2 ) << "itemValue" << true << "source" << badSource );
    runFailedScript( "HUB:item_control/scripts/item_update_handler", params, __LINE__ );

    // cleanup
    JsonObject broadcast;
    coreRemoteInterface.removeDevice( deviceId );
    broadcast = waitForBroadcast( "hub.item.removed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.item.removed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
}

TEST_F( CoredServiceTests, luaGetLocalTime ) {  // 1.10
    using namespace std::placeholders;
    using Sig1 = void ( CoredServiceTests::* )( const JsonObject&, const std::string&, int );
    auto checkExtensionsPluginRunProgressBroadcastBind = std::bind(
        static_cast< Sig1 >( &CoredServiceTests::checkExtensionsPluginRunProgressBroadcast ), this, _1, _2, _3 );

    auto checkPluginBroadcastFormat1 = [this]( const JsonObject& broadcast, int codeLine ) {
        auto codeLineStr = getCodeLineString( codeLine );
        auto& result = broadcast[ "result" ];
        auto localTime = json_get_str( result, "localTime" );
        EXPECT_EQ( localTime[ 4 ], '-' ) << codeLineStr;
        EXPECT_EQ( localTime[ 7 ], '-' ) << codeLineStr;
        EXPECT_EQ( localTime[ 10 ], 'T' ) << codeLineStr;
        EXPECT_EQ( localTime[ 13 ], ':' ) << codeLineStr;
        EXPECT_EQ( localTime[ 16 ], ':' ) << codeLineStr;
        EXPECT_EQ( localTime[ 19 ], '+' ) << codeLineStr;
    };

    auto checkPluginBroadcastFormat2 = [this]( const JsonObject& broadcast, int codeLine ) {
        auto codeLineStr = getCodeLineString( codeLine );
        auto& result = broadcast[ "result" ];
        auto localTime = json_get_str( result, "localTime" );
        EXPECT_EQ( localTime[ 2 ], '/' ) << codeLineStr;
        EXPECT_EQ( localTime[ 5 ], '/' ) << codeLineStr;
        EXPECT_EQ( localTime[ 10 ], ':' ) << codeLineStr;
        EXPECT_EQ( localTime[ 13 ], ':' ) << codeLineStr;
    };

    runScriptWithoutChecks( "HUB:item_control/scripts/get_local_time", JSON_EMPTY_OBJ, __LINE__ );

    checkBroadcasts(
        { { "hub.extensions.plugin.run.progress",
            std::bind( checkExtensionsPluginRunProgressBroadcastBind, _1, "started", __LINE__ ) },
          { "hub.extensions.plugin.ui_broadcast", std::bind( checkPluginBroadcastFormat1, _1, __LINE__ ) } },
        __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    runScriptWithoutChecks( "HUB:item_control/scripts/get_local_time",
                            JSON( "format"
                                  << "%x%X" ),
                            __LINE__ );

    checkBroadcasts(
        { { "hub.extensions.plugin.run.progress",
            std::bind( checkExtensionsPluginRunProgressBroadcastBind, _1, "started", __LINE__ ) },
          { "hub.extensions.plugin.ui_broadcast", std::bind( checkPluginBroadcastFormat2, _1, __LINE__ ) } },
        __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( CoredServiceTests, luaGetLocation ) {  // 1.11
    using namespace std::placeholders;
    using Sig1 = void ( CoredServiceTests::* )( const JsonObject&, const std::string&, int );
    auto checkExtensionsPluginRunProgressBroadcastBind = std::bind(
        static_cast< Sig1 >( &CoredServiceTests::checkExtensionsPluginRunProgressBroadcast ), this, _1, _2, _3 );

    auto checkPluginBroadcast = [this]( const JsonObject& broadcast, int codeLine ) {
        auto codeLineStr = getCodeLineString( codeLine );
        JsonObject result;
        EXPECT_TRUE( json_get( broadcast, "result", result ) );
        EXPECT_EQ( json_get_float( result, "latitude" ), 39.8338509f );
        EXPECT_EQ( json_get_float( result, "longitude" ), -74.8718262f );
    };

    runScriptWithoutChecks( "HUB:item_control/scripts/get_location", JSON_EMPTY_OBJ, __LINE__ );

    checkBroadcasts( { { "hub.extensions.plugin.run.progress",
                         std::bind( checkExtensionsPluginRunProgressBroadcastBind, _1, "started", __LINE__ ) },
                       { "hub.extensions.plugin.ui_broadcast", std::bind( checkPluginBroadcast, _1, __LINE__ ) } },
                     __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( CoredServiceTests, luaGetTimezone ) {  // 1.12
    CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( nma() );
    coreDRemoteInterface.setHubTimeZone( JSON( "location"
                                               << "Europe/Kiev1" ) );

    using namespace std::placeholders;
    using Sig1 = void ( CoredServiceTests::* )( const JsonObject&, const std::string&, int );
    auto checkExtensionsPluginRunProgressBroadcastBind = std::bind(
        static_cast< Sig1 >( &CoredServiceTests::checkExtensionsPluginRunProgressBroadcast ), this, _1, _2, _3 );

    auto checkPluginBroadcast = [this]( const JsonObject& broadcast, int codeLine ) {
        auto codeLineStr = getCodeLineString( codeLine );
        JsonObject result;
        EXPECT_TRUE( json_get( broadcast, "result", result ) );
        EXPECT_EQ( json_get_str( result, "timezone" ), "Europe/Kiev1" );
    };

    runScriptWithoutChecks( "HUB:item_control/scripts/get_timezone", JSON_EMPTY_OBJ, __LINE__ );

    checkBroadcasts( { { "hub.extensions.plugin.run.progress",
                         std::bind( checkExtensionsPluginRunProgressBroadcastBind, _1, "started", __LINE__ ) },
                       { "hub.extensions.plugin.ui_broadcast", std::bind( checkPluginBroadcast, _1, __LINE__ ) } },
                     __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( CoredServiceTests, luaHubInfo ) {  // 1.13
    CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( nma() );
    coreDRemoteInterface.setHubTimeZone( JSON( "location"
                                               << "Europe/Kiev2" ) );

    using namespace std::placeholders;
    using Sig1 = void ( CoredServiceTests::* )( const JsonObject&, const std::string&, int );
    auto checkExtensionsPluginRunProgressBroadcastBind = std::bind(
        static_cast< Sig1 >( &CoredServiceTests::checkExtensionsPluginRunProgressBroadcast ), this, _1, _2, _3 );

    auto checkPluginBroadcast = [this]( const JsonObject& broadcast, int codeLine ) {
        auto codeLineStr = getCodeLineString( codeLine );
        JsonObject result, location;
        EXPECT_TRUE( json_get( broadcast, "result", result ) );
        EXPECT_EQ( json_get_str( result, "architecture" ), "x86_64" );
        EXPECT_EQ( json_get_str( result, "model" ), "local" );
        EXPECT_EQ( json_get_str( result, "serial" ), HubInfoHelper::getPkDevice() );
        EXPECT_TRUE( result.find( "firmware" ) != result.end() );
        EXPECT_TRUE( result.find( "build" ) != result.end() );
        EXPECT_TRUE( result.find( "localtime" ) != result.end() );
        EXPECT_TRUE( result.find( "kernel" ) != result.end() );
        EXPECT_TRUE( json_get( result, "location", location ) );
        EXPECT_EQ( json_get_float( location, "latitude" ), 39.8338509f );
        EXPECT_EQ( json_get_float( location, "longitude" ), -74.8718262f );
        EXPECT_EQ( json_get_str( location, "state" ), "customTimezone" );
        EXPECT_EQ( json_get_str( location, "timezone" ), "Europe/Kiev2" );
    };

    runScriptWithoutChecks( "HUB:item_control/scripts/get_hub_info", JSON_EMPTY_OBJ, __LINE__ );

    checkBroadcasts( { { "hub.extensions.plugin.run.progress",
                         std::bind( checkExtensionsPluginRunProgressBroadcastBind, _1, "started", __LINE__ ) },
                       { "hub.extensions.plugin.ui_broadcast", std::bind( checkPluginBroadcast, _1, __LINE__ ) } },
                     __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( CoredServiceTests, addDeviceIdToTtemRemovedBroadcast ) {  // 1.14
    JsonObject broadcast;
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );
    auto& dbClient = nma().getDBClient();
    CoreDHouseModes::setAlarmDelay( dbClient, 1 );

    const std::string gatewayId = getMainGateway( remoteInterface, __LINE__ );

    Model::Device device( gatewayId, "Device 1", "sensor", "0_0_7", "security_sensor", false );
    device.setSubcategory( "co" );
    auto deviceAddResult = remoteInterface.addDevice( device );
    auto deviceId = deviceAddResult.first;
    EXPECT_FALSE( deviceAddResult.first.empty() );
    EXPECT_TRUE( deviceAddResult.second );
    checkBroadcast( "", "hub.device.added", {}, NMA_TEST_MAKE_MESSAGE( "item added" ) );

    Model::Item item( deviceAddResult.first,
                      "security_threat",
                      Model::ValueType::Enum::Bool,
                      true,
                      false,
                      Model::ValueFactory::createDefaultItemValue( Model::ValueType::Enum::Bool ) );
    auto itemAddResult = remoteInterface.addItem( item );
    EXPECT_FALSE( itemAddResult.first.empty() );
    EXPECT_TRUE( itemAddResult.second );
    checkBroadcast( "", "hub.item.added", {}, NMA_TEST_MAKE_MESSAGE( "item added" ) );
    checkBroadcast( "", "hub.device.updated", {}, NMA_TEST_MAKE_MESSAGE( "device updated: armed field" ) );

    auto itemsNumber = remoteInterface.getItemsByDeviceId( deviceId ).size();
    removeDevice( remoteInterface, deviceId, itemsNumber, 0, __LINE__ );
}

TEST_F( CoredServiceTests, luaPersistentDevice_CreateFromCore ) {  // 1.15
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );

    CoreD::Model::Device device(
        "testDevice1", "smoke", "134_3_24", "security_sensor", "smoke", false /*batteryPowered*/
    );
    device.setPersistent( true );

    std::string deviceId;
    try {
        deviceId = addDevice( coreRemoteInterface, device, __LINE__ );
        SUCCEED();
    }
    catch( const std::exception& err ) {
        FAIL() << "Failed to create a device: " << err.what();
    }

    auto itemId1 = addItem(
        coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );

    std::string scriptPath = "HUB:item_control/scripts/get_devices_list";
    Json scriptParams = JSON( "testSelector"
                              << "get_devices" );
    runScriptWithoutChecks( scriptPath, scriptParams, __LINE__ );

    JsonObject broadcast;
    broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", getCodeLineString( __LINE__ ) );
    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    EXPECT_FALSE( broadcast.empty() ) << cc::j( broadcast );
    EXPECT_EQ( json_get_str( broadcast, "id" ), "ui_broadcast" ) << cc::j( broadcast );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.extensions.plugin.ui_broadcast" ) << cc::j( broadcast );

    Json devices;
    std::vector< Json > keys{ "result", "devices" };
    ASSERT_TRUE( json_get( broadcast, keys, devices ) ) << "Json path is not found in the broadcast: " << broadcast;
    ASSERT_TRUE( devices.is_array() ) << "Should be an array";
    ASSERT_FALSE( devices.empty() ) << "Should be not empty";
    ASSERT_EQ( devices.size(), 1 ) << "Should have only 1 device";

    const Json& theDevice = devices.at( 0 );
    ASSERT_TRUE( theDevice.is_object() ) << "device should be an object";
    ASSERT_EQ( theDevice.count( "persistent" ), 1 ) << "should have field 'persistent'";
    ASSERT_EQ( theDevice[ "persistent" ], true ) << "Should be true!";

    coreRemoteInterface.removeDevice( deviceId );
    broadcast = waitForBroadcast( "hub.item.removed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
}

TEST_F( CoredServiceTests, luaPersistentDevice_CreateFromLuaScript ) {  // 1.16
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );

    std::string scriptPath = "HUB:item_control/scripts/get_devices_list";
    Json scriptParams = JSON( "testSelector"
                              << "add_device" );
    runScriptWithoutChecks( scriptPath, scriptParams, __LINE__ );

    JsonObject broadcast;
    broadcast = waitForBroadcast( "hub.device.added", getCodeLineString( __LINE__ ) );
    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.added" );
    std::string deviceId;

    {
        Json device;
        ASSERT_TRUE( json_get( broadcast, "result", device ) )
            << "Json path is not found in the broadcast: " << broadcast;
        ASSERT_TRUE( device.is_object() ) << "Should be an array";
        ASSERT_FALSE( device.empty() ) << "Should be not empty";

        ASSERT_TRUE( device.is_object() ) << "device should be an object";
        ASSERT_EQ( device.count( "persistent" ), 1 ) << "should have field 'persistent'";
        ASSERT_EQ( device[ "persistent" ], true ) << "Should be true!";

        deviceId = json_get_str( device, "_id" );
        ASSERT_FALSE( deviceId.empty() );
    }

    // run hub.devices.list
    {
        Json hubDevicesList;
        ASSERT_NO_THROW( hubDevicesList = Json::parse( hubDevicesListRequest ) );

        Json answer = nma().sendtoHubAndGetAnswer( hubDevicesList );
        ASSERT_FALSE( answer.empty() ) << "should not be empty";

        Json devices;
        ASSERT_TRUE( json_get( answer, { "result", "devices" }, devices ) ) << "should have result.devices collection";
        ASSERT_TRUE( devices.is_array() && devices.size() > 0 );
        Json device = devices.at( 0 );

        Log::trace( cc::j( device ) );
    }

    coreRemoteInterface.removeDevice( deviceId );
    broadcast = waitForBroadcast( "hub.device.removed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
}

TEST_F( CoredServiceTests, luaPersistentDevice_forceRemove ) {  // 1.17
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );

    CoreD::Model::Device device(
        "testDevice1", "smoke", "134_3_24", "security_sensor", "smoke", false /*batteryPowered*/
    );
    device.setPersistent( true );

    auto deviceId = addDevice( remoteInterface, device, __LINE__ );
    ASSERT_FALSE( deviceId.empty() );

    Json answer, error;
    Json commandForceRemove = R"({ "method": "hub.device.force_remove", "params": {} })"_json;
    commandForceRemove[ "params" ][ "_id" ] = deviceId;

    ASSERT_NO_THROW( answer = nma().sendtoHubAndGetAnswer( commandForceRemove ) );
    ASSERT_TRUE( json_get_obj( answer, "error", error ) ) << answer;
    ASSERT_LE( json_get_int( error, "code", 0 ), static_cast< int >( Errors::Codes::APPLICATION_ERROR ) ) << error;
    ASSERT_LE( json_get_str( error, "data" ), "ezlo.device.persistent" ) << error;

    Json broadcast;
    remoteInterface.removeDevice( deviceId );
    broadcast = waitForBroadcast( "hub.device.removed", getCodeLineString( __LINE__ ) );
    ASSERT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
}

TEST_F( CoredServiceTests, addItemValueIndent ) {  // 1.18.1
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );

    const std::string gatewayId = getMainGateway( remoteInterface, __LINE__ );
    CoreD::Model::Device device( gatewayId, "smoke", "134_3_24", "security_sensor", "smoke", false /*batteryPowered*/
    );

    auto deviceId = addDevice( remoteInterface, device, __LINE__ );
    ASSERT_FALSE( deviceId.empty() );

    auto itemId1 =
        addItem( remoteInterface, deviceId, "testItem", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    auto request =
        replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", "HUB:model_factory/scripts/core_add_item" } } );
    auto& scriptParams = request[ "params" ][ "scriptParams" ];
    scriptParams = JSON_EMPTY_OBJ;
    auto& item = scriptParams[ "item" ];
    item = JSON_EMPTY_OBJ;
    item[ "device_id" ] = deviceId;
    item[ "name" ] = "testItem1";
    item[ "value_type" ] = "string";
    item[ "value" ] = "value";
    item[ "has_getter" ] = true;
    item[ "has_setter" ] = true;
    auto& valueIndent = item[ "value_indent" ];
    valueIndent = JSON_EMPTY_OBJ;
    valueIndent[ "item" ] = itemId1;
    valueIndent[ "min_indent" ] = 3.3f;

    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    auto broadcast = waitForBroadcast( "hub.item.added", NMA_TEST_MAKE_MESSAGE( "remove item" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.added" );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    answer = nma().sendtoHubAndGetAnswer( hubItemsListRequest );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    auto items = answer[ "result" ][ "items" ];
    ASSERT_EQ( items.size(), 2 );
    std::map< std::string, const Json& > itemsMap;
    itemsMap.insert( { json_get_str( items[ 0 ], "_id" ), items[ 0 ] } );
    itemsMap.insert( { json_get_str( items[ 1 ], "_id" ), items[ 1 ] } );

    auto it = itemsMap.find( itemId1 );
    ASSERT_NE( it, itemsMap.end() );
    itemsMap.erase( it );
    it = itemsMap.begin();
    std::string itemId2 = it->first;

    std::string indentItemId;
    float minIndent;
    EXPECT_TRUE( json_get( it->second, std::vector< Json >{ "valueIndent", "item" }, indentItemId ) );
    EXPECT_TRUE( json_get( it->second, std::vector< Json >{ "valueIndent", "minIndent" }, minIndent ) );

    EXPECT_EQ( indentItemId, itemId1 );
    EXPECT_FLOAT_EQ( minIndent, 3.3f );

    request = replaceTokens(
        hubDataListRequest, { { "__PARAMS__", " \"items\" : { \"fields\": { \"include\":[  \"valueIndent\" ] } }" } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    items = answer[ "result" ][ "items" ];

    EXPECT_TRUE( items[ itemId1 ].empty() );

    auto& item2 = items[ itemId2 ];
    EXPECT_TRUE( json_get( item2, std::vector< Json >{ "valueIndent", "item" }, indentItemId ) );
    EXPECT_TRUE( json_get( item2, std::vector< Json >{ "valueIndent", "minIndent" }, minIndent ) );

    EXPECT_EQ( indentItemId, itemId1 );
    EXPECT_FLOAT_EQ( minIndent, 3.3f );
}

TEST_F( CoredServiceTests, addItemValueIndent_EmptyItem ) {  // 1.18.2
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );

    const std::string gatewayId = getMainGateway( remoteInterface, __LINE__ );
    CoreD::Model::Device device( gatewayId, "smoke", "134_3_24", "security_sensor", "smoke", false /*batteryPowered*/
    );

    auto deviceId = addDevice( remoteInterface, device, __LINE__ );
    ASSERT_FALSE( deviceId.empty() );

    auto request =
        replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", "HUB:model_factory/scripts/core_add_item" } } );
    auto& scriptParams = request[ "params" ][ "scriptParams" ];
    scriptParams = JSON_EMPTY_OBJ;
    auto& item = scriptParams[ "item" ];
    item = JSON_EMPTY_OBJ;
    item[ "device_id" ] = deviceId;
    item[ "name" ] = "testItem1";
    item[ "value_type" ] = "string";
    item[ "value" ] = "value";
    item[ "has_getter" ] = true;
    item[ "has_setter" ] = true;
    auto& valueIndent = item[ "value_indent" ];
    valueIndent = JSON_EMPTY_OBJ;
    valueIndent[ "min_indent" ] = 3.3f;

    checkFailure( request, "fail to add item : wrong 'item' field within 'value_indent'", __LINE__ );
}

TEST_F( CoredServiceTests, addItemValueIndent_EmptyMinIndent ) {  // 1.18.3
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );

    const std::string gatewayId = getMainGateway( remoteInterface, __LINE__ );
    CoreD::Model::Device device( gatewayId, "smoke", "134_3_24", "security_sensor", "smoke", false /*batteryPowered*/
    );

    auto deviceId = addDevice( remoteInterface, device, __LINE__ );
    ASSERT_FALSE( deviceId.empty() );

    auto itemId1 =
        addItem( remoteInterface, deviceId, "testItem", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    auto request =
        replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", "HUB:model_factory/scripts/core_add_item" } } );
    auto& scriptParams = request[ "params" ][ "scriptParams" ];
    scriptParams = JSON_EMPTY_OBJ;
    auto& item = scriptParams[ "item" ];
    item = JSON_EMPTY_OBJ;
    item[ "device_id" ] = deviceId;
    item[ "name" ] = "testItem1";
    item[ "value_type" ] = "string";
    item[ "value" ] = "value";
    item[ "has_getter" ] = true;
    item[ "has_setter" ] = true;
    auto& valueIndent = item[ "value_indent" ];
    valueIndent = JSON_EMPTY_OBJ;
    valueIndent[ "item" ] = itemId1;

    checkFailure( request, "fail to add item : wrong 'min_indent' field within 'value_indent'", __LINE__ );
}

TEST_F( CoredServiceTests, addItemValueIndent_WrongItem ) {  // 1.18.4
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );

    const std::string gatewayId = getMainGateway( remoteInterface, __LINE__ );
    CoreD::Model::Device device( gatewayId, "smoke", "134_3_24", "security_sensor", "smoke", false /*batteryPowered*/
    );

    auto deviceId = addDevice( remoteInterface, device, __LINE__ );
    ASSERT_FALSE( deviceId.empty() );

    auto request =
        replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", "HUB:model_factory/scripts/core_add_item" } } );
    auto& scriptParams = request[ "params" ][ "scriptParams" ];
    scriptParams = JSON_EMPTY_OBJ;
    auto& item = scriptParams[ "item" ];
    item = JSON_EMPTY_OBJ;
    item[ "device_id" ] = deviceId;
    item[ "name" ] = "testItem1";
    item[ "value_type" ] = "string";
    item[ "value" ] = "value";
    item[ "has_getter" ] = true;
    item[ "has_setter" ] = true;
    auto& valueIndent = item[ "value_indent" ];
    valueIndent = JSON_EMPTY_OBJ;
    valueIndent[ "item" ] = "itemId";
    valueIndent[ "min_indent" ] = 3.3f;

    checkFailure( request, "Item adding error", __LINE__ );
}

TEST_F( CoredServiceTests, modifyItemValueIndent ) {  // 1.19.1
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );

    const std::string gatewayId = getMainGateway( remoteInterface, __LINE__ );
    CoreD::Model::Device device( gatewayId, "smoke", "134_3_24", "security_sensor", "smoke", false /*batteryPowered*/
    );

    auto deviceId = addDevice( remoteInterface, device, __LINE__ );
    ASSERT_FALSE( deviceId.empty() );

    auto itemId1 =
        addItem( remoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    auto itemId2 =
        addItem( remoteInterface, deviceId, "testItem2", CoreD::Model::ValueType::Enum::String, true, true, __LINE__ );

    auto request =
        replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", "HUB:model_factory/scripts/core_modify_item" } } );
    auto& scriptParams = request[ "params" ][ "scriptParams" ];
    scriptParams = JSON_EMPTY_OBJ;
    scriptParams[ "item_id" ] = itemId2;
    auto& item = scriptParams[ "item" ];
    item = JSON_EMPTY_OBJ;
    item[ "device_id" ] = deviceId;
    item[ "name" ] = "testItem2";
    item[ "value_type" ] = "string";
    item[ "value" ] = "";
    item[ "has_getter" ] = true;
    item[ "has_setter" ] = true;
    auto& valueIndent = item[ "value_indent" ];
    valueIndent = JSON_EMPTY_OBJ;
    valueIndent[ "item" ] = itemId1;
    valueIndent[ "min_indent" ] = 3.3f;

    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    std::string indentItemId;
    float minIndent;
    auto broadcast = waitForBroadcast( "hub.item.updated", NMA_TEST_MAKE_MESSAGE( "no broadcast" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" );
    EXPECT_TRUE( json_get( broadcast, std::vector< Json >{ "result", "valueIndent", "item" }, indentItemId ) );
    EXPECT_TRUE( json_get( broadcast, std::vector< Json >{ "result", "valueIndent", "minIndent" }, minIndent ) );
    EXPECT_EQ( indentItemId, itemId1 );
    EXPECT_FLOAT_EQ( minIndent, 3.3f );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    // no broadcast should be after this request
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    valueIndent[ "min_indent" ] = 3.1f;
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    broadcast = waitForBroadcast( "hub.item.updated", NMA_TEST_MAKE_MESSAGE( "item updated" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" );
    EXPECT_TRUE( json_get( broadcast, std::vector< Json >{ "result", "valueIndent", "item" }, indentItemId ) );
    EXPECT_TRUE( json_get( broadcast, std::vector< Json >{ "result", "valueIndent", "minIndent" }, minIndent ) );
    EXPECT_EQ( indentItemId, itemId1 );
    EXPECT_FLOAT_EQ( minIndent, 3.1f );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( CoredServiceTests, modifyItemValueIndent_EmptyItem ) {  // 1.19.2
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );

    const std::string gatewayId = getMainGateway( remoteInterface, __LINE__ );
    CoreD::Model::Device device( gatewayId, "smoke", "134_3_24", "security_sensor", "smoke", false /*batteryPowered*/
    );

    auto deviceId = addDevice( remoteInterface, device, __LINE__ );
    ASSERT_FALSE( deviceId.empty() );

    auto itemId2 =
        addItem( remoteInterface, deviceId, "testItem2", CoreD::Model::ValueType::Enum::String, true, true, __LINE__ );

    auto request =
        replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", "HUB:model_factory/scripts/core_modify_item" } } );
    auto& scriptParams = request[ "params" ][ "scriptParams" ];
    scriptParams = JSON_EMPTY_OBJ;
    auto& item = scriptParams[ "item" ];
    scriptParams[ "item_id" ] = itemId2;
    item = JSON_EMPTY_OBJ;
    item[ "device_id" ] = deviceId;
    item[ "name" ] = "testItem2";
    item[ "value_type" ] = "string";
    item[ "value" ] = "";
    item[ "has_getter" ] = true;
    item[ "has_setter" ] = true;
    auto& valueIndent = item[ "value_indent" ];
    valueIndent = JSON_EMPTY_OBJ;
    valueIndent[ "min_indent" ] = 3.3f;

    checkFailure( request, "fail to modify item : wrong 'item' field within 'value_indent'", __LINE__ );
}

TEST_F( CoredServiceTests, modifyItemValueIndent_EmptyMinIndent ) {  // 1.19.3
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );

    const std::string gatewayId = getMainGateway( remoteInterface, __LINE__ );
    CoreD::Model::Device device( gatewayId, "smoke", "134_3_24", "security_sensor", "smoke", false /*batteryPowered*/
    );

    auto deviceId = addDevice( remoteInterface, device, __LINE__ );
    ASSERT_FALSE( deviceId.empty() );

    auto itemId1 =
        addItem( remoteInterface, deviceId, "testItem", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    auto itemId2 =
        addItem( remoteInterface, deviceId, "testItem2", CoreD::Model::ValueType::Enum::String, true, true, __LINE__ );

    auto request =
        replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", "HUB:model_factory/scripts/core_modify_item" } } );
    auto& scriptParams = request[ "params" ][ "scriptParams" ];
    scriptParams = JSON_EMPTY_OBJ;
    scriptParams[ "item_id" ] = itemId2;
    auto& item = scriptParams[ "item" ];
    item = JSON_EMPTY_OBJ;
    item[ "device_id" ] = deviceId;
    item[ "name" ] = "testItem2";
    item[ "value_type" ] = "string";
    item[ "value" ] = "value";
    item[ "has_getter" ] = true;
    item[ "has_setter" ] = true;
    auto& valueIndent = item[ "value_indent" ];
    valueIndent = JSON_EMPTY_OBJ;
    valueIndent[ "item" ] = itemId1;

    checkFailure( request, "fail to modify item : wrong 'min_indent' field within 'value_indent'", __LINE__ );
}

TEST_F( CoredServiceTests, modifyItemValueIndent_WrongItem ) {  // 1.19.4
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );

    const std::string gatewayId = getMainGateway( remoteInterface, __LINE__ );
    CoreD::Model::Device device( gatewayId, "smoke", "134_3_24", "security_sensor", "smoke", false /*batteryPowered*/
    );

    auto deviceId = addDevice( remoteInterface, device, __LINE__ );
    ASSERT_FALSE( deviceId.empty() );

    auto itemId2 =
        addItem( remoteInterface, deviceId, "testItem2", CoreD::Model::ValueType::Enum::String, true, true, __LINE__ );

    auto request =
        replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", "HUB:model_factory/scripts/core_modify_item" } } );
    auto& scriptParams = request[ "params" ][ "scriptParams" ];
    scriptParams = JSON_EMPTY_OBJ;
    scriptParams[ "item_id" ] = itemId2;
    auto& item = scriptParams[ "item" ];
    item = JSON_EMPTY_OBJ;
    item[ "device_id" ] = deviceId;
    item[ "name" ] = "testItem2";
    item[ "value_type" ] = "string";
    item[ "value" ] = "";
    item[ "has_getter" ] = true;
    item[ "has_setter" ] = true;
    auto& valueIndent = item[ "value_indent" ];
    valueIndent = JSON_EMPTY_OBJ;
    valueIndent[ "item" ] = "itemId";
    valueIndent[ "min_indent" ] = 3.3f;

    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( CoredServiceTests, onSecurityThreatTouchAllCamerasMakeRecordingItem ) { /* 1.20 */
    JsonObject broadcast;
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );
    auto& dbClient = nma().getDBClient();
    CoreDHouseModes::setAlarmDelay( dbClient, 1 );

    const std::string gatewayId = getMainGateway( remoteInterface, __LINE__ );

    Model::Device device1( gatewayId, "Device 1", "sensor", "0_0_7", "security_sensor", false );
    device1.setSubcategory( "co" );
    auto deviceAddResult1 = remoteInterface.addDevice( device1 );
    CoreDRemoteInterface::CoreDInterface::DeviceID& securitySensorId = deviceAddResult1.first;

    EXPECT_FALSE( securitySensorId.empty() );
    EXPECT_TRUE( deviceAddResult1.second );
    broadcast = waitForBroadcast( "hub.device.added", NMA_TEST_MAKE_MESSAGE( "device added" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.added" );

    Model::Item securityThreatItem( securitySensorId,
                                    "security_threat",
                                    Model::ValueType::Enum::Bool,
                                    true,
                                    false,
                                    Model::ValueFactory::createDefaultItemValue( Model::ValueType::Enum::Bool ) );
    std::string securityThreatItemId;
    bool isGood = false;
    auto itemAddResult1 = remoteInterface.addItem( securityThreatItem );
    std::tie( securityThreatItemId, isGood ) = itemAddResult1;

    EXPECT_FALSE( securityThreatItemId.empty() );
    EXPECT_TRUE( isGood );
    broadcast = waitForBroadcast( "hub.item.added", NMA_TEST_MAKE_MESSAGE( "item added" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.added" );
    broadcast =
        waitForBroadcast( "hub.device.updated", NMA_TEST_MAKE_MESSAGE( "hub.device.updated: add armed field" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );

    Model::Device device2( gatewayId, "Device 2", "camera", "0_7_0", "camera", false );
    auto deviceAddResult2 = remoteInterface.addDevice( device2 );
    const CoreDRemoteInterface::CoreDInterface::DeviceID& cameraDeviceId = deviceAddResult2.first;
    EXPECT_FALSE( cameraDeviceId.empty() );
    EXPECT_TRUE( deviceAddResult2.second );
    broadcast = waitForBroadcast( "hub.device.added", NMA_TEST_MAKE_MESSAGE( "device added" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.added" );

    Model::Item item2( cameraDeviceId,
                       "make_recording",
                       Model::ValueType::Enum::Int,
                       true,
                       true,
                       Model::ValueFactory::createDefaultItemValue( Model::ValueType::Enum::Int ) );
    auto itemAddResult2 = remoteInterface.addItem( item2 );
    CoreDRemoteInterface::CoreDInterface::ItemID& cameraMakeRecordingItemId = itemAddResult2.first;
    EXPECT_FALSE( cameraMakeRecordingItemId.empty() );
    EXPECT_TRUE( itemAddResult2.second );
    broadcast = waitForBroadcast( "hub.item.added", NMA_TEST_MAKE_MESSAGE( "item added" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.added" );

    auto houseModes = remoteInterface.getHouseModes();
    auto currentModeId = remoteInterface.getCurrentHouseModeId();

    houseModes = remoteInterface.getHouseModes();
    for( const auto& houseMode : houseModes ) {
        if( houseMode.m_id == currentModeId ) {
            LOGGER_TRACE( cc::trace( "House mode: " ).append( cc::j( houseMode.toJson() ) ) );
            break;
        }
    }

    int makeRecordingValue = 30;
    remoteInterface.updateItemValue(
        cameraMakeRecordingItemId, [makeRecordingValue]( CoreD::Model::ValueType valueType ) {
            return CoreD::Model::ValueFactory::createItemValue( std::move( valueType ), makeRecordingValue );
        } );
    checkBroadcast(
        cameraMakeRecordingItemId,
        "hub.item.updated",
        { { "name", "make_recording" }, { "value", makeRecordingValue } },
        NMA_TEST_MAKE_MESSAGE(
            "Artificial value change to enable broadcast from cored event about make_recording value change" ) );

    remoteInterface.updateItemValue( securityThreatItemId, []( CoreD::Model::ValueType valueType ) {
        return CoreD::Model::ValueFactory::createItemValue( std::move( valueType ), Json( true ) );
    } );
    checkBroadcast( securityThreatItemId,
                    "hub.item.updated",
                    { { "name", "security_threat" }, { "value", true } },
                    NMA_TEST_MAKE_MESSAGE( "" ) );
    testHouseModeAlarmedBroadcast( currentModeId, securitySensorId, __LINE__ );
    testHouseModeAlarmedBroadcast( currentModeId, securitySensorId, __LINE__ );
    testHouseModeAlarmedBroadcast( currentModeId, securitySensorId, __LINE__ );

    // No broadcasts from that item
    checkBroadcast(
        cameraMakeRecordingItemId, "hub.item.updated", { { "name", "make_recording" } }, NMA_TEST_MAKE_MESSAGE( "" ) );

    remoteInterface.removeDevice( securitySensorId );
    broadcast = waitForBroadcast( "hub.item.removed", NMA_TEST_MAKE_MESSAGE( "hub.item.removed" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", NMA_TEST_MAKE_MESSAGE( "hub.device.removed" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );

    remoteInterface.removeDevice( cameraDeviceId );
    broadcast = waitForBroadcast( "hub.item.removed", NMA_TEST_MAKE_MESSAGE( "hub.item.removed" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = waitForBroadcast( "hub.device.removed", NMA_TEST_MAKE_MESSAGE( "hub.device.removed" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
}

TEST_F( CoredServiceTests, removeSecurityThreatItem_removeDeviceFromHouseModes ) { /* 1.21 */
    JsonObject broadcast;
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );
    auto& dbClient = nma().getDBClient();
    CoreDHouseModes::setAlarmDelay( dbClient, 1 );

    const std::string gatewayId = getMainGateway( remoteInterface, __LINE__ );

    Model::Device device( gatewayId, "Device 1", "sensor", "0_0_7", "security_sensor", false );
    device.setSubcategory( "motion" );
    auto deviceAddResult = remoteInterface.addDevice( device );
    auto deviceId = deviceAddResult.first;
    EXPECT_FALSE( deviceAddResult.first.empty() );
    EXPECT_TRUE( deviceAddResult.second );
    checkBroadcast( "", "hub.device.added", {}, NMA_TEST_MAKE_MESSAGE( "item added" ) );

    Model::Item item( deviceAddResult.first,
                      "security_threat",
                      Model::ValueType::Enum::Bool,
                      true,
                      false,
                      Model::ValueFactory::createDefaultItemValue( Model::ValueType::Enum::Bool ) );
    bool isGood;
    std::string itemId;
    std::tie( itemId, isGood ) = remoteInterface.addItem( item );
    EXPECT_TRUE( isGood ) << "Item creation is good";
    EXPECT_FALSE( itemId.empty() );
    checkBroadcast( itemId, "hub.item.added", {}, NMA_TEST_MAKE_MESSAGE( "item added" ) );
    checkBroadcast( deviceId,
                    "hub.device.updated",
                    { { "armed", false } },
                    NMA_TEST_MAKE_MESSAGE( "device updated: armed field" ) );

    bool hasDisarmedDevices = false;
    auto houseModesEntries = remoteInterface.getHouseModes();
    for( auto& entry : houseModesEntries ) {
        LOGGER_TRACE( cc::trace( "House mode entry: " ) + cc::j( entry.toJsonDb() ) );
        hasDisarmedDevices |= entry.m_disarmedDeviceIds.empty();
        EXPECT_TRUE( entry.m_disarmedDefault );
        if( entry.m_id != "1" ) {
            EXPECT_TRUE( entry.m_disarmedDeviceIds.empty() );
        }
        else {
            EXPECT_FALSE( entry.m_disarmedDeviceIds.empty() )
                << "disarmedDevices should not be empty for house mode id=1";
        }
    }
    EXPECT_TRUE( hasDisarmedDevices ) << "motion should be in a list of disarmed devices for house mode = 1";

    LOGGER_TRACE( "removeItem called" );
    remoteInterface.removeItem( itemId );

    checkBroadcast( "",
                    "hub.modes.disarmed_devices.removed",
                    { { "disarmedDevice", deviceId }, { "modeId", "1" } },
                    NMA_TEST_MAKE_MESSAGE(
                        "remove device from house modes disarmed devices list, no changing of mode.disarmedDefault" ) );
    checkBroadcast( itemId, "hub.item.removed", {}, NMA_TEST_MAKE_MESSAGE( "remove security_threat item" ) );

    houseModesEntries = remoteInterface.getHouseModes();
    for( auto& entry : houseModesEntries ) {
        LOGGER_TRACE( cc::trace( "House mode entry: " ) + cc::j( entry.toJsonDb() ) );
        EXPECT_TRUE( entry.m_disarmedDefault );
        EXPECT_TRUE( entry.m_disarmedDeviceIds.empty() );
    }

    auto itemsNumber = remoteInterface.getItemsByDeviceId( deviceId ).size();
    removeDevice( remoteInterface, deviceId, itemsNumber, 0, __LINE__ );
}

TEST_F( CoredServiceTests, removeSecurityThreatItem_deviceArmedToFalse ) { /* 1.22 */
    JsonObject broadcast;
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );
    auto& dbClient = nma().getDBClient();
    CoreDHouseModes::setAlarmDelay( dbClient, 1 );

    const std::string gatewayId = getMainGateway( remoteInterface, __LINE__ );

    Model::Device device( gatewayId, "Device 1", "sensor", "0_0_7", "security_sensor", false );
    device.setSubcategory( "co" );
    auto deviceAddResult = remoteInterface.addDevice( device );
    auto deviceId = deviceAddResult.first;
    EXPECT_FALSE( deviceAddResult.first.empty() );
    EXPECT_TRUE( deviceAddResult.second );
    checkBroadcast( "", "hub.device.added", {}, NMA_TEST_MAKE_MESSAGE( "item added" ) );

    Model::Item item( deviceAddResult.first,
                      "security_threat",
                      Model::ValueType::Enum::Bool,
                      true,
                      false,
                      Model::ValueFactory::createDefaultItemValue( Model::ValueType::Enum::Bool ) );
    bool isGood;
    std::string itemId;
    std::tie( itemId, isGood ) = remoteInterface.addItem( item );
    EXPECT_TRUE( isGood ) << "Item creation is good";
    EXPECT_FALSE( itemId.empty() );
    checkBroadcast( itemId, "hub.item.added", {}, NMA_TEST_MAKE_MESSAGE( "item added" ) );
    checkBroadcast(
        deviceId, "hub.device.updated", { { "armed", true } }, NMA_TEST_MAKE_MESSAGE( "device updated: armed=true" ) );

    LOGGER_TRACE( "removeItem called" );
    remoteInterface.removeItem( itemId );

    checkBroadcast( itemId, "hub.item.removed", {}, NMA_TEST_MAKE_MESSAGE( "remove security_threat item" ) );
    checkBroadcast(
        deviceId, "hub.device.updated", { { "armed", false } }, NMA_TEST_MAKE_MESSAGE( "device.armed=false" ) );
}

namespace Resources {
const char ItemControlRoomsScript[] = "HUB:item_control/scripts/rooms";
const char HubRoomCreated[] = "hub.room.created";
const char HubRoomDeleted[] = "hub.room.deleted";
}  // namespace Resources

TEST_F( CoredServiceTests, coreGetRooms ) { /* 2.1 */
    auto& remoteInterface = makeRemote();
    Json room1 = remoteInterface.createRoom( "room1" );
    auto room1Id = json_get_str( room1, "_id" );
    EXPECT_FALSE( room1Id.empty() );
    checkBroadcast( room1Id, Resources::HubRoomCreated, {}, NMA_TEST_MAKE_MESSAGE( "room1 created" ) );

    Json room2 = remoteInterface.createRoom( "room2" );
    auto room2Id = json_get_str( room2, "_id" );
    EXPECT_FALSE( room2Id.empty() );
    checkBroadcast( room2Id, Resources::HubRoomCreated, {}, NMA_TEST_MAKE_MESSAGE( "room2 created" ) );

    Json getRoomsExpect2 = { { "test", "get_rooms" }, { "expect_size", 2 } };
    runScript( Resources::ItemControlRoomsScript, getRoomsExpect2, __LINE__ );

    remoteInterface.deleteAllRooms();
    checkBroadcast( "", Resources::HubRoomDeleted, {}, NMA_TEST_MAKE_MESSAGE( "room1 deleted" ) );
    checkBroadcast( "", Resources::HubRoomDeleted, {}, NMA_TEST_MAKE_MESSAGE( "room2 deleted" ) );

    Json getRoomsExpect0 = { { "test", "get_rooms" }, { "expect_size", 0 } };
    runScript( Resources::ItemControlRoomsScript, getRoomsExpect0, __LINE__ );
}

TEST_F( CoredServiceTests, coreGetRoom ) { /* 2.2 */
    auto& remoteInterface = makeRemote();
    Json room1 = remoteInterface.createRoom( "room1" );
    auto room1Id = json_get_str( room1, "_id" );
    EXPECT_FALSE( room1Id.empty() );

    checkBroadcast( room1Id, Resources::HubRoomCreated, {}, NMA_TEST_MAKE_MESSAGE( "room1 created" ) );

    Json room2 = remoteInterface.createRoom( "room2" );
    auto room2Id = json_get_str( room2, "_id" );
    EXPECT_FALSE( room2Id.empty() );
    checkBroadcast( room2Id, Resources::HubRoomCreated, {}, NMA_TEST_MAKE_MESSAGE( "room2 created" ) );

    Json getRoom1 = { { "test", "get_room" }, { "room_id", room1Id } };
    runScript( Resources::ItemControlRoomsScript, getRoom1, __LINE__ );

    Json getRoom2 = { { "test", "get_room" }, { "room_id", room2Id } };
    runScript( Resources::ItemControlRoomsScript, getRoom2, __LINE__ );

    Json getNonexistentRoom = { { "test", "get_room" }, { "room_id", "no_such_room" } };
    runFailedScript( Resources::ItemControlRoomsScript, getNonexistentRoom, __LINE__ );

    remoteInterface.deleteAllRooms();
    checkBroadcast( "", Resources::HubRoomDeleted, {}, NMA_TEST_MAKE_MESSAGE( "room1 deleted" ) );
    checkBroadcast( "", Resources::HubRoomDeleted, {}, NMA_TEST_MAKE_MESSAGE( "room2 deleted" ) );
}

TEST_F( CoredServiceTests, testItemValue ) /* 3.1 */ {
    const std::string scriptPath = "HUB:item_control/scripts/test_item_value";

    runScriptCheckReason( scriptPath,
                          JSON( "type"
                                << "string"
                                << "value" << 123 ),
                          "Unexpected value format: Unexpected Json format for value type 'string'",
                          __LINE__ );

    runScriptCheckReason( scriptPath,
                          JSON( "type"
                                << "345"
                                << "value"
                                << "123" ),
                          "Unexpected value format: Unexpected Json format for value type 'dictionary.string'",
                          __LINE__ );

    auto value = Json{ { "value", 1 }, { "scale", CoreD::Model::ValueScales::Ampere } };
    runScriptCheckReason( scriptPath,
                          JSON( "type"
                                << "temperature"
                                << "value" << value ),
                          "Scale not found: Scales not found for value type 'temperature'",
                          __LINE__ );

    value = Json{ "value", 1 };
    runScriptCheckReason( scriptPath,
                          JSON( "type"
                                << "temperature"
                                << "value" << value ),
                          "Unexpected value format: Unexpected Json format for value type 'temperature'",
                          __LINE__ );

    runScript( scriptPath,
               JSON( "type"
                     << "string"
                     << "value"
                     << "text" ),
               __LINE__ );
}

TEST_F( CoredServiceTests, checkFollowParentPropertyFromGetDevices ) {  // 4.1
    using namespace CoreD::Model::Resources;
    addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    std::string scriptPath = "HUB:item_control/scripts/get_devices_list";
    Json scriptParams = JSON( "testSelector"
                              << "get_devices" );
    runScriptWithoutChecks( scriptPath, scriptParams, __LINE__ );

    JsonObject broadcast;
    broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", getCodeLineString( __LINE__ ) );
    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    EXPECT_FALSE( broadcast.empty() ) << cc::j( broadcast );
    EXPECT_EQ( json_get_str( broadcast, "id" ), "ui_broadcast" ) << cc::j( broadcast );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.extensions.plugin.ui_broadcast" ) << cc::j( broadcast );

    Json devices;
    std::vector< Json > keys{ "result", "devices" };
    ASSERT_TRUE( json_get( broadcast, keys, devices ) ) << "Json path is not found in the broadcast: " << broadcast;
    ASSERT_TRUE( devices.is_array() );
    ASSERT_FALSE( devices.empty() );
    ASSERT_EQ( devices.size(), 1 );

    const Json& theDevice = devices.at( 0 );
    Json device;
    ASSERT_FALSE( json_get( theDevice, "parent_room", device ) );
}

TEST_F( CoredServiceTests, checkFollowParentPropertyFromGetChildDevices ) {  // 4.2
    using namespace CoreD::Model::Resources;
    auto parentDevice = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    auto childDevice =
        addChildDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, parentDevice.m_deviceId, __LINE__ );
    std::string scriptPath = "HUB:item_control/scripts/get_devices_list";
    Json scriptParams = JSON( "testSelector"
                              << "get_devices" );
    runScriptWithoutChecks( scriptPath, scriptParams, __LINE__ );

    JsonObject broadcast;
    broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", getCodeLineString( __LINE__ ) );
    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    EXPECT_FALSE( broadcast.empty() ) << cc::j( broadcast );
    EXPECT_EQ( json_get_str( broadcast, "id" ), "ui_broadcast" ) << cc::j( broadcast );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.extensions.plugin.ui_broadcast" ) << cc::j( broadcast );

    Json devices;
    std::vector< Json > keys{ "result", "devices" };
    ASSERT_TRUE( json_get( broadcast, keys, devices ) ) << "Json path is not found in the broadcast: " << broadcast;
    ASSERT_TRUE( devices.is_array() );
    ASSERT_FALSE( devices.empty() );
    ASSERT_EQ( devices.size(), 2 );

    for( auto& device : devices ) {
        ASSERT_TRUE( device.is_object() );
        if( !json_get_str( device, "parentDeviceId" ).empty() ) {
            ASSERT_EQ( device[ "parent_room" ], true );
        }
    }

    auto& remoteInterface = makeRemote();
    removeDevice( remoteInterface, childDevice.m_deviceId, 1, 0, __LINE__ );
    removeDevice( remoteInterface, parentDevice.m_deviceId, 1, 0, __LINE__ );
}

TEST_F( CoredServiceTests, checkFollowParentPropertyFromGetDevice ) {  // 4.3
    using namespace CoreD::Model::Resources;
    auto deviceInfo = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    std::string scriptPath = "HUB:item_control/scripts/get_device";
    Json scriptParams = JSON( "id" << deviceInfo.m_deviceId );
    runScriptWithoutChecks( scriptPath, scriptParams, __LINE__ );

    JsonObject broadcast;
    broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", getCodeLineString( __LINE__ ) );
    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    EXPECT_FALSE( broadcast.empty() ) << cc::j( broadcast );
    EXPECT_EQ( json_get_str( broadcast, "id" ), "ui_broadcast" ) << cc::j( broadcast );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.extensions.plugin.ui_broadcast" ) << cc::j( broadcast );

    Json device;
    std::vector< Json > keys{ "result", "device" };
    ASSERT_TRUE( json_get( broadcast, keys, device ) );
    ASSERT_FALSE( json_get( broadcast, "parent_room", device ) );
}

TEST_F( CoredServiceTests, checkFollowParentPropertyFromGetChildDevice ) {  // 4.4
    using namespace CoreD::Model::Resources;
    auto parentDevice = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    auto childDevice =
        addChildDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, parentDevice.m_deviceId, __LINE__ );
    std::string scriptPath = "HUB:item_control/scripts/get_device";
    Json scriptParams = JSON( "id" << childDevice.m_deviceId );
    runScriptWithoutChecks( scriptPath, scriptParams, __LINE__ );

    JsonObject broadcast;
    broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", getCodeLineString( __LINE__ ) );
    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    EXPECT_FALSE( broadcast.empty() ) << cc::j( broadcast );
    EXPECT_EQ( json_get_str( broadcast, "id" ), "ui_broadcast" ) << cc::j( broadcast );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.extensions.plugin.ui_broadcast" ) << cc::j( broadcast );

    Json device;
    std::vector< Json > keys{ "result", "device" };
    ASSERT_TRUE( json_get( broadcast, keys, device ) ) << "Json path is not found in the broadcast: " << broadcast;
    ASSERT_TRUE( device.is_object() );
    ASSERT_FALSE( device.empty() );
    ASSERT_EQ( device[ "parent_room" ], true );

    auto& remoteInterface = makeRemote();
    removeDevice( remoteInterface, childDevice.m_deviceId, 1, 0, __LINE__ );
    removeDevice( remoteInterface, parentDevice.m_deviceId, 1, 0, __LINE__ );
}

TEST_F( CoredServiceTests, checkHubGetModeForAlarmedDevice ) {
    auto& dbClient = nma().getDBClient();
    const auto AlarmDelay = 1;
    CoreDHouseModes::setAlarmDelay( dbClient, AlarmDelay );
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );
    auto device = addDeviceWithSecurityThreatItem( __LINE__ );
    setItemValue( remoteInterface, device.m_itemId, true, __LINE__ );
    auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.get" }, { "api", "2.0" } } );
    auto alarmed = answer[ "result" ][ "alarmed" ];
    EXPECT_TRUE( !alarmed.empty() );
    EXPECT_EQ( json_get_int( alarmed, "entryDelay" ), AlarmDelay );
    EXPECT_EQ( json_get_int( alarmed, "timeIsLeft" ), AlarmDelay );
    auto sources = alarmed[ "sources" ];
    auto source = sources.front();
    EXPECT_EQ( json_get_int( source, "delay" ), AlarmDelay );
    EXPECT_EQ( json_get_str( source, "deviceId" ), device.m_deviceId );
    EXPECT_TRUE( json_get_number< int64_t >( source, "timestamp" ) > 0 );
    const auto CurrentModeId = "1";
    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, device.m_deviceId, "entryDelay", "begin", __LINE__ );
    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, device.m_deviceId, "entryDelay", "done", __LINE__ );
    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, device.m_deviceId, "main", "begin", __LINE__ );

    answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.get" }, { "api", "2.0" } } );
    EXPECT_TRUE( answer[ "result" ][ "alarmed" ].empty() );
}

TEST_F( CoredServiceTests, checkHubGetModeForAlarmedAndThenDisarmedDevice ) {
    auto& dbClient = nma().getDBClient();
    const auto AlarmDelay = 1;
    CoreDHouseModes::setAlarmDelay( dbClient, AlarmDelay );
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );
    auto device = addDeviceWithSecurityThreatItem( __LINE__ );
    setItemValue( remoteInterface, device.m_itemId, true, __LINE__ );
    auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.get" }, { "api", "2.0" } } );
    auto alarmed = answer[ "result" ][ "alarmed" ];
    EXPECT_TRUE( !alarmed.empty() );

    const auto CurrentModeId = "1";
    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, device.m_deviceId, "entryDelay", "begin", __LINE__ );
    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, device.m_deviceId, "entryDelay", "done", __LINE__ );
    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, device.m_deviceId, "main", "begin", __LINE__ );

    remoteInterface.setDeviceArmed( device.m_deviceId, false );
    auto broadcast = waitForBroadcast( "hub.device.updated", NMA_TEST_MAKE_MESSAGE( "no broadcast hub.device.armed" ) );

    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );
    setItemValue( remoteInterface, device.m_itemId, false, __LINE__ );
    answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.get" }, { "api", "2.0" } } );
    EXPECT_TRUE( answer[ "result" ][ "alarmed" ].empty() );
}

TEST_F( CoredServiceTests, checkHouseModeAlarmWithFewDevices ) {
    auto& dbClient = nma().getDBClient();
    const auto AlarmDelay = 1;
    CoreDHouseModes::setAlarmDelay( dbClient, AlarmDelay );
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );
    auto device1 = addDeviceWithSecurityThreatItem( __LINE__ );
    auto device2 = addDeviceWithSecurityThreatItem( __LINE__ );
    auto device3 = addDeviceWithSecurityThreatItem( __LINE__ );

    const auto CurrentModeId = "1";
    setItemValue( remoteInterface, device1.m_itemId, true, __LINE__ );
    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, device1.m_deviceId, "entryDelay", "begin", __LINE__ );
    setItemValue( remoteInterface, device2.m_itemId, true, __LINE__ );
    setItemValue( remoteInterface, device3.m_itemId, true, __LINE__ );

    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, device1.m_deviceId, "entryDelay", "done", __LINE__ );
    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, device1.m_deviceId, "main", "begin", __LINE__ );

    auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.get" }, { "api", "2.0" } } );
    EXPECT_TRUE( answer[ "result" ][ "alarmed" ].empty() );
}

TEST_F( CoredServiceTests, checkHouseModeAlarmWithSwitchToNotArmedMode ) {
    auto& dbClient = nma().getDBClient();
    const auto AlarmDelay = 1;
    CoreDHouseModes::setAlarmDelay( dbClient, AlarmDelay );
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );
    CoreDHouseModes::setSwitchDelay( dbClient, 0 );
    remoteInterface.setHouseMode( "2" );
    waitForBroadcast( "hub.modes.switched", NMA_TEST_MAKE_MESSAGE( "ignoring hub.modes.switched" ) );
    waitForBroadcast( "hub.modes.switched", NMA_TEST_MAKE_MESSAGE( "ignoring hub.modes.switched" ) );
    auto device = addDeviceWithSmokeSensorItem( __LINE__ );
    CoreDHouseModes::setSwitchDelay( dbClient, 0 );
    setItemValue( remoteInterface, device.m_itemId, true, __LINE__ );
    testHouseModeAlarmedPhaseStatusBroadcast( "2", device.m_deviceId, "entryDelay", "begin", __LINE__ );
    remoteInterface.setHouseMode( "1" );
    waitForBroadcast( "hub.modes.switched", NMA_TEST_MAKE_MESSAGE( "ignoring hub.modes.switched" ) );
    waitForBroadcast( "hub.modes.switched", NMA_TEST_MAKE_MESSAGE( "ignoring hub.modes.switched" ) );
    testHouseModeAlarmedPhaseStatusBroadcast( "1", "", "entryDelay", "canceled", __LINE__ );
    auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.get" }, { "api", "2.0" } } );
    EXPECT_TRUE( answer[ "result" ][ "alarmed" ].empty() );
}

TEST_F( CoredServiceTests, checkHouseModeAlarmWithNoDalayMode ) {
    auto& dbClient = nma().getDBClient();
    const auto AlarmDelay = 0;
    CoreDHouseModes::setAlarmDelay( dbClient, AlarmDelay );
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );
    auto device = addDeviceWithSecurityThreatItem( __LINE__ );
    setItemValue( remoteInterface, device.m_itemId, true, __LINE__ );
    testHouseModeAlarmedPhaseStatusBroadcast( "1", device.m_deviceId, "main", "begin", __LINE__ );
    auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.get" }, { "api", "2.0" } } );
    EXPECT_TRUE( answer[ "result" ][ "alarmed" ].empty() );
}

TEST_F( CoredServiceTests, checkHouseModeAlarmWithCancelEntryDalay ) {
    auto& dbClient = nma().getDBClient();
    const auto AlarmDelay = 1;
    CoreDHouseModes::setAlarmDelay( dbClient, AlarmDelay );
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );
    auto device = addDeviceWithSecurityThreatItem( __LINE__ );
    setItemValue( remoteInterface, device.m_itemId, true, __LINE__ );
    const auto CurrentModeId = "1";
    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, device.m_deviceId, "entryDelay", "begin", __LINE__ );
    auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.entry_delay.cancel" } } );
    EXPECT_TRUE( answer[ "result" ].empty() );
    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, "", "entryDelay", "canceled", __LINE__ );
    answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.get" }, { "api", "2.0" } } );
    EXPECT_TRUE( answer[ "result" ][ "alarmed" ].empty() );
}

TEST_F( CoredServiceTests, checkHouseModeAlarmWithSkipEntryDalay ) {
    auto& dbClient = nma().getDBClient();
    const auto AlarmDelay = 2;
    CoreDHouseModes::setAlarmDelay( dbClient, AlarmDelay );
    CoreDRemoteInterface::CoreDInterface& remoteInterface = makeRemote( nma() );
    auto device = addDeviceWithSecurityThreatItem( __LINE__ );
    setItemValue( remoteInterface, device.m_itemId, true, __LINE__ );
    const auto CurrentModeId = "1";
    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, device.m_deviceId, "entryDelay", "begin", __LINE__ );
    auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.get" }, { "api", "2.0" } } );
    if( !( answer[ "result" ][ "alarmed" ].empty() ) ) {
        answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.entry_delay.skip" } } );
        EXPECT_TRUE( answer[ "error" ].empty() );
        testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, "", "main", "begin", __LINE__ );
    }
}

TEST_F( CoredServiceTests, checkHouseModeAlarmNotInEntryDalayState ) {
    auto& dbClient = nma().getDBClient();
    const auto AlarmDelay = 0;
    CoreDHouseModes::setAlarmDelay( dbClient, AlarmDelay );
    CoreDRemoteInterface::CoreDInterface& remoteInterface = makeRemote( nma() );
    auto device = addDeviceWithSecurityThreatItem( __LINE__ );
    setItemValue( remoteInterface, device.m_itemId, true, __LINE__ );
    const auto CurrentModeId = "1";
    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, device.m_deviceId, "main", "begin", __LINE__ );
    auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.get" }, { "api", "2.0" } } );
    if( answer[ "result" ][ "alarmed" ].empty() ) {
        answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.entry_delay.skip" } } );
        EXPECT_FALSE( answer[ "error" ].empty() );
        auto error = answer[ "error" ];
        EXPECT_EQ( error[ "data" ], "ezlo.house_mode.alarmed.phase.invalid" );
        EXPECT_EQ( error[ "message" ], "House Modes aren't in the Entry Delay state" );
    }
}

TEST_F( CoredServiceTests, checkHouseModeAlarmWithBeginAndDone ) {
    auto& dbClient = nma().getDBClient();
    const auto AlarmDelay = 0;
    CoreDHouseModes::setAlarmDelay( dbClient, AlarmDelay );
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );

    CoreDHouseModes::setSwitchDelay( dbClient, 0 );
    remoteInterface.setHouseMode( "2" );
    waitForBroadcast( "hub.modes.switched", NMA_TEST_MAKE_MESSAGE( "ignoring hub.modes.switched" ) );
    waitForBroadcast( "hub.modes.switched", NMA_TEST_MAKE_MESSAGE( "ignoring hub.modes.switched" ) );

    auto device = addDeviceWithSecurityThreatItem( __LINE__ );
    setItemValue( remoteInterface, device.m_itemId, true, __LINE__ );
    testHouseModeAlarmedPhaseStatusBroadcast( "2", device.m_deviceId, "main", "begin", __LINE__ );
    CoreDHouseModes::setSwitchDelay( dbClient, 0 );
    remoteInterface.setHouseMode( "1" );
    waitForBroadcast( "hub.modes.switched", NMA_TEST_MAKE_MESSAGE( "ignoring hub.modes.switched" ) );
    waitForBroadcast( "hub.modes.switched", NMA_TEST_MAKE_MESSAGE( "ignoring hub.modes.switched" ) );
    testHouseModeAlarmedPhaseStatusBroadcast( "1", device.m_deviceId, "main", "done", __LINE__ );
}

TEST_F( CoredServiceTests, luaCoreUiBroadcastFail ) {
    auto request =
        replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", "HUB:model_factory/scripts/send_ui_broadcast" } } );

    auto answer = nma().sendtoHubAndGetAnswer( request );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );

    auto broadcast = waitForBroadcast( "hub.extensions.plugin.run.progress",
                                       NMA_TEST_MAKE_MESSAGE( "hub.extensions.plugin.run.progress" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.extensions.plugin.run.progress" );
    JsonObject result, error;
    json_get_obj( broadcast, "result", result );
    EXPECT_FALSE( result[ "error" ].empty() );
    EXPECT_EQ( json_get_str( result, "status" ), "failed" );
    json_get_obj( result, "error", error );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.lua.script.error" );
    EXPECT_EQ( json_get_str( error, "message" ), "Script error" );
    auto reason = json_get_str( error, "reason" );
    EXPECT_TRUE( reason.find( "fail to send ui broadcast: cannot use operator[] with string" ) );
}

TEST_F( CoredServiceTests, tcdsFileSizeHubSetting ) {
    auto& dbClient = nma().getDBClient();
    Hub::SettingsReader reader( dbClient );
    auto fileSizeSetting = reader.findSetting( "tcds.file_size_limit" );

    EXPECT_TRUE( fileSizeSetting );
    EXPECT_EQ( fileSizeSetting->getIntValue(), 100 );
}

}  // namespace Tests
}  // namespace HubPlatform
