#include "networkd/ssh_tunnel/ssh_state_request_token.h"
#include "networkd/ssh_tunnel/ssh_state_wait_timeout.h"
#include "networkd/ssh_tunnel/ssh_state_stop_tunnels.h"
#include "networkd/ssh_tunnel/ssh_state_tunnels_created.h"
#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"
#include <tiny/tiny_consolecolors.h>

namespace HubPlatform {
namespace Network {
namespace SshTunnel {

RequestTokenState::RequestTokenState( SshTunnel& sshTunnel )
: UpdateMonitorState( sshTunnel ) {}

void RequestTokenState::init() {
    Log::info( cc::info( "Requesting token..." ) );
    m_sshTunnel.setStatus( Status::Connecting );

    if( m_sshTunnel.m_expirationTimer ) {
        m_sshTunnel.m_expirationTimer->stop();
    }
    IdentitiesData identities;
    for( const auto& tunnel : m_sshTunnel.m_tunnels ) {
        identities.push_back( { tunnel.first, tunnel.second.port } );
    }
    if( identities.empty() ) {
        m_sshTunnel.setStatus( Status::Disconnected );
        m_sshTunnel.m_dbClient.remove( TempNetworkCloudSshTunnelsDBName, JSON( "_id" << m_sshTunnel.m_method ) );
        setState< TunnelsCreatedState >( m_sshTunnel );
    }
    else {
        m_sshTunnel.setStatus( Status::Connecting );
        m_sshTunnel.requestTunnel( m_sshTunnel.m_method, identities );
    }
}

void RequestTokenState::onInvalidTokenFromCloud() {
    m_sshTunnel.setState( std::make_unique< WaitTimeoutState >( m_sshTunnel ) );
}

void RequestTokenState::onTokenIsReceived( const TunnelsData& tunnelsData ) {
    m_sshTunnel.setStatus( Status::Connecting );
    if( m_updated ) {
        setState< RequestTokenState >( m_sshTunnel );
    }
    else {
        setState< StopTunnelState >( m_sshTunnel, tunnelsData );
    }
}

}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
