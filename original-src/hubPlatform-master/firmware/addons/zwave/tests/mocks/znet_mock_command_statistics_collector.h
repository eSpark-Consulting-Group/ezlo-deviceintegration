#pragma once

#include <gmock/gmock.h>

#include <znet/iznet_command_statistics_collector.hpp>

namespace HubPlatform {
namespace Zwave {
namespace UT {

class MockCommandStatisticsCollector : public ::znet::ICommandStatisticsCollector {
public:
    MOCK_METHOD3( onNodeCommandDelay,
                  void( const uint16_t, std::chrono::system_clock::time_point, std::chrono::system_clock::duration ) );
    MOCK_METHOD2( onNodeCommandFailed, void( const uint16_t, std::chrono::system_clock::time_point ) );
};

}  // namespace UT
}  // namespace Zwave
}  // namespace HubPlatform
