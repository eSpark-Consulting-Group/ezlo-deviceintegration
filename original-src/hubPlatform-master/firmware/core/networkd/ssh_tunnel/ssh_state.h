#pragma once

#include "networkd/ssh_tunnel/ssh_defs.h"

#include <string>
#include <vector>

namespace HubPlatform {
namespace Network {
namespace SshTunnel {

class State {
public:
    virtual ~State() = default;
    virtual void createTunnel( const std::string& streamId, int localPort ) = 0;
    virtual void removeTunnel( const std::string& identity ) = 0;
    virtual void init() = 0;
    virtual void onTokenExpired() = 0;
    virtual void onInvalidTokenFromCloud() = 0;
    virtual void onTokenIsReceived( const TunnelsData& tunnelsData ) = 0;
    virtual void onTunnelWasRemoved( const TunnelData& tunnelData ) = 0;
    virtual void onTunnelWasClosed( const TunnelData& tunnelData ) = 0;
    virtual void onTunnelIsRejectedByCloud( const TunnelData& tunnelData ) = 0;
    virtual void onTunnelIsTimedOut( const TunnelData& tunnelData ) = 0;
    virtual void onTunnelIsClosedByUnknownReason( const TunnelData& tunnelData ) = 0;
    virtual void onIpAddressChanged() = 0;
};

}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
