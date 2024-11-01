#pragma once

#include "networkd/monitoring/nm_interface_monitor.h"

#include "mutex"
#include "nm_ping_statistics.h"
#include "nm_tcp_check.h"

namespace HubPlatform {
namespace Network {

class OpenwrtInterfaceMonitor : public InterfaceMonitor {
public:
    OpenwrtInterfaceMonitor( Services::TaskLoop& taskLoop,
                             InterfaceInfoListener& interfaceChangeListener,
                             std::string interfaceName,
                             ConfigReader& configReader,
                             ExternalDataProvider& externalDataProvider,
                             bool pingEnabled,
                             std::chrono::seconds pingPeriod,
                             IPVersion ipVersion = IPVersion::IPv4 );

    void start() override;
    void stop() override;
    bool isActive() override;

    const Json& manualUpdate() override;
    void setPingPeriod( std::chrono::seconds pingPeriod ) override;

    const Json& getInfo() override;
    bool isInternetAvailable() override;
    bool configure() override;

    const PingStatistics& pingStatistics() override;

private:
    void update( bool silent = false );
    void runPingTest( Json& actualInfo );
    void runTcpTest( Json& actualInfo );
    void runCloudConnectionTest( Json& actualInfo );
    bool isCloudConnected( const std::string& interfaceName );

    bool isNextPingTime() const;
    static bool isInternetAvailable( const Json& actualInfo );
    bool exchangeInterfaceInfo( const Json& actualInfo );

private:
    Services::TaskLoopTimer m_updateTimer;

    std::recursive_mutex m_interfaceInfoMutex;
    Json m_interfaceInfo;
    std::chrono::steady_clock::time_point m_nextPingTime;
    PingStatistics m_pingStatistics;
    size_t m_lastPingPrint{ 0 };
    Statistics m_tcpCheckStatistics;
    Json m_httpMethodConfig;
};

}  // namespace Network
}  // namespace HubPlatform
