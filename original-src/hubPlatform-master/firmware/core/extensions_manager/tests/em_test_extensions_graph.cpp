#include "extensions_manager/extensions/extensions_graph.h"
#include "extensions_manager/extensions/plugins/plugins_factory.h"
#include "extensions_manager/tests/fakes/em_fake_environment.h"
#include "extensions_manager/extensions/plugins/gateway/gateway_plugin.h"
#include "extensions_manager/resources/em_resources.h"
#include "database_client/dbqueryparser.h"

#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

/*

TESTPLAN

DONE    1 Simple registration
DONE        1.1 Register addon
DONE        1.2 Unregister addon
DONE        1.3 Register plugin
DONE        1.4 Unregister plugin
DONE        1.5 Create plugin and save plugin permissions to the db
DONE    2 Simple addon startup and shutdown
DONE        2.1 Registered addon started
DONE        2.2 Registered addon restarted
DONE        2.3 Registered addon stopped
DONE    3 Simple Dependencies
DONE        3.1 Plugin with satisfied addon dependency
DONE        3.2 Plugin with satisfied plugin dependency
DONE        3.3 Plugin with multiple satisfied dependencies
DONE        3.4 Plugin with unsatisfied addon dependency
DONE        3.5 Plugin with unsatisfied plugin dependency
DONE        3.6 Plugin with multiple unsatisfied dependencies
DONE    4 Plugins recovery
DONE        4.1 Required addon started after dependent plugin registered
DONE        4.2 Required addon restarted after dependent plugin registered
DONE        4.3 Required addon not actually restarted after dependent plugin registered
DONE    5 Unregister addons and plugins
DONE        5.1 Unregister addon with dependent plugins
DONE        5.2 Unregister plugin with dependent plugins
DONE        5.3 Register plugin after required addon unregistered
DONE        5.4 Register plugin after required plugin unregistered
DONE    6 Graph resolving
DONE        6.1 Required plugin registered after dependent plugin
DONE        6.2 Chain of plugins startups
DONE        6.3 Chain of plugins teardowns
DONE        6.4 Plugin dependency resolved on previously started plugin on addon start
DONE        6.5 Plugin dependency resolved on previously started plugin on addon start (case 2)
*/

namespace HubPlatform {
namespace ExtensionsManager {
namespace UT {

using namespace ::testing;

class ExtensionsManagerTestSuite : public Test {
public:
    ExtensionsManagerTestSuite()
    : m_pluginsFactory( m_environment )
    , m_extensionsGraph( m_environment ) {
        // Do not print errors and warnings on erroneous test cases
        // TODO: implement fake log and check error messages too.
        // Log::setLocalLogLevel( Log::FATAL );
    }

    void SetUp() override {
        getEnvironment().getDbClient().clean();
    }

    void TearDown() override {
        getEnvironment().getDbClient().clean();
    }

    Addon& registerAndStartAddon( const std::string& id, const std::string& version ) {
        auto& addon = getExtensionsGraph().registerAddon( id );
        auto summary = CommonUtils::AddonSummary::fromJson( JSON( "id" << id << "version" << version ) );

        getExtensionsGraph().onAddonStarted( summary, 7 /*some id*/ );
        return addon;
    }

    std::vector< std::string > grabScriptExecutionList() {
        auto result = m_environment.getMessenger().m_scriptsExecutionList;
        m_environment.getMessenger().m_scriptsExecutionList.clear();
        return result;
    }

    FakeEnvironment& getEnvironment() {
        return m_environment;
    }

    PluginsFactory& getPluginsFactory() {
        return m_pluginsFactory;
    }

    ExtensionsGraph& getExtensionsGraph() {
        return m_extensionsGraph;
    }

    std::unique_ptr< Plugin > createPlugin( Json&& joConfig ) {
        joConfig[ "type" ] = "functional_unit";
        joConfig[ "permissions" ] = JSON_ARRAY_( "zwave", "storage" );

        return m_pluginsFactory.createPlugin( joConfig );
    }

    Plugin& createAndRegisterPlugin( Json&& joConfig ) {
        auto pluginPtr = createPlugin( std::move( joConfig ) );
        auto& plugin = *pluginPtr;
        getExtensionsGraph().registerPlugin( std::move( pluginPtr ) );
        return plugin;
    }

private:
    FakeEnvironment m_environment;
    PluginsFactory m_pluginsFactory;
    ExtensionsGraph m_extensionsGraph;
};

TEST_F( ExtensionsManagerTestSuite, testRegisterAddon /*1.1*/ ) {
    auto& addon = getExtensionsGraph().registerAddon( "zwave" );

    EXPECT_FALSE( addon.hasSummary() );
    EXPECT_TRUE( getExtensionsGraph().isAddonRegistered( "zwave" ) );
    EXPECT_EQ( getExtensionsGraph().findRegisteredAddon( "zwave" ), &addon );
    EXPECT_EQ( addon.getStatus(), Addon::Status::Stopped );
}

TEST_F( ExtensionsManagerTestSuite, testUnregisterAddon /*1.2*/ ) {
    auto& addon = getExtensionsGraph().registerAddon( "zwave" );

    getExtensionsGraph().unregisterAddon( addon );

    EXPECT_FALSE( getExtensionsGraph().isAddonRegistered( "zwave" ) );
    EXPECT_EQ( getExtensionsGraph().findRegisteredAddon( "zwave" ), nullptr );
}

TEST_F( ExtensionsManagerTestSuite, testRegisterPlugin /*1.3*/ ) {
    auto pluginPtr = createPlugin( JSON( "id"
                                         << "zwave"
                                         << "version"
                                         << "1.0"
                                         << "startup"
                                         << "startup.lua"
                                         << "teardown"
                                         << "teardown.lua" ) );
    auto pluginSummary = pluginPtr->getBaseConfig().summary;
    auto pluginRawPtr = pluginPtr.get();
    getExtensionsGraph().registerPlugin( std::move( pluginPtr ) );

    EXPECT_TRUE( getExtensionsGraph().isPluginRegistered( pluginSummary.id ) );
    EXPECT_EQ( getExtensionsGraph().findRegisteredPlugin( pluginSummary.id ), pluginRawPtr );
    EXPECT_EQ( pluginRawPtr->getStatus(), Plugin::Status::Started );
    ASSERT_THAT( grabScriptExecutionList(), ElementsAre( "HUB:zwave/startup.lua" ) );
}

TEST_F( ExtensionsManagerTestSuite, testUnregisterPlugin /*1.4*/ ) {
    using namespace Resources;
    auto pluginPtr = createPlugin( JSON( "id"
                                         << "zwave"
                                         << "version"
                                         << "1.0"
                                         << "startup"
                                         << "startup.lua"
                                         << "teardown"
                                         << "teardown.lua" ) );
    auto pluginSummary = pluginPtr->getBaseConfig().summary;

    getExtensionsGraph().registerPlugin( std::move( pluginPtr ) );
    getExtensionsGraph().unregisterPlugin( *getExtensionsGraph().findRegisteredPlugin( pluginSummary.id ) );

    EXPECT_FALSE( getExtensionsGraph().isPluginRegistered( pluginSummary.id ) );
    EXPECT_EQ( getExtensionsGraph().findRegisteredPlugin( pluginSummary.id ), nullptr );
    ASSERT_THAT( grabScriptExecutionList(), ElementsAre( "HUB:zwave/startup.lua", "HUB:zwave/teardown.lua" ) );

    const auto result = getEnvironment().getDbClient().findAll(
        DBTableNames::Plugins, JSON( db::ID_FIELD_NAME << pluginSummary.id ), nullptr );
    EXPECT_EQ( result.size(), 1 );
}

TEST_F( ExtensionsManagerTestSuite, testCreatePluginAndSavePluginPermissionsToDb /*1.5*/ ) {
    using namespace Resources;

    const auto permissions = JSON_ARRAY_( "zwave", "storage" );
    const auto pluginConfig = JSON( "id"
                                    << "plugin"
                                    << "version"
                                    << "4.2"
                                    << "type"
                                    << "functional_unit" << PluginRecord::Permissions << permissions );

    auto plugin = getPluginsFactory().createPlugin( pluginConfig );
    ASSERT_TRUE( plugin );

    plugin->init();

    auto returnFields = JSON( PluginRecord::Permissions << 1 );
    const auto result = getEnvironment().getDbClient().findOne(
        DBTableNames::Plugins, JSON( db::ID_FIELD_NAME << "plugin" ), &returnFields );

    EXPECT_TRUE( result.is_object() && result.size() == 1 );

    JsonArray jsArray = JSON_EMPTY_ARR;
    json_get_arr( result, PluginRecord::Permissions, jsArray );
    EXPECT_EQ( jsArray, permissions );
}

TEST_F( ExtensionsManagerTestSuite, testUnregisterAndRemovePlugin /*1.6*/ ) {
    using namespace Resources;
    auto pluginPtr = createPlugin( JSON( "id"
                                         << "zwave"
                                         << "version"
                                         << "1.0"
                                         << "startup"
                                         << "startup.lua"
                                         << "teardown"
                                         << "teardown.lua" ) );
    auto pluginSummary = pluginPtr->getBaseConfig().summary;

    getExtensionsGraph().registerPlugin( std::move( pluginPtr ) );
    getExtensionsGraph().unregisterPlugin( *getExtensionsGraph().findRegisteredPlugin( pluginSummary.id ),
                                           /*uninstall*/ true );

    EXPECT_FALSE( getExtensionsGraph().isPluginRegistered( pluginSummary.id ) );
    EXPECT_EQ( getExtensionsGraph().findRegisteredPlugin( pluginSummary.id ), nullptr );
    ASSERT_THAT( grabScriptExecutionList(), ElementsAre( "HUB:zwave/startup.lua", "HUB:zwave/teardown.lua" ) );

    const auto result = getEnvironment().getDbClient().findAll(
        DBTableNames::Plugins, JSON( db::ID_FIELD_NAME << pluginSummary.id ), nullptr );
    EXPECT_EQ( result.size(), 0 );
}

TEST_F( ExtensionsManagerTestSuite, testRegisteredAddonStarted /*2.1*/ ) {
    auto& addon = getExtensionsGraph().registerAddon( "zwave" );
    auto summary = CommonUtils::AddonSummary::fromJson( JSON( "id"
                                                              << "zwave"
                                                              << "version"
                                                              << "4.2" ) );

    getExtensionsGraph().onAddonStarted( summary, 42 /*some id*/ );

    EXPECT_TRUE( addon.hasSummary() );
    EXPECT_EQ( addon.getSummary().toString(), summary.toString() );
    EXPECT_EQ( addon.getStatus(), Addon::Status::Started );
}

TEST_F( ExtensionsManagerTestSuite, testRegisteredAddonRestarted /*2.2*/ ) {
    auto& addon = getExtensionsGraph().registerAddon( "zwave" );
    auto summary = CommonUtils::AddonSummary::fromJson( JSON( "id"
                                                              << "zwave"
                                                              << "version"
                                                              << "4.2" ) );

    getExtensionsGraph().onAddonStarted( summary, 56 /*some id*/ );
    getExtensionsGraph().onAddonStarted( summary, 81 /*different id*/ );

    EXPECT_TRUE( addon.hasSummary() );
    EXPECT_EQ( addon.getSummary().toString(), summary.toString() );
    EXPECT_EQ( addon.getStatus(), Addon::Status::Started );
}

TEST_F( ExtensionsManagerTestSuite, testRegisteredAddonStopped /*2.3*/ ) {
    auto& addon = getExtensionsGraph().registerAddon( "zwave" );
    auto summary = CommonUtils::AddonSummary::fromJson( JSON( "id"
                                                              << "zwave"
                                                              << "version"
                                                              << "4.2" ) );

    getExtensionsGraph().onAddonStarted( summary, 56 /*some id*/ );
    getExtensionsGraph().onAddonStopped( summary );

    EXPECT_TRUE( addon.hasSummary() );
    EXPECT_EQ( addon.getSummary().toString(), summary.toString() );
    EXPECT_EQ( addon.getStatus(), Addon::Status::Stopped );
}

TEST_F( ExtensionsManagerTestSuite, testPluginWithSatisfiedAddonDependency /*3.1*/ ) {
    auto& addon = registerAndStartAddon( "lua", "4.2" );

    auto& plugin = createAndRegisterPlugin( JSON( "id"
                                                  << "zwave"
                                                  << "version"
                                                  << "1.0"
                                                  << "startup"
                                                  << "startup.lua"
                                                  << "dependencies"
                                                  << JSON( "addons" << JSON_ARRAY( JSON( "id"
                                                                                         << "lua"
                                                                                         << "version"
                                                                                         << "4.2" ) ) ) ) );

    EXPECT_EQ( plugin.getStatus(), Plugin::Status::Started );
    ASSERT_THAT( grabScriptExecutionList(), ElementsAre( "HUB:zwave/startup.lua" ) );
}

TEST_F( ExtensionsManagerTestSuite, testPluginWithSatisfiedPluginDependency /*3.2*/ ) {
    auto& plugin1 = createAndRegisterPlugin( JSON( "id"
                                                   << "zwave-common"
                                                   << "version"
                                                   << "4.2"
                                                   << "startup"
                                                   << "startup.lua" ) );

    auto& plugin2 = createAndRegisterPlugin( JSON( "id"
                                                   << "zwave"
                                                   << "version"
                                                   << "1.0"
                                                   << "startup"
                                                   << "startup.lua"
                                                   << "dependencies"
                                                   << JSON( "plugins" << JSON_ARRAY( JSON( "id"
                                                                                           << "zwave-common"
                                                                                           << "version"
                                                                                           << "4.2" ) ) ) ) );

    EXPECT_EQ( plugin1.getStatus(), Plugin::Status::Started );
    EXPECT_EQ( plugin2.getStatus(), Plugin::Status::Started );
    ASSERT_THAT( grabScriptExecutionList(), ElementsAre( "HUB:zwave-common/startup.lua", "HUB:zwave/startup.lua" ) );
}

TEST_F( ExtensionsManagerTestSuite, testPluginWithMultipleSatisfiedDependencies /*3.3*/ ) {
    auto& addon = registerAndStartAddon( "lua", "4.2" );

    auto& plugin1 = createAndRegisterPlugin( JSON( "id"
                                                   << "zwave-common"
                                                   << "version"
                                                   << "4.2"
                                                   << "startup"
                                                   << "startup.lua" ) );

    auto& plugin2 = createAndRegisterPlugin( JSON( "id"
                                                   << "zwave"
                                                   << "version"
                                                   << "1.0"
                                                   << "startup"
                                                   << "startup.lua"
                                                   << "dependencies"
                                                   << JSON( "addons" << JSON_ARRAY( JSON( "id"
                                                                                          << "lua"
                                                                                          << "version"
                                                                                          << "4.2" ) )
                                                                     << "plugins"
                                                                     << JSON_ARRAY( JSON( "id"
                                                                                          << "zwave-common"
                                                                                          << "version"
                                                                                          << "4.2" ) ) ) ) );
    EXPECT_EQ( plugin1.getStatus(), Plugin::Status::Started );
    EXPECT_EQ( plugin2.getStatus(), Plugin::Status::Started );
    ASSERT_THAT( grabScriptExecutionList(), ElementsAre( "HUB:zwave-common/startup.lua", "HUB:zwave/startup.lua" ) );
}

TEST_F( ExtensionsManagerTestSuite, testPluginWithUnsatisfiedAddonDependency /*3.4*/ ) {
    auto& plugin = createAndRegisterPlugin( JSON( "id"
                                                  << "zwave"
                                                  << "version"
                                                  << "1.0"
                                                  << "dependencies"
                                                  << JSON( "addons" << JSON_ARRAY( JSON( "id"
                                                                                         << "lua"
                                                                                         << "version"
                                                                                         << "4.2" ) ) ) ) );

    EXPECT_EQ( plugin.getStatus(), Plugin::Status::Stopped );
}

TEST_F( ExtensionsManagerTestSuite, testPluginWithUnsatisfiedPluginDependency /*3.5*/ ) {
    auto& plugin = createAndRegisterPlugin( JSON( "id"
                                                  << "zwave"
                                                  << "version"
                                                  << "1.0"
                                                  << "dependencies"
                                                  << JSON( "plugins" << JSON_ARRAY( JSON( "id"
                                                                                          << "zwave-common"
                                                                                          << "version"
                                                                                          << "4.2" ) ) ) ) );

    EXPECT_EQ( plugin.getStatus(), Plugin::Status::Stopped );
}

TEST_F( ExtensionsManagerTestSuite, testPluginWithMultipleUnsatisfiedDependencies /*3.6*/ ) {
    auto& plugin = createAndRegisterPlugin( JSON( "id"
                                                  << "zwave"
                                                  << "version"
                                                  << "1.0"
                                                  << "dependencies"
                                                  << JSON( "addons" << JSON_ARRAY( JSON( "id"
                                                                                         << "lua"
                                                                                         << "version"
                                                                                         << "4.2" ) )
                                                                    << "plugins"
                                                                    << JSON_ARRAY( JSON( "id"
                                                                                         << "zwave-common"
                                                                                         << "version"
                                                                                         << "4.2" ) ) ) ) );

    EXPECT_EQ( plugin.getStatus(), Plugin::Status::Stopped );
}

TEST_F( ExtensionsManagerTestSuite, testRequiredAddonStartedAfterDependentPluginRegistered /*4.1*/ ) {
    auto& plugin = createAndRegisterPlugin( JSON( "id"
                                                  << "zwave"
                                                  << "version"
                                                  << "1.0"
                                                  << "startup"
                                                  << "startup.lua"
                                                  << "dependencies"
                                                  << JSON( "addons" << JSON_ARRAY( JSON( "id"
                                                                                         << "lua"
                                                                                         << "version"
                                                                                         << "4.2" ) ) ) ) );

    EXPECT_EQ( plugin.getStatus(), Plugin::Status::Stopped );

    auto& addon = registerAndStartAddon( "lua", "4.2" );

    EXPECT_EQ( plugin.getStatus(), Plugin::Status::Started );
    ASSERT_THAT( grabScriptExecutionList(), ElementsAre( "HUB:zwave/startup.lua" ) );
}

TEST_F( ExtensionsManagerTestSuite, testRequiredAddonRestartedAfterDependentPluginRegistered /*4.2*/ ) {
    auto& addon = getExtensionsGraph().registerAddon( "lua" );
    auto summary = CommonUtils::AddonSummary::fromJson( JSON( "id"
                                                              << "lua"
                                                              << "version"
                                                              << "4.2" ) );
    getExtensionsGraph().onAddonStarted( summary, 1 /*some id*/ );

    auto& plugin = createAndRegisterPlugin( JSON( "id"
                                                  << "zwave"
                                                  << "version"
                                                  << "1.0"
                                                  << "startup"
                                                  << "startup.lua"
                                                  << "dependencies"
                                                  << JSON( "addons" << JSON_ARRAY( JSON( "id"
                                                                                         << "lua"
                                                                                         << "version"
                                                                                         << "4.2" ) ) ) ) );

    EXPECT_EQ( plugin.getStatus(), Plugin::Status::Started );

    getExtensionsGraph().onAddonStarted( addon.getSummary(), 2 /*different id*/ );

    EXPECT_EQ( plugin.getStatus(), Plugin::Status::Started );
    ASSERT_THAT( grabScriptExecutionList(), ElementsAre( "HUB:zwave/startup.lua", "HUB:zwave/startup.lua" ) );
}

TEST_F( ExtensionsManagerTestSuite, testRequiredAddonNotActuallyRestartedAfterDependentPluginRegistered /*4.3*/ ) {
    auto& addon = getExtensionsGraph().registerAddon( "lua" );
    auto summary = CommonUtils::AddonSummary::fromJson( JSON( "id"
                                                              << "lua"
                                                              << "version"
                                                              << "4.2" ) );
    getExtensionsGraph().onAddonStarted( summary, 1 /*some id*/ );

    auto& plugin = createAndRegisterPlugin( JSON( "id"
                                                  << "zwave"
                                                  << "version"
                                                  << "1.0"
                                                  << "startup"
                                                  << "startup.lua"
                                                  << "dependencies"
                                                  << JSON( "addons" << JSON_ARRAY( JSON( "id"
                                                                                         << "lua"
                                                                                         << "version"
                                                                                         << "4.2" ) ) ) ) );

    EXPECT_EQ( plugin.getStatus(), Plugin::Status::Started );

    getExtensionsGraph().onAddonStarted( addon.getSummary(), 1 /*same id*/ );

    EXPECT_EQ( plugin.getStatus(), Plugin::Status::Started );
    ASSERT_THAT( grabScriptExecutionList(), ElementsAre( "HUB:zwave/startup.lua" ) );
}

TEST_F( ExtensionsManagerTestSuite, testUnregisterAddonWithDependentPlugins /*5.1*/ ) {
    auto& addon = registerAndStartAddon( "lua", "4.2" );

    auto& plugin = createAndRegisterPlugin( JSON( "id"
                                                  << "zwave"
                                                  << "version"
                                                  << "1.0"
                                                  << "dependencies"
                                                  << JSON( "addons" << JSON_ARRAY( JSON( "id"
                                                                                         << "lua"
                                                                                         << "version"
                                                                                         << "4.2" ) ) ) ) );

    getExtensionsGraph().unregisterAddon( addon );

    EXPECT_EQ( plugin.getStatus(), Plugin::Status::Stopped );
}

TEST_F( ExtensionsManagerTestSuite, testUnregisterPluginWithDependentPlugins /*5.2*/ ) {
    auto& plugin1 = createAndRegisterPlugin( JSON( "id"
                                                   << "zwave-common"
                                                   << "version"
                                                   << "4.2" ) );

    auto& plugin2 = createAndRegisterPlugin( JSON( "id"
                                                   << "zwave"
                                                   << "version"
                                                   << "1.0"
                                                   << "dependencies"
                                                   << JSON( "plugins" << JSON_ARRAY( JSON( "id"
                                                                                           << "zwave-common"
                                                                                           << "version"
                                                                                           << "4.2" ) ) ) ) );

    getExtensionsGraph().unregisterPlugin( plugin1 );

    EXPECT_EQ( plugin1.getStatus(), Plugin::Status::Stopped );
    EXPECT_EQ( plugin2.getStatus(), Plugin::Status::Stopped );
}

TEST_F( ExtensionsManagerTestSuite, testRegisterPluginAfterRequiredAddonUnregistered /*5.3*/ ) {
    auto& addon1 = registerAndStartAddon( "lua", "4.1" );
    getExtensionsGraph().unregisterAddon( addon1 );

    auto& plugin = createAndRegisterPlugin( JSON( "id"
                                                  << "zwave"
                                                  << "version"
                                                  << "1.0"
                                                  << "dependencies"
                                                  << JSON( "addons" << JSON_ARRAY( JSON( "id"
                                                                                         << "lua"
                                                                                         << "version"
                                                                                         << "4.2" ) ) ) ) );

    EXPECT_EQ( plugin.getStatus(), Plugin::Status::Stopped );

    auto& addon2 = registerAndStartAddon( "lua", "4.2" );

    EXPECT_EQ( plugin.getStatus(), Plugin::Status::Started );
}

TEST_F( ExtensionsManagerTestSuite, testRegisterPluginAfterRequiredPluginUnregistered /*5.4*/ ) {
    auto& plugin11 = createAndRegisterPlugin( JSON( "id"
                                                    << "zwave-common"
                                                    << "version"
                                                    << "4.1" ) );
    getExtensionsGraph().unregisterPlugin( plugin11 );

    auto& plugin2 = createAndRegisterPlugin( JSON( "id"
                                                   << "zwave"
                                                   << "version"
                                                   << "1.0"
                                                   << "dependencies"
                                                   << JSON( "plugins" << JSON_ARRAY( JSON( "id"
                                                                                           << "zwave-common"
                                                                                           << "version"
                                                                                           << "4.2" ) ) ) ) );

    EXPECT_EQ( plugin2.getStatus(), Plugin::Status::Stopped );

    auto& plugin12 = createAndRegisterPlugin( JSON( "id"
                                                    << "zwave-common"
                                                    << "version"
                                                    << "4.2" ) );

    EXPECT_EQ( plugin2.getStatus(), Plugin::Status::Started );
}

TEST_F( ExtensionsManagerTestSuite, testRequiredPluginRegisteredAfterDependentPlugin /*6.1*/ ) {
    auto& plugin2 = createAndRegisterPlugin( JSON( "id"
                                                   << "zwave"
                                                   << "version"
                                                   << "1.0"
                                                   << "startup"
                                                   << "2.lua"
                                                   << "dependencies"
                                                   << JSON( "plugins" << JSON_ARRAY( JSON( "id"
                                                                                           << "zwave-common"
                                                                                           << "version"
                                                                                           << "4.2"

                                                                                           ) ) ) ) );

    auto& plugin1 = createAndRegisterPlugin( JSON( "id"
                                                   << "zwave-common"
                                                   << "version"
                                                   << "4.2"
                                                   << "startup"
                                                   << "1.lua" ) );

    EXPECT_EQ( plugin1.getStatus(), Plugin::Status::Started );
    EXPECT_EQ( plugin2.getStatus(), Plugin::Status::Started );

    ASSERT_THAT( grabScriptExecutionList(), ElementsAre( "HUB:zwave-common/1.lua", "HUB:zwave/2.lua" ) );
}

TEST_F( ExtensionsManagerTestSuite, testChainOfPluginsStartups /*6.2*/ ) {
    auto& plugin2 = createAndRegisterPlugin( JSON( "id"
                                                   << "plugin2"
                                                   << "version"
                                                   << "3.0"
                                                   << "startup"
                                                   << "startup.lua"
                                                   << "dependencies"
                                                   << JSON( "plugins" << JSON_ARRAY( JSON( "id"
                                                                                           << "plugin1"
                                                                                           << "version"
                                                                                           << "2.0" ) ) ) ) );

    auto& plugin3 = createAndRegisterPlugin( JSON( "id"
                                                   << "plugin3"
                                                   << "version"
                                                   << "4.0"
                                                   << "startup"
                                                   << "startup.lua"
                                                   << "dependencies"
                                                   << JSON( "plugins" << JSON_ARRAY( JSON( "id"
                                                                                           << "plugin2"
                                                                                           << "version"
                                                                                           << "3.0" ) ) ) ) );

    auto& plugin1 = createAndRegisterPlugin( JSON( "id"
                                                   << "plugin1"
                                                   << "version"
                                                   << "2.0"
                                                   << "startup"
                                                   << "startup.lua"
                                                   << "dependencies"
                                                   << JSON( "addons" << JSON_ARRAY( JSON( "id"
                                                                                          << "addon"
                                                                                          << "version"
                                                                                          << "1.0" ) ) ) ) );

    EXPECT_EQ( plugin1.getStatus(), Plugin::Status::Stopped );
    EXPECT_EQ( plugin2.getStatus(), Plugin::Status::Stopped );
    EXPECT_EQ( plugin3.getStatus(), Plugin::Status::Stopped );

    auto& addon = registerAndStartAddon( "addon", "1.0" );

    EXPECT_EQ( plugin1.getStatus(), Plugin::Status::Started );
    EXPECT_EQ( plugin2.getStatus(), Plugin::Status::Started );
    EXPECT_EQ( plugin3.getStatus(), Plugin::Status::Started );
    ASSERT_THAT( grabScriptExecutionList(),
                 ElementsAre( "HUB:plugin1/startup.lua", "HUB:plugin2/startup.lua", "HUB:plugin3/startup.lua" ) );
}

TEST_F( ExtensionsManagerTestSuite, testChainOfPluginsTeardowns /*6.3*/ ) {
    auto& plugin1 = createAndRegisterPlugin( JSON( "id"
                                                   << "plugin1"
                                                   << "version"
                                                   << "2.0"
                                                   << "startup"
                                                   << "startup.lua"
                                                   << "teardown"
                                                   << "teardown.lua" ) );

    auto& plugin2 = createAndRegisterPlugin( JSON( "id"
                                                   << "plugin2"
                                                   << "version"
                                                   << "3.0"
                                                   << "startup"
                                                   << "startup.lua"
                                                   << "teardown"
                                                   << "teardown.lua"
                                                   << "dependencies"
                                                   << JSON( "plugins" << JSON_ARRAY( JSON( "id"
                                                                                           << "plugin1"
                                                                                           << "version"
                                                                                           << "2.0" ) ) ) ) );

    auto& plugin3 = createAndRegisterPlugin( JSON( "id"
                                                   << "plugin3"
                                                   << "version"
                                                   << "4.0"
                                                   << "startup"
                                                   << "startup.lua"
                                                   << "teardown"
                                                   << "teardown.lua"
                                                   << "dependencies"
                                                   << JSON( "plugins" << JSON_ARRAY( JSON( "id"
                                                                                           << "plugin2"
                                                                                           << "version"
                                                                                           << "3.0" ) ) ) ) );

    EXPECT_EQ( plugin1.getStatus(), Plugin::Status::Started );
    EXPECT_EQ( plugin2.getStatus(), Plugin::Status::Started );
    EXPECT_EQ( plugin3.getStatus(), Plugin::Status::Started );

    ASSERT_THAT( grabScriptExecutionList(),
                 ElementsAre( "HUB:plugin1/startup.lua", "HUB:plugin2/startup.lua", "HUB:plugin3/startup.lua" ) );

    getExtensionsGraph().unregisterPlugin( plugin1 );

    EXPECT_EQ( plugin1.getStatus(), Plugin::Status::Stopped );
    EXPECT_EQ( plugin2.getStatus(), Plugin::Status::Stopped );
    EXPECT_EQ( plugin3.getStatus(), Plugin::Status::Stopped );

    ASSERT_THAT( grabScriptExecutionList(),
                 ElementsAre( "HUB:plugin3/teardown.lua", "HUB:plugin2/teardown.lua", "HUB:plugin1/teardown.lua" ) );
}

TEST_F( ExtensionsManagerTestSuite, testPluginDependencyResolvedOnPreviouslyStartedPluginOnAddonStart /*6.4*/ ) {
    auto& addon = getExtensionsGraph().registerAddon( "lua" );
    auto summary = CommonUtils::AddonSummary::fromJson( JSON( "id"
                                                              << "lua"
                                                              << "version"
                                                              << "4.2" ) );

    auto& ecobeePlugin = createAndRegisterPlugin( JSON( "id"
                                                        << "ecobee"
                                                        << "version"
                                                        << "1.0"
                                                        << "startup"
                                                        << "startup.lua"
                                                        << "dependencies"
                                                        << JSON( "addons" << JSON_ARRAY( JSON( "id"
                                                                                               << "lua"
                                                                                               << "version"
                                                                                               << "4.2" ) )
                                                                          << "plugins"
                                                                          << JSON_ARRAY( JSON( "id"
                                                                                               << "json"
                                                                                               << "version"
                                                                                               << "0.1" ) ) ) ) );

    auto& jsonPlugin = createAndRegisterPlugin( JSON( "id"
                                                      << "json"
                                                      << "version"
                                                      << "0.1"
                                                      << "startup"
                                                      << "startup.lua"
                                                      << "dependencies"
                                                      << JSON( "addons" << JSON_ARRAY( JSON( "id"
                                                                                             << "lua"
                                                                                             << "version"
                                                                                             << "4.2" ) ) ) ) );

    getExtensionsGraph().onAddonStarted( summary, 1 /*some id*/ );
    EXPECT_EQ( jsonPlugin.getStatus(), Plugin::Status::Started );
    EXPECT_EQ( ecobeePlugin.getStatus(), Plugin::Status::Started );
    ASSERT_THAT( grabScriptExecutionList(), ElementsAre( "HUB:json/startup.lua", "HUB:ecobee/startup.lua" ) );
}

TEST_F( ExtensionsManagerTestSuite, testPluginDependencyResolvedOnPreviouslyStartedPluginOnAddonStartCase2 /*6.5*/ ) {
    auto& addon = getExtensionsGraph().registerAddon( "lua" );
    auto summary = CommonUtils::AddonSummary::fromJson( JSON( "id"
                                                              << "lua"
                                                              << "version"
                                                              << "4.2" ) );

    auto& jsonPlugin = createAndRegisterPlugin( JSON( "id"
                                                      << "json"
                                                      << "version"
                                                      << "0.1"
                                                      << "startup"
                                                      << "startup.lua"
                                                      << "dependencies"
                                                      << JSON( "addons" << JSON_ARRAY( JSON( "id"
                                                                                             << "lua"
                                                                                             << "version"
                                                                                             << "4.2" ) ) ) ) );

    auto& honeywellPlugin = createAndRegisterPlugin( JSON( "id"
                                                           << "honeywell"
                                                           << "version"
                                                           << "1.0"
                                                           << "startup"
                                                           << "startup.lua"
                                                           << "dependencies"
                                                           << JSON( "addons" << JSON_ARRAY( JSON( "id"
                                                                                                  << "lua"
                                                                                                  << "version"
                                                                                                  << "4.2" ) )
                                                                             << "plugins"
                                                                             << JSON_ARRAY( JSON( "id"
                                                                                                  << "json"
                                                                                                  << "version"
                                                                                                  << "0.1" ) ) ) ) );

    getExtensionsGraph().onAddonStarted( summary, 1 /*some id*/ );
    EXPECT_EQ( jsonPlugin.getStatus(), Plugin::Status::Started );
    EXPECT_EQ( honeywellPlugin.getStatus(), Plugin::Status::Started );
    ASSERT_THAT( grabScriptExecutionList(), ElementsAre( "HUB:json/startup.lua", "HUB:honeywell/startup.lua" ) );
}

}  // namespace UT
}  // namespace ExtensionsManager
}  // namespace HubPlatform
