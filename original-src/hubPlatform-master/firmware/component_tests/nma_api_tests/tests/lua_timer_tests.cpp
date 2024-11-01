#include "common_test_resources/fixtures/core_test_fixture.h"
#include "common_test_resources/resources/cored_resources.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {
class LuaTimerTests : public CoreNmaTest {
public:
    Json executeScript( const Json& scriptParams, const std::string& script, int codeLine );
};

Json LuaTimerTests::executeScript( const Json& scriptParams, const std::string& script, int codeLine ) {
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", script } } );
    request[ "params" ][ "scriptParams" ] = scriptParams;

    Json result;

    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    EXPECT_TRUE( error.empty() ) << error.dump( 4 ) << getCodeLineString( codeLine );
    if( error.empty() ) {
        auto broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", getCodeLineString( codeLine ) );
        EXPECT_FALSE( broadcast.empty() ) << getCodeLineString( codeLine );

        Json broadcastResult;
        EXPECT_TRUE( json_get_obj( broadcast, "result", broadcastResult ) ) << getCodeLineString( codeLine );
        if( !broadcastResult.empty() )
            EXPECT_TRUE( json_get( broadcastResult, "value", result ) );

        checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
        checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
    }

    return result;
}

TEST_F( LuaTimerTests, timerTests_setTimeout ) {
    const char* scriptCommand = "test_set_timeout";
    const Json expected = JSON( "command" << scriptCommand );
    auto script = "HUB:network_dhcp/scripts/timer_tests";
    const Json scriptParams = { { "command", scriptCommand }, { "user_data", 1 } };

    const Json scriptResult = executeScript( scriptParams, script, __LINE__ );
    EXPECT_EQ( expected[ "command" ], scriptResult[ "command" ] );

    const Json destroyTimerScriptParams = { { "command", "cancel_timer" }, { "timer_id", scriptResult[ "timer_id" ] } };
    const Json expectedResult = JSON( "destroyed" << scriptResult[ "timer_id" ] );
    const Json destroyTimerResult = executeScript( destroyTimerScriptParams, script, __LINE__ );
    EXPECT_EQ( expectedResult, destroyTimerResult );
}

TEST_F( LuaTimerTests, timerTests_setTimeoutWithId ) {
    const char* scriptCommand = "test_set_timeout_with_id";
    const Json expected = JSON( "command" << scriptCommand );
    auto script = "HUB:network_dhcp/scripts/timer_tests";
    Json scriptParams = { { "command", scriptCommand }, { "user_data", 2 } };

    const Json scriptResult = executeScript( scriptParams, script, __LINE__ );
    EXPECT_EQ( expected[ "command" ], scriptResult[ "command" ] );

    const Json destroyTimerScriptParams = { { "command", "cancel_timer" }, { "timer_id", scriptResult[ "timer_id" ] } };
    const Json expectedResult = JSON( "destroyed" << scriptResult[ "timer_id" ] );
    const Json destroyTimerResult = executeScript( destroyTimerScriptParams, script, __LINE__ );
    EXPECT_EQ( expectedResult, destroyTimerResult );
}

TEST_F( LuaTimerTests, timerTests_setInterval ) {
    const char* scriptCommand = "test_set_interval";
    const Json expected = JSON( "command" << scriptCommand );
    auto script = "HUB:network_dhcp/scripts/timer_tests";
    Json scriptParams = { { "command", scriptCommand }, { "user_data", 3 } };

    const Json scriptResult = executeScript( scriptParams, script, __LINE__ );
    EXPECT_EQ( expected[ "command" ], scriptResult[ "command" ] );

    const Json destroyTimerScriptParams = { { "command", "cancel_timer" }, { "timer_id", scriptResult[ "timer_id" ] } };
    const Json expectedResult = JSON( "destroyed" << scriptResult[ "timer_id" ] );
    const Json destroyTimerResult = executeScript( destroyTimerScriptParams, script, __LINE__ );
    EXPECT_EQ( expectedResult, destroyTimerResult );
}

TEST_F( LuaTimerTests, timerTests_setIntervalWithId ) {
    const char* scriptCommand = "test_set_interval_with_id";
    const Json expected = JSON( "command" << scriptCommand );
    auto script = "HUB:network_dhcp/scripts/timer_tests";
    Json scriptParams = { { "command", scriptCommand }, { "user_data", 4 } };

    const Json scriptResult = executeScript( scriptParams, script, __LINE__ );
    EXPECT_EQ( expected[ "command" ], scriptResult[ "command" ] );

    const Json destroyTimerScriptParams = { { "command", "cancel_timer" }, { "timer_id", scriptResult[ "timer_id" ] } };
    const Json expectedResult = JSON( "destroyed" << scriptResult[ "timer_id" ] );
    const Json destroyTimerResult = executeScript( destroyTimerScriptParams, script, __LINE__ );
    EXPECT_EQ( expectedResult, destroyTimerResult );
}

}  // namespace Tests
}  // namespace HubPlatform
