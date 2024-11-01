#include "networkd/ssh_tunnel/ssh_state_init.h"
#include "networkd/ssh_tunnel/ssh_state_wait_streams.h"
#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"
#include <tiny/tiny_consolecolors.h>

namespace HubPlatform {
namespace Network {
namespace SshTunnel {

InitState::InitState( SshTunnel& sshTunnel )
: QueueState( sshTunnel ) {}

void InitState::init() {
    Log::info( cc::info( "Tunnels initializing..." ) );

    auto tunnelsData = m_sshTunnel.dbGetTunnelsData();
    for( const auto& tunnel : tunnelsData ) {
        m_sshTunnel.m_tunnels.emplace( tunnel.identity,
                                       SshTunnel::Tunnel( m_sshTunnel.m_tunnelManager,
                                                          m_sshTunnel.m_dbClient,
                                                          tunnel.identity,
                                                          SshTunnel::TunnelState::Stopped,
                                                          tunnel.data.ownPort ) );
    }
    setState< WaitStreamsState >( m_sshTunnel, std::move( tunnelsData ) );
}

}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
