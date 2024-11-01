#include "uid/cluster_manager/controller_connection.h"
#include "uid/cluster_manager/cluster_manager.h"
#include "uid/ui_service.h"
#include "uid/stat_handlers.h"

namespace HubPlatform {
namespace Uid {

ControllerConnection::ControllerConnection( ClusterManager& clusterManager,
                                            std::string controllerId,
                                            Credentials credentials,
                                            ConnectionInfo connectionInfo,
                                            bool externalControllerConnected )
: m_clusterManager( clusterManager )
, m_controllerId( std::move( controllerId ) )
, m_controllerClient( *this )
, m_credentials( std::move( credentials ) )
, m_connectionInfo( std::move( connectionInfo ) )
, m_currentAddressIdx( -1 )
, m_currentTokenIdx( -1 )
, m_externalControllerConnected( externalControllerConnected )
, m_stopping( false ) {
    std::lock_guard< std::mutex > lock( m_mutex );
    manage();
}

ControllerConnection::~ControllerConnection() {
    m_stopping = true;
    m_controllerClient.onDestruct();
}

const std::string& ControllerConnection::getControllerId() const {
    return m_controllerId;
}

Services::TaskLoop& ControllerConnection::getTaskLoop() {
    return m_clusterManager.getUiService();
}

Uid::Service& ControllerConnection::getUiService() {
    return m_clusterManager.getUiService();
}

void ControllerConnection::onCredentialsUpdated( Credentials credentials ) {
    std::lock_guard< std::mutex > lock( m_mutex );
    Log::debug( cc::debug( "Updating credentials for controller " ) + cc::note( m_controllerId ) +
                cc::debug( " connection" ) );
    m_credentials = std::move( credentials );
    m_currentTokenIdx = -1;
    manage();
}

void ControllerConnection::onCertificatesUpdated() {
    std::lock_guard< std::mutex > lock( m_mutex );
    manage();
}

void ControllerConnection::onConnectionInfoUpdated( ConnectionInfo connectionInfo ) {
    std::lock_guard< std::mutex > lock( m_mutex );
    Log::debug( cc::debug( "Updating connection info for controller " ) + cc::note( m_controllerId ) +
                cc::debug( " connection" ) );
    m_connectionInfo = std::move( connectionInfo );
    m_currentAddressIdx = -1;
    manage();
}

bool ControllerConnection::isExternalControllerLoggingIn() const {
    return m_externalControllerConnected;
}

bool ControllerConnection::onExternalControllerLoggingIn() {
    std::lock_guard< std::mutex > lock( m_mutex );
    if( isConnected() ) {
        return false;
    }
    Log::debug( cc::debug( "Disabling controller " ) + cc::note( m_controllerId ) + cc::debug( " connection" ) );
    m_externalControllerConnected = true;

    getUiService().sendBroadcast( EXTERNAL_CONTROLLERS_CONNECTED,
                                  JSON( "controllerId" << getControllerId() ),
                                  GroupsNames::EXTERNAL_CONTROLLERS );

    // NOTE: do not explicitly stop connection, wait for reconnect
    return true;
}

void ControllerConnection::onExternalControllerLoggedOut() {
    std::lock_guard< std::mutex > lock( m_mutex );
    Log::debug( cc::debug( "Enabling controller " ) + cc::note( m_controllerId ) + cc::debug( " connection" ) );
    m_externalControllerConnected = false;
    manage();
}

bool ControllerConnection::isConnected() const {
    return m_controllerClient.isHubRegistered();
}

bool ControllerConnection::postMessage( const std::string& msg, tiny::ws::client::opcv eOpCode ) {
    if( !isConnected() ) {
        return false;
    }

    return m_controllerClient.postMessage( msg, eOpCode );
}

bool ControllerConnection::enabled() const {
    std::lock_guard< std::mutex > lock( m_mutex );
    return !m_externalControllerConnected && !m_stopping;
}

tiny::ws::security_args ControllerConnection::getSecurityArgs() const {
    return m_clusterManager.getSecurityArgs();
}

std::unique_lock< std::mutex > ControllerConnection::lockCertificateFiles() {
    return m_clusterManager.lockCertificateFiles();
}

void ControllerConnection::manage() {
    if( m_externalControllerConnected ) {
        if( m_controllerClient.isConnected() ) {
            stop();
        }
        return;
    }

    bool canStart = true;
    if( !m_credentials.valid() ) {
        Log::error( cc::error( "Connection to controller " ) + cc::note( m_controllerId ) +
                    cc::error( " is not possible: no valid credentials" ) );
        canStart = false;
    }

    if( !m_clusterManager.getSecurityArgs().valid() ) {
        Log::error( cc::error( "Connection to controller " ) + cc::note( m_controllerId ) +
                    cc::error( " is not possible: no valid certificates" ) );
        canStart = false;
    }

    if( !m_connectionInfo.valid() ) {
        Log::error( cc::error( "Connection to controller " ) + cc::note( m_controllerId ) +
                    cc::error( " is not possible: no valid connection info" ) );
        canStart = false;
    }

    if( m_controllerClient.isConnected() ) {
        if( !canStart ) {
            stop();
        }
        else {
            // Here we may try to reconnect with new data, but for now we will wait for
            // natural disconnect and use new data when reconnecting after that
        }
    }
    else {
        if( canStart ) {
            start();
        }
    }
}

void ControllerConnection::start() {
    Log::info( cc::info( "Establishing connection to controller " ) + cc::note( m_controllerId ) );
    std::thread( [&]() { m_controllerClient.establishConnection(); } ).detach();
}

void ControllerConnection::stop() {
    Log::info( cc::info( "Closing connection to controller " ) + cc::note( m_controllerId ) );
    m_controllerClient.closeConnection();
}

std::string ControllerConnection::getNextAddress() {
    std::lock_guard< std::mutex > lock( m_mutex );

    if( m_currentAddressIdx < m_connectionInfo.ips.size() - 1 ) {
        m_currentAddressIdx++;
    }
    else {
        m_currentAddressIdx = 0;
    }

    return getCurrentAddress();
}

std::string ControllerConnection::getControllerUuid() {
    std::lock_guard< std::mutex > lock( m_mutex );
    return m_credentials.uuid;
}

std::string ControllerConnection::getCurrentAddress() const {
    if( m_currentAddressIdx < 0 || m_currentAddressIdx >= m_connectionInfo.ips.size() )
        return "";

    auto& address = m_connectionInfo.ips[ m_currentAddressIdx ];
    return "wss://" + address + ( address.find( ':' ) == std::string::npos ? ":17000" : "" );
}

std::string ControllerConnection::getNextToken() {
    std::lock_guard< std::mutex > lock( m_mutex );
    if( m_currentTokenIdx < m_credentials.tokens.size() - 1 ) {
        m_currentTokenIdx++;
    }
    else {
        m_currentTokenIdx = 0;
    }

    return getCurrentToken();
}

std::string ControllerConnection::getCurrentToken() const {
    static std::string defaultToken;
    if( m_currentTokenIdx < 0 || m_currentTokenIdx >= m_credentials.tokens.size() )
        return defaultToken;

    return m_credentials.tokens[ m_currentTokenIdx ];
}

}  // namespace Uid
}  // namespace HubPlatform
