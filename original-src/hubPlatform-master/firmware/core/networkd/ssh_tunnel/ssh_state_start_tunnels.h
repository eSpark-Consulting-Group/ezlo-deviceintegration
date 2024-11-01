#pragma once

#include "networkd/ssh_tunnel/ssh_helper_states.h"

namespace HubPlatform {
namespace Network {
namespace SshTunnel {

class StartTunnelsState : public QueueState {
public:
    StartTunnelsState( SshTunnel& sshTunnel, const TunnelsData& tunnelsData );
    StartTunnelsState( SshTunnel& sshTunnel, TunnelsData&& tunnelsData );
    void init() override;
    void onTunnelIsRejectedByCloud( const TunnelData& tunnelData ) override;
    void onTunnelIsTimedOut( const TunnelData& tunnelData ) override;
    void onTunnelIsClosedByUnknownReason( const TunnelData& tunnelData ) override;

private:
    TunnelsData m_tunnelsData;
};

}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
