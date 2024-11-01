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

DONE    1. cored.modifyItem Lua API
DONE        1.1 Failed
DONE        1.2 Success

*/

namespace HubPlatform {
namespace Tests {

class LuaCoredItemTests : public CoreNmaTest {
public:
    void SetUp() override;
    void TearDown() override;

protected:
    void addItemLuaApi( const std::string& deviceId );
    std::string m_itemId;
};

void LuaCoredItemTests::SetUp() {
    CoreNmaTest::SetUp();
    clearExistingDevices( __LINE__ );
    callingFile( __FILE__ );
}

void LuaCoredItemTests::TearDown() {
    clearExistingDevices( __LINE__ );
    CoreNmaTest::TearDown();
}

void LuaCoredItemTests::addItemLuaApi( const std::string& deviceId ) {
    auto request =
        replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", "HUB:model_factory/scripts/core_add_item" } } );
    auto& scriptParams = request[ "params" ][ "scriptParams" ];
    scriptParams = JSON_EMPTY_OBJ;
    auto& item = scriptParams[ "item" ];
    item = JSON_EMPTY_OBJ;
    item[ "device_id" ] = deviceId;
    item[ "name" ] = "testItem1";
    item[ "value_type" ] = "int";
    item[ "value" ] = 0;
    item[ "has_getter" ] = true;
    item[ "has_setter" ] = true;
    item[ "value_min" ] = 0;
    item[ "value_max" ] = 100;

    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    JsonObject result;
    auto broadcast = waitForBroadcast( "hub.item.added", "code line: " + std::to_string( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.added" );
    json_get_obj( broadcast, "result", result );
    m_itemId = json_get_str( result, "_id" );
    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( LuaCoredItemTests, addAndModifyItemWithMinMaxFieldsFailed ) {
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );

    const std::string gatewayId = getMainGateway( remoteInterface, __LINE__ );
    CoreD::Model::Device device( gatewayId, "smoke", "134_3_24", "security_sensor", "smoke", false /*batteryPowered*/
    );

    auto deviceId = addDevice( remoteInterface, device, __LINE__ );
    ASSERT_FALSE( deviceId.empty() );

    addItemLuaApi( deviceId );

    {
        auto request =
            replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", "HUB:model_factory/scripts/core_modify_item" } } );
        auto& scriptParams = request[ "params" ][ "scriptParams" ];
        scriptParams = JSON_EMPTY_OBJ;
        scriptParams[ "item_id" ] = m_itemId;
        auto& item = scriptParams[ "item" ];
        item = JSON_EMPTY_OBJ;
        item[ "device_id" ] = deviceId;
        item[ "name" ] = "testItem2";
        item[ "value_type" ] = "temperature";
        item[ "has_getter" ] = true;
        item[ "has_setter" ] = true;
        auto& value = item[ "value" ];
        value = JSON_EMPTY_OBJ;
        value[ "value" ] = 0;
        value[ "scale" ] = "celsius";

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
        EXPECT_TRUE( reason.find( "Modify item should contain value_min and value_max fields" ) );
    }

    auto answer = nma().sendtoHubAndGetAnswer( hubItemsListRequest );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    auto items = answer[ "result" ][ "items" ];
    ASSERT_EQ( items.size(), 1 );
    ASSERT_EQ( json_get_str( items[ 0 ], "_id" ), m_itemId );
    ASSERT_EQ( json_get_str( items[ 0 ], "name" ), "testItem1" );
    ASSERT_EQ( json_get_str( items[ 0 ], "valueType" ), "int" );
}

TEST_F( LuaCoredItemTests, addAndModifyItemWithMinMaxFieldsSuccess ) {
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );

    const std::string gatewayId = getMainGateway( remoteInterface, __LINE__ );
    CoreD::Model::Device device( gatewayId, "smoke", "134_3_24", "security_sensor", "smoke", false /*batteryPowered*/
    );

    auto deviceId = addDevice( remoteInterface, device, __LINE__ );
    ASSERT_FALSE( deviceId.empty() );

    addItemLuaApi( deviceId );

    {
        auto request =
            replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", "HUB:model_factory/scripts/core_modify_item" } } );
        auto& scriptParams = request[ "params" ][ "scriptParams" ];
        scriptParams = JSON_EMPTY_OBJ;
        scriptParams[ "item_id" ] = m_itemId;
        auto& item = scriptParams[ "item" ];
        item = JSON_EMPTY_OBJ;
        item[ "device_id" ] = deviceId;
        item[ "name" ] = "testItem2";
        item[ "value_type" ] = "temperature";
        item[ "has_getter" ] = true;
        item[ "has_setter" ] = true;
        auto& value = item[ "value" ];
        value = JSON_EMPTY_OBJ;
        value[ "value" ] = 0;
        value[ "scale" ] = "celsius";
        auto& valueMin = item[ "value_min" ];
        valueMin[ "value" ] = 0;
        valueMin[ "scale" ] = "celsius";
        auto& valueMax = item[ "value_max" ];
        valueMax[ "value" ] = 100;
        valueMax[ "scale" ] = "celsius";

        auto answer = nma().sendtoHubAndGetAnswer( request );
        auto error = answer[ "error" ];
        ASSERT_TRUE( error.empty() ) << error.dump( 4 );

        auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.item.updated" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" );

        checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
        checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
    }

    auto answer = nma().sendtoHubAndGetAnswer( hubItemsListRequest );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );
    auto items = answer[ "result" ][ "items" ];
    ASSERT_EQ( items.size(), 1 );
    ASSERT_EQ( json_get_str( items[ 0 ], "_id" ), m_itemId );
    ASSERT_EQ( json_get_str( items[ 0 ], "name" ), "testItem2" );
    ASSERT_EQ( json_get_str( items[ 0 ], "valueType" ), "temperature" );
}

}  // namespace Tests
}  // namespace HubPlatform
