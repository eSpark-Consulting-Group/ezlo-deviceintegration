#include "connection_nma_reporter.h"
#include "base_utils/timeutils.h"
#include "connection_report_db_info.h"
#include "base_utils/timeutils.h"

namespace HubPlatform {
namespace Uid {

const char* const monotonicNow = "monotonicNow";

ConnectionNmaReporter::ConnectionNmaReporter( IDbClient& dbClient )
: m_dbClient( dbClient ) {
    initFomDBConnectionReport();
}

void ConnectionNmaReporter::initFomDBConnectionReport() {
    using namespace Uid::Reporter;
    auto record = ConnectionReportDbInfo::loadFromDb( FirmwareConnectionReporter, m_dbClient );
    ConnectionReportDbInfo info( record );
    m_previousUptime = info.getUptime();
    m_uptimeCumulative = m_previousUptime;
    m_previousReconnects = info.getReconnects();
    m_reconnectsCumulative = m_previousReconnects;
}

void ConnectionNmaReporter::setNmaConnectionStatus( const JsonObject& status ) {
    m_nmaConnected = true;
    m_reconnectsCumulative++;
    m_lastMonotonicConnected = std::chrono::seconds( json_get_int( status, monotonicNow ) );
}

void ConnectionNmaReporter::setNmaDisconnectionStatus( const JsonObject& status ) {
    m_nmaConnected = false;
    auto lastMonotonicConnected = m_lastMonotonicConnected;
    m_lastMonotonicConnected = std::chrono::seconds( 0 );

    auto disconnectedAt = std::chrono::seconds( json_get_int( status, monotonicNow ) );

    if( lastMonotonicConnected.count() > 0 && disconnectedAt > lastMonotonicConnected ) {
        m_uptimeCumulative = m_uptimeCumulative + disconnectedAt - lastMonotonicConnected;
        addConnectionInfoToDb( true, false, m_uptimeCumulative, 0 );
    }
}

void ConnectionNmaReporter::addConnectionInfoToDb( bool updateUptime,
                                                   bool updateReconnects,
                                                   std::chrono::seconds uptime,
                                                   int reconnects ) {
    using namespace Uid::Reporter;
    auto pid = getpid();
    auto record = ConnectionReportDbInfo::loadFromDb( FirmwareConnectionReporter, m_dbClient );
    if( record.empty() ) {
        ConnectionReportDbInfo::createDbRecord( JSON( "_id"
                                                      << "uid" ),
                                                FirmwareConnectionReporter,
                                                m_dbClient );
        record = ConnectionReportDbInfo::loadFromDb( FirmwareConnectionReporter, m_dbClient );
    }
    ConnectionReportDbInfo info( record );
    info.setId( "uid" );
    if( updateUptime ) {
        info.setUptime( uptime );
    }

    if( updateReconnects ) {
        info.setReconnects( reconnects );
    }
    ConnectionReportDbInfo::updateDbRecord( info.toJson(), FirmwareConnectionReporter, m_dbClient );
}

void ConnectionNmaReporter::updateNmaConnectionReport() {
    auto now = TimeUtils::currentSteadyTime();
    updateConnectionReportForTime( now );
}

void ConnectionNmaReporter::updateConnectionReportForTime( std::chrono::seconds now ) {
    if( m_nmaConnected ) {
        if( m_lastMonotonicConnected.count() ) {
            m_uptimeCumulative += now - m_lastMonotonicConnected;
            m_lastMonotonicConnected = now;
        }
    }

    addConnectionInfoToDb( true, true, m_uptimeCumulative, m_reconnectsCumulative );
}

std::chrono::seconds ConnectionNmaReporter::getNmaCumulativeUptime() {
    using namespace Uid::Reporter;
    auto record = ConnectionReportDbInfo::loadFromDb( FirmwareConnectionReporter, m_dbClient );
    ConnectionReportDbInfo info( record );
    return info.getUptime();
}

std::chrono::seconds ConnectionNmaReporter::getNmaUptime() {
    using namespace Uid::Reporter;
    auto record = ConnectionReportDbInfo::loadFromDb( FirmwareConnectionReporter, m_dbClient );
    ConnectionReportDbInfo info( record );
    return info.getUptime() - m_previousUptime;
}

int ConnectionNmaReporter::getNmaCumulativeReconnects() {
    using namespace Uid::Reporter;
    auto record = ConnectionReportDbInfo::loadFromDb( FirmwareConnectionReporter, m_dbClient );
    ConnectionReportDbInfo info( record );
    return info.getReconnects();
}

int ConnectionNmaReporter::getNmaReconnects() {
    using namespace Uid::Reporter;
    auto record = ConnectionReportDbInfo::loadFromDb( FirmwareConnectionReporter, m_dbClient );
    ConnectionReportDbInfo info( record );
    return info.getReconnects() - m_previousReconnects;
}

JsonObject ConnectionNmaReporter::updateAndGetCurrentNmaConnectionReport() {
    updateNmaConnectionReport();
    auto uptime = getNmaUptime();
    auto cumulativeUptime = getNmaCumulativeUptime();
    auto cumulativeReconnects = getNmaCumulativeReconnects();
    auto reconnects = getNmaReconnects();
    Json report =
        JSON( "uptimeFormatted" << TimeUtils::formatUptime( uptime.count() ) << "cumulativeUptimeFormatted"
                                << TimeUtils::formatUptime( cumulativeUptime.count() ) << "uptime" << uptime.count()
                                << "cumulativeUptime" << cumulativeUptime.count() << "cumulativeReconnects"
                                << cumulativeReconnects << "reconnects" << reconnects );
    return report;
}

}  // namespace Uid
}  // namespace HubPlatform
