#pragma once

#include <string>

namespace HubPlatform {
namespace Network {

class InterfaceTrafficMonitor final {
public:
    struct TrafficStatistic {
        std::size_t m_input = 0;
        std::size_t m_output = 0;
    };

    InterfaceTrafficMonitor( std::string interfaceName );

    TrafficStatistic getTrafficStatistics() const;

private:
    std::string m_interfaceName;
};

}  // namespace Network
}  // namespace HubPlatform
