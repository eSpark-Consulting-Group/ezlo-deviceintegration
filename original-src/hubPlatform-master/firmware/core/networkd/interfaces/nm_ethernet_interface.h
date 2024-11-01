#pragma once

#include "networkd/monitoring/nm_interface_change_listener.h"
#include "networkd/monitoring/nm_interface_monitor_factory.h"
#include "networkd/configuration/nm_config_manager.h"
#include "networkd/interfaces/nm_enable_policy.h"
#include "networkd/interfaces/nm_network_interface.h"
#include "networkd/monitoring/nm_interface_traffic_monitor.h"

#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace Network {

class PingStatistics;

class EthernetInterface : public InterfaceInfoListener, public NetworkInterface {
public:
    EthernetInterface( NetworkConfigManager& configManager,
                       const InterfaceMonitorFactory& interfaceMonitorFactory,
                       InterfaceInfoListener& interfaceInfoListener );
    ~EthernetInterface() override;

    void startMonitoring();
    void stopMonitoring();

    Json getInfo() const;
    bool isInternetAvailable() const;

    EnablePolicy getEnablePolicy() const;
    bool setEnablePolicy( EnablePolicy policy );

    void onNewInterfaceInfo( Json dynamicInfo ) override;

    InterfaceTrafficMonitor::TrafficStatistic getTrafficStatistics() const override;
    Json getUINetworkConfiguration() const;

    const PingStatistics& getPingStatistics() const;

private:
    Json getInfo( Json dynamicInfo ) const;
    const char* getName() const override;

private:
    NetworkConfigManager& m_configManager;
    InterfaceInfoListener& m_interfaceInfoListener;
    std::unique_ptr< InterfaceMonitor > m_monitor;
    InterfaceTrafficMonitor m_trafficMonitor;
};

}  // namespace Network
}  // namespace HubPlatform
