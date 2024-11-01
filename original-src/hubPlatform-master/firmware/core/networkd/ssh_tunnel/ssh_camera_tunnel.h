#pragma once

#include "networkd/ssh_tunnel/ssh_tunnel.h"

namespace HubPlatform {
namespace Network {
namespace SshTunnel {

class SshCameraTunnel : public SshTunnel {
public:
    SshCameraTunnel( Services::Service& service, TunnelManager& tunnelManager );
    ~SshCameraTunnel() = default;

    void createCameraTunnel( const std::string& streamId, int localPort );
    bool tunnelEnabled() override;
};

}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
