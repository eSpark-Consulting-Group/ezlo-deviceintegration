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

DONE    1. Protect signal list API
DONE        1.1 Success
DONE        1.2 Version field test
DONE        1.3 Version field error
DONE    2. Protect security config list API
DONE        2.1 Success
DONE        2.2 Version field test
DONE        2.3 Version field error
DONE        2.4 Ids parameter
DONE        2.5 Persistent parameter
DONE    3. By pass devices list filter
DONE    4. By pass devices api
DONE        4.1 allow to add
DONE        4.2 deny to add
DONE        4.3 partially allow
DONE    5. Set protectConfig shortcut method
*/

using namespace CoreD;
using namespace Services;

class ProtectTest : public CoreNmaTest {
public:
    using DeviceIds = std::vector< std::string >;

    void SetUp() override;

    void TearDown() override;

    std::string createDevice( const std::string& gatewayId, const std::string& name, const std::string& protectConfig );

    void removeBypassDevices( const std::string& houseModeId, const std::vector< std::string >& deviceIds );

    void setDeviceProtectConfigPass( const std::string& deviceId, const std::string& protectConfig );
    void setDeviceProtectConfigFail( const std::string& deviceId, const std::string& protectConfig );
};

void ProtectTest::SetUp() {
    CoreNmaTest::SetUp();
    callingFile( __FILE__ );
}

void ProtectTest::TearDown() {
    clearExistingDevices( __LINE__ );
    CoreNmaTest::TearDown();
}

std::string ProtectTest::createDevice( const std::string& gatewayId,
                                       const std::string& name,
                                       const std::string& protectConfig ) {
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );
    Model::Device device1( gatewayId, name, "test_device", "139_21586_21570", "sensor", false, protectConfig );
    device1.setSubcategory( "motion" );
    auto deviceId1 = remoteInterface.addDevice( device1 ).first;

    auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.added" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.added" );
    return deviceId1;
}

void ProtectTest::removeBypassDevices( const std::string& houseModeId, const std::vector< std::string >& deviceIds ) {
    auto answer = nma().sendtoHubAndGetAnswer(
        { { "method", "hub.modes.bypass_devices.remove" },
          { "params", JSON( "modeId" << houseModeId << "deviceIds" << Json( deviceIds ) ) } } );

    auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.bypass_devices.removed" );
    JsonObject result;
    json_get_obj( broadcast, "result", result );

    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.bypass_devices.removed" );
}

void ProtectTest::setDeviceProtectConfigPass( const std::string& deviceId, const std::string& protectConfig ) {
    JsonObject result, error;
    auto answer =
        nma().sendtoHubAndGetAnswer( { { "method", "hub.device.protect_config.set" },
                                       { "params", JSON( "_id" << deviceId << "protectConfig" << protectConfig ) } } );

    json_get_obj( answer, "error", error );
    ASSERT_TRUE( error.empty() );

    auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.updated" );
    json_get_obj( broadcast, "result", result );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );
    EXPECT_EQ( json_get_str( result, "protectConfig" ), protectConfig );
    EXPECT_TRUE( json_get_bool( result, "syncNotification" ) );
}

void ProtectTest::setDeviceProtectConfigFail( const std::string& deviceId, const std::string& protectConfig ) {
    JsonObject error;
    auto answer =
        nma().sendtoHubAndGetAnswer( { { "method", "hub.device.protect_config.set" },
                                       { "params", JSON( "_id" << deviceId << "protectConfig" << protectConfig ) } } );

    json_get_obj( answer, "error", error );
    ASSERT_FALSE( error.empty() );
}

TEST_F( ProtectTest, protectSignalListPass /* 1.1 */ ) {
    JsonObject result, error;

    const auto answer =
        nma().sendtoHubAndGetAnswer( { { "method", "hub.protect.signal.list" }, { "params", JSON_EMPTY_OBJ } } );

    json_get_obj( answer, "error", error );
    ASSERT_TRUE( error.empty() );

    json_get_obj( answer, "result", result );
    ASSERT_TRUE( json_contains( result, "aliases" ) );
    ASSERT_TRUE( json_contains( result, "version" ) );

    Json aliases;
    json_get( result, "aliases", aliases );
    ASSERT_TRUE( aliases.is_array() );
    ASSERT_TRUE( aliases.size() > 1 );

    for( const auto& alias : aliases ) {
        ASSERT_TRUE( json_contains( alias, "_id" ) );
        ASSERT_TRUE( json_contains( alias, "text" ) );
        ASSERT_TRUE( json_contains( alias, "on" ) );
        ASSERT_TRUE( json_contains( alias, "off" ) );
        ASSERT_TRUE( json_contains( alias, "persistent" ) );
    }
}

TEST_F( ProtectTest, protectSignalListVersion /* 1.2 */ ) {
    JsonObject result, error;

    auto answer =
        nma().sendtoHubAndGetAnswer( { { "method", "hub.protect.signal.list" }, { "params", JSON_EMPTY_OBJ } } );

    json_get_obj( answer, "error", error );
    ASSERT_TRUE( error.empty() );

    json_get_obj( answer, "result", result );
    ASSERT_TRUE( json_contains( result, "aliases" ) );
    ASSERT_TRUE( json_contains( result, "version" ) );
    auto version = json_get_str( result, "version" );

    answer = nma().sendtoHubAndGetAnswer(
        { { "method", "hub.protect.signal.list" }, { "params", JSON( "version" << version ) } } );

    json_get_obj( answer, "error", error );
    ASSERT_TRUE( error.empty() );

    json_get_obj( answer, "result", result );
    ASSERT_FALSE( json_contains( result, "aliases" ) );
    ASSERT_TRUE( json_contains( result, "version" ) );
    EXPECT_EQ( json_get_str( result, "version" ), version );
}

TEST_F( ProtectTest, protectSignalListVersionFailed /* 1.3 */ ) {
    JsonObject result, error;

    auto answer = nma().sendtoHubAndGetAnswer(
        { { "method", "hub.protect.signal.list" }, { "params", JSON( "version" << 12354 ) } } );

    json_get_obj( answer[ "result" ], "error", error );
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    EXPECT_EQ( json_get_str( error, "field" ), "version" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );
    EXPECT_EQ( json_get_str( error, "reason" ), "Wrong value type" );
}

TEST_F( ProtectTest, protectSecurityConfigsListPass /* 2.1 */ ) {
    JsonObject result, error;

    const auto answer = nma().sendtoHubAndGetAnswer(
        { { "method", "hub.protect.security.configs.list" }, { "params", JSON_EMPTY_OBJ } } );

    json_get_obj( answer, "error", error );
    ASSERT_TRUE( error.empty() );

    json_get_obj( answer, "result", result );
    ASSERT_TRUE( json_contains( result, "securityConfigs" ) );
    ASSERT_TRUE( json_contains( result, "version" ) );

    Json securityConfigs;
    json_get( result, "securityConfigs", securityConfigs );
    ASSERT_TRUE( securityConfigs.is_array() );
    ASSERT_TRUE( securityConfigs.size() > 1 );

    for( const auto& securityConfig : securityConfigs ) {
        ASSERT_TRUE( json_contains( securityConfig, "_id" ) );
        ASSERT_TRUE( json_contains( securityConfig, "name" ) );
        ASSERT_TRUE( json_contains( securityConfig, "description" ) );
        ASSERT_TRUE( json_contains( securityConfig, "alarm" ) );
        ASSERT_TRUE( json_contains( securityConfig, "soundType" ) );
        ASSERT_TRUE( json_contains( securityConfig, "entryDelay" ) );
        ASSERT_TRUE( json_contains( securityConfig, "chime" ) );
        ASSERT_TRUE( json_contains( securityConfig, "follower" ) );
        ASSERT_TRUE( json_contains( securityConfig, "armedWhen" ) );
        ASSERT_TRUE( json_contains( securityConfig, "recommendCategories" ) );
        ASSERT_TRUE( json_contains( securityConfig, "signal" ) );
        ASSERT_TRUE( securityConfig[ "alarm" ].is_boolean() );
        ASSERT_TRUE( securityConfig[ "armedWhen" ].is_array() );
        ASSERT_TRUE( securityConfig[ "recommendCategories" ].is_array() );
        ASSERT_TRUE( securityConfig[ "signal" ].is_object() );
    }
}

TEST_F( ProtectTest, protectSecurityConfigListVersion /* 2.2 */ ) {
    JsonObject result, error;

    auto answer = nma().sendtoHubAndGetAnswer(
        { { "method", "hub.protect.security.configs.list" }, { "params", JSON_EMPTY_OBJ } } );

    json_get_obj( answer, "error", error );
    ASSERT_TRUE( error.empty() );

    json_get_obj( answer, "result", result );
    ASSERT_TRUE( json_contains( result, "securityConfigs" ) );
    ASSERT_TRUE( json_contains( result, "version" ) );
    auto version = json_get_str( result, "version" );

    answer = nma().sendtoHubAndGetAnswer(
        { { "method", "hub.protect.security.configs.list" }, { "params", JSON( "version" << version ) } } );

    json_get_obj( answer, "error", error );
    ASSERT_TRUE( error.empty() );

    json_get_obj( answer, "result", result );
    ASSERT_FALSE( json_contains( result, "securityConfigs" ) );
    ASSERT_TRUE( json_contains( result, "version" ) );
    EXPECT_EQ( json_get_str( result, "version" ), version );
}

TEST_F( ProtectTest, protectSecurityConfigListVersionFailed /* 2.3 */ ) {
    JsonObject error;

    auto answer = nma().sendtoHubAndGetAnswer(
        { { "method", "hub.protect.security.configs.list" }, { "params", JSON( "version" << 12354 ) } } );

    json_get_obj( answer[ "result" ], "error", error );
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    EXPECT_EQ( json_get_str( error, "field" ), "version" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );
    EXPECT_EQ( json_get_str( error, "reason" ), "Wrong value type" );
}

TEST_F( ProtectTest, protectSecurityConfigsListIdsParams /* 2.4 */ ) {
    JsonObject result, error;

    const auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.protect.security.configs.list" },
                                                       { "params",
                                                         JSON( "ids" << JSON_ARRAY( "1"
                                                                                    << "2" ) ) } } );

    json_get_obj( answer, "error", error );
    ASSERT_TRUE( error.empty() );

    json_get_obj( answer, "result", result );
    ASSERT_TRUE( json_contains( result, "securityConfigs" ) );
    ASSERT_TRUE( json_contains( result, "version" ) );

    Json securityConfigs;
    json_get( result, "securityConfigs", securityConfigs );
    ASSERT_TRUE( securityConfigs.is_array() );
    ASSERT_EQ( securityConfigs.size(), 2 );
}

TEST_F( ProtectTest, protectSecurityConfigsListPersistentParams /* 2.5 */ ) {
    JsonObject result, error;

    const auto answer = nma().sendtoHubAndGetAnswer(
        { { "method", "hub.protect.security.configs.list" }, { "params", JSON( "persistent" << false ) } } );

    json_get_obj( answer, "error", error );
    ASSERT_TRUE( error.empty() );

    json_get_obj( answer, "result", result );
    ASSERT_TRUE( json_contains( result, "securityConfigs" ) );
    ASSERT_TRUE( json_contains( result, "version" ) );

    Json securityConfigs;
    json_get( result, "securityConfigs", securityConfigs );
    ASSERT_TRUE( securityConfigs.is_array() );
    ASSERT_EQ( securityConfigs.size(), 1 );

    for( const auto& securityConfig : securityConfigs ) {
        ASSERT_EQ( json_get_str( securityConfig, "_id" ), "default" );
        ASSERT_EQ( json_get_str( securityConfig, "name" ), "Default" );
        ASSERT_EQ( json_get_str( securityConfig, "description" ), "Default Security Config" );
        ASSERT_EQ( json_get_bool( securityConfig, "alarm" ), true );
        ASSERT_EQ( json_get_str( securityConfig, "soundType" ), "police" );
        ASSERT_EQ( json_get_str( securityConfig, "entryDelay" ), "normal" );
        ASSERT_EQ( json_get_str( securityConfig, "chime" ), "none" );
        ASSERT_EQ( json_get_str( securityConfig, "follower" ), "no" );
    }
}

TEST_F( ProtectTest, bypassDevicesListFilter /* 3 */ ) {
    JsonObject broadcast;
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );

    const std::string gatewayId = getMainGateway( remoteInterface, __LINE__ );

    createDevice( gatewayId, "Test Device 1", "0" );
    createDevice( gatewayId, "Test Device 2", "38" );  // pre define security config has bypass false
    createDevice( gatewayId, "Test Device 3", "39" );  // pre define security config has bypass false

    JsonObject error, result;

    {
        const auto answer =
            nma().sendtoHubAndGetAnswer( { { "method", "hub.devices.list" }, { "params", JSON( "" ) } } );

        json_get_obj( answer, "error", error );
        ASSERT_TRUE( error.empty() );

        json_get_obj( answer, "result", result );

        ASSERT_TRUE( result[ "devices" ].is_array() );
        EXPECT_EQ( result[ "devices" ].size(), 3 );
    }

    {
        const auto answer = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.devices.list" }, { "params", JSON( "filter" << JSON( "bypassAllowed" << true ) ) } } );

        json_get_obj( answer, "error", error );
        ASSERT_TRUE( error.empty() );

        json_get_obj( answer, "result", result );

        ASSERT_TRUE( result[ "devices" ].is_array() );
        EXPECT_EQ( result[ "devices" ].size(), 1 );
        EXPECT_EQ( json_get_str( result[ "devices" ][ 0 ], "name" ), "Test Device 1" );
    }
}

TEST_F( ProtectTest, bypassDevicesAddAllow /* 4.1 */ ) {
    DeviceIds deviceIds;
    JsonObject broadcast;
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );
    const std::string houseModeId = remoteInterface.getCurrentHouseModeId();

    const std::string gatewayId = getMainGateway( remoteInterface, __LINE__ );

    auto deviceId1 = createDevice( gatewayId, "Test Device 1", "0" );
    deviceIds.push_back( deviceId1 );
    auto deviceId2 = createDevice( gatewayId, "Test Device 2", "0" );
    deviceIds.push_back( deviceId2 );

    JsonObject error, result;

    const auto answer = nma().sendtoHubAndGetAnswer(
        { { "method", "hub.modes.bypass_devices.add" },
          { "params", JSON( "modeId" << houseModeId << "deviceIds" << Json( deviceIds ) ) } } );

    json_get_obj( answer, "error", error );
    ASSERT_TRUE( error.empty() );

    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.bypass_devices.added" );
    json_get_obj( broadcast, "result", result );

    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.bypass_devices.added" );
    EXPECT_EQ( json_get_str( result, "modeId" ), houseModeId );

    ASSERT_TRUE( result[ "deviceIds" ].is_array() );
    EXPECT_EQ( result[ "deviceIds" ].size(), deviceIds.size() );
    EXPECT_EQ( result[ "deviceIds" ][ 0 ], deviceId1 );
    EXPECT_EQ( result[ "deviceIds" ][ 1 ], deviceId2 );

    removeBypassDevices( houseModeId, deviceIds );
}

TEST_F( ProtectTest, bypassDevicesAddDeny /* 4.2 */ ) {
    DeviceIds deviceIds;
    JsonObject broadcast;
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );
    const std::string houseModeId = remoteInterface.getCurrentHouseModeId();

    const std::string gatewayId = getMainGateway( remoteInterface, __LINE__ );

    auto deviceId1 = createDevice( gatewayId, "Test Device 1", "38" );  // pre define security config has bypass false
    deviceIds.push_back( deviceId1 );
    auto deviceId2 = createDevice( gatewayId, "Test Device 2", "38" );  // pre define security config has bypass false
    deviceIds.push_back( deviceId2 );

    JsonObject error, result;

    const auto answer = nma().sendtoHubAndGetAnswer(
        { { "method", "hub.modes.bypass_devices.add" },
          { "params", JSON( "modeId" << houseModeId << "deviceIds" << Json( deviceIds ) ) } } );

    json_get_obj( answer, "error", error );
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( error[ "reason" ], "No valid devices to add" );
}

TEST_F( ProtectTest, bypassDevicesPartialAdd /* 4.3 */ ) {
    DeviceIds deviceIds;
    JsonObject broadcast;
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );
    const std::string houseModeId = remoteInterface.getCurrentHouseModeId();

    const std::string gatewayId = getMainGateway( remoteInterface, __LINE__ );

    auto deviceId1 = createDevice( gatewayId, "Test Device 1", "0" );
    deviceIds.push_back( deviceId1 );
    auto deviceId2 = createDevice( gatewayId, "Test Device 2", "38" );  // pre define security config has bypass false
    deviceIds.push_back( deviceId2 );
    auto deviceId3 = createDevice( gatewayId, "Test Device 3", "39" );  // pre define security config has bypass false
    deviceIds.push_back( deviceId3 );

    JsonObject error, result;

    const auto answer = nma().sendtoHubAndGetAnswer(
        { { "method", "hub.modes.bypass_devices.add" },
          { "params", JSON( "modeId" << houseModeId << "deviceIds" << Json( deviceIds ) ) } } );

    json_get_obj( answer, "error", error );
    ASSERT_TRUE( error.empty() );

    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.bypass_devices.added" );
    json_get_obj( broadcast, "result", result );

    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.bypass_devices.added" );
    EXPECT_EQ( json_get_str( result, "modeId" ), houseModeId );

    ASSERT_TRUE( result[ "deviceIds" ].is_array() );
    EXPECT_EQ( result[ "deviceIds" ].size(), 1 );
    EXPECT_EQ( result[ "deviceIds" ][ 0 ], deviceId1 );

    removeBypassDevices( houseModeId, { deviceId1 } );
}

TEST_F( ProtectTest, setDeviceProtectConfig /* 5 */ ) {
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );
    const std::string gatewayId = getMainGateway( remoteInterface, __LINE__ );

    auto deviceId = createDevice( gatewayId, "Test Device 1", "default" );

    setDeviceProtectConfigPass( deviceId, "0" );
    setDeviceProtectConfigFail( deviceId, "wibble" );
    setDeviceProtectConfigPass( deviceId, "38" );
    setDeviceProtectConfigFail( deviceId, "BadId" );
    setDeviceProtectConfigPass( deviceId, "39" );
    setDeviceProtectConfigFail( deviceId, "" );
    setDeviceProtectConfigPass( deviceId, "default" );
    setDeviceProtectConfigFail( "123456", "default" );
}

}  // namespace Tests
}  // namespace HubPlatform
