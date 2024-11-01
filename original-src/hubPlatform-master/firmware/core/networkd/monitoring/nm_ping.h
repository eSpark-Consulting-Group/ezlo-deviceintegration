#pragma once

#include "networkd/monitoring/nm_ip_version.h"
#include "networkd/monitoring/nm_ping_hosts.h"

#include <string>

namespace HubPlatform {
namespace Network {

class PingStatistics;

bool ping( const std::string& interfaceName,
           const std::vector< PingHost >& hosts,
           bool stopOnFirstHit,
           PingStatistics* statistics );

}  // namespace Network
}  // namespace HubPlatform
