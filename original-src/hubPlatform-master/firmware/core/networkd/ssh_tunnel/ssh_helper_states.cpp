#include "networkd/ssh_tunnel/ssh_helper_states.h"
#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"
#include <tiny/tiny_consolecolors.h>

namespace HubPlatform {
namespace Network {
namespace SshTunnel {

QueueState::QueueState( SshTunnel& sshTunnel )
: m_sshTunnel( sshTunnel ) {}

void QueueState::createTunnel( const std::string& identity, int localPort ) {
    m_sshTunnel.addNewTunnel( identity, localPort, SshTunnel::TunnelState::Stopped );
}

void QueueState::removeTunnel( const std::string& identity ) {
    m_sshTunnel.m_tunnels.erase( identity );
    Log::info( cc::info( "Removed ssh tunnel: " ) + cc::note( identity ) );
}

UpdateMonitorState::UpdateMonitorState( SshTunnel& sshTunnel )
: QueueState( sshTunnel ) {}

void UpdateMonitorState::createTunnel( const std::string& identity, int localPort ) {
    m_updated = true;
    QueueState::createTunnel( identity, localPort );
}

void UpdateMonitorState::removeTunnel( const std::string& identity ) {
    m_updated = true;
    QueueState::removeTunnel( identity );
}

}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
