#include "networkd/monitoring/nm_interface_traffic_monitor.h"
#include "system_utils/system.h"
#include <utility>

namespace HubPlatform {
namespace Network {

InterfaceTrafficMonitor::InterfaceTrafficMonitor( std::string interfaceName )
: m_interfaceName( std::move( interfaceName ) ) {}

InterfaceTrafficMonitor::TrafficStatistic InterfaceTrafficMonitor::getTrafficStatistics() const {
    TrafficStatistic statistic;

    auto output =
        System::getCommandOutput( ( "cat /sys/class/net/" + m_interfaceName + "/statistics/tx_bytes" ).c_str() );
    auto input =
        System::getCommandOutput( ( "cat /sys/class/net/" + m_interfaceName + "/statistics/rx_bytes" ).c_str() );

    if( input.second == 0 ) {
        statistic.m_input = std::stol( input.first );
    }
    if( output.second == 0 ) {
        statistic.m_output = std::stol( output.first );
    }

    return statistic;
}

}  // namespace Network
}  // namespace HubPlatform
