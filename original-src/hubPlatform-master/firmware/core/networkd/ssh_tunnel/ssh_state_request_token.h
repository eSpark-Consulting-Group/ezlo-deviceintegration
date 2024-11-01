#pragma once

#include "networkd/ssh_tunnel/ssh_helper_states.h"

namespace HubPlatform {
namespace Network {
namespace SshTunnel {

class RequestTokenState : public UpdateMonitorState {
public:
    RequestTokenState( SshTunnel& sshTunnel );
    void init() override;
    void onInvalidTokenFromCloud() override;
    void onTokenIsReceived( const TunnelsData& tunnelsData ) override;
};

}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
