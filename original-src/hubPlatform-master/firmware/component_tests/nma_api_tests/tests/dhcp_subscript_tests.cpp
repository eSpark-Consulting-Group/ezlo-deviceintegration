#include "common_test_resources/fixtures/core_test_fixture.h"
#include "common_test_resources/resources/cored_resources.h"

#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. lua network/dhcp events subscriptions addon
DONE        1.0 dhcp subscribe
DONE        1.1 dhcp unsubscribe

*/

namespace HubPlatform {
namespace Tests {

class DhcpNetworkLuadTests : public CoreNmaTest {
public:
    JsonObject checkUiBroadcast( const std::string& expectedValue, int line ) {
        JsonObject broadcast =
            waitForBroadcast( "hub.extensions.plugin.ui_broadcast", "line: " + std::to_string( line ) );
        EXPECT_EQ( broadcast.empty(), false );  // << "line: " << line;
        JsonObject params;
        EXPECT_TRUE( json_get_obj( broadcast, "result", params ) ) << "line: " << line;
        auto value = json_get_str( params, "value" );
        if( !expectedValue.empty() )
            EXPECT_EQ( value, expectedValue ) << "line: " << line;

        return broadcast;
    }

protected:
    static std::string m_subscriptionId;
};

std::string DhcpNetworkLuadTests::m_subscriptionId;

// resources
// gets dhcp subscribe/unsubscribe commands
static char subscribeScript[] = "HUB:network_dhcp/scripts/dhcp_subscribe";
// gets events fired by dhcp listener
static char eventsScript[] = "HUB:network_dhcp/scripts/dhcp_events";
static char startScanScript[] = "HUB:network_dhcp/scripts/start_scan";

TEST_F( DhcpNetworkLuadTests, dhcpSubscribe ) {
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", subscribeScript } } );

    request[ "params" ][ "scriptParams" ] = JSON_EMPTY_OBJ;

    request[ "params" ][ "scriptParams" ][ "command" ] = "subscribe";
    request[ "params" ][ "scriptParams" ][ "path" ] = eventsScript;
    request[ "params" ][ "scriptParams" ][ "operations" ] = JSON_ARRAY_( "request", "offer", "response", "discovery" );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    auto broadcast = checkUiBroadcast( "", __LINE__ );
    m_subscriptionId = json_get_str( broadcast[ "result" ], "value" );
    ASSERT_FALSE( m_subscriptionId.empty() ) << "Should not be empty";

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( DhcpNetworkLuadTests, dhcpUnsubscribe ) {
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", subscribeScript } } );

    request[ "params" ][ "scriptParams" ] = JSON_EMPTY_OBJ;

    request[ "params" ][ "scriptParams" ][ "command" ] = "unsubscribe";
    request[ "params" ][ "scriptParams" ][ "id" ] = m_subscriptionId;

    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    std::string expectedValue = "Ok";
    auto broadcast = checkUiBroadcast( expectedValue, __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( DhcpNetworkLuadTests, startScanNetwork ) {
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", startScanScript } } );

    Json params = JSON_EMPTY_OBJ;

    params[ "command" ] = "start_scan";
    params[ "script" ] = startScanScript;
    params[ "ip_addresses" ] = "127.0.0.1";

    request[ "params" ][ "scriptParams" ] = params;

    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error;

    auto broadcast = checkUiBroadcast( "", __LINE__ );
    int scanId = json_get_int( broadcast[ "result" ], "value" );
    ASSERT_GT( scanId, 0 ) << "Should be positive number";

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
