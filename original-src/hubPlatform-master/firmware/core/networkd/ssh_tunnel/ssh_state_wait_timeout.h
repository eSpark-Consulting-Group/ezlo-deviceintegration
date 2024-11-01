#pragma once

#include "networkd/ssh_tunnel/ssh_helper_states.h"

namespace HubPlatform {
namespace Network {
namespace SshTunnel {

class WaitTimeoutState : public QueueState {
public:
    WaitTimeoutState( SshTunnel& sshTunnel );
    void init() override;
};

}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
