#pragma once

#include "system_utils/ssh.h"

namespace HubPlatform {
namespace Network {
namespace SshTunnel {

class TunnelManager {
public:
    using Data = Ssh::TunnelManager::Data;
    using OnCloseSshTunnel = Ssh::TunnelManager::OnCloseSshTunnel;

    virtual pid_t createSshTunnel( const Data& data, OnCloseSshTunnel OnCloseSshTunnel ) = 0;
    virtual void killSshTunnel( pid_t processId ) = 0;
};

class TunnelManagerImpl : public SshTunnel::TunnelManager {
public:
    pid_t createSshTunnel( const Data& data, OnCloseSshTunnel OnCloseSshTunnel ) override;
    void killSshTunnel( pid_t processId ) override;

private:
    Ssh::TunnelManager m_tunnelManager;
};

}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
