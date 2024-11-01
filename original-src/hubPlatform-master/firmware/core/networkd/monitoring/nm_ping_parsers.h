#pragma once

#include <string>

namespace HubPlatform {
namespace Network {
namespace PingParsers {

struct Result {
    size_t packetsSent = 0;
    size_t packetsReceived = 0;
    double roundTripMin = 0;
    double roundTripAvg = 0;
    double roundTripMax = 0;
};

Result parsePingOutput( const std::string& pingOutput );
Result parseOpenWrtPingOutput( const std::string& pingOutput );
Result parseDebianPingOutput( const std::string& pingOutput );

}  // namespace PingParsers
}  // namespace Network
}  // namespace HubPlatform
