#include "networkd/monitoring/nm_ping_statistics.h"
#include "networkd/monitoring/nm_aux_servers.h"
#include "networkd/resources/nm_resources.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "system_utils/system.h"

#include <chrono>
#include <utility>
#include <sstream>

namespace HubPlatform {
namespace Network {

static void fillGenericStatsData( const PingStatistics::PingHealth& pingHealth,
                                  PingStatistics::PingStatisticsData::GenericStats& stats ) {
    stats.hits = pingHealth.hits;
    stats.fails = pingHealth.fails;
    stats.sequentialFails = pingHealth.sequentialFails;
    stats.packetsSent = pingHealth.packetsSent;
    stats.packetsReceived = pingHealth.packetsReceived;
    stats.packetsLost = pingHealth.packetsLost;
}

static std::string percentage( double portion, double total ) {
    if( total == 0.0 )
        return "0.0";

    std::ostringstream stream;
    stream << std::fixed << std::setprecision( 1 ) << portion / total * 100;
    return stream.str();
}

void PingStatistics::addPingStatEntry( PingStatEntry entry ) {
    if( m_interfaceName.empty() ) {
        m_interfaceName = entry.interfaceName;
    }
    else if( m_interfaceName != entry.interfaceName ) {
        LOGGER_ERROR( cc::error( "Wrong interface name provided: " ) + cc::note( entry.interfaceName ) +
                      cc::error( ", expected interface: " ) + cc::note( m_interfaceName ) );
        return;
    }

    auto& pingHealth =
        ( entry.hostLocation == PingStatEntry::HostLocation::EXTERNAL ) ? m_externalHealth : m_localHealth;

    pingHealth.lastPingResult = entry.pingResult;

    ++pingHealth.hits;
    pingHealth.packetsSent += entry.packetsSent;
    pingHealth.packetsReceived += entry.packetsReceived;
    pingHealth.packetsLost = pingHealth.packetsSent - pingHealth.packetsReceived;

    if( entry.pingResult ) {
        pingHealth.sequentialFails = 0;
    }
    else {
        ++pingHealth.fails;
        ++pingHealth.sequentialFails;
    }

    m_pingStatsList.push_back( std::move( entry ) );
    while( m_pingStatsList.size() > PingStatLimit ) {
        m_pingStatsList.pop_front();
    }
}

PingStatistics::PingStatisticsData PingStatistics::getPingStatisticsData() const {
    PingStatisticsData data;

    data.interfaceName = m_interfaceName;

    fillGenericStatsData( m_externalHealth, data.externalStats );
    fillGenericStatsData( m_localHealth, data.localStats );

    using HostPingStatsList = std::vector< const PingStatEntry* >;
    std::map< std::string, HostPingStatsList > hostsMap;

    for( auto& element : m_pingStatsList ) {
        hostsMap[ element.hostAddress ].emplace_back( &element );
    }

    for( auto& hostMapElement : hostsMap ) {
        auto& hostUrl = hostMapElement.first;
        auto& hostStatsList = hostMapElement.second;

        PingStatisticsData::HostStats hostData;

        hostData.hostUrl = hostUrl;
        hostData.hostLocation = hostStatsList.front()->hostLocation;
        hostData.timestampMin = hostStatsList.front()->timestamp;
        hostData.timestampMax = hostStatsList.back()->timestamp;

        size_t roundTripCount = 0;

        for( auto& hostStats : hostStatsList ) {
            hostData.hostDescription = hostStats->hostDescription;
            ++hostData.hits;

            if( hostStats->pingResult ) {
                if( hostData.roundTripMin > hostStats->roundTripMin || !roundTripCount )
                    hostData.roundTripMin = hostStats->roundTripMin;

                if( hostData.roundTripMax < hostStats->roundTripMax )
                    hostData.roundTripMax = hostStats->roundTripMax;

                hostData.roundTripAvg += hostStats->roundTripAvg;

                ++roundTripCount;
            }
            else {
                ++hostData.fails;
            }

            hostData.packetsSent += hostStats->packetsSent;
            hostData.packetsReceived += hostStats->packetsReceived;
        }

        hostData.packetsLost = hostData.packetsSent - hostData.packetsReceived;

        if( roundTripCount ) {
            hostData.roundTripAvg /= roundTripCount;
        }

        data.hosts.emplace_back( std::move( hostData ) );
    }

    return data;
}

const PingStatistics::PingHealth& PingStatistics::getPingHealthExternal() {
    return m_externalHealth;
}

const PingStatistics::PingHealth& PingStatistics::getPingHealthLocal() {
    return m_localHealth;
}

void PingStatistics::print() const {
    using namespace Resources;

    auto data = getPingStatisticsData();

    LOGGER_INFO_RAW(
        cc::info( "Ping stats (external) for interface: " ) + cc::note( m_interfaceName ) + cc::info( ", hits: " ) +
        cc::num10( data.externalStats.hits ) + cc::info( ", fails: " ) + cc::num10( data.externalStats.fails ) +
        cc::info( ", fails in a row: " ) + cc::num10( data.externalStats.sequentialFails ) +
        cc::info( ", packets lost: " ) + cc::num10( data.externalStats.packetsLost ) +
        cc::note( " (" + percentage( data.externalStats.packetsLost, data.externalStats.packetsSent ) + "%)" ) );

    LOGGER_INFO_RAW( cc::info( "Ping stats (local) for interface: " ) + cc::note( m_interfaceName ) +
                     cc::info( ", hits: " ) + cc::num10( data.localStats.hits ) + cc::info( ", fails: " ) +
                     cc::num10( data.localStats.fails ) + cc::info( ", fails in a row: " ) +
                     cc::num10( data.localStats.sequentialFails ) + cc::info( ", packets lost: " ) +
                     cc::num10( data.localStats.packetsLost ) +
                     cc::note( " (" + percentage( data.localStats.packetsLost, data.localStats.packetsSent ) + "%)" ) );

    for( auto& hostData : data.hosts ) {
        std::string location =
            hostData.hostLocation == PingStatisticsData::HostStats::HostLocation::EXTERNAL ? "external" : "local";

        LOGGER_INFO_RAW( cc::info( "host url: " ) + cc::note( hostData.hostUrl ) + cc::info( ", location: " ) +
                         cc::note( location ) + cc::info( ", description: " ) + cc::note( hostData.hostDescription ) +
                         cc::info( ", hits: " ) + cc::num10( hostData.hits ) + cc::info( ", fails: " ) +
                         cc::num10( hostData.fails ) + cc::info( ", packets lost: " ) +
                         cc::num10( hostData.packetsLost ) +
                         cc::note( " (" + percentage( hostData.packetsLost, hostData.packetsSent ) + "%)" ) +
                         cc::info( ", round-trip min/avg/max: " ) + cc::num10( hostData.roundTripMin ) + "/" +
                         cc::num10( hostData.roundTripAvg ) + "/" + cc::num10( hostData.roundTripMax ) +
                         cc::info( ", timestamp min: " ) + cc::time2string( hostData.timestampMin, true ) +
                         cc::info( ", timestamp max: " ) + cc::time2string( hostData.timestampMax, true ) );
    }
}

bool PingStatistics::PingHealth::isLastPingFailed() const {
    return hits && lastPingResult == false;
}

}  // namespace Network
}  // namespace HubPlatform
