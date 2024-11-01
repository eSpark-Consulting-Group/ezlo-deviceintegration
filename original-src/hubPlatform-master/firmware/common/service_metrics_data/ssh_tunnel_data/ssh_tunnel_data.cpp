#include "service_metrics_data/ssh_tunnel_data/ssh_tunnel_data.h"

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace ServiceMetricsData {

namespace {
namespace Fields {

const char* const Pid = "pid";
const char* const Id = "id";
const char* const SshTunnelType = "ssh_tunnel_type";
const char* const SshTunnelStatus = "ssh_tunnel_status";
const char* const TcpState = "tcp_state";
const char* const ServerURL = "server_url";
const char* const ServerPort = "server_port";
const char* const RemotePort = "remote_port";
const char* const LocalPort = "local_port";
const char* const StartTime = "start_time";
const char* const Uptime = "uptime";
const char* const ConnectionStatistics = "connection_statistics";

}  // namespace Fields

const std::map< SshTunnelData::SshTunnelType, std::string >& tunnelTypeMap() {
    static const std::map< SshTunnelData::SshTunnelType, std::string > map = {
        { SshTunnelData::SshTunnelType::TYPE_UNKNOWN, "unknown" },
        { SshTunnelData::SshTunnelType::TYPE_HTTP_SERVER, "http_server" },
        { SshTunnelData::SshTunnelType::TYPE_CAMERA, "camera" },
        { SshTunnelData::SshTunnelType::TYPE_SUPPORT, "support" },
    };
    return map;
}

const std::map< SshTunnelData::SshTunnelStatus, std::string >& tunnelStatusMap() {
    static const std::map< SshTunnelData::SshTunnelStatus, std::string > map = {
        { SshTunnelData::SshTunnelStatus::STATUS_UNKNOWN, "unknown" },
        { SshTunnelData::SshTunnelStatus::STATUS_OPEN, "open" },
        { SshTunnelData::SshTunnelStatus::STATUS_BROKEN, "broken" },
        { SshTunnelData::SshTunnelStatus::STATUS_CLOSED, "closed" },
    };
    return map;
}

const std::map< SshTunnelData::SshTunnelConnectionStatus, std::string >& tunnelConnectionStatusMap() {
    static const std::map< SshTunnelData::SshTunnelConnectionStatus, std::string > map = {
        { SshTunnelData::SshTunnelConnectionStatus::STATUS_UNKNOWN, "unknown" },
        { SshTunnelData::SshTunnelConnectionStatus::STATUS_CONNECTED, "connected" },
        { SshTunnelData::SshTunnelConnectionStatus::STATUS_CONNECTION_REFUSED, "connection_refused" },
    };
    return map;
}

const std::map< std::string, SshTunnelData::SshTunnelConnectionStatus >& reverseTunnelConnectionStatusMap() {
    static const auto map = []() {
        std::map< std::string, SshTunnelData::SshTunnelConnectionStatus > reverseMap;
        for( auto& element : tunnelConnectionStatusMap() ) {
            reverseMap.emplace( element.second, element.first );
        }
        return reverseMap;
    }();
    return map;
}

}  // namespace

Json SshTunnelData::toJson() {
    JsonObject j;
    json_add_int( j, Fields::Pid, pid );
    json_add_str( j, Fields::Id, id );
    json_add_str( j, Fields::SshTunnelType, tunnelTypeMap().at( sshTunnelType ) );
    json_add_str( j, Fields::SshTunnelStatus, tunnelStatusMap().at( sshTunnelStatus ) );

    json_add_str( j, Fields::TcpState, tcpState );
    json_add_str( j, Fields::ServerURL, serverUrl );
    json_add_int( j, Fields::ServerPort, serverPort );
    json_add_int( j, Fields::RemotePort, remotePort );
    json_add_int( j, Fields::LocalPort, localPort );
    json_add_int( j, Fields::StartTime, startTime );
    json_add_int( j, Fields::Uptime, uptime );

    auto statistics = JSON_EMPTY_OBJ;
    for( auto& element : connectionStatistics ) {
        statistics[ tunnelConnectionStatusMap().at( element.first ) ] = element.second;
    }
    j[ Fields::ConnectionStatistics ] = std::move( statistics );

    return j;
}

SshTunnelData SshTunnelData::fromJson( const Json& json ) {
    if( json.empty() ) {
        return {};
    }

    SshTunnelData data;
    data.pid = json_get_int( json, Fields::Pid, int( -1 ) );
    data.id = json_get_str( json, Fields::Id );

    auto tunnelType = json_get_str( json, Fields::SshTunnelType );
    for( auto& element : tunnelTypeMap() ) {
        if( tunnelType == element.second ) {
            data.sshTunnelType = element.first;
            break;
        }
    }

    auto tunnelStatus = json_get_str( json, Fields::SshTunnelStatus );
    for( auto& element : tunnelStatusMap() ) {
        if( tunnelStatus == element.second ) {
            data.sshTunnelStatus = element.first;
            break;
        }
    }

    data.tcpState = json_get_str( json, Fields::TcpState );
    data.serverUrl = json_get_str( json, Fields::ServerURL );
    data.serverPort = json_get_int( json, Fields::ServerPort, data.serverPort );
    data.remotePort = json_get_int( json, Fields::RemotePort, data.remotePort );
    data.localPort = json_get_int( json, Fields::LocalPort, data.localPort );
    data.startTime = json_get_int( json, Fields::StartTime, data.startTime );
    data.uptime = json_get_int( json, Fields::Uptime, data.uptime );

    auto statisticsJson = JSON_EMPTY_OBJ;
    if( json_get_obj( json, Fields::ConnectionStatistics, statisticsJson ) ) {
        for( auto it = statisticsJson.begin(); it != statisticsJson.end(); ++it ) {
            const std::string& connectionStatus = it.key();
            int count = it.value();
            data.connectionStatistics[ reverseTunnelConnectionStatusMap().at( connectionStatus ) ] = count;
        }
    }

    return data;
}

}  // namespace ServiceMetricsData
}  // namespace HubPlatform
