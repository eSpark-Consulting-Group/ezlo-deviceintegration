#include "cored/model/cored_device.h"
#include "cored/model/values/value_factory.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "jsonrpc/jsonrpc.h"
#include "common_utils/hub_info_helper.h"
#include "bus/base_bus.h"
#include "common_test_resources/fixtures/core_test_fixture.h"
#include "common_test_resources/resources/cored_resources.h"
#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. cored.modifyDevice Lua API
DONE        1.1 Failed
DONE        1.2 Success

*/

namespace HubPlatform {
namespace Tests {

class LuaCoredDeviceTests : public CoreNmaTest {
public:
    void SetUp() override;
    void TearDown() override;
};

void LuaCoredDeviceTests::SetUp() {
    CoreNmaTest::SetUp();
    clearExistingDevices( __LINE__ );
    callingFile( __FILE__ );
}

void LuaCoredDeviceTests::TearDown() {
    clearExistingDevices( __LINE__ );
    CoreNmaTest::TearDown();
}

TEST_F( LuaCoredDeviceTests, addAndModifyDeviceWithParentRoomFieldFailed ) {
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );

    const std::string gatewayId = getMainGateway( remoteInterface, __LINE__ );

    CoreD::Model::Device device1( gatewayId, "smoke", "134_3_24", "security_sensor", "smoke", false /*batteryPowered*/
    );
    auto deviceId1 = addDevice( remoteInterface, device1, __LINE__ );
    ASSERT_FALSE( deviceId1.empty() );

    CoreD::Model::Device device2( gatewayId, "smoke", "134_3_24", "security_sensor", "smoke", false /*batteryPowered*/
    );
    device2.setParentDeviceId( deviceId1 );
    device2.setParentRoom( true );

    auto deviceId2 = addDevice( remoteInterface, device2, __LINE__ );
    ASSERT_FALSE( deviceId2.empty() );

    {
        auto request = replaceTokens( hubExtensionsPluginRun,
                                      { { "__SCRIPT__", "HUB:model_factory/scripts/core_modify_device" } } );
        auto& scriptParams = request[ "params" ][ "scriptParams" ];
        scriptParams = JSON_EMPTY_OBJ;
        scriptParams[ "device_id" ] = deviceId2;
        auto& deviceJson = scriptParams[ "device" ];
        deviceJson = JSON_EMPTY_OBJ;
        deviceJson[ "name" ] = "smoke";
        deviceJson[ "type" ] = "134_3_24";
        deviceJson[ "device_type_id" ] = "security_sensor";
        deviceJson[ "category" ] = "smoke";
        deviceJson[ "battery_powered" ] = false;
        deviceJson[ "parent_room" ] = false;

        auto answer = nma().sendtoHubAndGetAnswer( request );
        auto error = answer[ "error" ];
        ASSERT_TRUE( error.empty() ) << error.dump( 4 );

        checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );

        auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.extensions.plugin.run.progress" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.extensions.plugin.run.progress" );
        JsonObject result;
        json_get_obj( broadcast, "result", result );
        EXPECT_FALSE( result[ "error" ].empty() );
        EXPECT_EQ( json_get_str( result, "status" ), "failed" );
        json_get_obj( result, "error", error );
        EXPECT_EQ( json_get_str( error, "data" ), "ezlo.lua.script.error" );
        EXPECT_EQ( json_get_str( error, "message" ), "Script error" );
        auto reason = json_get_str( error, "reason" );
        EXPECT_TRUE( reason.find( "Failed to modify parent_room field: allowed only for child devices" ) );
    }

    {
        Json hubDevicesList;
        ASSERT_NO_THROW( hubDevicesList = Json::parse( hubDevicesListRequest ) );

        Json answer = nma().sendtoHubAndGetAnswer( hubDevicesList );
        ASSERT_FALSE( answer.empty() ) << "should not be empty";

        Json devices;
        ASSERT_TRUE( json_get( answer, { "result", "devices" }, devices ) ) << "should have result.devices collection";
        ASSERT_TRUE( devices.is_array() && devices.size() == 2 );
    }
}

TEST_F( LuaCoredDeviceTests, addAndModifyDeviceWithParentRoomFieldSuccess ) {
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );

    const std::string gatewayId = getMainGateway( remoteInterface, __LINE__ );

    CoreD::Model::Device device1( gatewayId, "smoke", "134_3_24", "security_sensor", "smoke", false /*batteryPowered*/
    );
    auto deviceId1 = addDevice( remoteInterface, device1, __LINE__ );
    ASSERT_FALSE( deviceId1.empty() );

    CoreD::Model::Device device2( gatewayId, "smoke", "134_3_24", "security_sensor", "smoke", false /*batteryPowered*/
    );
    device2.setParentDeviceId( deviceId1 );
    device2.setParentRoom( true );

    auto deviceId2 = addDevice( remoteInterface, device2, __LINE__ );
    ASSERT_FALSE( deviceId2.empty() );

    {
        auto request = replaceTokens( hubExtensionsPluginRun,
                                      { { "__SCRIPT__", "HUB:model_factory/scripts/core_modify_device" } } );
        auto& scriptParams = request[ "params" ][ "scriptParams" ];
        scriptParams = JSON_EMPTY_OBJ;
        scriptParams[ "device_id" ] = deviceId2;
        auto& deviceJson = scriptParams[ "device" ];
        deviceJson = JSON_EMPTY_OBJ;
        deviceJson[ "name" ] = "smoke";
        deviceJson[ "type" ] = "134_3_24";
        deviceJson[ "device_type_id" ] = "security_sensor";
        deviceJson[ "category" ] = "smoke";
        deviceJson[ "battery_powered" ] = false;
        deviceJson[ "parent_device_id" ] = deviceId1;
        deviceJson[ "parent_room" ] = false;

        auto answer = nma().sendtoHubAndGetAnswer( request );
        auto error = answer[ "error" ];
        ASSERT_TRUE( error.empty() ) << error.dump( 4 );

        auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.updated" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );

        checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
        checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
    }

    {
        Json hubDevicesList;
        ASSERT_NO_THROW( hubDevicesList = Json::parse( hubDevicesListRequest ) );

        Json answer = nma().sendtoHubAndGetAnswer( hubDevicesList );
        ASSERT_FALSE( answer.empty() ) << "should not be empty";

        Json devices;
        ASSERT_TRUE( json_get( answer, { "result", "devices" }, devices ) ) << "should have result.devices collection";
        ASSERT_TRUE( devices.is_array() && devices.size() == 2 );
    }
}

}  // namespace Tests
}  // namespace HubPlatform
