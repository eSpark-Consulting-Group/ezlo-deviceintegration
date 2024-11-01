#pragma once

#include "networkd/ssh_tunnel/ssh_helper_states.h"

namespace HubPlatform {
namespace Network {
namespace SshTunnel {

class StopTunnelState : public UpdateMonitorState {
public:
    StopTunnelState( SshTunnel& sshTunnel, const TunnelsData& tunnelsData );
    StopTunnelState( SshTunnel& sshTunnel, TunnelsData&& tunnelsData );
    void init() override;
    void removeTunnel( const std::string& identity ) override;
    void onTunnelWasClosed( const TunnelData& ) override;

private:
    void setNextState();

private:
    TunnelsData m_tunnelsData;
    std::set< Identity > m_runningTunnels;
    Services::PostponedTaskPtr m_waitStopTimer;

    constexpr static std::chrono::seconds WaitTimeout = std::chrono::seconds( 10 );
};

}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
