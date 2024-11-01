#include "networkd/ssh_tunnel/ssh_state_tunnels_created.h"
#include "networkd/ssh_tunnel/ssh_state_wait_timeout.h"
#include "networkd/ssh_tunnel/ssh_state_request_token.h"
#include "networkd/ssh_tunnel/ssh_state_stop_tunnels.h"
#include "networkd/ssh_tunnel/ssh_state_start_tunnels.h"
#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"
#include <tiny/tiny_consolecolors.h>

namespace HubPlatform {
namespace Network {
namespace SshTunnel {

TunnelsCreatedState::TunnelsCreatedState( SshTunnel& sshTunnel )
: QueueState( sshTunnel ) {}

void TunnelsCreatedState::init() {
    Log::info( cc::info( "Tunnels monitoring..." ) );
}

void TunnelsCreatedState::createTunnel( const std::string& identity, int localPort ) {
    TunnelData data;
    SshTunnel::TunnelState state;
    if( m_sshTunnel.restoreTunnelFromCache( identity, data ) ) {
        setState< StartTunnelsState >( m_sshTunnel, std::vector< TunnelData >( { std::move( data ) } ) );
    }
    else if( m_sshTunnel.getTunnelData( identity, data, state ) ) {
        if( SshTunnel::TunnelState::Running == state ) {
            Log::info( cc::info( "Ssh tunnel is already running: " ) + cc::note( identity ) );
        }
        else if( SshTunnel::TunnelState::Stopped == state ) {
            setState< StartTunnelsState >( m_sshTunnel, std::vector< TunnelData >( { std::move( data ) } ) );
        }
    }
    else {
        QueueState::createTunnel( identity, localPort );
        setState< RequestTokenState >( m_sshTunnel );
    }
}

void TunnelsCreatedState::removeTunnel( const std::string& identity ) {
    m_sshTunnel.stopAndCacheTunnel( identity );
}

void TunnelsCreatedState::onTunnelIsRejectedByCloud( const TunnelData& ) {
    setState< WaitTimeoutState >( m_sshTunnel );
}

void TunnelsCreatedState::onTunnelIsTimedOut( const TunnelData& ) {
    setState< WaitTimeoutState >( m_sshTunnel );
}

void TunnelsCreatedState::onTunnelIsClosedByUnknownReason( const TunnelData& ) {
    setState< WaitTimeoutState >( m_sshTunnel );
}

void TunnelsCreatedState::onTokenExpired() {
    setState< RequestTokenState >( m_sshTunnel );
}

void TunnelsCreatedState::onIpAddressChanged() {
    setState< RequestTokenState >( m_sshTunnel );
}

}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
