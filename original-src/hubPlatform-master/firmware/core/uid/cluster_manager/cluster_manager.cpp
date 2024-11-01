#include "uid/cluster_manager/cluster_manager.h"
#include "uid/icc_interface/icc_common.h"
#include "uid/ui_service.h"

#include "common_utils/cpp14.h"
#include "common_utils/hub_info_helper.h"

#include "database_client/dbqueryparser.h"

namespace HubPlatform {
namespace Uid {

ClusterManager* ClusterManager::m_instance = nullptr;

ClusterManager::ClusterManager( Uid::Service& uiService, const ProvisionDataUtils::ProvisionData& provisionData )
: m_uiService( uiService )
, m_provisionData( provisionData ) {
    m_instance = this;
}

ClusterManager::~ClusterManager() {
    m_instance = nullptr;
}

ClusterManager* ClusterManager::getInstance() {
    if( !m_instance ) {
        Log::warn( cc::warn( "Cluster manager is not yet started" ) );
    }

    return m_instance;
}

Uid::Service& ClusterManager::getUiService() {
    return m_uiService;
}

// "params": {
//     "_id": "6fc13be5-19e4-4bdc-b593-7cd86869ea32",
//     "ip": [ "192.168.0.1", "192.168.10.110" ]
// }
void ClusterManager::handleBridgeControllerConnectionSet( rpc::Message& rpcMessage ) {
    using namespace InterControllerCommunication::Common;

    auto params = rpcMessage.getParams();
    if( !checkField( params, "_id", rpcMessage.getAnswer() ) || !checkField( params, "ip", rpcMessage.getAnswer() ) ) {
        Log::error( cc::error( "Unsupported bridge controller connection set request format" ) );
        return;
    }

    auto uuid = json_get_str( params, "_id" );
    ControllerConnection::ConnectionInfo info;
    info.ips = json_get_vec< std::string >( params, "ip" );

    if( info.ips.empty() ) {
        m_uiService.getDBClient().remove( ControllersInfoDBName, JSON( db::ID_FIELD_NAME << uuid ) );
    }
    else {
        m_uiService.getDBClient().update( ControllersInfoDBName, JSON_EMPTY_OBJ, params, true );
    }

    auto controllerId = getControllerIdByUUID( m_uiService.getDBClient(), uuid );
    if( controllerId.empty() ) {
        Log::warn( cc::warn( "Updating connection info of controller without credentials" ) );
        return;
    }

    auto it = m_controllerConnections.find( controllerId );
    if( it != m_controllerConnections.end() ) {
        auto mdnsInfo = m_discoveryService->getControllerMdnsInfo( controllerId );
        auto mdnsIps = getMdnsAddresses( mdnsInfo );
        if( !mdnsIps.empty() ) {
            info.ips.insert( info.ips.begin(), mdnsIps.begin(), mdnsIps.end() );
        }
        it->second->onConnectionInfoUpdated( info );
    }
}

void ClusterManager::handleBridgeConnectionsList( rpc::Message& rpcMessage ) {
    using namespace InterControllerCommunication::Common;
    JsonObject& joAnswer = rpcMessage.getAnswer();

    std::map< std::string, JsonObject > controllersInfoByUuid;

    // Fill credentials
    auto allCredentials =
        m_uiService.getDBClient().findAll( CredentialsDBName, JSON( "serial" << JSON( "$exists" << true ) ), nullptr );
    for( auto& credentialsJson : allCredentials ) {
        auto controllerId = json_get_str( credentialsJson, "serial" );
        auto controllerUuid = json_get_str( credentialsJson, "uuid" );
        auto& info = controllersInfoByUuid[ controllerUuid ];
        info[ "_id" ] = controllerUuid;
        info[ "serial" ] = controllerId;
    }

    // Fill Ips
    auto allControllerInfos = m_uiService.getDBClient().findAll( ControllersInfoDBName, JSON_EMPTY_OBJ, nullptr );
    for( auto& controllerInfoJson : allControllerInfos ) {
        auto controllerUuid = json_get_str( controllerInfoJson, db::ID_FIELD_NAME );
        auto ips = json_get_vec< std::string >( controllerInfoJson, "ip" );
        auto& info = controllersInfoByUuid[ controllerUuid ];
        info[ "_id" ] = controllerUuid;
        info[ "ip" ] = ips;
    }

    // Fill status
    for( auto& infoIt : controllersInfoByUuid ) {
        auto& info = infoIt.second;
        auto controllerId = json_get_str( info, "serial" );
        if( !controllerId.empty() ) {
            if( isExternalControllerConnected( controllerId ) ) {
                info[ "status" ] = "direct_connection";
            }
            else {
                auto it = m_controllerConnections.find( controllerId );
                if( it != m_controllerConnections.end() ) {
                    if( it->second->isConnected() ) {
                        info[ "status" ] = "direct_connection";
                    }
                }
            }
        }
    }

    JsonArrayBuilder controllers;
    for( auto& infoIt : controllersInfoByUuid ) {
        auto& info = infoIt.second;
        if( info.count( "serial" ) == 0 ) {
            info[ "serial" ] = "";
        }
        if( info.count( "status" ) == 0 ) {
            info[ "status" ] = "not_connected";
        }
        if( info.count( "ip" ) == 0 ) {
            info[ "ip" ] = JSON_EMPTY_ARR;
        }

        controllers << std::move( info );
    }
    joAnswer[ "result" ][ "controllers" ] = std::move( controllers );
}

void ClusterManager::handleBridgeControllersList( rpc::Message& rpcMessage ) {
    using namespace InterControllerCommunication::Common;
    JsonObject& joAnswer = rpcMessage.getAnswer();
    joAnswer[ "result" ][ "controllers" ] = m_discoveryService->listControllersMdnsInfo();
}

void ClusterManager::handleRegisterControllerConnection( const JsonObject& request, const std::string& sender ) {
    auto controllerId = json_get_str( request, "controllerId" );
    auto query = JSON( db::ID_FIELD_NAME << controllerId );
    if( !m_uiService.getDBClient().has( RegisteredControllersDBName, query ) ) {
        m_uiService.getDBClient().insert( RegisteredControllersDBName, query );
        initControllerConnection( controllerId );
    }
    m_uiService.getDBClient().update(
        RegisteredControllersDBName, query, JSON( "$addToSet" << JSON( "services" << sender ) ) );
}

void ClusterManager::handleUnregisterControllerConnection( const JsonObject& request, const std::string& sender ) {
    auto controllerId = json_get_str( request, "controllerId" );
    auto query = JSON( db::ID_FIELD_NAME << controllerId );
    m_uiService.getDBClient().update(
        RegisteredControllersDBName, query, JSON( "$pull" << JSON( "services" << sender ) ) );

    auto registration = m_uiService.getDBClient().findOne( RegisteredControllersDBName, query, nullptr );
    if( !registration.empty() ) {
        auto services = json_get_vec< std::string >( registration, "services" );
        if( services.empty() ) {
            m_uiService.getDBClient().remove( RegisteredControllersDBName, query );
            resetControllerConnection( controllerId );
        }
    }
}

void ClusterManager::setUp() {
    updateCertificates();

    m_discoveryService = std::make_unique< ControllersDiscoveryService >(
        m_uiService, m_uiService.getDBClient(), [&]( const Json& info ) { onMdnsInfoUpdated( info ); } );
    m_discoveryService->setUp();

    auto registrations = m_uiService.getDBClient().findAll( RegisteredControllersDBName, JSON_EMPTY_OBJ, nullptr );
    for( auto& registration : registrations ) {
        auto controllerId = json_get_str( registration, db::ID_FIELD_NAME );
        initControllerConnection( controllerId );
    }
}

void ClusterManager::tearDown() {
    m_discoveryService->tearDown();
}

bool ClusterManager::hasControllerConnection( const std::string& controllerId ) const {
    if( m_loggedInExternalControllers.count( controllerId ) != 0 ) {
        return true;
    }
    auto it = m_controllerConnections.find( controllerId );
    if( it == m_controllerConnections.end() ) {
        return false;
    }
    return it->second->isConnected();
}

bool ClusterManager::postMessage( const std::string& controllerId,
                                  const std::string& msg,
                                  tiny::ws::client::opcv eOpCode ) {
    if( controllerId >= HubInfoHelper::getSerial() ) {
        if( postMessageThroughServerConnection( controllerId, msg, eOpCode ) ) {
            return true;
        }
        return postMessageThroughClientConnection( controllerId, msg, eOpCode );
    }
    else {
        if( postMessageThroughClientConnection( controllerId, msg, eOpCode ) ) {
            return true;
        }
        return postMessageThroughServerConnection( controllerId, msg, eOpCode );
    }
}

bool ClusterManager::postMessage( const std::string& controllerId, const JsonObject& joMsg ) {
    return postMessage( controllerId, joMsg.dump() );
}

bool ClusterManager::postMessageThroughServerConnection( const std::string& controllerId,
                                                         const std::string& msg,
                                                         tiny::ws::client::opcv eOpCode ) {
    auto itServer = m_loggedInExternalControllers.find( controllerId );
    if( itServer != m_loggedInExternalControllers.end() ) {
        if( itServer->second->postMessage( msg, eOpCode ) ) {
            return true;
        }
    }
    return false;
}

bool ClusterManager::postMessageThroughClientConnection( const std::string& controllerId,
                                                         const std::string& msg,
                                                         tiny::ws::client::opcv eOpCode ) {
    auto itClient = m_controllerConnections.find( controllerId );
    if( itClient != m_controllerConnections.end() ) {
        if( itClient->second->postMessage( msg, eOpCode ) ) {
            return true;
        }
    }
    return false;
}

void ClusterManager::postMessageToAllConnectedControllers(
    const std::string& msg,
    tiny::ws::server::opcv eOpCode /*= tiny::ws::server::opcv::text*/ ) {
    for( auto& connectionPair : m_controllerConnections ) {
        connectionPair.second->postMessage( msg, eOpCode );
    }
}

void ClusterManager::initControllerConnection( const std::string& controllerId ) {
    Log::debug( cc::debug( "Registering connection to controller " ) + cc::note( controllerId ) );

    m_controllerConnections[ controllerId ] =
        std::make_unique< ControllerConnection >( *this,
                                                  controllerId,
                                                  readCredentials( controllerId ),
                                                  readConnectionInfo( controllerId ),
                                                  isExternalControllerConnected( controllerId ) );
}

void ClusterManager::resetControllerConnection( const std::string& controllerId ) {
    Log::debug( cc::debug( "Unregistering connection to controller " ) + cc::note( controllerId ) );

    auto it = m_controllerConnections.find( controllerId );
    if( it == m_controllerConnections.end() ) {
        return;
    }

    m_controllerConnections.erase( it );
}

bool ClusterManager::onExternalControllerLoggingIn( const std::string& controllerUuid, ServerPeerPtr peer ) {
    using namespace InterControllerCommunication::Common;
    auto controllerId = getControllerIdByUUID( m_uiService.getDBClient(), controllerUuid );

    if( controllerId.empty() ) {
        Log::error( cc::error( "Controllers credentials integrity check failed" ) );
        return false;
    }

    m_idsByUuidsCache[ controllerUuid ] = controllerId;
    m_loggedInExternalControllers[ controllerId ] = std::move( peer );

    auto it = m_controllerConnections.find( controllerId );
    if( it != m_controllerConnections.end() ) {
        return it->second->onExternalControllerLoggingIn();
    }

    return true;
}

void ClusterManager::onExternalControllerLoggedOut( const std::string& controllerUuid ) {
    using namespace InterControllerCommunication::Common;
    auto cacheIt = m_idsByUuidsCache.find( controllerUuid );

    if( cacheIt == m_idsByUuidsCache.end() ) {
        Log::error( cc::error( "Controllers credentials cache integrity check failed" ) );
        return;
    }

    auto controllerId = cacheIt->second;
    m_idsByUuidsCache.erase( cacheIt );
    m_loggedInExternalControllers.erase( controllerId );

    auto it = m_controllerConnections.find( controllerId );
    if( it != m_controllerConnections.end() ) {
        it->second->onExternalControllerLoggedOut();
    }
}

void ClusterManager::onCertificatesUpdated() {
    Log::debug( cc::debug( "Updating certificates for external controllers connections" ) );
    updateCertificates();
    for( auto& connectionPair : m_controllerConnections ) {
        connectionPair.second->onCertificatesUpdated();
    }
}

void ClusterManager::onCredentialsUpdated( const std::string& controllerId ) {
    auto it = m_controllerConnections.find( controllerId );
    if( it != m_controllerConnections.end() ) {
        it->second->onCredentialsUpdated( readCredentials( controllerId ) );
        // NOTE: connection info does not contain controllerId, so if it is set prior to
        //       credentials we may not be able to link those to proper connection
        it->second->onConnectionInfoUpdated( readConnectionInfo( controllerId ) );
    }
}

void ClusterManager::onMdnsInfoUpdated( const Json& info ) {
    auto controllerId = json_get_str( info[ "records" ], "Serial" );
    auto it = m_controllerConnections.find( controllerId );
    if( it != m_controllerConnections.end() ) {
        it->second->onConnectionInfoUpdated( readConnectionInfo( controllerId, info ) );
    }
}

tiny::ws::security_args ClusterManager::getSecurityArgs() const {
    std::lock_guard< std::mutex > lock( m_certificatesMutex );
    return m_securityArgs;
}

std::unique_lock< std::mutex > ClusterManager::lockCertificateFiles() {
    return m_uiService.lockCertificateFiles();
}

void ClusterManager::updateCertificates() {
    std::lock_guard< std::mutex > lock( m_certificatesMutex );
    m_securityArgs = m_uiService.getSecurityArgs();
}

ControllerConnection::Credentials ClusterManager::readCredentials( const std::string& controllerId ) const {
    ControllerConnection::Credentials credentials;

    auto allCredentials =
        m_uiService.getDBClient().findAll( CredentialsDBName, JSON( "serial" << controllerId ), nullptr );

    for( auto& credentialsJson : allCredentials ) {
        auto decryptedKey = m_provisionData.decryptSensitiveData( json_get_str( credentialsJson, "key" ) );
        auto uuid = json_get_str( credentialsJson, "uuid" );
        credentials.tokens.push_back( decryptedKey );
        if( credentials.uuid.empty() ) {
            credentials.uuid = uuid;
        }
        else {
            if( credentials.uuid != uuid ) {
                Log::error( cc::error( "Credentials consistency check failed" ) );
            }
        }
    }

    return credentials;
}

ControllerConnection::ConnectionInfo ClusterManager::readConnectionInfo( const std::string& controllerId,
                                                                         const Json& mdnsInfo ) const {
    using namespace InterControllerCommunication::Common;
    ControllerConnection::ConnectionInfo connectionInfo;

    auto uuid = getControllerUUIDById( m_uiService.getDBClient(), controllerId );
    if( uuid.empty() ) {
        Log::warn( "Trying to read connection info for controller without credentials" );
        return connectionInfo;
    }

    auto controllerInfo =
        m_uiService.getDBClient().findOne( ControllersInfoDBName, JSON( db::ID_FIELD_NAME << uuid ), nullptr );

    auto controllerMdnsInfo = mdnsInfo;
    if( controllerMdnsInfo.empty() ) {
        controllerMdnsInfo = m_discoveryService->getControllerMdnsInfo( controllerId );
    }

    auto cloudIps = json_get_vec< std::string >( controllerInfo, "ip" );
    if( controllerMdnsInfo.empty() ) {
        connectionInfo.ips = std::move( cloudIps );
    }
    else {
        auto mdnsIps = getMdnsAddresses( controllerMdnsInfo );
        connectionInfo.ips = std::move( mdnsIps );
        connectionInfo.ips.insert( connectionInfo.ips.end(), cloudIps.begin(), cloudIps.end() );
    }
    return connectionInfo;
}

std::vector< std::string > ClusterManager::getMdnsAddresses( const Json& mdnsInfo ) const {
    std::vector< std::string > result;
    if( mdnsInfo.count( "services" ) == 0 ) {
        return result;
    }
    JsonArray services;
    json_get_arr( mdnsInfo, "services", services );
    for( auto& service : services ) {
        auto address = json_get_str( service, "address" );
        if( !address.empty() ) {
            result.push_back( address );
        }
    }
    return result;
}

bool ClusterManager::isExternalControllerConnected( const std::string& controllerId ) const {
    return m_loggedInExternalControllers.count( controllerId ) != 0;
}

}  // namespace Uid
}  // namespace HubPlatform
