#pragma once

#include "service_interfaces/task_loop.h"
#include "database_client/idbclient.h"
#include "json_utils/json_fwd.h"

#include <functional>
#include <unordered_map>
#include <vector>
#include <string>
#include <thread>
#include <atomic>

namespace HubPlatform {
namespace Uid {

class ControllersDiscoveryService final {
public:
    using InfoCallback = std::function< void( const Json& ) >;

public:
    ControllersDiscoveryService( Services::TaskLoop& taskLoop, IDbClient& dbClient, InfoCallback onUpdated );
    ~ControllersDiscoveryService();

    void setUp();
    void tearDown();

    Json getControllerMdnsInfo( const std::string& controllerId ) const;
    std::vector< Json > listControllersMdnsInfo() const;

private:
    void monitoringThread();
    void scanForControllers();
    static void restartAvahiServiceIfItIsNotRunning();

private:
    Services::TaskLoop& m_taskLoop;
    IDbClient& m_dbClient;

    std::atomic_bool m_isMonitoringTreadUp;
    std::thread m_monitoringThread;

    InfoCallback m_onUpdated;
};

}  // namespace Uid
}  // namespace HubPlatform
