#pragma once

#include "uid/cluster_manager/controller_connection.h"
#include "uid/cluster_manager/controllers_discovery_service.h"
#include "uid/server/server_peer.h"

#include "provision_data_utils/provision_data.h"
#include "jsonrpc/message.h"

#include <unordered_set>

namespace HubPlatform {
namespace Uid {

class Service;

class ClusterManager final {
public:
    using ServerPeerPtr = tiny::retain_release_ptr< server_peer, rpc::LegacySynchronizationTraits< server_peer > >;

public:
    explicit ClusterManager( Uid::Service& uiService, const ProvisionDataUtils::ProvisionData& provisionData );
    ~ClusterManager();

    static ClusterManager* getInstance();
    Uid::Service& getUiService();

    void setUp();
    void tearDown();

    bool hasControllerConnection( const std::string& controllerId ) const;

    bool postMessage( const std::string& controllerId,
                      const std::string& msg,
                      tiny::ws::client::opcv eOpCode = tiny::ws::client::opcv::text );
    bool postMessage( const std::string& controllerId, const JsonObject& joMsg );
    void postMessageToAllConnectedControllers( const std::string& msg,
                                               tiny::ws::server::opcv eOpCode = tiny::ws::server::opcv::text );

public:
    void handleBridgeControllerConnectionSet( rpc::Message& rpcMessage );
    void handleBridgeConnectionsList( rpc::Message& rpcMessage );
    void handleBridgeControllersList( rpc::Message& rpcMessage );

    void handleRegisterControllerConnection( const JsonObject& request, const std::string& sender );
    void handleUnregisterControllerConnection( const JsonObject& request, const std::string& sender );

    bool onExternalControllerLoggingIn( const std::string& controllerUuid, ServerPeerPtr peer );
    void onExternalControllerLoggedOut( const std::string& controllerUuid );

    void onCertificatesUpdated();
    void onCredentialsUpdated( const std::string& controllerId );
    void onMdnsInfoUpdated( const Json& info );

    tiny::ws::security_args getSecurityArgs() const;
    std::unique_lock< std::mutex > lockCertificateFiles();

private:
    void initControllerConnection( const std::string& controllerId );
    void resetControllerConnection( const std::string& controllerId );

    ControllerConnection::Credentials readCredentials( const std::string& controllerId ) const;
    ControllerConnection::ConnectionInfo readConnectionInfo( const std::string& controllerId,
                                                             const Json& mdnsInfo = JSON_EMPTY_OBJ ) const;
    std::vector< std::string > getMdnsAddresses( const Json& mdnsInfo ) const;

    bool isExternalControllerConnected( const std::string& controllerId ) const;
    void updateCertificates();

    bool postMessageThroughServerConnection( const std::string& controllerId,
                                             const std::string& msg,
                                             tiny::ws::client::opcv eOpCode );
    bool postMessageThroughClientConnection( const std::string& controllerId,
                                             const std::string& msg,
                                             tiny::ws::client::opcv eOpCode );

private:
    static ClusterManager* m_instance;

private:
    Uid::Service& m_uiService;
    const ProvisionDataUtils::ProvisionData& m_provisionData;
    std::unique_ptr< ControllersDiscoveryService > m_discoveryService;
    mutable std::mutex m_certificatesMutex;
    tiny::ws::security_args m_securityArgs;

    std::unordered_map< std::string, std::unique_ptr< ControllerConnection > > m_controllerConnections;

    std::unordered_map< std::string, std::string > m_idsByUuidsCache;
    std::unordered_map< std::string, ServerPeerPtr > m_loggedInExternalControllers;
};

}  // namespace Uid
}  // namespace HubPlatform
