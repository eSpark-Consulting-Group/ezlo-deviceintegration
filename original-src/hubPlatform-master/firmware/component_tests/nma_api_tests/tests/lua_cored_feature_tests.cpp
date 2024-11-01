#include "common_test_resources/fixtures/core_test_fixture.h"
#include "cored/model/resources/cored_resources.h"
#include "jsonrpc/jsonrpc.h"
#include "common_test_resources/resources/cored_resources.h"
#include "base_utils/utils.h"
#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. Lua feature update event
DONE    2. Lua get feature status
*/

namespace HubPlatform {
namespace Tests {

namespace Resources {
static char SubscribeScript[] = "HUB:model_factory/scripts/core_event_subscribe";
static char UnSubscribeScript[] = "HUB:model_factory/scripts/core_event_unsubscribe";
}  // namespace Resources

using namespace CoreD;
using namespace CoreDHouseModes;
using namespace Services;

static const char* const Feature = "feature";

class LuaFeatureTests : public CoreNmaTest {
public:
    void SetUp() override;
    void TearDown() override;
    static void SetUpTestCase();
    JsonObject checkUiBroadcast( int line );
    void executeScript( const std::string& scriptPath, int line );
};

void LuaFeatureTests::SetUp() {
    CoreNmaTest::SetUp();
    clearExistingDevices( __LINE__ );
    callingFile( __FILE__ );
}

void LuaFeatureTests::TearDown() {
    clearExistingDevices( __LINE__ );
    CoreNmaTest::TearDown();
}

void LuaFeatureTests::SetUpTestCase() {
    auto& dbClient = nma().getDBClient();
    const bool editable = true;
    auto table = CoreD::Model::Resources::DBNames::FeaturesDB;
    Json record = JSON( "_id" << Feature << "editable" << editable << "is_custom" << false << "status"
                              << "off"
                              << "version"
                              << "1.0" );
    dbClient.insert( table, record );
}

JsonObject LuaFeatureTests::checkUiBroadcast( int line ) {
    JsonObject broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", "line: " + std::to_string( line ) );
    EXPECT_EQ( broadcast.empty(), false ) << __FILE__ << ":" << getCodeLineString( line );
    JsonObject params;
    EXPECT_TRUE( json_get_obj( broadcast, "result", params ) ) << "line: " << getCodeLineString( line );
    return params;
}

void LuaFeatureTests::executeScript( const std::string& scriptPath, int line ) {
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", scriptPath } } );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    checkExtensionsPluginRunProgressBroadcast( "started", line );

    checkExtensionsPluginRunProgressBroadcast( "finished", line );
}

TEST_F( LuaFeatureTests, featureUpdated ) {
    executeScript( Resources::SubscribeScript, __LINE__ );

    utils::AfterReturn onExit( [&]() { executeScript( Resources::UnSubscribeScript, __LINE__ ); } );
    auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.feature.status.set" },
                                                 { "params",
                                                   JSON( "feature" << Feature << "status"
                                                                   << "on" ) } } );

    JsonObject broadcast;
    broadcast = waitForBroadcast( "hub.feature.updated", getCodeLineString( __LINE__ ) );

    EXPECT_FALSE( broadcast.empty() ) << cc::j( broadcast );
    EXPECT_EQ( json_get_str( broadcast, "id" ), "ui_broadcast" ) << cc::j( broadcast );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.feature.updated" ) << cc::j( broadcast );
    EXPECT_TRUE( !broadcast[ "result" ].empty() );
    EXPECT_EQ( broadcast[ "result" ][ "_id" ], Feature );
    EXPECT_EQ( broadcast[ "result" ][ "status" ], true );

    broadcast = checkUiBroadcast( __LINE__ );
    EXPECT_EQ( json_get_str( broadcast, "event" ), "feature_updated" );
    EXPECT_EQ( json_get_str( broadcast, "name" ), Feature );
    EXPECT_EQ( json_get_str( broadcast, "status" ), "on" );
}

TEST_F( LuaFeatureTests, luaGetFeatureStatus ) {
    auto& dbClient = nma().getDBClient();
    const bool editable = true;

    auto table = CoreD::Model::Resources::DBNames::FeaturesDB;
    Json record =
        JSON( "_id" << Feature << "editable" << editable << "is_custom" << false << "status" << true << "version"
                    << "1.0" );
    dbClient.insert( table, record );

    auto request =
        replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", "HUB:model_factory/scripts/get_feature_status" } } );
    auto& scriptParams = request[ "params" ][ "scriptParams" ];

    scriptParams[ "feature_name" ] = Feature;

    auto answer = nma().sendtoHubAndGetAnswer( request );

    auto broadcast = checkUiBroadcast( __LINE__ );
    EXPECT_EQ( json_get_str( broadcast, "status" ), "on" );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
