#include "networkd/monitoring/nm_ping.h"

#include "networkd/monitoring/nm_ping_parsers.h"
#include "networkd/monitoring/nm_ping_statistics.h"
#include "networkd/monitoring/nm_aux_servers.h"
#include "networkd/resources/nm_resources.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "system_utils/system.h"

#include <chrono>
#include <utility>
#include <ctime>

namespace HubPlatform {
namespace Network {

namespace {
std::string createPingCommand( std::chrono::seconds pingTimeout,
                               const std::string& interfaceName,
                               const std::string& server,
                               IPVersion ipVersion,
                               int packetsCount = 1 ) {
    std::string result = ( ipVersion == IPVersion::IPv4 ) ? "ping " : "ping6 ";
    result += "-W " + std::to_string( pingTimeout.count() ) + " ";

    if( !interfaceName.empty() ) {
        result += "-I " + interfaceName + " ";
    }
    if( packetsCount > 1 ) {
        // -c N: Do up to packetsCount requests to a single server; -A: exit on first success
        result += "-c " + std::to_string( packetsCount ) + " -A ";
    }
    else {
        result += "-c 1 ";
    }

    result += server;

    return result;
}
}  // namespace

bool ping( const std::string& interfaceName,
           const std::vector< PingHost >& hosts,
           bool stopOnFirstHit,
           PingStatistics* statistics ) {
    bool hostsReachable = false;
    for( auto& host : hosts ) {
        const bool isExternal = host.location == PingHost::Location::EXTERNAL;
        const int MultiPacketsCount = 4;
        int packetsCount = 1;
        if( isExternal && statistics && statistics->getPingHealthExternal().isLastPingFailed() )
            packetsCount = MultiPacketsCount;
        auto pingCommand =
            createPingCommand( getPingTimeout(), interfaceName, host.address, host.ipVersion, packetsCount );

        auto result = System::getCommandOutput( pingCommand );
        bool isOk = result.second == 0;

        if( isOk ) {
            LOGGER_TRACE_RAW( "Ping " + host.address + " on " + interfaceName + " success" );
        }
        else {
            LOGGER_ERROR_RAW( "Ping " + host.address + " on " + interfaceName + " fail" );
        }

        if( statistics != nullptr ) {
            PingStatistics::PingStatEntry stats;
            stats.interfaceName = interfaceName;
            stats.ipVersion = host.ipVersion;
            stats.hostAddress = host.address;
            stats.hostLocation = isExternal ? PingStatistics::PingStatEntry::HostLocation::EXTERNAL :
                                              PingStatistics::PingStatEntry::HostLocation::LOCAL;
            stats.hostDescription = host.description;
            stats.pingResult = isOk;
            stats.timestamp = std::time( nullptr );

            auto pingOutput = PingParsers::parsePingOutput( result.first );
            stats.packetsSent = pingOutput.packetsSent;
            stats.packetsReceived = pingOutput.packetsReceived;
            stats.roundTripMin = pingOutput.roundTripMin;
            stats.roundTripAvg = pingOutput.roundTripAvg;
            stats.roundTripMax = pingOutput.roundTripMax;

            statistics->addPingStatEntry( std::move( stats ) );
        }

        hostsReachable |= isOk;
        if( isOk && stopOnFirstHit ) {
            break;
        }
    };

    return hostsReachable;
}

}  // namespace Network
}  // namespace HubPlatform
