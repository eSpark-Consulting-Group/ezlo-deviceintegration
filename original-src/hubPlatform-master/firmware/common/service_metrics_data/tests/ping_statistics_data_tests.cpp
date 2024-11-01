#include "service_metrics_data/ping_statistics_data/ping_statistics_data.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

/*

TESTPLAN

DONE    1. Ping statistics data tests
DONE        1.1 Serialize and deserialize

*/

namespace HubPlatform {
namespace UT {

using namespace ::testing;

TEST( PingStatisticsDataTests, serializeDeserialize ) /* 1.1 */ {
    ServiceMetricsData::PingStatisticsData data;

    data.interfaceName = "interface-name";

    {
        auto& stats = data.externalStats;
        stats.hits = 1;
        stats.fails = 2;
        stats.sequentialFails = 3;
        stats.packetsSent = 4;
        stats.packetsReceived = 5;
        stats.packetsLost = 6;
    }

    {
        auto& stats = data.localStats;
        stats.hits = 10;
        stats.fails = 20;
        stats.sequentialFails = 30;
        stats.packetsSent = 40;
        stats.packetsReceived = 50;
        stats.packetsLost = 60;
    }

    {
        data.hosts.emplace_back();
        auto& host = data.hosts.back();

        host.hostUrl = "external-url";
        host.hostLocation = ServiceMetricsData::PingStatisticsData::HostStats::HostLocation::EXTERNAL;
        host.hostDescription = "host-description";

        host.hits = 100;
        host.fails = 101;

        host.packetsSent = 102;
        host.packetsReceived = 103;
        host.packetsLost = 104;

        host.roundTripMin = 105;
        host.roundTripAvg = 106;
        host.roundTripMax = 107;

        host.timestampMin = 108;
        host.timestampMax = 109;
    }

    {
        data.hosts.emplace_back();
        auto& host = data.hosts.back();

        host.hostUrl = "local-url";
        host.hostLocation = ServiceMetricsData::PingStatisticsData::HostStats::HostLocation::LOCAL;
        host.hostDescription = "host-description";

        host.hits = 200;
        host.fails = 201;

        host.packetsSent = 202;
        host.packetsReceived = 203;
        host.packetsLost = 204;

        host.roundTripMin = 205;
        host.roundTripAvg = 206;
        host.roundTripMax = 207;

        host.timestampMin = 208;
        host.timestampMax = 209;
    }

    Json json;
    EXPECT_NO_THROW( json = data.toJson() );

    auto newData = ServiceMetricsData::PingStatisticsData::fromJson( json );

    EXPECT_EQ( data.interfaceName, newData.interfaceName );

    auto externalStats = std::make_pair( &data.externalStats, &newData.externalStats );
    auto localStats = std::make_pair( &data.localStats, &newData.localStats );

    for( auto& element : { externalStats, localStats } ) {
        auto& stats = *element.first;
        auto& newStats = *element.second;

        EXPECT_EQ( stats.hits, newStats.hits );
        EXPECT_EQ( stats.fails, newStats.fails );
        EXPECT_EQ( stats.sequentialFails, newStats.sequentialFails );
        EXPECT_EQ( stats.packetsSent, newStats.packetsSent );
        EXPECT_EQ( stats.packetsReceived, newStats.packetsReceived );
        EXPECT_EQ( stats.packetsLost, newStats.packetsLost );
    }

    ASSERT_EQ( data.hosts.size(), newData.hosts.size() );

    for( size_t i = 0; i < data.hosts.size(); ++i ) {
        auto& stats = data.hosts.at( i );
        auto& newStats = newData.hosts.at( i );

        EXPECT_EQ( stats.hostUrl, newStats.hostUrl );
        EXPECT_EQ( stats.hostLocation, newStats.hostLocation );
        EXPECT_EQ( stats.hostDescription, newStats.hostDescription );

        EXPECT_EQ( stats.hits, newStats.hits );
        EXPECT_EQ( stats.fails, newStats.fails );
        EXPECT_EQ( stats.packetsSent, newStats.packetsSent );
        EXPECT_EQ( stats.packetsReceived, newStats.packetsReceived );
        EXPECT_EQ( stats.packetsLost, newStats.packetsLost );
        EXPECT_EQ( stats.roundTripMin, newStats.roundTripMin );
        EXPECT_EQ( stats.roundTripAvg, newStats.roundTripAvg );
        EXPECT_EQ( stats.roundTripMax, newStats.roundTripMax );
        EXPECT_EQ( stats.timestampMin, newStats.timestampMin );
        EXPECT_EQ( stats.timestampMax, newStats.timestampMax );
    }
}

}  // namespace UT
}  // namespace HubPlatform
