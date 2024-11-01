#include "service_metrics_data/ssh_tunnel_data/ssh_tunnel_data.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

/*

TESTPLAN

DONE    1. SSH tunnel data metrics tests
DONE        1.1 Serialize and deserialize

*/

namespace HubPlatform {
namespace UT {

using namespace ::testing;

TEST( SshTunnelDataMetricsTests, serializeDeserialize ) /* 1.1 */ {
    ServiceMetricsData::SshTunnelData data;
    data.pid = 1;
    data.id = "2";
    data.sshTunnelType = ServiceMetricsData::SshTunnelData::SshTunnelType::TYPE_SUPPORT;
    data.sshTunnelStatus = ServiceMetricsData::SshTunnelData::SshTunnelStatus::STATUS_OPEN;
    data.tcpState = "ESTABLISHED";
    data.serverUrl = "server-url";
    data.remotePort = 3;
    data.localPort = 4;
    data.startTime = 5;
    data.uptime = 6;
    data.connectionStatistics = {
        { ServiceMetricsData::SshTunnelData::SshTunnelConnectionStatus::STATUS_CONNECTED, 2 },
        { ServiceMetricsData::SshTunnelData::SshTunnelConnectionStatus::STATUS_CONNECTION_REFUSED, 1 },
    };

    Json json;
    EXPECT_NO_THROW( json = data.toJson() );

    auto newData = ServiceMetricsData::SshTunnelData::fromJson( json );

    EXPECT_EQ( data.pid, newData.pid );
    EXPECT_EQ( data.id, newData.id );
    EXPECT_EQ( data.sshTunnelType, newData.sshTunnelType );
    EXPECT_EQ( data.tcpState, newData.tcpState );
    EXPECT_EQ( data.serverUrl, newData.serverUrl );
    EXPECT_EQ( data.remotePort, newData.remotePort );
    EXPECT_EQ( data.localPort, newData.localPort );
    EXPECT_EQ( data.startTime, newData.startTime );
    EXPECT_EQ( data.uptime, newData.uptime );
    EXPECT_EQ( data.connectionStatistics, newData.connectionStatistics );
}

}  // namespace UT
}  // namespace HubPlatform
