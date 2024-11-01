#include "networkd/monitoring/nm_ping_hosts.h"

#include "networkd/monitoring/nm_aux_servers.h"
#include "networkd/utils/nm_utils.h"
#include "networkd/resources/nm_resources.h"
#include "json_utils/json_utils.h"
#include "base_utils/string_utils.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "system_utils/system.h"

#include <mutex>

namespace HubPlatform {
namespace Network {

namespace {
// TODO: read from config
const std::vector< std::string > DefaultPublicServers = { "1.1.1.1", "8.8.8.8" };
const std::vector< std::string > DefaultIPv6PublicServers = { "2606:4700:4700::1111", "2001:4860:4860::8888" };

std::mutex& getHostsMutex() {
    static std::mutex mutex;
    return mutex;
}

std::vector< PingHost >& camerasToPing() {
    static std::vector< PingHost > cameras;
    return cameras;
}

}  // namespace

std::vector< PingHost > getPublicPingHosts( const std::string& interface, IPVersion ipVersion ) {
    auto& publicServers = ipVersion == ( IPVersion::IPv4 ) ? DefaultPublicServers : DefaultIPv6PublicServers;
    auto additionalServers = getAdditionalServers();

    std::vector< PingHost > pingHosts;
    pingHosts.reserve( publicServers.size() + additionalServers.size() );

    for( auto& address : publicServers ) {
        pingHosts.emplace_back();
        auto& host = pingHosts.back();
        host.address = address;
        host.ipVersion = ipVersion;
        host.location = PingHost::Location::EXTERNAL;
        host.description = "dns";
    }

    for( auto& address : additionalServers ) {
        pingHosts.emplace_back();
        auto& host = pingHosts.back();
        host.address = address;
        host.ipVersion = ipVersion;
        host.location = PingHost::Location::EXTERNAL;
    }

    return pingHosts;
}

std::vector< PingHost > getLocalPingHosts( const std::string& interface, IPVersion ipVersion ) {
    // GSM does not have "local" hosts
    if( interface != Resources::EthernetDeviceName && interface != Resources::ClientWifiDeviceName ) {
        return {};
    }

    std::vector< PingHost > pingHosts;
    if( ipVersion == IPVersion::IPv4 ) {
        auto gateway = Utils::getDefaultGatewayIPv4( interface );
        if( !gateway.empty() ) {
            pingHosts.emplace_back();
            auto& host = pingHosts.back();
            host.address = gateway;
            host.location = PingHost::Location::LOCAL;
            host.description = "gateway";
        }
    }

    std::lock_guard< std::mutex > lock( getHostsMutex() );
    pingHosts.insert( pingHosts.end(), camerasToPing().begin(), camerasToPing().end() );

    return pingHosts;
}

std::vector< PingHost > cameraStreamsToPingHosts( std::vector< Json >& streams ) {
    std::vector< PingHost > hosts;
    hosts.reserve( streams.size() );

    for( auto& element : streams ) {
        if( !element.is_object() ) {
            LOGGER_WARN( cc::warn( "Invalid camera stream array" ) );
            return {};
        }

        auto streamId = json_get_str( element, "streamId" );
        if( streamId.empty() ) {
            LOGGER_WARN( cc::warn( "Empty camera id" ) );
            continue;
        }

        auto rtspUrl = json_get_str( element, "rtspUrl" );
        if( rtspUrl.empty() ) {
            continue;
        }

        // URL example: rtsp://admin:1f59600011230f4c9310794417c57a5e807f5d94@192.168.88.18:88/videoMain
        auto tokens = StringUtils::split( rtspUrl, "/" );
        if( tokens.size() < 4 || tokens.at( 0 ) != "rtsp:" ) {
            LOGGER_WARN( cc::warn( "Invalid camera RTSP url format" ) );
            continue;
        }

        auto url = StringUtils::split( tokens.at( 2 ), "@" ).back();
        auto cameraIp = url.substr( 0, url.rfind( ":" ) );

        if( cameraIp.empty() ) {
            LOGGER_WARN( cc::warn( "Invalid camera RTSP url: missing IP" ) );
            continue;
        }

        hosts.emplace_back();
        auto& host = hosts.back();

        host.address = cameraIp;
        host.ipVersion = IPVersion::IPv4;
        host.location = PingHost::Location::LOCAL;
        host.description = "stream_" + streamId;
    }

    return hosts;
}

void updateCameraPingHosts( std::vector< PingHost > cameras ) {
    std::lock_guard< std::mutex > lock( getHostsMutex() );
    camerasToPing() = std::move( cameras );
}

}  // namespace Network
}  // namespace HubPlatform
