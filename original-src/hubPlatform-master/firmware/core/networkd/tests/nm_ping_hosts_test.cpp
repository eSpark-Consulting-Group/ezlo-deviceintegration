#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "networkd/monitoring/nm_ping_hosts.h"
#include "json_utils/json_utils.h"

/*

TESTPLAN

DONE    1. Ping hosts tests
DONE        1.1 Camera streams parser
*/

namespace HubPlatform {
namespace UT {

class PingHostsTestSuite : public ::testing::Test {};

TEST_F( PingHostsTestSuite, cameraStreamsParser ) /*1.1*/ {
    {  // Full RTSP url
        std::vector< Json > cameras;
        cameras.emplace_back( JSON( "streamId"
                                    << "id1"
                                    << "rtspUrl"
                                    << "rtsp://login:pass@192.168.88.1:88/videoMain" ) );
        cameras.emplace_back( JSON( "streamId"
                                    << "id2"
                                    << "rtspUrl"
                                    << "rtsp://login:pass@192.168.88.2:88/videoMain" ) );

        auto result = Network::cameraStreamsToPingHosts( cameras );
        ASSERT_EQ( result.size(), 2 );

        {
            auto& host = result.front();
            EXPECT_EQ( host.address, "192.168.88.1" );
            EXPECT_EQ( host.ipVersion, Network::IPVersion::IPv4 );
            EXPECT_EQ( host.location, Network::PingHost::Location::LOCAL );
            EXPECT_EQ( host.description, "stream_id1" );
        }

        {
            auto& host = result.back();
            EXPECT_EQ( host.address, "192.168.88.2" );
            EXPECT_EQ( host.ipVersion, Network::IPVersion::IPv4 );
            EXPECT_EQ( host.location, Network::PingHost::Location::LOCAL );
            EXPECT_EQ( host.description, "stream_id2" );
        }
    }

    {  // Short RTSP url
        std::vector< Json > cameras;
        cameras.emplace_back( JSON( "streamId"
                                    << "id1"
                                    << "rtspUrl"
                                    << "rtsp://192.168.88.1:88/videoMain" ) );

        auto result = Network::cameraStreamsToPingHosts( cameras );
        ASSERT_EQ( result.size(), 1 );

        auto& host = result.front();
        EXPECT_EQ( host.address, "192.168.88.1" );
        EXPECT_EQ( host.ipVersion, Network::IPVersion::IPv4 );
        EXPECT_EQ( host.location, Network::PingHost::Location::LOCAL );
        EXPECT_EQ( host.description, "stream_id1" );
    }
}

}  // namespace UT
}  // namespace HubPlatform
