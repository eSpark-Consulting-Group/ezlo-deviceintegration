#include "luad/modules/http/http_dispatch.h"
#include "luad/tests/fakes/fake_plugin_status_events_subscription.h"
#include "luad/tests/fakes/fake_event_emitter.h"
#include "luad/tests/fakes/fake_lua_plugin_resources_provider.h"

#include "common_utils/cpp14.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

/*

TESTPLAN

DONE    1. Plugin status events subscription
DONE        1.1 Check startup subscription
DONE        1.2 Check closing plugin connections in a response of a plugin status change
DONE            1.2.1 Check closing plugin connections in a response of a plugin restart
DONE            1.2.2 Check closing plugin connections in a response of a plugin shutdown
DONE            1.2.3 Check closing plugin connections in a response of a plugin uninstall
*/

namespace HubPlatform {
namespace Luad {
namespace UT {

class HttpDispatchTestSuite : public ::testing::Test {
public:
    void SetUp() override {
        m_fakeLuaPluginResourcesProvider.ignorePluginPermissionCheck( true );
        m_http = std::make_unique< HttpDispatch >(
            m_fakeLuaPluginResourcesProvider, m_fakeEventEmitter, m_fakePluginStatusEventsSubscription );
    }

    void TearDown() override {}

    FakePluginStatusEventsSubscription& getFakePluginStatusEventsSubscription() {
        return m_fakePluginStatusEventsSubscription;
    }

    FakeLuaPluginResourcesProvider& getFakeLuaPluginResourcesProvider() {
        return m_fakeLuaPluginResourcesProvider;
    }

    HttpDispatch& getHttpDispatcher() {
        return *m_http;
    }

private:
    FakeLuaPluginResourcesProvider m_fakeLuaPluginResourcesProvider;
    FakeEventEmitter m_fakeEventEmitter;
    FakePluginStatusEventsSubscription m_fakePluginStatusEventsSubscription;

    std::unique_ptr< HttpDispatch > m_http;
};

TEST_F( HttpDispatchTestSuite, testCheckStartupSubscription /* 1.1 */ ) {
    const auto pluginStatusEventsSubscription = getFakePluginStatusEventsSubscription();

    EXPECT_EQ( pluginStatusEventsSubscription.m_pluginRestartedCallbacks.size(), 1 );
    EXPECT_EQ( pluginStatusEventsSubscription.m_pluginTeardownCallbacks.size(), 1 );
    EXPECT_EQ( pluginStatusEventsSubscription.m_pluginUninstallCallbacks.size(), 1 );
}

TEST_F( HttpDispatchTestSuite, testClosePluginConnectionsAfterPluginRestarted /* 1.2.1 */ ) {
    const auto request = R"(
    {
		"url": "http://localhost:8000/reflect/get?suite=HttpTests&method=httpCloseConnectionTypeKeepAlive",
		"handler": "HUB:http_test/scripts/http_events"
    }
    )"_json;

    getFakeLuaPluginResourcesProvider().setupPluginId( "plugin1" );
    auto requestId1 = getHttpDispatcher().request( "plugin1", request );
    auto requestId2 = getHttpDispatcher().request( "plugin1", request );

    getFakeLuaPluginResourcesProvider().setupPluginId( "plugin2" );
    auto requestId3 = getHttpDispatcher().request( "plugin2", request );

    const auto pluginStatusEventsSubscription = getFakePluginStatusEventsSubscription();

    pluginStatusEventsSubscription.m_pluginRestartedCallbacks[ 0 ]( "plugin1" );
    EXPECT_THROW( getHttpDispatcher().getConnectionInfo( "plugin1", requestId1 ), HttpConnectionDoesntExistError );
    EXPECT_THROW( getHttpDispatcher().getConnectionInfo( "plugin1", requestId2 ), HttpConnectionDoesntExistError );
    EXPECT_NO_THROW( getHttpDispatcher().getConnectionInfo( "plugin2", requestId3 ) );
}

TEST_F( HttpDispatchTestSuite, testClosePluginConnectionsAfterPluginShutdown /* 1.2.2 */ ) {
    const auto request = R"(
    {
		"url": "http://localhost:8000/reflect/get?suite=HttpTests&method=httpCloseConnectionTypeKeepAlive",
		"handler": "HUB:http_test/scripts/http_events"
    }
    )"_json;

    getFakeLuaPluginResourcesProvider().setupPluginId( "plugin1" );
    auto requestId1 = getHttpDispatcher().request( "plugin1", request );
    auto requestId2 = getHttpDispatcher().request( "plugin1", request );

    getFakeLuaPluginResourcesProvider().setupPluginId( "plugin2" );
    auto requestId3 = getHttpDispatcher().request( "plugin2", request );

    const auto pluginStatusEventsSubscription = getFakePluginStatusEventsSubscription();

    pluginStatusEventsSubscription.m_pluginTeardownCallbacks[ 0 ]( "plugin1" );
    EXPECT_THROW( getHttpDispatcher().getConnectionInfo( "plugin1", requestId1 ), HttpConnectionDoesntExistError );
    EXPECT_THROW( getHttpDispatcher().getConnectionInfo( "plugin1", requestId2 ), HttpConnectionDoesntExistError );
    EXPECT_NO_THROW( getHttpDispatcher().getConnectionInfo( "plugin2", requestId3 ) );
}

TEST_F( HttpDispatchTestSuite, testClosePluginConnectionsAfterPluginUninstalled /* 1.2.3 */ ) {
    const auto request = R"(
    {
		"url": "http://localhost:8000/reflect/get?suite=HttpTests&method=httpCloseConnectionTypeKeepAlive",
		"handler": "HUB:http_test/scripts/http_events"
    }
    )"_json;

    getFakeLuaPluginResourcesProvider().setupPluginId( "plugin1" );
    auto requestId1 = getHttpDispatcher().request( "plugin1", request );
    auto requestId2 = getHttpDispatcher().request( "plugin1", request );

    getFakeLuaPluginResourcesProvider().setupPluginId( "plugin2" );
    auto requestId3 = getHttpDispatcher().request( "plugin2", request );

    const auto pluginStatusEventsSubscription = getFakePluginStatusEventsSubscription();

    pluginStatusEventsSubscription.m_pluginUninstallCallbacks[ 0 ]( "plugin1" );
    EXPECT_THROW( getHttpDispatcher().getConnectionInfo( "plugin1", requestId1 ), HttpConnectionDoesntExistError );
    EXPECT_THROW( getHttpDispatcher().getConnectionInfo( "plugin1", requestId2 ), HttpConnectionDoesntExistError );
    EXPECT_NO_THROW( getHttpDispatcher().getConnectionInfo( "plugin2", requestId3 ) );
}

}  // namespace UT
}  // namespace Luad
}  // namespace HubPlatform
