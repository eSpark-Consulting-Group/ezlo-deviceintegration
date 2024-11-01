#pragma once

#include "json_utils/json_fwd.h"

#include <string>
#include <map>
#include <cstdint>

namespace HubPlatform {
namespace ServiceMetricsData {

struct SshTunnelData {
    enum class SshTunnelType {
        TYPE_UNKNOWN,
        TYPE_HTTP_SERVER,
        TYPE_CAMERA,
        TYPE_SUPPORT,
    };

    enum class SshTunnelStatus {
        STATUS_UNKNOWN,
        STATUS_OPEN,
        STATUS_BROKEN,
        STATUS_CLOSED,
    };

    enum class SshTunnelConnectionStatus {
        STATUS_UNKNOWN,
        STATUS_CONNECTED,
        STATUS_CONNECTION_REFUSED,
    };

    int32_t pid = -1;
    std::string id;
    SshTunnelType sshTunnelType{};
    SshTunnelStatus sshTunnelStatus{};
    std::string tcpState;
    std::string serverUrl;
    int32_t serverPort{};
    int32_t remotePort{};
    int32_t localPort{};
    uint32_t startTime{};
    uint32_t uptime{};
    std::map< SshTunnelConnectionStatus, size_t > connectionStatistics;

    Json toJson();
    static SshTunnelData fromJson( const Json& json );
};

}  // namespace ServiceMetricsData
}  // namespace HubPlatform
