#include "networkd/ssh_tunnel/ssh_tunnel_manager.h"

namespace HubPlatform {
namespace Network {
namespace SshTunnel {

pid_t TunnelManagerImpl::createSshTunnel( const TunnelManager::Data& data, OnCloseSshTunnel OnCloseSshTunnel ) {
    return m_tunnelManager.createSshTunnel( data, std::move( OnCloseSshTunnel ) );
}

void TunnelManagerImpl::killSshTunnel( pid_t processId ) {
    m_tunnelManager.killSshTunnel( processId );
}

}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
