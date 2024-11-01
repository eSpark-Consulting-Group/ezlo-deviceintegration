#include "cored/model/cored_item.h"
#include "cored/model/resources/cored_resources.h"

#include "cored/remote_interface/cored_remote_interface.h"
#include "jsonrpc/jsonrpc.h"
#include "common_utils/hub_info_helper.h"
#include "bus/base_bus.h"
#include "common_test_resources/resources/cored_resources.h"

#include "common_test_resources/fixtures/core_test_fixture.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

using namespace CoreD;
using namespace CoreDHouseModes;
using namespace Services;

class BusLimitationTests : public CoreNmaTest {
public:
    void SetUp() override;
    void TearDown() override;
};

void BusLimitationTests::SetUp() {
    CoreNmaTest::SetUp();
    clearExistingDevices( __LINE__ );
    callingFile( __FILE__ );
}

void BusLimitationTests::TearDown() {
    clearExistingDevices( __LINE__ );
    CoreNmaTest::TearDown();
}

TEST_F( BusLimitationTests, itemUpdateWithBigPayload ) {
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );

    auto deviceId = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::String );

    coreRemoteInterface.setItemValue(
        JSON( ItemFields::IdFieldName << deviceId.m_itemId << ItemFields::ValueFieldName << getBigString() ),
        []( const JsonObject& response ) {
            auto error = response[ "error" ];
            EXPECT_EQ( json_get_int( error, "code" ), -32603 );
            EXPECT_EQ( json_get_str( error, "data" ), "bus.message.too.long" );
            EXPECT_EQ( json_get_str( error, "message" ), "Bus message is too long" );
        } );
}

TEST_F( BusLimitationTests, luaItemValueWithPaylod ) {
    auto deviceId = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::String );
    auto params = JSON( "testSelector"
                        << "set_item_value"
                        << "itemId" << deviceId.m_itemId << "itemValue" << true );

    runScriptCheckReason(
        "HUB:item_control/scripts/item_update_with_big_data_handler.lua", params, "Bus message is too long", __LINE__ );
}

TEST_F( BusLimitationTests, luaAddItemWithPaylod ) {
    auto deviceId = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::String );
    auto params = JSON( "testSelector"
                        << "add_item"
                        << "deviceId" << deviceId.m_deviceId );
    std::string codeLineStr = getCodeLineString( __LINE__ );
    runScriptWithoutChecks( "HUB:item_control/scripts/item_update_with_big_data_handler.lua", params, __LINE__ );
    auto broadcast = waitForBroadcast( "hub.device.added", codeLineStr );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.added" ) << codeLineStr;

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcastByReason( "failed", "Bus message is too long", __LINE__ );
}

TEST_F( BusLimitationTests, runBigScript ) {
    auto params = JSON_EMPTY_OBJ;
    params[ "value" ] = getBigString();
    auto scriptWithParams = replaceTokens( hubExtensionsPluginRunWithParams,
                                           { { "__SCRIPT__", "scriptPath" }, { "__PARAMS__", params.dump() } } );
    auto answer = nma().sendtoHubAndGetAnswer( scriptWithParams );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_int( error, "code" ), -32603 );
    EXPECT_EQ( json_get_str( error, "data" ), "bus.message.too.long" );
    EXPECT_EQ( json_get_str( error, "message" ), "Bus message is too long" );
}

TEST_F( BusLimitationTests, requestValidHubItemValueSet ) /* 1.1.1 */ {
    auto deviceId = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::String );
    Json request = R"({ "method": "hub.item.value.set" })"_json;
    request[ "params" ][ "_id" ] = deviceId.m_itemId;
    request[ "params" ][ "value" ] = getBigString();

    auto answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_FALSE( answer.empty() );
    auto error = answer[ "error" ];

    EXPECT_EQ( json_get_int( error, "code" ), -32603 );
    EXPECT_EQ( json_get_str( error, "data" ), "bus.message.too.long" );
    EXPECT_EQ( json_get_str( error, "message" ), "Bus message is too long" );
}

}  // namespace Tests
}  // namespace HubPlatform
