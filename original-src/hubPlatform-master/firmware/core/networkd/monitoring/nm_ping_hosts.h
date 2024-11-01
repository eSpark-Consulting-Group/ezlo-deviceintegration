#pragma once

#include "networkd/monitoring/nm_ip_version.h"
#include "json_utils/json_fwd.h"

#include <string>
#include <vector>

namespace HubPlatform {
namespace Network {

struct PingHost {
    std::string address;
    IPVersion ipVersion = IPVersion::IPv4;

    enum class Location { EXTERNAL, LOCAL };
    Location location = Location::EXTERNAL;
    std::string description;
};

std::vector< PingHost > getPublicPingHosts( const std::string& interface, IPVersion ipVersion );
std::vector< PingHost > getLocalPingHosts( const std::string& interface, IPVersion ipVersion );

std::vector< PingHost > cameraStreamsToPingHosts( std::vector< Json >& streams );
void updateCameraPingHosts( std::vector< PingHost > cameras );

}  // namespace Network
}  // namespace HubPlatform
