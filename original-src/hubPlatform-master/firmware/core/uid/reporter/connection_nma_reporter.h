#pragma once

#include "service_interfaces/reporter.h"
#include "uid/environment.h"
#include "uid/nma_client.h"
#include "uid/service_settings.h"
#include "uid/ezlo.h"

namespace HubPlatform {
namespace Uid {
class ConnectionNmaReporter {
public:
    explicit ConnectionNmaReporter( IDbClient& dbClient );
    void setNmaConnectionStatus( const JsonObject& status );
    void setNmaDisconnectionStatus( const JsonObject& status );
    void updateNmaConnectionReport( Json* status );
    Json updateAndGetCurrentNmaConnectionReport();
    void updateNmaConnectionReport();
    std::chrono::seconds getNmaCumulativeUptime();
    std::chrono::seconds getNmaUptime();
    int getNmaCumulativeReconnects();
    int getNmaReconnects();

private:
    void addConnectionInfoToDb( bool updateUptime, bool updateReconnects, std::chrono::seconds uptime, int reconnects );

    void initFomDBConnectionReport();

    void updateConnectionReportForTime( std::chrono::seconds now );

private:
    IDbClient& m_dbClient;
    std::chrono::seconds m_previousUptime;
    std::chrono::seconds m_uptimeCumulative;
    std::chrono::seconds m_lastMonotonicConnected;
    int m_pid = 0;
    int m_reconnectsCumulative = 0;
    int m_previousReconnects = 0;
    bool m_nmaConnected = false;
};
}  // namespace Uid
}  // namespace HubPlatform
