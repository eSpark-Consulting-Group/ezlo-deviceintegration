#pragma once

#include "networkd/monitoring/nm_interface_change_listener.h"
#include "networkd/monitoring/nm_ip_version.h"

#include "service_interfaces/task_loop.h"
#include "service_interfaces/task_loop_timer.h"

#include "json_utils/json_fwd.h"
#include "config_reader/config_reader.h"

#include <chrono>

namespace HubPlatform {
namespace Network {

class PingStatistics;

/** Provide data from ha-uid: cloud connection status */
class ExternalDataProvider {
public:
    ExternalDataProvider() = default;
    ~ExternalDataProvider() = default;

    void setNmaClientConnectionState( Json state, bool isConnected );
    bool isNmaConnected( Json* status );
    std::chrono::seconds getNmaConnectedDuration() const;
    void appendNmaConnectedDuration( std::chrono::seconds period );
    size_t nmaReconnectsCount() const;

private:
    JsonObject m_nmaConnectionStatus;
    bool m_isConnected{ false };
    std::chrono::seconds m_nmaConnectedDuration{ 0 };
    std::time_t m_lastConnected{ 0 };
    std::time_t m_lastMonotonicConnected{ 0 };
    size_t m_nmaReconnects{ 0 };

    void onNmaConnected();

    void onNmaDisconnected();
};

class InterfaceMonitor {
public:
    InterfaceMonitor( Services::TaskLoop& taskLoop,
                      InterfaceInfoListener& interfaceInfoListener,
                      std::string interfaceName,
                      ConfigReader& configReader,
                      ExternalDataProvider& externalDataProvider,
                      bool ping,
                      std::chrono::seconds pingPeriod,
                      IPVersion ipVersion = IPVersion::IPv4 );

    virtual ~InterfaceMonitor() = default;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool isActive() = 0;

    virtual const Json& manualUpdate() = 0;
    virtual void setPingPeriod( std::chrono::seconds pingPeriod );
    virtual void setIPVersion( IPVersion ipVersion );

    virtual const Json& getInfo() = 0;
    virtual bool isInternetAvailable() = 0;
    virtual bool configure() = 0;

    virtual const PingStatistics& pingStatistics() = 0;

protected:
    Services::TaskLoop& m_taskLoop;
    InterfaceInfoListener& m_interfaceChangeListener;
    std::string m_interfaceName;
    bool m_pingEnabled = false;
    std::chrono::seconds m_pingPeriod;
    IPVersion m_ipVersion;
    ConfigReader& m_configReader;
    ExternalDataProvider& m_externalDataProvider;
};

}  // namespace Network
}  // namespace HubPlatform
