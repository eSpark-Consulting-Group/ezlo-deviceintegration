#include "extensions_manager/extensions/plugins/base_config.h"

#include "json_utils/json_utils.h"

#include "bus/messagesclasses.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

/*

TESTPLAN

DONE    1. Conversion from JSON
DONE        1.0 Incorrect required fields
DONE            1.0.1 Incorrect name
DONE            1.0.2 Incorrect version
DONE            1.0.3 Incorrect type
DONE            1.0.4 Incorrect plugin permissions
DONE        1.1 Correct required fields, no optional fields
DONE        1.2 No required fields
DONE            1.2.1 No name
DONE            1.2.2 No version
DONE            1.2.3 No type
DONE            1.2.4 No plugin permissions
DONE        1.3 Optional fields
DONE            1.3.1 Startup script
DONE            1.3.2 Teardown script
DONE            1.3.2 Startup and teardown script
DONE        1.4 Malformed version
DONE            1.4.1 Extra component is ignored
DONE            1.4.2 Missing component is treated as 0
DONE            1.4.3 Bad component is treated as 0
DONE            1.4.4 Not a version
DONE        1.5 Addon Dependencies
DONE            1.5.1 Single correct dependency
DONE            1.5.2 Malformed dependency
DONE            1.5.3 Few correct dependencies
DONE            1.5.4 Few correct and malformed dependencies
DONE        1.6 Plugin Dependencies
DONE            1.6.1 Single correct dependency
DONE            1.6.2 Malformed dependency
DONE            1.6.3 Few correct dependencies
DONE            1.6.4 Few correct and malformed dependencies
DONE        1.7 Addon and plugin Dependencies
DONE        1.8 Plugin Permissions
DONE            1.8.1 Empty plugin permissions
DONE            1.8.2 Mailformed plugin permissions
DONE        1.9 Extensions
DONE            1.9.1 Single config
DONE            1.9.2 Several configs
DONE            1.9.3 Single plugin
DONE            1.9.4 Several plugins
DONE            1.9.5 Several plugin extensions

*/

std::ostream& operator<<( std::ostream& stream,
                          typename HubPlatform::ExtensionsManager::RequiredVersion const& version ) {
    return stream << version.toString();
}

using namespace ::testing;
using namespace HubPlatform::ExtensionsManager;

class PluginInfoFromJSONTestSuite : public Test {
public:
    PluginInfoFromJSONTestSuite() {
        // Do not print errors and warnings on erroneous test cases
        // TODO: implement fake log and check error messages too.
        Log::setLocalLogLevel( Log::FATAL );
    }
};

TEST_F( PluginInfoFromJSONTestSuite, testIncorrectRequiredFieldsIncorrectName /*1.0.1*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "id"
                                                       << ""
                                                       << "version"
                                                       << "4.2"
                                                       << "type"
                                                       << "functional_unit" ) );

    EXPECT_FALSE( pluginInfo );
}

TEST_F( PluginInfoFromJSONTestSuite, testIncorrectRequiredFieldsIncorrectVersion /*1.0.2*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "id"
                                                       << "zwave"
                                                       << "version"
                                                       << ""
                                                       << "type"
                                                       << "functional_unit" ) );

    EXPECT_FALSE( pluginInfo );
}

TEST_F( PluginInfoFromJSONTestSuite, testIncorrectRequiredFieldsIncorrectType /*1.0.3*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "id"
                                                       << "zwave"
                                                       << "version"
                                                       << "4.2"
                                                       << "type"
                                                       << "" ) );

    EXPECT_FALSE( pluginInfo );
}

TEST_F( PluginInfoFromJSONTestSuite, testIncorrectRequiredFieldsIncorrectPluginPermissions /*1.0.4*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "id"
                                                       << "zwave"
                                                       << "version"
                                                       << "4.2"
                                                       << "type"
                                                       << "gateway"
                                                       << "permission"
                                                       << "2810" ) );

    EXPECT_FALSE( pluginInfo );
}

TEST_F( PluginInfoFromJSONTestSuite, testCorrectRequiredFieldsNoOptionslFields /*1.1*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "id"
                                                       << "zwave"
                                                       << "version"
                                                       << "4.2"
                                                       << "type"
                                                       << "functional_unit"
                                                       << "permissions" << JSON_ARRAY_( "zwave", "storage" ) ) );

    EXPECT_TRUE( pluginInfo );

    EXPECT_TRUE( pluginInfo->summary.isValid() );
    EXPECT_EQ( pluginInfo->summary.id, "zwave" );
    EXPECT_TRUE( pluginInfo->summary.version.isValid() );
    EXPECT_EQ( pluginInfo->summary.version, HubPlatform::CommonUtils::PluginVersion( 4, 2 ) );
    EXPECT_TRUE( pluginInfo->dependencies.requiredAddons.empty() );
    EXPECT_TRUE( pluginInfo->dependencies.requiredPlugins.empty() );
}

TEST_F( PluginInfoFromJSONTestSuite, testNoRequiredFieldsNoName /*1.2.1*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "alias"
                                                       << "zwave"
                                                       << "version"
                                                       << "4.2"
                                                       << "type"
                                                       << "functional_unit" ) );

    EXPECT_FALSE( pluginInfo );
}

TEST_F( PluginInfoFromJSONTestSuite, testNoRequiredFieldsNoVersion /*1.2.2*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "name"
                                                       << "zwave" ) );

    EXPECT_FALSE( pluginInfo );
}

TEST_F( PluginInfoFromJSONTestSuite, testNoRequiredFieldsNoType /*1.2.3*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "name"
                                                       << "zwave"
                                                       << "version"
                                                       << "4.2" ) );

    EXPECT_FALSE( pluginInfo );
}

TEST_F( PluginInfoFromJSONTestSuite, testNoRequiredFieldsNoPluginPermissions /*1.2.4*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "id"
                                                       << "zwave"
                                                       << "version"
                                                       << "4.2"
                                                       << "type"
                                                       << "gateway" ) );

    EXPECT_FALSE( pluginInfo );
}

TEST_F( PluginInfoFromJSONTestSuite, testOptionalFieldsStartupScript /*1.3.1*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "id"
                                                       << "zwave"
                                                       << "version"
                                                       << "4.2"
                                                       << "type"
                                                       << "functional_unit"
                                                       << "startup"
                                                       << "scripts/startup.lua"
                                                       << "permissions" << JSON_ARRAY_( "zwave", "storage" ) ) );

    EXPECT_TRUE( pluginInfo );

    EXPECT_EQ( pluginInfo->startupScript, "scripts/startup.lua" );
}

TEST_F( PluginInfoFromJSONTestSuite, testOptionalFieldsTeardownScript /*1.3.2*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "id"
                                                       << "zwave"
                                                       << "version"
                                                       << "4.2"
                                                       << "type"
                                                       << "functional_unit"
                                                       << "teardown"
                                                       << "scripts/teardown.lua"
                                                       << "permissions" << JSON_ARRAY_( "zwave", "storage" ) ) );

    EXPECT_TRUE( pluginInfo );

    EXPECT_EQ( pluginInfo->teardownScript, "scripts/teardown.lua" );
}

TEST_F( PluginInfoFromJSONTestSuite, testOptionalFieldsStartupAndTeardownScript /*1.3.3*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "id"
                                                       << "zwave"
                                                       << "version"
                                                       << "4.2"
                                                       << "type"
                                                       << "functional_unit"
                                                       << "startup"
                                                       << "scripts/startup.lua"
                                                       << "teardown"
                                                       << "scripts/teardown.lua"
                                                       << "permissions" << JSON_ARRAY_( "zwave", "storage" ) ) );

    EXPECT_TRUE( pluginInfo );

    EXPECT_EQ( pluginInfo->startupScript, "scripts/startup.lua" );
    EXPECT_EQ( pluginInfo->teardownScript, "scripts/teardown.lua" );
}

TEST_F( PluginInfoFromJSONTestSuite, testMalformedVersionExtraComponentIsIgnored /*1.4.1*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "id"
                                                       << "zwave"
                                                       << "version"
                                                       << "4.2.1.6.8"
                                                       << "type"
                                                       << "functional_unit"
                                                       << "permissions" << JSON_ARRAY_( "zwave", "storage" ) ) );

    EXPECT_TRUE( pluginInfo );

    EXPECT_TRUE( pluginInfo->summary.version.isValid() );
    EXPECT_EQ( pluginInfo->summary.version, HubPlatform::CommonUtils::PluginVersion( 4, 2, "1.6.8" ) );
}

TEST_F( PluginInfoFromJSONTestSuite, testMalformedVersionMissingComponentIsTreatedAs0 /*1.4.2*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "id"
                                                       << "zwave"
                                                       << "version"
                                                       << "4"
                                                       << "type"
                                                       << "functional_unit"
                                                       << "permissions" << JSON_ARRAY_( "zwave", "storage" ) ) );

    EXPECT_TRUE( pluginInfo );

    EXPECT_TRUE( pluginInfo->summary.version.isValid() );
    EXPECT_EQ( pluginInfo->summary.version, HubPlatform::CommonUtils::PluginVersion( 4, 0 ) );
}

TEST_F( PluginInfoFromJSONTestSuite, testMalformedVersionBadComponentIsTreatedAs0 /*1.4.3*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "id"
                                                       << "zwave"
                                                       << "version"
                                                       << "4.x"
                                                       << "type"
                                                       << "functional_unit"
                                                       << "permissions" << JSON_ARRAY_( "zwave", "storage" ) ) );

    EXPECT_TRUE( pluginInfo );

    EXPECT_TRUE( pluginInfo->summary.version.isValid() );
    EXPECT_EQ( pluginInfo->summary.version, HubPlatform::CommonUtils::PluginVersion( 4, 0 ) );
}

TEST_F( PluginInfoFromJSONTestSuite, testMalformedVersionNotAVersion /*1.4.4*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "id"
                                                       << "zwave"
                                                       << "version"
                                                       << "any"
                                                       << "type"
                                                       << "functional_unit" ) );

    EXPECT_FALSE( pluginInfo );
}

TEST_F( PluginInfoFromJSONTestSuite, testAddonDependenciesSingleCorrectDependency /*1.5.1*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "id"
                                                       << "zwave"
                                                       << "version"
                                                       << "4.2"
                                                       << "type"
                                                       << "functional_unit"
                                                       << "dependencies"
                                                       << JSON( "addons" << JSON_ARRAY( JSON( "id"
                                                                                              << "lua"
                                                                                              << "version"
                                                                                              << "1.2" ) ) )
                                                       << "permissions" << JSON_ARRAY_( "zwave", "storage" ) ) );

    EXPECT_TRUE( pluginInfo );

    EXPECT_EQ( pluginInfo->dependencies.requiredAddons.size(), 1 );
    auto& dependencies = pluginInfo->dependencies.requiredAddons;

    EXPECT_TRUE( dependencies[ 0 ].isValid() );
    EXPECT_EQ( dependencies[ 0 ].id, "lua" );
    EXPECT_EQ( dependencies[ 0 ].version, RequiredVersion( 1, 2 ) );
}

TEST_F( PluginInfoFromJSONTestSuite, testAddonDependenciesMalformedDependency /*1.5.2*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "id"
                                                       << "zwave"
                                                       << "version"
                                                       << "4.2"
                                                       << "type"
                                                       << "functional_unit"
                                                       << "dependencies"
                                                       << JSON( "addons" << JSON_ARRAY( JSON( "addon_name"
                                                                                              << "lua"
                                                                                              << "version"
                                                                                              << "any" ) ) )
                                                       << "permissions" << JSON_ARRAY_( "zwave", "storage" ) ) );

    EXPECT_TRUE( pluginInfo );

    EXPECT_TRUE( pluginInfo->dependencies.requiredAddons.empty() );
}

TEST_F( PluginInfoFromJSONTestSuite, testAddonDependenciesFewCorrectDependencies /*1.5.3*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "id"
                                                       << "zwave"
                                                       << "version"
                                                       << "4.2"
                                                       << "type"
                                                       << "functional_unit"
                                                       << "dependencies"
                                                       << JSON( "addons" << JSON_ARRAY( JSON( "id"
                                                                                              << "lua"
                                                                                              << "version"
                                                                                              << "1.2" )
                                                                                        << JSON( "id"
                                                                                                 << "zwave"
                                                                                                 << "version"
                                                                                                 << "3.4" ) ) )
                                                       << "permissions" << JSON_ARRAY_( "zwave", "storage" ) ) );

    EXPECT_TRUE( pluginInfo );

    EXPECT_EQ( pluginInfo->dependencies.requiredAddons.size(), 2 );
    auto& dependencies = pluginInfo->dependencies.requiredAddons;

    EXPECT_TRUE( dependencies[ 0 ].isValid() );
    EXPECT_EQ( dependencies[ 0 ].id, "lua" );
    EXPECT_EQ( dependencies[ 0 ].version, RequiredVersion( 1, 2 ) );

    EXPECT_TRUE( dependencies[ 1 ].isValid() );
    EXPECT_EQ( dependencies[ 1 ].id, "zwave" );
    EXPECT_EQ( dependencies[ 1 ].version, RequiredVersion( 3, 4 ) );
}

TEST_F( PluginInfoFromJSONTestSuite, testAddonDependenciesFewCorrectAndMalformedDependencies /*1.5.4*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "id"
                                                       << "zwave"
                                                       << "version"
                                                       << "4.2"
                                                       << "type"
                                                       << "functional_unit"
                                                       << "dependencies"
                                                       << JSON( "addons" << JSON_ARRAY( JSON( "id"
                                                                                              << "cameras"
                                                                                              << "version"
                                                                                              << "1.2" )
                                                                                        << JSON( "alias"
                                                                                                 << "lua"
                                                                                                 << "version"
                                                                                                 << "3.4" )
                                                                                        << JSON( "id"
                                                                                                 << "luup"
                                                                                                 << "version"
                                                                                                 << "5.6" )
                                                                                        << JSON( "id"
                                                                                                 << "zwave"
                                                                                                 << "version"
                                                                                                 << "any" ) ) )
                                                       << "permissions" << JSON_ARRAY_( "zwave", "storage" ) ) );

    EXPECT_TRUE( pluginInfo );

    EXPECT_EQ( pluginInfo->dependencies.requiredAddons.size(), 2 );
    auto& dependencies = pluginInfo->dependencies.requiredAddons;

    EXPECT_TRUE( dependencies[ 0 ].isValid() );
    EXPECT_EQ( dependencies[ 0 ].id, "cameras" );
    EXPECT_EQ( dependencies[ 0 ].version, RequiredVersion( 1, 2 ) );

    EXPECT_TRUE( dependencies[ 1 ].isValid() );
    EXPECT_EQ( dependencies[ 1 ].id, "luup" );
    EXPECT_EQ( dependencies[ 1 ].version, RequiredVersion( 5, 6 ) );
}

TEST_F( PluginInfoFromJSONTestSuite, testPluginDependenciesSingleCorrectDependency /*1.6.1*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "id"
                                                       << "zwave"
                                                       << "version"
                                                       << "4.2"
                                                       << "type"
                                                       << "functional_unit"
                                                       << "dependencies"
                                                       << JSON( "plugins" << JSON_ARRAY( JSON( "id"
                                                                                               << "zwave_common"
                                                                                               << "version"
                                                                                               << "1.2" ) ) )
                                                       << "permissions" << JSON_ARRAY_( "zwave", "storage" ) ) );

    EXPECT_TRUE( pluginInfo );

    EXPECT_EQ( pluginInfo->dependencies.requiredPlugins.size(), 1 );
    auto& dependencies = pluginInfo->dependencies.requiredPlugins;

    EXPECT_TRUE( dependencies[ 0 ].isValid() );
    EXPECT_EQ( dependencies[ 0 ].id, "zwave_common" );
    EXPECT_EQ( dependencies[ 0 ].version, RequiredVersion( 1, 2 ) );
}

TEST_F( PluginInfoFromJSONTestSuite, testPluginDependenciesMalformedDependency /*1.6.2*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "id"
                                                       << "zwave"
                                                       << "version"
                                                       << "4.2"
                                                       << "type"
                                                       << "functional_unit"
                                                       << "dependencies"
                                                       << JSON( "plugins" << JSON_ARRAY( JSON( "addon_name"
                                                                                               << "lua"
                                                                                               << "version"
                                                                                               << "any" ) ) )
                                                       << "permissions" << JSON_ARRAY_( "zwave", "storage" ) ) );

    EXPECT_TRUE( pluginInfo );

    EXPECT_TRUE( pluginInfo->dependencies.requiredPlugins.empty() );
}

TEST_F( PluginInfoFromJSONTestSuite, testPluginDependenciesFewCorrectDependencies /*1.6.3*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "id"
                                                       << "zwave"
                                                       << "version"
                                                       << "4.2"
                                                       << "type"
                                                       << "functional_unit"
                                                       << "dependencies"
                                                       << JSON( "plugins" << JSON_ARRAY( JSON( "id"
                                                                                               << "zwave_common"
                                                                                               << "version"
                                                                                               << "1.2" )
                                                                                         << JSON( "id"
                                                                                                  << "plugins_common"
                                                                                                  << "version"
                                                                                                  << "3.4" ) ) )
                                                       << "permissions" << JSON_ARRAY_( "zwave", "storage" ) ) );

    EXPECT_TRUE( pluginInfo );

    EXPECT_EQ( pluginInfo->dependencies.requiredPlugins.size(), 2 );
    auto& dependencies = pluginInfo->dependencies.requiredPlugins;

    EXPECT_TRUE( dependencies[ 0 ].isValid() );
    EXPECT_EQ( dependencies[ 0 ].id, "zwave_common" );
    EXPECT_EQ( dependencies[ 0 ].version, RequiredVersion( 1, 2 ) );

    EXPECT_TRUE( dependencies[ 1 ].isValid() );
    EXPECT_EQ( dependencies[ 1 ].id, "plugins_common" );
    EXPECT_EQ( dependencies[ 1 ].version, RequiredVersion( 3, 4 ) );
}

TEST_F( PluginInfoFromJSONTestSuite, testPluginDependenciesFewCorrectAndMalformedDependencies /*1.6.4*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "id"
                                                       << "zwave"
                                                       << "version"
                                                       << "4.2"
                                                       << "type"
                                                       << "functional_unit"
                                                       << "dependencies"
                                                       << JSON( "plugins" << JSON_ARRAY( JSON( "alias"
                                                                                               << "bluetooth"
                                                                                               << "version"
                                                                                               << "1.2" )
                                                                                         << JSON( "id"
                                                                                                  << "zwave_common"
                                                                                                  << "version"
                                                                                                  << "3.4" )
                                                                                         << JSON( "id"
                                                                                                  << "ZigBee"
                                                                                                  << "version"
                                                                                                  << "any" )
                                                                                         << JSON( "id"
                                                                                                  << "plugins_common"
                                                                                                  << "version"
                                                                                                  << "7.8" ) ) )
                                                       << "permissions" << JSON_ARRAY_( "zwave", "storage" ) ) );

    EXPECT_TRUE( pluginInfo );

    EXPECT_EQ( pluginInfo->dependencies.requiredPlugins.size(), 2 );
    auto& dependencies = pluginInfo->dependencies.requiredPlugins;

    EXPECT_TRUE( dependencies[ 0 ].isValid() );
    EXPECT_EQ( dependencies[ 0 ].id, "zwave_common" );
    EXPECT_EQ( dependencies[ 0 ].version, RequiredVersion( 3, 4 ) );

    EXPECT_TRUE( dependencies[ 1 ].isValid() );
    EXPECT_EQ( dependencies[ 1 ].id, "plugins_common" );
    EXPECT_EQ( dependencies[ 1 ].version, RequiredVersion( 7, 8 ) );
}

TEST_F( PluginInfoFromJSONTestSuite, testAddonAndPluginDependencies /*1.7*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "id"
                                                       << "zwave"
                                                       << "version"
                                                       << "4.2"
                                                       << "type"
                                                       << "functional_unit"
                                                       << "dependencies"
                                                       << JSON( "addons" << JSON_ARRAY( JSON( "id"
                                                                                              << "lua"
                                                                                              << "version"
                                                                                              << "1.2" )
                                                                                        << JSON( "id"
                                                                                                 << "zwave"
                                                                                                 << "version"
                                                                                                 << "3.4" ) )
                                                                         << "plugins"
                                                                         << JSON_ARRAY( JSON( "id"
                                                                                              << "zwave_common"
                                                                                              << "version"
                                                                                              << "5.6" )
                                                                                        << JSON( "id"
                                                                                                 << "plugins_common"
                                                                                                 << "version"
                                                                                                 << "7.8" ) ) )
                                                       << "permissions" << JSON_ARRAY_( "zwave", "storage" ) ) );

    EXPECT_TRUE( pluginInfo );

    EXPECT_EQ( pluginInfo->dependencies.requiredAddons.size(), 2 );
    auto& addonDependencies = pluginInfo->dependencies.requiredAddons;

    EXPECT_TRUE( addonDependencies[ 0 ].isValid() );
    EXPECT_EQ( addonDependencies[ 0 ].id, "lua" );
    EXPECT_EQ( addonDependencies[ 0 ].version, RequiredVersion( 1, 2 ) );

    EXPECT_TRUE( addonDependencies[ 1 ].isValid() );
    EXPECT_EQ( addonDependencies[ 1 ].id, "zwave" );
    EXPECT_EQ( addonDependencies[ 1 ].version, RequiredVersion( 3, 4 ) );

    EXPECT_EQ( pluginInfo->dependencies.requiredPlugins.size(), 2 );
    auto& pluginDependencies = pluginInfo->dependencies.requiredPlugins;

    EXPECT_TRUE( pluginDependencies[ 0 ].isValid() );
    EXPECT_EQ( pluginDependencies[ 0 ].id, "zwave_common" );
    EXPECT_EQ( pluginDependencies[ 0 ].version, RequiredVersion( 5, 6 ) );

    EXPECT_TRUE( pluginDependencies[ 1 ].isValid() );
    EXPECT_EQ( pluginDependencies[ 1 ].id, "plugins_common" );
    EXPECT_EQ( pluginDependencies[ 1 ].version, RequiredVersion( 7, 8 ) );
}

TEST_F( PluginInfoFromJSONTestSuite, testPluginPermissionsEmptyPluginPermissions /*1.8.1*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "id"
                                                       << "zwave"
                                                       << "version"
                                                       << "4.2"
                                                       << "type"
                                                       << "functional_unit"
                                                       << "permissions" << JSON_ARRAY_() ) );

    EXPECT_TRUE( pluginInfo );
    EXPECT_EQ( pluginInfo->permissions.size(), 0 );
}

TEST_F( PluginInfoFromJSONTestSuite, testPluginPermissionsMailformedPluginPermissions /*1.8.2*/ ) {
    auto pluginInfo = BaseConfig::getBaseConfig( JSON( "id"
                                                       << "zwave"
                                                       << "version"
                                                       << "4.2"
                                                       << "type"
                                                       << "functional_unit"
                                                       << "permissions"
                                                       << JSON_ARRAY_( 1,
                                                                       "zwave",
                                                                       JSON( "key"
                                                                             << "value" ) ) ) );

    EXPECT_TRUE( pluginInfo );

    EXPECT_EQ( pluginInfo->permissions.size(), 1 );
    EXPECT_EQ( pluginInfo->permissions.front(), "zwave" );
}

TEST_F( PluginInfoFromJSONTestSuite, testExtensionsSingleConfig /*1.9.1*/ ) {
    auto pluginInfo =
        BaseConfig::getBaseConfig( JSON( "id"
                                         << "zwave"
                                         << "version"
                                         << "4.2"
                                         << "type"
                                         << "functional_unit"
                                         << "permissions" << JSON_ARRAY_( "zwave", "storage" ) << "extensions"
                                         << JSON( "configs" << JSON_ARRAY_( JSON( "name"
                                                                                  << "test_config"
                                                                                  << "description"
                                                                                  << "test_config_desc" ) ) ) ) );

    EXPECT_TRUE( pluginInfo );

    EXPECT_TRUE( pluginInfo->extensions );
    EXPECT_EQ( pluginInfo->extensions->pluginExtensions.size(), 0 );
    EXPECT_EQ( pluginInfo->extensions->configs.size(), 1 );
    EXPECT_EQ( pluginInfo->extensions->configs.at( 0 ).name, "test_config" );
    EXPECT_EQ( pluginInfo->extensions->configs.at( 0 ).description, "test_config_desc" );
}

TEST_F( PluginInfoFromJSONTestSuite, testExtensionsSeveralConfigs /*1.9.2*/ ) {
    auto pluginInfo =
        BaseConfig::getBaseConfig( JSON( "id"
                                         << "zwave"
                                         << "version"
                                         << "4.2"
                                         << "type"
                                         << "functional_unit"
                                         << "permissions" << JSON_ARRAY_( "zwave", "storage" ) << "extensions"
                                         << JSON( "configs" << JSON_ARRAY_( JSON( "name"
                                                                                  << "test_config"
                                                                                  << "description"
                                                                                  << "test_config_desc" ),
                                                                            JSON( "name"
                                                                                  << "test_config2"
                                                                                  << "description"
                                                                                  << "test_config_desc2" ) ) ) ) );

    EXPECT_TRUE( pluginInfo );

    EXPECT_TRUE( pluginInfo->extensions );
    EXPECT_EQ( pluginInfo->extensions->pluginExtensions.size(), 0 );
    EXPECT_EQ( pluginInfo->extensions->configs.size(), 2 );
    EXPECT_EQ( pluginInfo->extensions->configs.at( 0 ).name, "test_config" );
    EXPECT_EQ( pluginInfo->extensions->configs.at( 0 ).description, "test_config_desc" );
    EXPECT_EQ( pluginInfo->extensions->configs.at( 1 ).name, "test_config2" );
    EXPECT_EQ( pluginInfo->extensions->configs.at( 1 ).description, "test_config_desc2" );
}

TEST_F( PluginInfoFromJSONTestSuite, testExtensionsSinglePlugin /*1.9.3*/ ) {
    auto pluginInfo =
        BaseConfig::getBaseConfig( JSON( "id"
                                         << "zwave"
                                         << "version"
                                         << "4.2"
                                         << "type"
                                         << "functional_unit"
                                         << "permissions" << JSON_ARRAY_( "zwave", "storage" ) << "extensions"
                                         << JSON( "plugins" << JSON_ARRAY_(
                                                      JSON( "id"
                                                            << "zwave"
                                                            << "configs"
                                                            << JSON_ARRAY_( JSON( "name"
                                                                                  << "test_name"
                                                                                  << "config"
                                                                                  << "test_config" ) ) ) ) ) ) );

    EXPECT_TRUE( pluginInfo );

    EXPECT_TRUE( pluginInfo->extensions );
    EXPECT_EQ( pluginInfo->extensions->configs.size(), 0 );
    EXPECT_EQ( pluginInfo->extensions->pluginExtensions.size(), 1 );
    EXPECT_EQ( pluginInfo->extensions->pluginExtensions.at( 0 ).pluginId, "zwave" );
    EXPECT_EQ( pluginInfo->extensions->pluginExtensions.at( 0 ).configs.size(), 1 );
    EXPECT_EQ( pluginInfo->extensions->pluginExtensions.at( 0 ).configs.at( 0 ).name, "test_name" );
    EXPECT_EQ( pluginInfo->extensions->pluginExtensions.at( 0 ).configs.at( 0 ).configFile, "test_config" );
}

TEST_F( PluginInfoFromJSONTestSuite, testExtensionsSeveralPlugins /*1.9.4*/ ) {
    auto pluginInfo =
        BaseConfig::getBaseConfig( JSON( "id"
                                         << "zwave"
                                         << "version"
                                         << "4.2"
                                         << "type"
                                         << "functional_unit"
                                         << "permissions" << JSON_ARRAY_( "zwave", "storage" ) << "extensions"
                                         << JSON( "plugins" << JSON_ARRAY_(
                                                      JSON( "id"
                                                            << "zwave"
                                                            << "configs"
                                                            << JSON_ARRAY_( JSON( "name"
                                                                                  << "test_name"
                                                                                  << "config"
                                                                                  << "test_config" ) ) ),
                                                      JSON( "id"
                                                            << "zigbee"
                                                            << "configs"
                                                            << JSON_ARRAY_( JSON( "name"
                                                                                  << "test_name2"
                                                                                  << "config"
                                                                                  << "test_config2" ) ) ) ) ) ) );

    EXPECT_TRUE( pluginInfo );

    EXPECT_TRUE( pluginInfo->extensions );
    EXPECT_EQ( pluginInfo->extensions->configs.size(), 0 );
    EXPECT_EQ( pluginInfo->extensions->pluginExtensions.size(), 2 );
    EXPECT_EQ( pluginInfo->extensions->pluginExtensions.at( 0 ).pluginId, "zwave" );
    EXPECT_EQ( pluginInfo->extensions->pluginExtensions.at( 0 ).configs.size(), 1 );
    EXPECT_EQ( pluginInfo->extensions->pluginExtensions.at( 0 ).configs.at( 0 ).name, "test_name" );
    EXPECT_EQ( pluginInfo->extensions->pluginExtensions.at( 0 ).configs.at( 0 ).configFile, "test_config" );
    EXPECT_EQ( pluginInfo->extensions->pluginExtensions.at( 1 ).pluginId, "zigbee" );
    EXPECT_EQ( pluginInfo->extensions->pluginExtensions.at( 1 ).configs.size(), 1 );
    EXPECT_EQ( pluginInfo->extensions->pluginExtensions.at( 1 ).configs.at( 0 ).name, "test_name2" );
    EXPECT_EQ( pluginInfo->extensions->pluginExtensions.at( 1 ).configs.at( 0 ).configFile, "test_config2" );
}

TEST_F( PluginInfoFromJSONTestSuite, testExtensionsSeveralPluginExtensions /*1.9.5*/ ) {
    auto pluginInfo =
        BaseConfig::getBaseConfig( JSON( "id"
                                         << "zwave"
                                         << "version"
                                         << "4.2"
                                         << "type"
                                         << "functional_unit"
                                         << "permissions" << JSON_ARRAY_( "zwave", "storage" ) << "extensions"
                                         << JSON( "plugins" << JSON_ARRAY_(
                                                      JSON( "id"
                                                            << "zwave"
                                                            << "configs"
                                                            << JSON_ARRAY_( JSON( "name"
                                                                                  << "test_name"
                                                                                  << "config"
                                                                                  << "test_config" ),
                                                                            JSON( "name"
                                                                                  << "test_name2"
                                                                                  << "config"
                                                                                  << "test_config2" ) ) ) ) ) ) );

    EXPECT_TRUE( pluginInfo );

    EXPECT_TRUE( pluginInfo->extensions );
    EXPECT_EQ( pluginInfo->extensions->configs.size(), 0 );
    EXPECT_EQ( pluginInfo->extensions->pluginExtensions.size(), 1 );
    EXPECT_EQ( pluginInfo->extensions->pluginExtensions.at( 0 ).pluginId, "zwave" );
    EXPECT_EQ( pluginInfo->extensions->pluginExtensions.at( 0 ).configs.size(), 2 );
    EXPECT_EQ( pluginInfo->extensions->pluginExtensions.at( 0 ).configs.at( 0 ).name, "test_name" );
    EXPECT_EQ( pluginInfo->extensions->pluginExtensions.at( 0 ).configs.at( 0 ).configFile, "test_config" );
    EXPECT_EQ( pluginInfo->extensions->pluginExtensions.at( 0 ).configs.at( 1 ).name, "test_name2" );
    EXPECT_EQ( pluginInfo->extensions->pluginExtensions.at( 0 ).configs.at( 1 ).configFile, "test_config2" );
}
