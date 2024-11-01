#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "networkd/monitoring/nm_ping_statistics.h"

/*

TESTPLAN

DONE    1. Ping statistics tests
DONE        1.1 externalHosts
*/

namespace HubPlatform {
namespace UT {

class PingStatisticsTestSuite : public ::testing::Test {
public:
    using GenericStats = Network::PingStatistics::PingStatisticsData::GenericStats;
    using HostStats = Network::PingStatistics::PingStatisticsData::HostStats;
    using HostLocation = Network::PingStatistics::PingStatisticsData::HostStats::HostLocation;

    void compareGenericStats( const GenericStats& lhs, const GenericStats& rhs, int lineNum ) {
        EXPECT_EQ( lhs.hits, rhs.hits ) << "line: " << lineNum;
        EXPECT_EQ( lhs.fails, rhs.fails ) << "line: " << lineNum;
        EXPECT_EQ( lhs.sequentialFails, rhs.sequentialFails ) << "line: " << lineNum;
        EXPECT_EQ( lhs.packetsSent, rhs.packetsSent ) << "line: " << lineNum;
        EXPECT_EQ( lhs.packetsReceived, rhs.packetsReceived ) << "line: " << lineNum;
        EXPECT_EQ( lhs.packetsLost, rhs.packetsLost ) << "line: " << lineNum;
    }

    void compareHostStats( const HostStats& lhs, const HostStats& rhs, int lineNum ) {
        EXPECT_EQ( lhs.hostUrl, rhs.hostUrl ) << "line: " << lineNum;
        EXPECT_EQ( lhs.hostLocation, rhs.hostLocation ) << "line: " << lineNum;
        EXPECT_EQ( lhs.hostDescription, rhs.hostDescription ) << "line: " << lineNum;
        EXPECT_EQ( lhs.hits, rhs.hits ) << "line: " << lineNum;
        EXPECT_EQ( lhs.fails, rhs.fails ) << "line: " << lineNum;
        EXPECT_EQ( lhs.packetsSent, rhs.packetsSent ) << "line: " << lineNum;
        EXPECT_EQ( lhs.packetsReceived, rhs.packetsReceived ) << "line: " << lineNum;
        EXPECT_EQ( lhs.packetsLost, rhs.packetsLost ) << "line: " << lineNum;
        EXPECT_EQ( lhs.roundTripMin, rhs.roundTripMin ) << "line: " << lineNum;
        EXPECT_EQ( lhs.roundTripAvg, rhs.roundTripAvg ) << "line: " << lineNum;
        EXPECT_EQ( lhs.roundTripMax, rhs.roundTripMax ) << "line: " << lineNum;
        EXPECT_EQ( lhs.timestampMin, rhs.timestampMin ) << "line: " << lineNum;
        EXPECT_EQ( lhs.timestampMax, rhs.timestampMax ) << "line: " << lineNum;
    }
};

TEST_F( PingStatisticsTestSuite, externalHosts ) /*1.1*/ {
    Network::PingStatistics::PingStatEntry inputA;
    inputA.interfaceName = "interface-name";
    inputA.hostAddress = "host-address";
    inputA.hostLocation = HostLocation::EXTERNAL;
    inputA.hostDescription = "host-description";
    inputA.pingResult = true;
    inputA.packetsSent = 4;
    inputA.packetsReceived = 3;
    inputA.roundTripMin = 10;
    inputA.roundTripAvg = 20;
    inputA.roundTripMax = 30;
    inputA.timestamp = 1000;

    Network::PingStatistics::PingStatEntry inputB;
    inputB.interfaceName = "interface-name";
    inputB.hostAddress = "host-address";
    inputB.hostLocation = HostLocation::EXTERNAL;
    inputB.hostDescription = "host-description";
    inputB.pingResult = false;
    inputB.packetsSent = 5;
    inputB.packetsReceived = 0;
    inputB.timestamp = 2000;

    Network::PingStatistics statistics;

    {  // Add inputA
        statistics.addPingStatEntry( inputA );

        auto stats = statistics.getPingStatisticsData();

        EXPECT_EQ( stats.interfaceName, inputA.interfaceName );

        {
            GenericStats expected;
            expected.hits = 1;
            expected.fails = 0;
            expected.sequentialFails = 0;
            expected.packetsSent = inputA.packetsSent;
            expected.packetsReceived = inputA.packetsReceived;
            expected.packetsLost = expected.packetsSent - expected.packetsReceived;

            compareGenericStats( stats.externalStats, expected, __LINE__ );
        }

        {
            HostStats expected;
            expected.hostUrl = inputA.hostAddress;
            expected.hostLocation = inputA.hostLocation;
            expected.hostDescription = inputA.hostDescription;
            expected.hits = 1;
            expected.fails = 0;
            expected.packetsSent = inputA.packetsSent;
            expected.packetsReceived = inputA.packetsReceived;
            expected.packetsLost = inputA.packetsSent - inputA.packetsReceived;
            expected.roundTripMin = inputA.roundTripMin;
            expected.roundTripAvg = inputA.roundTripAvg;
            expected.roundTripMax = inputA.roundTripMax;
            expected.timestampMin = inputA.timestamp;
            expected.timestampMax = inputA.timestamp;

            ASSERT_EQ( stats.hosts.size(), 1 );
            compareHostStats( stats.hosts.at( 0 ), expected, __LINE__ );
        }
    }

    {  // Add inputB
        statistics.addPingStatEntry( inputB );

        auto stats = statistics.getPingStatisticsData();

        EXPECT_EQ( stats.interfaceName, inputB.interfaceName );

        {
            GenericStats expected;
            expected.hits = 2;
            expected.fails = 1;
            expected.sequentialFails = 1;
            expected.packetsSent = inputA.packetsSent + inputB.packetsSent;
            expected.packetsReceived = inputA.packetsReceived + inputB.packetsReceived;
            expected.packetsLost = expected.packetsSent - expected.packetsReceived;

            compareGenericStats( stats.externalStats, expected, __LINE__ );
        }

        {
            HostStats expected;
            expected.hostUrl = inputB.hostAddress;
            expected.hostLocation = inputB.hostLocation;
            expected.hostDescription = inputB.hostDescription;
            expected.hits = 2;
            expected.fails = 1;
            expected.packetsSent = inputA.packetsSent + inputB.packetsSent;
            expected.packetsReceived = inputA.packetsReceived + inputB.packetsReceived;
            expected.packetsLost = expected.packetsSent - expected.packetsReceived;
            expected.roundTripMin = inputA.roundTripMin;  // Round trip ignored on failed ping
            expected.roundTripAvg = 0;
            expected.roundTripMax = inputA.roundTripMax;  // Round trip ignored on failed ping
            expected.timestampMin = inputA.timestamp;
            expected.timestampMax = inputB.timestamp;

            ASSERT_EQ( stats.hosts.size(), 1 );

            auto& host = stats.hosts.at( 0 );
            EXPECT_GT( host.roundTripAvg, expected.roundTripMin );
            EXPECT_LT( host.roundTripAvg, expected.roundTripMax );
            host.roundTripAvg = 0;

            compareHostStats( host, expected, __LINE__ );
        }
    }
}

}  // namespace UT
}  // namespace HubPlatform
