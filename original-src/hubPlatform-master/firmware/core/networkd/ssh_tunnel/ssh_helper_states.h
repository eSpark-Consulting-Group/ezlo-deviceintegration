#pragma once

#include "networkd/ssh_tunnel/ssh_state.h"
#include "networkd/ssh_tunnel/ssh_tunnel.h"

#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace Network {
namespace SshTunnel {

class QueueState : public State {
public:
    QueueState( SshTunnel& sshTunnel );
    virtual ~QueueState() = default;
    void createTunnel( const std::string& identity, int localPort ) override;
    void removeTunnel( const std::string& identity ) override;
    void onTokenExpired() override {}
    void onInvalidTokenFromCloud() override {}
    void onTokenIsReceived( const TunnelsData& ) override {}
    void onTunnelWasRemoved( const TunnelData& ) override {}
    void onTunnelWasClosed( const TunnelData& ) override {}
    void onTunnelIsRejectedByCloud( const TunnelData& ) override {}
    void onTunnelIsTimedOut( const TunnelData& ) override {}
    void onTunnelIsClosedByUnknownReason( const TunnelData& ) override {}
    void onIpAddressChanged() override {}

protected:
    template < typename T, typename... Args >
    void setState( Args&&... args ) {
        m_sshTunnel.setState( std::make_unique< T >( std::forward< Args >( args )... ) );
    }

    SshTunnel& m_sshTunnel;
};

class UpdateMonitorState : public QueueState {
public:
    UpdateMonitorState( SshTunnel& sshTunnel );
    virtual ~UpdateMonitorState() = default;
    void createTunnel( const std::string& identity, int localPort ) override;
    void removeTunnel( const std::string& identity ) override;

protected:
    bool m_updated = false;
};

}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
