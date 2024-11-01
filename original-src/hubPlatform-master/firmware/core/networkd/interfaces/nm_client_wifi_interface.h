#pragma once

#include "networkd/monitoring/nm_interface_change_listener.h"
#include "networkd/monitoring/nm_interface_monitor_factory.h"
#include "networkd/configuration/nm_config_manager.h"
#include "networkd/interfaces/nm_interface_settings.h"
#include "networkd/scan/nm_wifi_passive_scanner.h"
#include "networkd/interfaces/nm_network_interface.h"
#include "networkd/monitoring/nm_interface_traffic_monitor.h"

#include "database_client/idbclient.h"
#include "provision_data_utils/provision_data.h"
#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace Network {

class PingStatistics;

class ClientWifiInterface : public InterfaceInfoListener, public NetworkInterface {
public:
    ClientWifiInterface( Services::TaskLoop& taskLoop,
                         IDbClient& dbClient,
                         const ProvisionDataUtils::ProvisionData& provisionData,
                         NetworkConfigManager& configManager,
                         WifiPassiveScanner& wifiPassiveScanner,
                         const InterfaceMonitorFactory& interfaceMonitorFactory,
                         InterfaceInfoListener& interfaceInfoListener );
    ~ClientWifiInterface() override;

    bool hasSettings() const;
    Json getInfo() const;
    bool isInternetAvailable() const;
    std::string getIpAddress();
    std::pair< std::string, bool > getSSID() const;

    bool enable( bool force = false );
    bool disable( bool force = false );
    bool isEnabled() const;
    bool isUp() const;

    void connect( int channel,
                  const std::string& ssid,
                  const std::string& bssid,
                  const std::string& encryption,
                  const std::string& key,
                  const bool force );
    void reconnect();
    bool reset();

    EnablePolicy getEnablePolicy() const;
    bool setEnablePolicy( EnablePolicy policy );

    const char* getName() const override;
    const PingStatistics& getPingStatistics() const;

private:
    Json getInfo( Json dynamicInfo ) const;

private:
    void checkNewChannelAndBssid();

    void startMonitoring();
    void stopMonitoring();
    bool configure();

    ClientWifiConfiguration makeSystemNetworkConfiguration( const Json& config ) const;
    Json getUINetworkConfiguration() const;
    Json getUIWirelessConfiguration() const;

    void backupWirelessNetworkSettings();
    void restoreWirelessNetworkSettings();

    Json getWirelessNetworkSettings( const std::string& id ) const;
    void setWirelessNetworkSettings( const std::string& id, Json network );

    Json getWirelessConfig() const;
    Json getSavedWirelessConfig() const;

    Json getNetworkConfig() const;
    Json getDefaultNetworkConfig() const;

    InterfaceTrafficMonitor::TrafficStatistic getTrafficStatistics() const override;

private:
    void onNewInterfaceInfo( Json dynamicInfo ) override;

private:
    IDbClient& m_dbClient;
    const ProvisionDataUtils::ProvisionData& m_provisionData;
    NetworkConfigManager& m_configManager;
    WifiPassiveScanner& m_wifiPassiveScanner;
    Services::TaskLoopTimer m_rescanTimer;
    InterfaceInfoListener& m_interfaceInfoListener;
    InterfaceSettings m_settings;
    std::unique_ptr< InterfaceMonitor > m_monitor;
    InterfaceTrafficMonitor m_trafficMonitor;

    bool m_connecting;
    bool m_enabled;
};

}  // namespace Network
}  // namespace HubPlatform
