#include "networkd/ssh_tunnel/ssh_state_wait_streams.h"
#include "networkd/ssh_tunnel/ssh_state_request_token.h"
#include "networkd/ssh_tunnel/ssh_state_stop_tunnels.h"
#include "networkd/ssh_tunnel/ssh_state_tunnels_created.h"
#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"
#include <tiny/tiny_consolecolors.h>

namespace HubPlatform {
namespace Network {
namespace SshTunnel {

WaitStreamsState::WaitStreamsState( SshTunnel& sshTunnel, const TunnelsData& tunnelsData )
: UpdateMonitorState( sshTunnel )
, m_tunnelsData( tunnelsData ) {}

WaitStreamsState::WaitStreamsState( SshTunnel& sshTunnel, TunnelsData&& tunnelsData )
: UpdateMonitorState( sshTunnel )
, m_tunnelsData( std::move( tunnelsData ) ) {}

void WaitStreamsState::init() {
    Log::info( cc::info( "Waiting for new streams creating..." ) );
    auto timer = m_sshTunnel.createTimer( [this]( Services::PostponedTask& ) {
        if( m_sshTunnel.m_tunnels.empty() ) {
            m_sshTunnel.setStatus( Status::Disconnected );
            setState< TunnelsCreatedState >( m_sshTunnel );
            return;
        }
        if( m_updated ) {
            m_sshTunnel.setStatus( Status::Connecting );
            setState< RequestTokenState >( m_sshTunnel );
            return;
        }
        else {
            setState< StopTunnelState >( m_sshTunnel, std::move( m_tunnelsData ) );
            return;
        }
    } );
    timer->start( std::chrono::seconds( 20 ), false );
}

}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
