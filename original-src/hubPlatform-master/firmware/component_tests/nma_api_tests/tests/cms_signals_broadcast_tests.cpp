#include "cored/model/cored_device.h"
#include "cored/model/cored_item.h"
#include "cored/model/values/value_factory.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "jsonrpc/jsonrpc.h"
#include "common_utils/hub_info_helper.h"
#include "bus/base_bus.h"
#include "common_test_resources/fixtures/core_test_fixture.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN:

DONE    1 Protect Notification Flag
DONE        1.1 Device reachable
DONE        1.2 protect item
DONE        1.3 Intrusion item
DONE        1.4 tampered item
DONE        1.5 battery item
DONE        1.6 Motion item

DONE    2 Protect Notification in Disarmed houseMode
DONE        2.1 smoke sensor
DONE        2.2 co sensor
DONE        2.3 glass breakage sensor

*/

using namespace CoreD;
using namespace CoreDHouseModes;
using namespace Services;

class CmsSignalsBroadcastTest : public CoreNmaTest {
public:
    void SetUp() override;

    void TearDown() override;

    static void checkProtectFeatureStatus();
    void setHouseModeToArmedState();
    void resetHouseModes();
    void checkItemUpdatedBroadcast( const std::string& itemId,
                                    const std::string& itemName,
                                    const std::string& itemValue,
                                    bool protectNotification );
    void checkItemUpdatedBroadcast( const std::string& itemId,
                                    const std::string& itemName,
                                    int itemValue,
                                    bool protectNotification );
    void checkItemUpdatedBroadcast( const std::string& itemId,
                                    const std::string& itemName,
                                    bool itemValue,
                                    bool protectNotification );
};

void CmsSignalsBroadcastTest::SetUp() {
    CoreNmaTest::SetUp();
    clearExistingDevices( __LINE__ );
    callingFile( __FILE__ );
    checkProtectFeatureStatus();
}

void CmsSignalsBroadcastTest::TearDown() {
    clearExistingDevices( __LINE__ );
    CoreNmaTest::TearDown();
}

void CmsSignalsBroadcastTest::checkProtectFeatureStatus() {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto protectFeature = coreRemoteInterface.findFeature( "protect" );
    if( !protectFeature->getStatus() ) {
        coreRemoteInterface.setFeatureStatus( *protectFeature );
    }
}

void CmsSignalsBroadcastTest::setHouseModeToArmedState() {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    if( coreRemoteInterface.getCurrentHouseModeId() == "1" ) {
        coreRemoteInterface.setAlarmDelay( 0 );
        coreRemoteInterface.setSwitchDelay( 0 );
        coreRemoteInterface.setHouseMode( "2" );
        waitForBroadcast( "hub.modes.switched", NMA_TEST_MAKE_MESSAGE( "hub.modes.switched" ) );
        waitForBroadcast( "hub.modes.switched", NMA_TEST_MAKE_MESSAGE( "hub.modes.switched" ) );
    }
}

void CmsSignalsBroadcastTest::resetHouseModes() {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    coreRemoteInterface.setHouseMode( "1" );
    waitForBroadcast( "hub.modes.switched", NMA_TEST_MAKE_MESSAGE( "hub.modes.switched" ) );
    waitForBroadcast( "hub.device.updated", NMA_TEST_MAKE_MESSAGE( "hub.device.updated" ) );
    waitForBroadcast( "hub.modes.switched", NMA_TEST_MAKE_MESSAGE( "hub.modes.switched" ) );
}

void CmsSignalsBroadcastTest::checkItemUpdatedBroadcast( const std::string& itemId,
                                                         const std::string& itemName,
                                                         const std::string& itemValue,
                                                         bool protectNotification ) {
    auto broadcast = waitForBroadcast( "hub.item.updated", NMA_TEST_MAKE_MESSAGE( "item updated" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" );
    EXPECT_EQ( json_get_str( broadcast[ "result" ], "_id" ), itemId );
    EXPECT_EQ( json_get_str( broadcast[ "result" ], "name" ), itemName );
    EXPECT_EQ( json_get_bool( broadcast[ "result" ], "protectNotification" ), protectNotification );
    EXPECT_EQ( json_get_str( broadcast[ "result" ], "value" ), itemValue );
}

void CmsSignalsBroadcastTest::checkItemUpdatedBroadcast( const std::string& itemId,
                                                         const std::string& itemName,
                                                         int itemValue,
                                                         bool protectNotification ) {
    auto broadcast = waitForBroadcast( "hub.item.updated", NMA_TEST_MAKE_MESSAGE( "item updated" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" );
    EXPECT_EQ( json_get_str( broadcast[ "result" ], "_id" ), itemId );
    EXPECT_EQ( json_get_str( broadcast[ "result" ], "name" ), itemName );
    EXPECT_EQ( json_get_bool( broadcast[ "result" ], "protectNotification" ), protectNotification );
    EXPECT_EQ( json_get_int( broadcast[ "result" ], "value" ), itemValue );
}

void CmsSignalsBroadcastTest::checkItemUpdatedBroadcast( const std::string& itemId,
                                                         const std::string& itemName,
                                                         bool itemValue,
                                                         bool protectNotification ) {
    auto broadcast = waitForBroadcast( "hub.item.updated", NMA_TEST_MAKE_MESSAGE( "item updated" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" );
    EXPECT_EQ( json_get_str( broadcast[ "result" ], "_id" ), itemId );
    EXPECT_EQ( json_get_str( broadcast[ "result" ], "name" ), itemName );
    EXPECT_EQ( json_get_bool( broadcast[ "result" ], "protectNotification" ), protectNotification );
    EXPECT_EQ( json_get_bool( broadcast[ "result" ], "value" ), itemValue );
}

TEST_F( CmsSignalsBroadcastTest, deviceReachableFieldBroadcast ) {  // 1.1
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );

    {
        CoreD::Model::Device device(
            "testDevice1", "smoke", "134_3_24", "security_sensor", "smoke", false /*batteryPowered*/
        );

        std::string deviceId;

        deviceId = addDevice( coreRemoteInterface, device, __LINE__ );

        coreRemoteInterface.updateReachableState( deviceId, false );

        JsonObject broadcast, result;
        broadcast = waitForBroadcast( "hub.device.updated", getCodeLineString( __LINE__ ) );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );
        json_get_obj( broadcast, "result", result );
        EXPECT_FALSE( json_contains( result, "protectNotification" ) );
    }

    {
        setHouseModeToArmedState();
        CoreD::Model::Device device(
            "empty", "test device", "controller.panic", "door.sensor", "security_sensor", true );
        device.setSubcategory( "door" );
        auto deviceId = addDevice( coreRemoteInterface, device, __LINE__ );

        auto itemId = addItem( coreRemoteInterface,
                               deviceId,
                               "security_threat",
                               CoreD::Model::ValueType::Enum::Bool,
                               true,
                               true,
                               __LINE__ );

        checkDeviceUpdateBroadcast( deviceId, { { "armed", true } }, __LINE__ );

        coreRemoteInterface.updateReachableState( deviceId, false );

        JsonObject broadcast, result;
        broadcast = waitForBroadcast( "hub.device.updated", getCodeLineString( __LINE__ ) );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );
        json_get_obj( broadcast, "result", result );
        EXPECT_TRUE( json_contains( result, "protectNotification" ) );
        EXPECT_EQ( json_get_bool( result, "protectNotification" ), true );

        coreRemoteInterface.updateReachableState( deviceId, true );
        broadcast = waitForBroadcast( "hub.device.updated", getCodeLineString( __LINE__ ) );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );
        json_get_obj( broadcast, "result", result );
        EXPECT_TRUE( json_contains( result, "protectNotification" ) );
        EXPECT_EQ( json_get_bool( result, "protectNotification" ), true );
    }
}

TEST_F( CmsSignalsBroadcastTest, testProtectItemUpdateBroadcast ) /* 1.2 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    CoreD::Model::Device sosButton(
        "empty", "SOS Button", "controller.panic", "sos_protect_alarm", "sos_button", true );
    sosButton.setSubcategory( "police_alarm" );
    auto deviceId = addDevice( coreRemoteInterface, sosButton, __LINE__ );

    auto itemId = addItem(
        coreRemoteInterface, deviceId, "protect_alarm", CoreD::Model::ValueType::Enum::Token, true, true, __LINE__ );

    coreRemoteInterface.updateItemValue( itemId, []( CoreD::Model::ValueType valueType ) {
        return CoreD::Model::ValueFactory::createItemValue( std::move( valueType ), Json( "alarm_active" ) );
    } );
    std::string value = "alarm_active";
    checkItemUpdatedBroadcast( itemId, "protect_alarm", value, true );
    testHouseModeAlarmedBroadcast( "2", deviceId, __LINE__ );

    coreRemoteInterface.updateItemValue( itemId, []( CoreD::Model::ValueType valueType ) {
        return CoreD::Model::ValueFactory::createItemValue( std::move( valueType ), Json( "alarm_inactive" ) );
    } );
    value = "alarm_inactive";
    checkItemUpdatedBroadcast( itemId, "protect_alarm", value, true );
}

TEST_F( CmsSignalsBroadcastTest, testIntrusionItemUpdateBroadcast ) /* 1.3 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    CoreD::Model::Device device(
        "empty", "test device", "controller.panic", "intrusion.device", "security_sensor", true );
    device.setSubcategory( "intrusion" );
    auto deviceId = addDevice( coreRemoteInterface, device, __LINE__ );

    auto itemId = addItem(
        coreRemoteInterface, deviceId, "intrusion_alarm", CoreD::Model::ValueType::Enum::Token, true, true, __LINE__ );

    coreRemoteInterface.updateItemValue( itemId, []( CoreD::Model::ValueType valueType ) {
        return CoreD::Model::ValueFactory::createItemValue( std::move( valueType ), Json( "intrusion" ) );
    } );
    std::string value = "intrusion";
    checkItemUpdatedBroadcast( itemId, "intrusion_alarm", value, true );

    coreRemoteInterface.updateItemValue( itemId, []( CoreD::Model::ValueType valueType ) {
        return CoreD::Model::ValueFactory::createItemValue( std::move( valueType ), Json( "no_intrusion" ) );
    } );
    value = "no_intrusion";
    checkItemUpdatedBroadcast( itemId, "intrusion_alarm", value, true );
}

TEST_F( CmsSignalsBroadcastTest, testTamperedItemUpdateBroadcast ) /* 1.4 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    CoreD::Model::Device device(
        "empty", "test device", "controller.panic", "tampered.device", "security_sensor", true );
    device.setSubcategory( "tampered" );
    auto deviceId = addDevice( coreRemoteInterface, device, __LINE__ );

    auto itemId = addItem( coreRemoteInterface,
                           deviceId,
                           "tampering_cover_alarm",
                           CoreD::Model::ValueType::Enum::Token,
                           true,
                           true,
                           __LINE__ );

    coreRemoteInterface.updateItemValue( itemId, []( CoreD::Model::ValueType valueType ) {
        return CoreD::Model::ValueFactory::createItemValue( std::move( valueType ), Json( "tampering_cover" ) );
    } );
    std::string value = "tampering_cover";
    checkItemUpdatedBroadcast( itemId, "tampering_cover_alarm", value, true );

    coreRemoteInterface.updateItemValue( itemId, []( CoreD::Model::ValueType valueType ) {
        return CoreD::Model::ValueFactory::createItemValue( std::move( valueType ), Json( "no_tampering_cover" ) );
    } );
    value = "no_tampering_cover";
    checkItemUpdatedBroadcast( itemId, "tampering_cover_alarm", value, true );
}

TEST_F( CmsSignalsBroadcastTest, testBatteryItemUpdateBroadcast ) /* 1.5 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    CoreD::Model::Device device( "empty", "test device", "controller.panic", "batter.device", "security_sensor", true );
    device.setSubcategory( "battery" );
    auto deviceId = addDevice( coreRemoteInterface, device, __LINE__ );

    auto itemId =
        addItem( coreRemoteInterface, deviceId, "battery", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    coreRemoteInterface.updateItemValue( itemId, []( CoreD::Model::ValueType valueType ) {
        return CoreD::Model::ValueFactory::createItemValue( std::move( valueType ), Json( 15 ) );
    } );
    checkItemUpdatedBroadcast( itemId, "battery", 15, true );
}

TEST_F( CmsSignalsBroadcastTest, testMotionItemUpdateBroadcast ) /* 1.6 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    CoreD::Model::Device device( "empty", "test device", "controller.panic", "motion.sensor", "security_sensor", true );
    device.setSubcategory( "motion" );
    auto deviceId = addDevice( coreRemoteInterface, device, __LINE__ );

    auto itemId = addItem(
        coreRemoteInterface, deviceId, "security_threat", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );

    checkDeviceUpdateBroadcast( deviceId, { { "armed", true } }, __LINE__ );
    coreRemoteInterface.updateItemValue( itemId, []( CoreD::Model::ValueType valueType ) {
        return CoreD::Model::ValueFactory::createItemValue( std::move( valueType ), Json( true ) );
    } );
    checkItemUpdatedBroadcast( itemId, "security_threat", true, true );
    testHouseModeAlarmedBroadcast( "2", deviceId, __LINE__ );

    coreRemoteInterface.updateItemValue( itemId, []( CoreD::Model::ValueType valueType ) {
        return CoreD::Model::ValueFactory::createItemValue( std::move( valueType ), Json( false ) );
    } );
    checkItemUpdatedBroadcast( itemId, "security_threat", false, true );

    resetHouseModes();
    broadcast =
        waitForBroadcast( "hub.modes.alarmed", NMA_TEST_MAKE_MESSAGE( "ignoring housemodes alarmed broadcast" ) );
}

TEST_F( CmsSignalsBroadcastTest, smokeProtectNotification ) /* 2.1 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    EXPECT_EQ( coreRemoteInterface.getCurrentHouseModeId(), "1" );
    CoreD::Model::Device device( "empty", "test device", "controller.panic", "smoke.device", "security_sensor", true );
    device.setSubcategory( "smoke" );
    auto deviceId = addDevice( coreRemoteInterface, device, __LINE__ );

    auto itemId = addItem(
        coreRemoteInterface, deviceId, "smoke_alarm", CoreD::Model::ValueType::Enum::Token, true, true, __LINE__ );

    coreRemoteInterface.updateItemValue( itemId, []( CoreD::Model::ValueType valueType ) {
        return CoreD::Model::ValueFactory::createItemValue( std::move( valueType ), Json( "smoke_detected" ) );
    } );
    std::string value = "smoke_detected";
    checkItemUpdatedBroadcast( itemId, "smoke_alarm", value, true );

    coreRemoteInterface.updateItemValue( itemId, []( CoreD::Model::ValueType valueType ) {
        return CoreD::Model::ValueFactory::createItemValue( std::move( valueType ), Json( "no_smoke" ) );
    } );
    value = "no_smoke";
    checkItemUpdatedBroadcast( itemId, "smoke_alarm", value, true );
}

TEST_F( CmsSignalsBroadcastTest, coProtectNotification ) /* 2.2 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    EXPECT_EQ( coreRemoteInterface.getCurrentHouseModeId(), "1" );
    CoreD::Model::Device device( "empty", "test device", "controller.panic", "co.device", "security_sensor", true );
    device.setSubcategory( "co" );
    auto deviceId = addDevice( coreRemoteInterface, device, __LINE__ );

    auto itemId = addItem(
        coreRemoteInterface, deviceId, "co_alarm", CoreD::Model::ValueType::Enum::Token, true, true, __LINE__ );

    coreRemoteInterface.updateItemValue( itemId, []( CoreD::Model::ValueType valueType ) {
        return CoreD::Model::ValueFactory::createItemValue( std::move( valueType ), Json( "co_detected" ) );
    } );
    std::string value = "co_detected";
    checkItemUpdatedBroadcast( itemId, "co_alarm", value, true );

    coreRemoteInterface.updateItemValue( itemId, []( CoreD::Model::ValueType valueType ) {
        return CoreD::Model::ValueFactory::createItemValue( std::move( valueType ), Json( "no_co" ) );
    } );
    value = "no_co";
    checkItemUpdatedBroadcast( itemId, "co_alarm", value, true );
}

TEST_F( CmsSignalsBroadcastTest, glassBreakageProtectNotification ) /* 2.3 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    EXPECT_EQ( coreRemoteInterface.getCurrentHouseModeId(), "1" );
    CoreD::Model::Device device( "empty", "test device", "controller.panic", "glass.device", "security_sensor", true );
    device.setSubcategory( "glass_breakage" );
    auto deviceId = addDevice( coreRemoteInterface, device, __LINE__ );

    auto itemId = addItem( coreRemoteInterface,
                           deviceId,
                           "glass_breakage_alarm",
                           CoreD::Model::ValueType::Enum::Token,
                           true,
                           true,
                           __LINE__ );

    coreRemoteInterface.updateItemValue( itemId, []( CoreD::Model::ValueType valueType ) {
        return CoreD::Model::ValueFactory::createItemValue( std::move( valueType ), Json( "glass_breakage" ) );
    } );
    std::string value = "glass_breakage";
    checkItemUpdatedBroadcast( itemId, "glass_breakage_alarm", value, true );

    coreRemoteInterface.updateItemValue( itemId, []( CoreD::Model::ValueType valueType ) {
        return CoreD::Model::ValueFactory::createItemValue( std::move( valueType ), Json( "no_glass_breakage" ) );
    } );
    value = "no_glass_breakage";
    checkItemUpdatedBroadcast( itemId, "glass_breakage_alarm", value, true );
}

}  // namespace Tests
}  // namespace HubPlatform
