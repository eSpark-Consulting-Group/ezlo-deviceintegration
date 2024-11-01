#include "common_test_resources/fixtures/core_test_fixture.h"
#include "common_test_resources/resources/cored_resources.h"

#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. lua network socket timeout
DONE        1.0 socket set_timeout
DONE    2. lua network socket option
DONE        2.0 setoption broadcast
DONE        2.1 setoption ip-add-membership failed
DONE        2.2 setoption ip-add-membership success
DONE        2.3 setoption ip-multicast-ttl failed
DONE        2.4 setoption ip-multicast-ttl success

*/

namespace HubPlatform {
namespace Tests {

class SocketNetworkLuadTests : public CoreNmaTest {
public:
    void SetUp() override {
        CoreNmaTest::SetUp();
        callingFile( __FILE__ );
    }

    JsonObject checkUiBroadcast( const std::string& expectedValue, int line ) {
        JsonObject broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", getCodeLineString( line ) );
        EXPECT_EQ( broadcast.empty(), false );  // << "line: " << line;
        JsonObject params;
        EXPECT_TRUE( json_get_obj( broadcast, "result", params ) ) << "line: " << line;
        auto value = json_get_str( params, "value" );
        if( !expectedValue.empty() )
            EXPECT_EQ( value, expectedValue ) << "line: " << line;

        return broadcast;
    }

    JsonObject checkProgressBroadcast( const std::string& expectedValue, int line ) {
        JsonObject broadcast = waitForBroadcast( "hub.extensions.plugin.run.progress", getCodeLineString( line ) );
        EXPECT_EQ( broadcast.empty(), false );  // << "line: " << line;
        JsonObject params;
        EXPECT_TRUE( json_get_obj( broadcast, "result", params ) ) << "line: " << line;
        auto value = json_get_str( params, "value" );
        if( !expectedValue.empty() )
            EXPECT_EQ( value, expectedValue ) << "line: " << line;

        return broadcast;
    }
};

// resources
// gets network sockets commands
static char socketScript[] = "HUB:network_socket/scripts/socket_timeout.lua";

TEST_F( SocketNetworkLuadTests, setTimeout ) {
    auto timeoutMs = std::chrono::microseconds( 20 );
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", socketScript } } );

    request[ "params" ][ "scriptParams" ] = JSON_EMPTY_OBJ;

    request[ "params" ][ "scriptParams" ][ "command" ] = "set_timeout";
    request[ "params" ][ "scriptParams" ][ "timeout" ] = timeoutMs.count();

    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    auto broadcast = checkUiBroadcast( "", __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( SocketNetworkLuadTests, setOptionBroadcast ) {
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", socketScript } } );

    request[ "params" ][ "scriptParams" ] = JSON_EMPTY_OBJ;

    request[ "params" ][ "scriptParams" ][ "command" ] = "setoption";
    request[ "params" ][ "scriptParams" ][ "option" ] = "broadcast";

    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    auto broadcast = checkUiBroadcast( "", __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( SocketNetworkLuadTests, setOptionIpAddMembershipFailed ) {
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", socketScript } } );

    request[ "params" ][ "scriptParams" ] = JSON_EMPTY_OBJ;

    request[ "params" ][ "scriptParams" ][ "command" ] = "setoption";
    request[ "params" ][ "scriptParams" ][ "option" ] = "ip-add-membership";
    request[ "params" ][ "scriptParams" ][ "jsonData" ] = JSON( "interface"
                                                                << "127.0.0.1" );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    auto broadcast = checkProgressBroadcast( "", __LINE__ );
    broadcast = checkProgressBroadcast( "", __LINE__ );
    auto status = json_get_str( broadcast[ "result" ], "status" );
    ASSERT_EQ( status, "failed" );
    json_get( broadcast[ "result" ], "error", error );
    ASSERT_FALSE( error.empty() );
    auto expectedError = "fail to set option: ip-add-membership, no multicast IP provided";
    auto reason = json_get_str( error, "reason" );
    ASSERT_TRUE( reason.find( expectedError ) );
}

TEST_F( SocketNetworkLuadTests, setOptionIpAddMembershipSuccess ) {
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", socketScript } } );

    request[ "params" ][ "scriptParams" ] = JSON_EMPTY_OBJ;

    request[ "params" ][ "scriptParams" ][ "command" ] = "setoption";
    request[ "params" ][ "scriptParams" ][ "option" ] = "ip-add-membership";
    request[ "params" ][ "scriptParams" ][ "jsonData" ] = JSON( "multi_addr"
                                                                << "239.255.255.250"
                                                                << "interface"
                                                                << "127.0.0.1" );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    auto broadcast = checkUiBroadcast( "", __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( SocketNetworkLuadTests, setOptionIpMulticastTtlFailed ) {
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", socketScript } } );

    request[ "params" ][ "scriptParams" ] = JSON_EMPTY_OBJ;

    request[ "params" ][ "scriptParams" ][ "command" ] = "setoption";
    request[ "params" ][ "scriptParams" ][ "option" ] = "ip-multicast-ttl";
    request[ "params" ][ "scriptParams" ][ "jsonData" ] = JSON( "ttl_value" << 10 );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    auto broadcast = checkProgressBroadcast( "", __LINE__ );
    broadcast = checkProgressBroadcast( "", __LINE__ );
    auto status = json_get_str( broadcast[ "result" ], "status" );
    ASSERT_EQ( status, "failed" );
    json_get( broadcast[ "result" ], "error", error );
    ASSERT_FALSE( error.empty() );
    auto expectedError = "fail to set option: ip-multicast-ttl, no ttl value provided";
    auto reason = json_get_str( error, "reason" );
    ASSERT_TRUE( reason.find( expectedError ) );
}

TEST_F( SocketNetworkLuadTests, setOptionIpMulticastTtlSuccess ) {
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", socketScript } } );

    request[ "params" ][ "scriptParams" ] = JSON_EMPTY_OBJ;

    request[ "params" ][ "scriptParams" ][ "command" ] = "setoption";
    request[ "params" ][ "scriptParams" ][ "option" ] = "ip-multicast-ttl";
    request[ "params" ][ "scriptParams" ][ "jsonData" ] = 10;
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    auto broadcast = checkUiBroadcast( "", __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
