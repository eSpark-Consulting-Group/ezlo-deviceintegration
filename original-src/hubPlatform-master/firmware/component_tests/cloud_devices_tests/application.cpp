#include "cloud_devices_tests/application.h"
#include "test_utils/test_environment.h"
#include "update_manager/resources/um_resources.h"

#include "json_utils/json_utils.h"
#include "jsonrpc/message.h"
#include "common_utils/cpp14.h"
#include "system_utils/process/shared_mutex.h"
#include "system_utils/find_free_local_port.h"
#include "system_utils/environment.h"

namespace HubPlatform {
namespace Tests {

CloudDevicesTestsApplication::CloudDevicesTestsApplication()
: ComponentTestApplication( Tests::getComponentTestsFolder() + "/cloud_devices_tests/root" ) {
    m_forker = Process::Forker::getInstance();
}

void CloudDevicesTestsApplication::beforeApplicationsStartup() {
    ComponentTestApplication::beforeApplicationsStartup();
    setenv( "RUNNING_COMPONENT_TESTS", "1", 1 );
    join( GroupsNames::EXTERNAL_DEVICE_GROUP );

    setRequestsProcessFunction( [this]( rpc::Message& message ) {
        std::unique_lock< std::mutex > lock( m_requestsMutex );
        m_requestsMessages.push_back( message.getMessage() );
        lock.unlock();
        m_requestsCondVar.notify_all();
    } );
    m_gatewaysChecker = std::make_unique< GatewaysChecker >( getDBClient() );
    setBroadcastsProcessFunction( [this]( rpc::Message& message ) { m_gatewaysChecker->handleMessage( message ); } );
}

void CloudDevicesTestsApplication::setUpAdditionalApplications() {
    createLinkToAddonInTestFolder( "ha-luad" );
    executeCoreService( "ha-backupd", "backupd" );
    executeCoreService( "ha-cored", "cored" );
    std::this_thread::sleep_for( std::chrono::seconds( 3 ) );  // cored should be startuped before extensions_manager.

    executeCoreService( "ha-extensions_manager", "extensions_manager" );
    executeCoreService( "ha-external_controllers", "external_controllers" );
    executeCoreService( "ha-cloud_devices", "cloud_devices" );

    executeAddonService( "ha-luad", "lua" );

    // Need this for extensions manager if we do not run update_manager service
    getDBClient().update( UpdateManager::Resources::DBTableNames::UnavailablePlugins,
                          JSON_EMPTY_OBJ,
                          JSON( "_id"
                                << "UM.initialized" ),
                          true,
                          false );
    setUpCustomLocalCloudDataWebServer();
}

void CloudDevicesTestsApplication::afterApplicationsStartup() {
    std::unique_lock< std::mutex > lock( m_requestsMutex );
    m_requestsCondVar.wait_for( lock, std::chrono::seconds( 30 ), [this] { return !m_requestsMessages.empty(); } );
    auto it = std::find_if( m_requestsMessages.rbegin(), m_requestsMessages.rend(), []( const JsonObject& value ) {
        return json_get_str( value, "method" ) == "cloud.controller_abstracts_list";
    } );
    if( it == m_requestsMessages.rend() ) {
        throw ComponentTestApplicationError( "Expected cloud.controller_abstracts_list request!" );
    }
    auto& request = *it;
    sendResponseToHub( request, parseJson( R"({ "version": -1, "not_modified": 1 })" ) );
    m_gatewaysChecker->checkGatewayReady( { "model_factory" } );
    m_gatewaysChecker.reset();
    resetBroadcastsProcessFunction();
    resetRequestsProcessFunction();

    ComponentTestApplication::afterApplicationsStartup();
}

bool CloudDevicesTestsApplication::shouldUseRandomLocalPort() {
    return true;
}

void CloudDevicesTestsApplication::setUpCustomLocalCloudDataWebServer() {
    Process::SharedMutex processMutex( "uniquePortMutex" );
    std::unique_lock< Process::SharedMutex > lock( processMutex );
    m_customLocalCloudDataWebServerPort = system_utils::getUniqueFreeLocalPort();

    const auto wwwPath = HubPlatform::Environment::getFirmwareRootFolder() + "/www";

    m_customLocalCloudDataWebServerPid =
        m_forker->fork( "execute() { cd " + wwwPath + "; python2 -m SimpleHTTPServer " +
                        std::to_string( m_customLocalCloudDataWebServerPort ) + "; }; execute 2> " +
                        getLogFolderPath() + "/pythonSimpleHTTPserver.log" );
}

void CloudDevicesTestsApplication::tearDown() {
    m_forker->kill( m_customLocalCloudDataWebServerPid, SIGTERM );
    ComponentTestApplication::tearDown();
}

int CloudDevicesTestsApplication::getCustomLocalCloudDataWebServerPort() const {
    return m_customLocalCloudDataWebServerPort;
}
}  // namespace Tests
}  // namespace HubPlatform
