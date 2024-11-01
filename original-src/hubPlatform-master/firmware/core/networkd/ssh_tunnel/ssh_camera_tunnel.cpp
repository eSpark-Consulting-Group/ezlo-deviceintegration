#include "networkd/ssh_tunnel/ssh_camera_tunnel.h"

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace Network {
namespace SshTunnel {

SshCameraTunnel::SshCameraTunnel( Services::Service& service, TunnelManager& tunnelManager )
: SshTunnel( service, tunnelManager, "cloud.tunnel_camera_coordinator" ) {}

void SshCameraTunnel::createCameraTunnel( const std::string& streamId, int localPort ) {
    Log::info( cc::info( "Creating ssh tunnel for camera: " ) + cc::note( streamId ) );
    setStatus( Status::Connecting );
    if( m_state ) {
        m_state->createTunnel( streamId, localPort );
    }
}

bool SshCameraTunnel::tunnelEnabled() {
    return true;
}

}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
