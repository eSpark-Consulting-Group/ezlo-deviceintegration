#pragma once

#include "networkd/monitoring/nm_interface_change_listener.h"
#include "networkd/monitoring/nm_interface_monitor_factory.h"
#include "networkd/configuration/nm_config_manager.h"
#include "networkd/interfaces/nm_ap_interface_settings.h"
#include "networkd/interfaces/nm_network_interface.h"
#include "networkd/monitoring/nm_interface_traffic_monitor.h"

#include "provision_data_utils/provision_data.h"
#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace Network {

class ApWifiInterface : public InterfaceInfoListener, public NetworkInterface {
public:
    ApWifiInterface( IDbClient& dbClient,
                     const ProvisionDataUtils::ProvisionData& provisionData,
                     NetworkConfigManager& configManager,
                     const InterfaceMonitorFactory& interfaceMonitorFactory,
                     InterfaceInfoListener& interfaceInfoListener );

    Json getInfo() const;
    std::string getIpAddress();

    bool enable( bool force = false );
    bool disable( bool force = false );
    bool isEnabled() const;
    bool isUp() const;

    bool reset();

    EnablePolicy getEnablePolicy() const;
    bool setEnablePolicy( EnablePolicy policy );

    bool setCustomSettings( Json customSettings ) override;

    void onNewInterfaceInfo( Json dynamicInfo ) override;

    const char* getName() const override;

private:
    Json getInfo( Json dynamicInfo ) const;

private:
    Json getWirelessConfig() const;
    Json getDefaultWirelessConfig() const;

    Json getNetworkConfig() const;
    Json getDefaultNetworkConfig() const;

    void notifyChanges();
    void applyCustomSettings( Json& wirelessConfig ) const;

    APWifiConfiguration makeSystemNetworkConfiguration( const Json& config ) const;
    Json getUINetworkConfiguration() const;
    Json getUIWirelessConfiguration() const;

    InterfaceTrafficMonitor::TrafficStatistic getTrafficStatistics() const override;

private:
    const ProvisionDataUtils::ProvisionData& m_provisionData;
    NetworkConfigManager& m_configManager;
    InterfaceInfoListener& m_interfaceInfoListener;
    APInterfaceSettings m_settings;
    std::unique_ptr< InterfaceMonitor > m_monitor;
    InterfaceTrafficMonitor m_trafficMonitor;

    bool m_enabled = false;
};

}  // namespace Network
}  // namespace HubPlatform
