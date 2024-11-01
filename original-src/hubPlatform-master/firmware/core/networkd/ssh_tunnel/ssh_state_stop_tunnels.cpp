#include "networkd/ssh_tunnel/ssh_state_stop_tunnels.h"
#include "networkd/ssh_tunnel/ssh_state_start_tunnels.h"
#include "networkd/ssh_tunnel/ssh_state_request_token.h"
#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"
#include <tiny/tiny_consolecolors.h>

namespace HubPlatform {
namespace Network {
namespace SshTunnel {

constexpr std::chrono::seconds StopTunnelState::WaitTimeout;

StopTunnelState::StopTunnelState( SshTunnel& sshTunnel, const TunnelsData& tunnelsData )
: UpdateMonitorState( sshTunnel )
, m_tunnelsData( tunnelsData ) {}

StopTunnelState::StopTunnelState( SshTunnel& sshTunnel, TunnelsData&& tunnelsData )
: UpdateMonitorState( sshTunnel )
, m_tunnelsData( std::move( tunnelsData ) ) {}

void StopTunnelState::init() {
    // find all running tunnels
    for( const auto& tunnelIt : m_sshTunnel.m_tunnels ) {
        auto& tunnel = tunnelIt.second;
        if( tunnel.state == SshTunnel::TunnelState::Running ) {
            m_runningTunnels.insert( tunnelIt.first );
        }
    }
    if( m_runningTunnels.empty() ) {
        Log::info( cc::info( "No tunnels for stopping" ) );
        m_sshTunnel.setStatus( Status::Disconnected );
        setNextState();
    }
    else {
        Log::info( cc::info( "Tunnels stopping..." ) );
        m_waitStopTimer = m_sshTunnel.createTimer( [this]( Services::PostponedTask& ) {
            Log::warn( cc::warn( "Tunnels stopping timeout" ) );
            setNextState();
        } );
        m_waitStopTimer->start( WaitTimeout, false );
        m_sshTunnel.stopAllTunnels();
        m_sshTunnel.setStatus( Status::Disconnected );
    }
}

void StopTunnelState::removeTunnel( const std::string& identity ) {
    UpdateMonitorState::removeTunnel( identity );
    m_runningTunnels.erase( identity );
    if( m_runningTunnels.empty() ) {
        m_waitStopTimer->stop();
        setNextState();
    }
}

void StopTunnelState::onTunnelWasClosed( const TunnelData& tunnelData ) {
    m_runningTunnels.erase( tunnelData.identity );
    if( m_runningTunnels.empty() ) {
        m_waitStopTimer->stop();
        setNextState();
    }
}

void StopTunnelState::setNextState() {
    if( m_updated ) {
        setState< RequestTokenState >( m_sshTunnel );
    }
    else {
        setState< StartTunnelsState >( m_sshTunnel, std::move( m_tunnelsData ) );
    }
}

}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
