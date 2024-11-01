#pragma once

#include "networkd/ssh_tunnel/ssh_helper_states.h"

namespace HubPlatform {
namespace Network {
namespace SshTunnel {

class TunnelsCreatedState : public QueueState {
public:
    TunnelsCreatedState( SshTunnel& sshTunnel );
    void init() override;
    void createTunnel( const std::string& identity, int localPort ) override;
    void removeTunnel( const std::string& identity ) override;
    void onTunnelIsRejectedByCloud( const TunnelData& tunnelData ) override;
    void onTunnelIsTimedOut( const TunnelData& tunnelData ) override;
    void onTunnelIsClosedByUnknownReason( const TunnelData& tunnelData ) override;
    void onTokenExpired() override;
    void onIpAddressChanged() override;
};
}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
