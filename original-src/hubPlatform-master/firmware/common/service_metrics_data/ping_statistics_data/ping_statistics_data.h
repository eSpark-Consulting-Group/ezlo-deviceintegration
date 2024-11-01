#pragma once

#include "json_utils/json_fwd.h"

#include <string>
#include <vector>

namespace HubPlatform {
namespace ServiceMetricsData {

struct PingStatisticsData {
    std::string interfaceName;

    struct GenericStats {
        size_t hits = 0;
        size_t fails = 0;
        size_t sequentialFails = 0;

        size_t packetsSent = 0;
        size_t packetsReceived = 0;
        size_t packetsLost = 0;
    };

    GenericStats externalStats;
    GenericStats localStats;

    struct HostStats {
        std::string hostUrl;

        enum class HostLocation {
            EXTERNAL,
            LOCAL,
        };
        HostLocation hostLocation;
        std::string hostDescription;

        size_t hits = 0;
        size_t fails = 0;

        size_t packetsSent = 0;
        size_t packetsReceived = 0;
        size_t packetsLost = 0;

        size_t roundTripMin = 0;
        size_t roundTripAvg = 0;
        size_t roundTripMax = 0;

        std::time_t timestampMin{ 0 };
        std::time_t timestampMax{ 0 };
    };

    std::vector< HostStats > hosts;

    Json toJson();
    static PingStatisticsData fromJson( const Json& json );
};

}  // namespace ServiceMetricsData
}  // namespace HubPlatform
