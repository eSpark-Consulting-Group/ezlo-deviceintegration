#include "extensions_manager/resources/em_resources.h"

#include "plugin_utils/plugin_utils.h"
#include "fakes/em_fake_idbclient.h"
#include "common_utils/cpp14.h"
#include "database_client/dbqueryparser.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <sstream>

/*

TESTPLAN

DONE    1 Plugin utils cases
DONE        1.1 Get internal plugin template
DONE        1.2 Get internal plugin template by a wrong path
DONE        1.3 Check scriptFilePath
DONE        1.4 Check splitScriptId

*/

namespace HubPlatform {
namespace ExtensionsManager {
namespace UT {

namespace {

static const std::string DefaultPluginId = "myplugin";

class StubPluginUtils : public PluginUtils {
public:
    StubPluginUtils( std::string filePath, std::string fileContent )
    : m_filePath( std::move( filePath ) )
    , m_fileContent( std::move( fileContent ) ) {}

    PluginsIds getInstalledPlugins() const override {
        return { DefaultPluginId };
    }

    PluginsIds getCustomPlugins() const override {
        return {};
    }

    StreamUPtr getInternalPluginFile( const std::string& pluginId,
                                      const std::string& internalFilePath ) const override {
        if( pluginId != DefaultPluginId || internalFilePath != m_filePath ) {
            return nullptr;
        }

        return std::make_unique< std::istringstream >( m_fileContent );
    }

private:
    std::string m_filePath;
    std::string m_fileContent;
};

class StubDbClient : public FakeIDbClient {
public:
    std::vector< Json > findAll( const std::string& dbName,
                                 const db::Query& query,
                                 const Json* fieldsToReturn,
                                 int nToReturn,
                                 int nToSkip,
                                 const bool lockIgnore ) const override {
        if( dbName != Resources::DBTableNames::Plugins ) {
            return {};
        }

        if( query.getFilter() != JSON( db::ID_FIELD_NAME << DefaultPluginId ) ) {
            return {};
        }

        if( fieldsToReturn && fieldsToReturn->find( "version" ) == fieldsToReturn->end() ) {
            return {};
        }

        return { JSON( "version"
                       << "42.42" ) };
    }
};

}  // namespace

class PluginUtilsTestSuite : public ::testing::Test {};

TEST_F( PluginUtilsTestSuite, getInternalPluginTemplate ) {
    StubPluginUtils pluginUtils( "mypath/add_device.template", "template content" );

    {
        auto templateStream = pluginUtils.getInternalPluginTemplate( "myplugin:mypath/add_device.template" );

        ASSERT_TRUE( templateStream );
        const std::string templateContent( std::istreambuf_iterator< char >( *templateStream ), {} );
        EXPECT_EQ( templateContent, "template content" );
    }

    {
        auto templateStream = pluginUtils.getInternalPluginTemplate( "myplugin:/mypath/add_device.template" );

        ASSERT_TRUE( templateStream );
        const std::string templateContent( std::istreambuf_iterator< char >( *templateStream ), {} );
        EXPECT_EQ( templateContent, "template content" );
    }
}

TEST_F( PluginUtilsTestSuite, getInternalPluginTemplateByWrongPath ) {
    StubPluginUtils pluginUtils( "mypath/add_device.template", "template content" );

    EXPECT_FALSE( pluginUtils.getInternalPluginTemplate( "mypluginmypath/add_device.template" ) );
    EXPECT_FALSE( pluginUtils.getInternalPluginTemplate( "myplugin/mypath/add_device.template" ) );
    EXPECT_FALSE( pluginUtils.getInternalPluginTemplate( "myplugi:mypath/add_device.template" ) );
    EXPECT_FALSE( pluginUtils.getInternalPluginTemplate( "myplugin/mypath/add_device.templat" ) );
    EXPECT_FALSE( pluginUtils.getInternalPluginTemplate( "/myplugin:mypath/add_device.template" ) );
    EXPECT_FALSE( pluginUtils.getInternalPluginTemplate( "myplugin::mypath/add_device.template" ) );
    EXPECT_FALSE( pluginUtils.getInternalPluginTemplate( ":" ) );
    EXPECT_FALSE( pluginUtils.getInternalPluginTemplate( "" ) );
}

TEST_F( PluginUtilsTestSuite, checkScriptFilePath ) {
    StubDbClient dbClient;
    StubPluginUtils pluginUtils( "", "" );

    std::string nativeName = "HUB:zwave/scripts/start_include";
    std::string plaginInfo = "HUB:zwave/scripts/start_include.lua";

    pluginUtils.checkScriptFilePath( plaginInfo );

    EXPECT_EQ( plaginInfo, nativeName );

    plaginInfo = "zwave/scripts/start_include.lua";
    pluginUtils.checkScriptFilePath( plaginInfo );

    EXPECT_EQ( plaginInfo, nativeName );

    plaginInfo = "zwave/scripts/start_include";
    pluginUtils.checkScriptFilePath( plaginInfo );

    EXPECT_EQ( plaginInfo, nativeName );

    plaginInfo = "zwave/scripts/.lua/start_include";
    pluginUtils.checkScriptFilePath( plaginInfo );
    EXPECT_NE( plaginInfo, nativeName );
}

TEST_F( PluginUtilsTestSuite, checksplitScriptId ) {
    StubPluginUtils pluginUtils( "", "" );
    std::string componentName;
    std::string relativeFilePath;

    std::tie( componentName, relativeFilePath ) =
        pluginUtils.splitScriptId( "HUB:component_name/relative/path/best_script_ever" );
    EXPECT_EQ( componentName, "component_name" );
    EXPECT_EQ( relativeFilePath, "relative/path/best_script_ever" );
}

}  // namespace UT
}  // namespace ExtensionsManager
}  // namespace HubPlatform
