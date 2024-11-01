#include "watcher/issues_monitor/handlers/uninterruptible_sleep_handler.h"

#include "watcher/utils/utils.h"

#include "system_utils/system.h"
#include "system_utils/environment.h"
#include "common_utils/cpp14.h"
#include "common_utils/exception_utils.h"
#include "base_utils/string_utils.h"
#include "base_utils/utils.h"
#include "database_client/idbclient.h"
#include "service_metrics_data/hub_errors_table/hub_errors_table.h"
#include "log_utils/ezlo_log.h"
#include <tiny/tiny_consolecolors.h>

#include <algorithm>
#include <sstream>

namespace HubPlatform {
namespace Watcher {
namespace IssueHandlers {

namespace {
class OpenwrtImpl : public UninterruptibleSleepHandler::Impl {
public:
    std::vector< Types::ProcessInfo > readProcessInfoList() override {
        return Utils::readProcessInfoListFromProcFs();
    }

    void runRecoveryAction() override {
        LOGGER_INFO( cc::warn( "Run recovery action for uninterruptible processes" ) );

        // NOTE: flag --no-system-logs is required to prevent logcmd from running
        // network related tools that will hang logcmd due to corrupt network stack
        Watcher::Utils::storeLogsStuckSafe( { "--no-system-logs", "--add-to-name=uninterruptible_sleep" } );

        std::stringstream recoveryCmd;
        recoveryCmd << "(";
        recoveryCmd << "echo 'Rebooting hub to fix uninterruptible sleep' > /dev/kmsg ; ";
        recoveryCmd << Environment::getBinariesFolder() + "/stop.sh ; ";
        recoveryCmd << "sync ; ";
        recoveryCmd << "reboot -f ; sleep 15 ; echo 'b' > /proc/sysrq-trigger ; ";
        recoveryCmd << ")&";

        std::system( recoveryCmd.str().c_str() );
    }
};

inline bool isSpaceChar( unsigned char c ) {
    return std::isspace( c );
};

bool isNetworkProcess( const std::string& cmd ) {
    static const std::set< std::string > networkProcesses{ "ip", "netifd", "ha-networkd" };

    auto binaryName = StringUtils::split( StringUtils::split( cmd ).at( 0 ), "/" ).back();
    return networkProcesses.count( binaryName );
}

}  // namespace

UninterruptibleSleepHandler::UninterruptibleSleepHandler( IDbClient& dbClient )
: m_dbClient( dbClient )
, m_impl( std::make_unique< OpenwrtImpl >() ) {}

UninterruptibleSleepHandler::UninterruptibleSleepHandler( IDbClient& dbClient,
                                                          std::unique_ptr< UninterruptibleSleepHandler::Impl > impl )
: m_dbClient( dbClient )
, m_impl( std::move( impl ) ) {}

void UninterruptibleSleepHandler::run() {
    try {
        auto processInfoList = m_impl->readProcessInfoList();
        auto uninterruptibleMap = filterUninterruptibleProcesses( processInfoList );
        updateTrackingProcesses( uninterruptibleMap );
        detectStuckedProcesses( uninterruptibleMap );
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( "Exception in handler: " + CommonUtils::whatError( e ) );
    }
}

UninterruptibleSleepHandler::ProcessInfoMap UninterruptibleSleepHandler::filterUninterruptibleProcesses(
    const ProcessInfoList& processList ) {
    ProcessInfoMap map;

    for( auto& info : processList ) {
        auto& pid = info.pid;
        bool isUninterruptibleSleep = info.state.find( 'D' ) != std::string::npos;
        if( isUninterruptibleSleep && isNetworkProcess( info.cmd ) )
            map.emplace( info.pid, info );
    }
    return map;
}

void UninterruptibleSleepHandler::updateTrackingProcesses( const ProcessInfoMap& uninterruptibleMap ) {
    // Stop tracking processes that are no longer uninterruptible
    for( auto it = m_trackedProcesses.begin(); it != m_trackedProcesses.end(); ) {
        if( uninterruptibleMap.find( it->first ) == uninterruptibleMap.end() ) {
            it = m_trackedProcesses.erase( it );
        }
        else {
            ++it;
        }
    }

    auto now = std::chrono::steady_clock::now();
    for( auto& element : uninterruptibleMap ) {
        m_trackedProcesses.emplace( element.second.pid, now );
    }
}

void UninterruptibleSleepHandler::detectStuckedProcesses( const ProcessInfoMap& uninterruptibleMap ) {
    auto now = std::chrono::steady_clock::now();
    bool timeoutDetected = false;

    for( auto& element : m_trackedProcesses ) {
        auto& pid = element.first;
        auto& time = element.second;
        auto stuckDuration = std::chrono::duration_cast< std::chrono::milliseconds >( now - time );
        if( stuckDuration >= m_uninterruptibleSleepTimeout ) {
            timeoutDetected = true;
            auto& cmd = uninterruptibleMap.at( pid ).cmd;
            LOGGER_WARN( cc::warn( "Stucked uninterruptible process detected! pid: " ) + cc::num10( pid ) +
                         cc::warn( ", stucked for: " ) +
                         cc::num10( std::chrono::duration_cast< std::chrono::seconds >( stuckDuration ).count() ) +
                         cc::note( " seconds" ) + cc::warn( ", cmd: " ) + cc::note( cmd ) );
        }
    }

    if( timeoutDetected ) {
        ++m_recoveryAttempts;
        if( m_recoveryAttempts == 1 ) {
            ServiceMetricsData::HubErrorsTable table( m_dbClient );
            table.incrementError( ServiceMetricsData::HubErrorsTable::ErrorType::UninterruptibleSleep );
        }

        m_impl->runRecoveryAction();
    }
    else {
        m_recoveryAttempts = 0;
    }
}

void UninterruptibleSleepHandler::setUninterruptibleSleepTimeout( std::chrono::milliseconds timeout ) {
    m_uninterruptibleSleepTimeout = timeout;
}

}  // namespace IssueHandlers
}  // namespace Watcher
}  // namespace HubPlatform
