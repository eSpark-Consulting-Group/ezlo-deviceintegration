#pragma once

#include "system_utils/ssh.h"
#include <string>
#include <vector>

namespace HubPlatform {
namespace Network {
namespace SshTunnel {

using Identity = std::string;

struct IdentityData {
    Identity identity;
    int port;
};
using IdentitiesData = std::vector< IdentityData >;

struct TunnelData {
    Identity identity;
    Ssh::TunnelManager::Data data;
};
using TunnelsData = std::vector< TunnelData >;

enum class Status { Connected, Connecting, Failed, Disconnected, Rejected };
std::string toString( Status status );

}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
