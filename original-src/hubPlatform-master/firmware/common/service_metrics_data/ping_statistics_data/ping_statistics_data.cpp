#include "service_metrics_data/ping_statistics_data/ping_statistics_data.h"

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace ServiceMetricsData {

namespace {
namespace Fields {

const char* const InterfaceName = "interface";
const char* const Hits = "hits";
const char* const Fails = "fails";
const char* const SequentialFails = "sequential_fails";
const char* const PacketsSent = "packets_sent";
const char* const PacketsReceived = "packets_received";
const char* const PacketsLost = "packets_lost";

const char* const HostUrl = "host_url";
const char* const HostLocation = "host_location";
const char* const HostDescription = "host_description";
const char* const RoundTripMin = "round_trip_min";
const char* const RoundTripAvg = "round_trip_avg";
const char* const RoundTripMax = "round_trip_max";

const char* const TimestampMin = "timestamp_min";
const char* const TimestampMax = "timestamp_max";

const char* const SectionExternalStats = "external_stats";
const char* const SectionLocalStats = "local_stats";
const char* const SectionHosts = "hosts";

}  // namespace Fields

const std::map< PingStatisticsData::HostStats::HostLocation, std::string >& locationMap() {
    using Location = PingStatisticsData::HostStats::HostLocation;
    static const std::map< Location, std::string > map = {
        { Location::EXTERNAL, "external" },
        { Location::LOCAL, "local" },
    };
    return map;
}

const std::map< std::string, PingStatisticsData::HostStats::HostLocation >& reverseLocationMap() {
    using Location = PingStatisticsData::HostStats::HostLocation;
    static const auto map = []() {
        std::map< std::string, Location > reverseMap;
        for( auto& element : locationMap() ) {
            reverseMap.emplace( element.second, element.first );
        }
        return reverseMap;
    }();
    return map;
}

Json genericStatsToJson( const PingStatisticsData::GenericStats& stats ) {
    JsonObject j;

    json_add_int( j, Fields::Hits, stats.hits );
    json_add_int( j, Fields::Fails, stats.fails );
    json_add_int( j, Fields::SequentialFails, stats.sequentialFails );

    json_add_int( j, Fields::PacketsSent, stats.packetsSent );
    json_add_int( j, Fields::PacketsReceived, stats.packetsReceived );
    json_add_int( j, Fields::PacketsLost, stats.packetsLost );

    return j;
}

PingStatisticsData::GenericStats genericStatsFromJson( const Json& j ) {
    PingStatisticsData::GenericStats stats;

    stats.hits = json_get_int( j, Fields::Hits, stats.hits );
    stats.fails = json_get_int( j, Fields::Fails, stats.fails );
    stats.sequentialFails = json_get_int( j, Fields::SequentialFails, stats.sequentialFails );

    stats.packetsSent = json_get_int( j, Fields::PacketsSent, stats.packetsSent );
    stats.packetsReceived = json_get_int( j, Fields::PacketsReceived, stats.packetsReceived );
    stats.packetsLost = json_get_int( j, Fields::PacketsLost, stats.packetsLost );

    return stats;
}

Json hostStatsToJson( const PingStatisticsData::HostStats& stats ) {
    JsonObject j;

    json_add_str( j, Fields::HostUrl, stats.hostUrl );
    json_add_str( j, Fields::HostLocation, locationMap().at( stats.hostLocation ) );
    json_add_str( j, Fields::HostDescription, stats.hostDescription );

    json_add_int( j, Fields::Hits, stats.hits );
    json_add_int( j, Fields::Fails, stats.fails );

    json_add_int( j, Fields::PacketsSent, stats.packetsSent );
    json_add_int( j, Fields::PacketsReceived, stats.packetsReceived );
    json_add_int( j, Fields::PacketsLost, stats.packetsLost );

    json_add_int( j, Fields::RoundTripMin, stats.roundTripMin );
    json_add_int( j, Fields::RoundTripAvg, stats.roundTripAvg );
    json_add_int( j, Fields::RoundTripMax, stats.roundTripMax );

    json_add_int( j, Fields::TimestampMin, stats.timestampMin );
    json_add_int( j, Fields::TimestampMax, stats.timestampMax );

    return j;
}

PingStatisticsData::HostStats hostStatsFromJson( const Json& j ) {
    PingStatisticsData::HostStats stats;

    stats.hostUrl = json_get_str( j, Fields::HostUrl );
    stats.hostLocation = reverseLocationMap().at( json_get_str( j, Fields::HostLocation ) );
    stats.hostDescription = json_get_str( j, Fields::HostDescription );

    stats.hits = json_get_int( j, Fields::Hits, stats.hits );
    stats.fails = json_get_int( j, Fields::Fails, stats.fails );

    stats.packetsSent = json_get_int( j, Fields::PacketsSent, stats.packetsSent );
    stats.packetsReceived = json_get_int( j, Fields::PacketsReceived, stats.packetsReceived );
    stats.packetsLost = json_get_int( j, Fields::PacketsLost, stats.packetsLost );

    stats.roundTripMin = json_get_int( j, Fields::RoundTripMin, stats.roundTripMin );
    stats.roundTripAvg = json_get_int( j, Fields::RoundTripAvg, stats.roundTripAvg );
    stats.roundTripMax = json_get_int( j, Fields::RoundTripMax, stats.roundTripMax );

    stats.timestampMin = json_get_int( j, Fields::TimestampMin, stats.timestampMin );
    stats.timestampMax = json_get_int( j, Fields::TimestampMax, stats.timestampMax );

    return stats;
}

}  // namespace

Json PingStatisticsData::toJson() {
    JsonObject j;
    json_add_str( j, Fields::InterfaceName, interfaceName );

    j[ Fields::SectionExternalStats ] = genericStatsToJson( externalStats );
    j[ Fields::SectionLocalStats ] = genericStatsToJson( localStats );

    auto hostsJson = JSON_EMPTY_ARR;
    for( auto& host : hosts ) {
        hostsJson.push_back( hostStatsToJson( host ) );
    }
    j[ Fields::SectionHosts ] = hostsJson;

    return j;
}

PingStatisticsData PingStatisticsData::fromJson( const Json& json ) {
    if( json.empty() ) {
        return {};
    }

    PingStatisticsData data;
    data.interfaceName = json_get_str( json, Fields::InterfaceName );

    data.externalStats = genericStatsFromJson( json[ Fields::SectionExternalStats ] );
    data.localStats = genericStatsFromJson( json[ Fields::SectionLocalStats ] );

    for( auto& hostJson : json[ Fields::SectionHosts ] ) {
        data.hosts.emplace_back( hostStatsFromJson( hostJson ) );
    }

    return data;
}

}  // namespace ServiceMetricsData
}  // namespace HubPlatform
