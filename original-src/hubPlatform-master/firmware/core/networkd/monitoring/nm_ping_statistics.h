#pragma once

#include "networkd/monitoring/nm_ip_version.h"
#include "json_utils/json_fwd.h"
#include "service_metrics_data/ping_statistics_data/ping_statistics_data.h"

#include <chrono>
#include <deque>

namespace HubPlatform {
namespace Network {

class PingStatistics {
public:
    using PingStatisticsData = ServiceMetricsData::PingStatisticsData;

    struct PingStatEntry {
        using HostLocation = PingStatisticsData::HostStats::HostLocation;

        std::string interfaceName;
        std::string hostAddress;
        HostLocation hostLocation;
        std::string hostDescription;
        IPVersion ipVersion{ IPVersion::IPv4 };

        bool pingResult = false;
        size_t packetsSent = 0;
        size_t packetsReceived = 0;
        size_t roundTripMin = 0;
        size_t roundTripAvg = 0;
        size_t roundTripMax = 0;
        time_t timestamp{ 0 };
    };
    enum { PingStatLimit = 100 };

    struct PingHealth {
        size_t hits{ 0 };
        size_t fails{ 0 };
        size_t sequentialFails{ 0 };
        size_t packetsSent{ 0 };
        size_t packetsReceived{ 0 };
        size_t packetsLost{ 0 };
        bool lastPingResult = false;

        bool isLastPingFailed() const;
    };

public:
    const PingHealth& getPingHealthExternal();
    const PingHealth& getPingHealthLocal();

    void addPingStatEntry( PingStatEntry entry );
    PingStatisticsData getPingStatisticsData() const;
    void print() const;

private:
    std::string m_interfaceName;
    std::deque< PingStatEntry > m_pingStatsList;

    PingHealth m_externalHealth;
    PingHealth m_localHealth;
};

}  // namespace Network
}  // namespace HubPlatform
