#include "networkd/ssh_tunnel/ssh_state_start_tunnels.h"
#include "networkd/ssh_tunnel/ssh_state_tunnels_created.h"
#include "networkd/ssh_tunnel/ssh_state_wait_timeout.h"
#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"
#include <tiny/tiny_consolecolors.h>

namespace HubPlatform {
namespace Network {
namespace SshTunnel {

StartTunnelsState::StartTunnelsState( SshTunnel& sshTunnel, const TunnelsData& tunnelsData )
: QueueState( sshTunnel )
, m_tunnelsData( tunnelsData ) {}

StartTunnelsState::StartTunnelsState( SshTunnel& sshTunnel, TunnelsData&& tunnelsData )
: QueueState( sshTunnel )
, m_tunnelsData( std::move( tunnelsData ) ) {}

void StartTunnelsState::init() {
    Log::info( cc::info( "Tunnels starting..." ) );

    if( m_tunnelsData.empty() || !m_sshTunnel.tunnelEnabled() ) {
        Log::info( cc::info( "No tunnels are started" ) );
        m_sshTunnel.setStatus( Status::Disconnected );
        setState< TunnelsCreatedState >( m_sshTunnel );
        return;
    }

    for( auto& tunnel : m_tunnelsData ) {
        if( tunnel.data.logFilename.empty() ) {
            tunnel.data.logFilename = m_sshTunnel.logFileName( tunnel );
        }
        m_sshTunnel.startTunnel( tunnel );
    }
    setState< TunnelsCreatedState >( m_sshTunnel );
}

void StartTunnelsState::onTunnelIsRejectedByCloud( const TunnelData& ) {
    setState< WaitTimeoutState >( m_sshTunnel );
}

void StartTunnelsState::onTunnelIsTimedOut( const TunnelData& ) {
    setState< WaitTimeoutState >( m_sshTunnel );
}

void StartTunnelsState::onTunnelIsClosedByUnknownReason( const TunnelData& ) {
    setState< WaitTimeoutState >( m_sshTunnel );
}

}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
