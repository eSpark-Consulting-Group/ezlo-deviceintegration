#include "scene_tests/application.h"
#include "test_utils/test_environment.h"
#include "update_manager/resources/um_resources.h"

#include "jsonrpc/message.h"
#include "log_utils/ezlo_log.h"

#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace Tests {

ScenesApiTestsApplication::ScenesApiTestsApplication()
: ComponentTestApplication( Tests::getComponentTestsFolder() + "/scene_tests/root" ) {}

void ScenesApiTestsApplication::beforeApplicationsStartup() {
    ComponentTestApplication::beforeApplicationsStartup();

    m_gatewaysChecker = std::make_unique< GatewaysChecker >( getDBClient() );
    setBroadcastsProcessFunction( [this]( rpc::Message& message ) { m_gatewaysChecker->handleMessage( message ); } );
}
void ScenesApiTestsApplication::setUpAdditionalApplications() {
    createLinkToAddonInTestFolder( "ha-luad" );
    setenv( "RUNNING_COMPONENT_TESTS", "1", 1 );
    executeCoreService( "ha-backupd", "backupd" );
    executeCoreService( "ha-cored", "cored" );
    std::this_thread::sleep_for( std::chrono::seconds( 3 ) );  // cored should be startuped before extensions_manager.
                                                               // TODO: implement dependency mechanism between deamons
    executeCoreService( "ha-extensions_manager", "extensions_manager" );
    executeCoreService( "ha-scenes_manager", "scenes_manager" );

    executeAddonService( "ha-luad", "lua" );

    // Need this for extensions manager if we do not run update_manager service
    getDBClient().update( UpdateManager::Resources::DBTableNames::UnavailablePlugins,
                          JSON_EMPTY_OBJ,
                          JSON( "_id"
                                << "UM.initialized" ),
                          true,
                          false );
}

void ScenesApiTestsApplication::afterApplicationsStartup() {
    m_gatewaysChecker->checkGatewayReady( { "model_factory", "item_control", "scenes_test" } );
    m_gatewaysChecker.reset();
    resetBroadcastsProcessFunction();
    ComponentTestApplication::afterApplicationsStartup();
}

}  // namespace Tests
}  // namespace HubPlatform
