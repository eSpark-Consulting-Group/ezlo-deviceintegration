#pragma once

#include "networkd/ssh_tunnel/ssh_helper_states.h"

namespace HubPlatform {
namespace Network {
namespace SshTunnel {

class WaitStreamsState : public UpdateMonitorState {
public:
    WaitStreamsState( SshTunnel& sshTunnel, const TunnelsData& tunnelsData );
    WaitStreamsState( SshTunnel& sshTunnel, TunnelsData&& tunnelsData );
    void init() override;

private:
    TunnelsData m_tunnelsData;
};

}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
