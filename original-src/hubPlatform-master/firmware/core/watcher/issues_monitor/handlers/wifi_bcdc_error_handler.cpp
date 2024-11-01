#include "watcher/issues_monitor/handlers/wifi_bcdc_error_handler.h"

#include "watcher/utils/utils.h"
#include "watcher/resources/watcher_resources.h"

#include "system_utils/system.h"
#include "system_utils/environment.h"
#include "common_utils/cpp14.h"
#include "common_utils/exception_utils.h"
#include "base_utils/string_utils.h"
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
const char* const WifiBcdcErrorHandlerId = "WifiBcdcErrorHandlerId";

class OpenwrtImpl : public WifiBcdcErrorHandler::Impl {
    void runRecoveryAction() override {
        LOGGER_INFO( cc::warn( "Run recovery action for Broadcom BCDC error" ) );

        Watcher::Utils::storeLogsStuckSafe( { "--add-to-name=wifi_bcdc_error" } );

        std::stringstream recoveryCmd;
        recoveryCmd << "(";
        recoveryCmd << "echo 'Rebooting hub to fix Broadcom BCDC error' > /dev/kmsg ; ";
        recoveryCmd << Environment::getBinariesFolder() + "/stop.sh ; ";
        recoveryCmd << "sync ; ";
        recoveryCmd << "reboot -f ; sleep 15 ; echo 'b' > /proc/sysrq-trigger ; ";
        recoveryCmd << ")&";

        std::system( recoveryCmd.str().c_str() );
    }
};

}  // namespace

WifiBcdcErrorHandler::WifiBcdcErrorHandler( IDbClient& dbClient, SystemLogInterface& systemLog )
: WifiBcdcErrorHandler( dbClient, systemLog, std::make_unique< OpenwrtImpl >() ) {}

WifiBcdcErrorHandler::WifiBcdcErrorHandler( IDbClient& dbClient,
                                            SystemLogInterface& systemLog,
                                            std::unique_ptr< WifiBcdcErrorHandler::Impl > impl )
: m_dbClient( dbClient )
, m_impl( std::move( impl ) )
, m_systemLog( systemLog ) {
    m_systemLog.subscribe( WifiBcdcErrorHandlerId,
                           std::bind( &WifiBcdcErrorHandler::handleLogRecordEvent, this, std::placeholders::_1 ) );
}

WifiBcdcErrorHandler::~WifiBcdcErrorHandler() {
    m_systemLog.unsubscribe( WifiBcdcErrorHandlerId );
}

void WifiBcdcErrorHandler::run() {
    try {
        bool errorDetected = false;

        std::unique_lock< std::mutex > lock( m_mutex );
        for( auto& record : m_logRecords ) {
            if( record.find( Resources::IssueHandlers::WifiBcdcErrorPattern ) != std::string::npos ) {
                errorDetected = true;
                break;
            }
        }
        m_logRecords.clear();
        lock.unlock();

        if( errorDetected ) {
            ServiceMetricsData::HubErrorsTable table( m_dbClient );
            table.incrementError( ServiceMetricsData::HubErrorsTable::ErrorType::BCDCError );

            m_impl->runRecoveryAction();
        }
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( "Exception in handler: " + CommonUtils::whatError( e ) );
    }
}

void WifiBcdcErrorHandler::handleLogRecordEvent( const std::string& logRecord ) {
    std::lock_guard< std::mutex > lock( m_mutex );
    m_logRecords.emplace_back( logRecord );
}

}  // namespace IssueHandlers
}  // namespace Watcher
}  // namespace HubPlatform
