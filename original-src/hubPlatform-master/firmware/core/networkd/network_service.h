#pragma once

#include "service_interfaces/service.h"
#include "networkd/network_manager.h"
#include "networkd/configuration/nm_config_manager.h"
#include "networkd/configuration/nm_config_file_manager.h"
#include "networkd/monitoring/nm_interface_change_listener.h"
#include "networkd/scan/nm_scan_result_listener.h"
#include "networkd/network_change_listener.h"
#include "networkd/ssh_tunnel/ssh_camera_tunnel.h"
#include "networkd/ssh_tunnel/ssh_support_tunnel.h"
#include "networkd/remote_interface/network_statistics_settings_manager.h"
#include "networkd/remote_interface/network_remote_interface.h"
#include "networkd/communication_test/communication_manual_test.h"

#include "cored/remote_interface/cored_interface.h"
#include "service_metrics_data/ssh_tunnel_data/ssh_tunnel_data.h"

#include <mutex>
#include <map>
#include <chrono>
#include <cloud_storage/request_builder.h>

namespace HubPlatform {
namespace Network {

class TunnelManagerImpl;

class Service : public Services::Service, public NetworkChangeListener, public WifiScanResultListener {
public:
    friend class NetworkReporter;
    Service();

    int restoreBackup( const std::string& tempBackupFolder ) override;

protected:
    void setUp() override;

    void tearDown() override;
    void handleMethodCall( const JsonObject& request,
                           MessageSubClasses requestType,
                           const std::string& sender ) override;
    JsonObject handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) override;
    bool handleAsyncRequestFromBus( const JsonObject& request,
                                    MessageSubClasses requestType,
                                    const std::string& uuid,
                                    const std::string& sender ) override;
    void handleMessageFromBus( const Services::Message& message, const std::string& sender ) override;

private:
    void tryHandleMethodCall( const JsonObject& request, MessageSubClasses requestType, const std::string& sender );
    JsonObject tryHandleRequestFromBus( const JsonObject& request, MessageSubClasses requestType );
    void handleHotplugEvents( const Json& hotplugEvent );
    void handleCamerasStartRtspStreamingBroadcast( const Json& message );
    void handleCamerasStopRtspStreamingBroadcast( const Json& message );
    void handleSshTunnelSupportEnabledSet( const Json& message );
    void onNetworkInfoChanged( Json diff ) override;
    void onInternetAvailabilityChanged( bool internetAvailable ) override;
    void onInterfaceStatusChanged( const std::string& interfaceId,
                                   const std::string& interfaceType,
                                   bool isConnected ) override;
    void onCurrentNetworkChanged( const NetworkInfo& newIpAddress ) override;
    void onBtIncomingConnectionEstablished( const std::string& hostMac, const std::string& peerMac );

    Json handleCustomSettingsSet( const std::string& interfaceId, const Json& settings );
    Json handleStatisticSettingsSet( const StatisticsSettingsManager::StatisticsSettings& settings );
    void handleStatisticsGet( const RemoteInterface::StatisticsPeriod& period );

    void handleMobileMethodCalls( const JsonObject& request, MessageSubClasses requestType, const std::string& sender );
    JsonObject handleExecuteATCommandRequestEmbedded( const std::string& ATCommand,
                                                      const std::chrono::milliseconds& timeout );

    void handleExecuteATCommandEmbedded( const JsonObject& request,
                                         const std::string& uuid,
                                         const std::string& sender );

    void handleNmaClientConnected( const JsonObject& message );
    void handleNmaClientDisconnected( const JsonObject& message );
    virtual JsonObject getServiceInfo() override;
    JsonArray dumpSshTunnelsMetricsData();

private:
    void onScanStarted( const std::string& interfaceName ) override;
    void onScanProcess( const std::string& interfaceName, JsonArray&& networks ) override;
    void onScanFinished( const std::string& interfaceName ) override;
    void onScanFailed( const std::string& interfaceName, const std::string& errorDescription ) override;

    void onCameraTunnelStatusChanged( const SshTunnel::Identity& identity, SshTunnel::Status status );

    void sendPostponedLogs();
    void extractKernelCrashLogs();

    void startBleAp();
    void stopBleAp();

    bool isSshTunnelStopScheduled( const SshTunnel::Identity& identity );
    void scheduleSshTunnelStop( const SshTunnel::Identity& identity );
    void discardSshTunnelStop( const SshTunnel::Identity& identity );
    void discardAllSshTunnelStops();

    JsonObject getCloudStorageToken();

private:
    std::unique_ptr< NetworkConfigManager > m_networkConfigManager;
    std::unique_ptr< NetworkConfigFileManager > m_networkConfigFileManager;
    std::shared_ptr< ConfigReader > m_configReaderPtr;
    std::shared_ptr< NetworkManager > m_networkManager;
    std::unique_ptr< SshTunnel::SshCameraTunnel > m_sshCameraTunnel;
    std::unique_ptr< SshTunnel::SshSupportTunnel > m_sshSupportTunnel;
    std::unique_ptr< SshTunnel::TunnelManagerImpl > m_tunnelManager;
    std::map< std::string, ServiceMetricsData::SshTunnelData::SshTunnelType > m_sshTunnelTypes;
    std::unique_ptr< CoreDRemoteInterface::CoreDInterface > m_cored;
    std::unique_ptr< StatisticsSettingsManager > m_statisticsSettingsManager;
    std::unique_ptr< CommunicationTest::ManualCommunicationTest > m_manualCommunicationTest;

    std::mutex m_sendPostponedLogsMutex;
    Services::PostponedTaskPtr m_sendPostponedLogsTimer;

    std::mutex m_sshTunnelStopTasksMutex;
    std::chrono::seconds m_sshTunnelStopDelay = std::chrono::minutes( 10 );
    std::map< SshTunnel::Identity, Services::PostponedTaskPtr > m_sshTunnelStopTasks;

    const int HttpServerPort = 17000;
    static const SshTunnel::Identity HttpServerIdentity;
};

}  // namespace Network
}  // namespace HubPlatform
