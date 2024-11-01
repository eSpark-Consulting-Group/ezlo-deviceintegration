#include "extensions_manager/tests/em_remote_interface_factory.h"
#include "extensions_manager/tests/fakes/em_fake_plugin_utils.h"
#include "extensions_manager/resources/em_resources.h"
#include "extensions_manager/exceptions.h"

#include "service_interfaces/tests/fixtures/fake_service_fixture.h"
#include "database_client/dbqueryparser.h"
#include "jsonrpc/message.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace HubPlatform {
namespace ExtensionsManager {
namespace UT {
namespace {

class FakeMessage : public rpc::Message {
public:
    GenericSenderPtr getNetworkObject() const override {
        return {};
    }
};

class StubPluginUtils : public FakePluginUtils {
public:
    StreamUPtr getInternalPluginTemplate( const std::string& templateName ) const override {
        if( templateName == "myplugin:path/myfile.templ" ) {
            return std::make_unique< std::istringstream >( "template content" );
        }

        return nullptr;
    }
};

}  // namespace

class RemoteInterfaceTestSuite : public Services::UT::FakeServiceTestFixture, public RemoteInterfaceFactory {};

TEST_F( RemoteInterfaceTestSuite, getTemplate ) {
    auto remoteInterface = createRemoteInterface(
        getHostService(), std::make_unique< StubPluginUtils >(), std::make_unique< FakeAnonymousPluginUtils >() );

    auto templateContent = remoteInterface->getTemplateForDevice( "myplugin:path/myfile.templ" );

    EXPECT_EQ( templateContent, "template content" );
}

TEST_F( RemoteInterfaceTestSuite, getTemplateWrongTemplateNameError ) {
    auto remoteInterface = createRemoteInterface(
        getHostService(), std::make_unique< StubPluginUtils >(), std::make_unique< FakeAnonymousPluginUtils >() );

    EXPECT_THROW( remoteInterface->getTemplateForDevice( "myplugin:wrongPath/myfile.templ" ),
                  ExtensionsManager::TemplateNotFoundError );
}

TEST_F( RemoteInterfaceTestSuite, getPluginPermissions ) {
    using namespace Resources;

    auto remoteInterface = createRemoteInterface(
        getHostService(), std::make_unique< StubPluginUtils >(), std::make_unique< FakeAnonymousPluginUtils >() );

    const auto pluginId = "id_1";
    const auto pluginRecord =
        JSON( db::ID_FIELD_NAME << pluginId << PluginRecord::Permissions << JSON_ARRAY_( "zwave", "storage" ) );

    getHostService().getDBClient().insert( DBTableNames::Plugins, pluginRecord );

    EXPECT_EQ( remoteInterface->checkPluginPermission( pluginId, "storage" ), true );
}

}  // namespace UT
}  // namespace ExtensionsManager
}  // namespace HubPlatform
