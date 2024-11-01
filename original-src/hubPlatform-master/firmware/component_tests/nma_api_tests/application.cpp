#include "nma_api_tests/application.h"
#include "test_utils/test_environment.h"
#include "update_manager/resources/um_resources.h"
#include "database_client/db_tables_names.h"
#include "system_utils/environment.h"
#include "system_utils/process/shared_mutex.h"
#include "system_utils/find_free_local_port.h"

#include "jsonrpc/message.h"
#include "log_utils/ezlo_log.h"

#include <common_utils/cpp14.h>
#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

NmaApiTestsApplication::NmaApiTestsApplication()
: ComponentTestApplication( Tests::getComponentTestsFolder() + "/nma_api_tests/root" ) {
    m_forker = Process::Forker::getInstance();
}

void NmaApiTestsApplication::beforeApplicationsStartup() {
    ComponentTestApplication::beforeApplicationsStartup();

    m_gatewaysChecker = std::make_unique< GatewaysChecker >( getDBClient() );
    setBroadcastsProcessFunction( [this]( rpc::Message& message ) { m_gatewaysChecker->handleMessage( message ); } );
}
void NmaApiTestsApplication::setUpAdditionalApplications() {
    createLinkToAddonInTestFolder( "ha-luad" );
    createLinkToAddonInTestFolder( "ha-zwaved" );

    setenv( "RUNNING_COMPONENT_TESTS", "1", 1 );  // to stop initiate backup scheduler
    executeCoreService( "ha-backupd", "backupd" );
    executeCoreService( "ha-cored", "cored" );
    std::this_thread::sleep_for( std::chrono::seconds( 3 ) );  // cored should be startuped before extensions_manager.
                                                               // TODO: implement dependency mechanism between deamons
    executeCoreService( "ha-extensions_manager", "extensions_manager" );
    executeCoreService( "ha-scenes_manager", "scenes_manager" );

    executeAddonService( "ha-luad", "lua" );
    executeAddonService( "ha-zwaved", "zwave", { "--test_mode" } );

    executeCoreService( "ha-update_manager", "update_manager" );

    setUpCustomPluginsWebServer();
}

void NmaApiTestsApplication::afterApplicationsStartup() {
    m_gatewaysChecker->checkGatewayReady( getRequiredGateways() );
    m_gatewaysChecker.reset();
    resetBroadcastsProcessFunction();
    ComponentTestApplication::afterApplicationsStartup();
}

bool NmaApiTestsApplication::shouldUseRandomLocalPort() {
    return true;
}

void NmaApiTestsApplication::tearDown() {
    m_forker->kill( m_customPluginsWebServerPid, SIGTERM );
    ComponentTestApplication::tearDown();
}

int NmaApiTestsApplication::getCustomPluginsWebServerPort() const {
    return m_customPluginsWebServerPort;
}

void NmaApiTestsApplication::setUpCustomPluginsWebServer() {
    Process::SharedMutex processMutex( "uniquePortMutex" );
    std::unique_lock< Process::SharedMutex > lock( processMutex );
    m_customPluginsWebServerPort = system_utils::getUniqueFreeLocalPort();

    const auto wwwPath = HubPlatform::Environment::getFirmwareRootFolder() + "/www";

    m_customPluginsWebServerPid = m_forker->fork( "execute() { cd " + wwwPath + "; python2 -m SimpleHTTPServer " +
                                                  std::to_string( m_customPluginsWebServerPort ) + "; }; execute 2> " +
                                                  getLogFolderPath() + "/pythonSimpleHTTPserver.log" );
}

ComponentTestApplication::RequiredGateways NmaApiTestsApplication::getRequiredGateways() const {
    return { "model_factory", "item_control", "network_socket", "cloud_storage", "house_modes_test" };
}

}  // namespace Tests
}  // namespace HubPlatform
