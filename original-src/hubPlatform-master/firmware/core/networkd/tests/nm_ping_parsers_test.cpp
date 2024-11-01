#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "networkd/monitoring/nm_ping_parsers.h"

/*

TESTPLAN

DONE    1. Ping parsers tests
DONE        1.1 OpenWRT parser with round trip
DONE        1.2 OpenWRT parser without round trip
DONE        1.3 Debian/Ubuntu parser with round trip
DONE        1.4 Debian/Ubuntu parser without round trip
*/

namespace HubPlatform {
namespace UT {

class PingParsersTestSuite : public ::testing::Test {};

TEST_F( PingParsersTestSuite, openwrtParserWithRoundTrip /*1.1*/ ) {
    // clang-format off
    const std::string testString = \
R"(PING 8.8.8.8 (8.8.8.8): 56 data bytes
64 bytes from 8.8.8.8: seq=0 ttl=112 time=25.613 ms
64 bytes from 8.8.8.8: seq=1 ttl=112 time=25.319 ms
64 bytes from 8.8.8.8: seq=2 ttl=112 time=25.401 ms
64 bytes from 8.8.8.8: seq=3 ttl=112 time=25.355 ms
--- 8.8.8.8 ping statistics ---
5 packets transmitted, 4 packets received, 20% packet loss
round-trip min/avg/max = 24.319/25.417/26.613 ms)";
    // clang-format on

    auto result = Network::PingParsers::parseOpenWrtPingOutput( testString );
    EXPECT_EQ( result.packetsSent, 5 );
    EXPECT_EQ( result.packetsReceived, 4 );

    EXPECT_EQ( result.roundTripMin, 24.319 );
    EXPECT_EQ( result.roundTripAvg, 25.417 );
    EXPECT_EQ( result.roundTripMax, 26.613 );
}

TEST_F( PingParsersTestSuite, openwrtParserWithoutRoundTrip /*1.2*/ ) {
    // clang-format off
    const std::string testString = \
R"(--- 8.8.8.8 ping statistics ---
5 packets transmitted, 0 packets received, 100% packet loss)";
    // clang-format on

    auto result = Network::PingParsers::parseOpenWrtPingOutput( testString );
    EXPECT_EQ( result.packetsSent, 5 );
    EXPECT_EQ( result.packetsReceived, 0 );

    EXPECT_EQ( result.roundTripMin, 0 );
    EXPECT_EQ( result.roundTripAvg, 0 );
    EXPECT_EQ( result.roundTripMax, 0 );
}

TEST_F( PingParsersTestSuite, debianParserWithRoundTrip /*1.3*/ ) {
    // clang-format off
    const std::string testString = \
R"(PING 8.8.8.8 (8.8.8.8) 56(84) bytes of data.
64 bytes from 8.8.8.8: icmp_seq=2 ttl=112 time=25.8 ms
64 bytes from 8.8.8.8: icmp_seq=3 ttl=112 time=25.9 ms
64 bytes from 8.8.8.8: icmp_seq=4 ttl=112 time=26.1 ms
64 bytes from 8.8.8.8: icmp_seq=5 ttl=112 time=25.8 ms
--- 8.8.8.8 ping statistics ---
5 packets transmitted, 4 received, 20% packet loss, time 4004ms
rtt min/avg/max/mdev = 24.828/25.961/26.241/0.162 ms)";
    // clang-format on

    auto result = Network::PingParsers::parseDebianPingOutput( testString );
    EXPECT_EQ( result.packetsSent, 5 );
    EXPECT_EQ( result.packetsReceived, 4 );

    EXPECT_EQ( result.roundTripMin, 24.828 );
    EXPECT_EQ( result.roundTripAvg, 25.961 );
    EXPECT_EQ( result.roundTripMax, 26.241 );
}

TEST_F( PingParsersTestSuite, debianParserWithoutRoundTrip /*1.4*/ ) {
    // clang-format off
    const std::string testString = \
R"(--- 8.8.8.8 ping statistics ---
5 packets transmitted, 0 received, 100% packet loss, time 4004ms)";
    // clang-format on

    auto result = Network::PingParsers::parseDebianPingOutput( testString );
    EXPECT_EQ( result.packetsSent, 5 );
    EXPECT_EQ( result.packetsReceived, 0 );

    EXPECT_EQ( result.roundTripMin, 0 );
    EXPECT_EQ( result.roundTripAvg, 0 );
    EXPECT_EQ( result.roundTripMax, 0 );
}

}  // namespace UT
}  // namespace HubPlatform
