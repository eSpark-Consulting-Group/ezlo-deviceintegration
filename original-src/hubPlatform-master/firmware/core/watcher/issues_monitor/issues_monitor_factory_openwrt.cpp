#include "watcher/issues_monitor/issues_monitor_factory_openwrt.h"

#include "watcher/issues_monitor/handlers/uninterruptible_sleep_handler.h"
#include "watcher/issues_monitor/handlers/unresponsive_services_handler.h"
#include "watcher/issues_monitor/handlers/zwave_node_expired_handler.h"
#include "watcher/issues_monitor/handlers/wifi_bcdc_error_handler.h"

#include "watcher/utils/utils.h"
#include "watcher/resources/watcher_resources.h"
#include "watcher/service_activity.h"
#include "watcher/system_log/system_log_interface.h"

#include "config_reader/config_reader.h"
#include "system_utils/environment.h"
#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include "database_client/idbclient.h"

namespace HubPlatform {
namespace Watcher {
namespace IssuesMonitorFactoryOpenwrt {

static void addUninterruptibleSleepHandler( IssuesMonitor& monitor, ConfigReader& config, IDbClient& dbClient ) {
    const auto timeoutKey = Resources::ConfigKeys::WatcherIssueHandlersUninterruptibleSleepTimeout;
    const auto checkIntervalKey = Resources::ConfigKeys::WatcherIssueHandlersUninterruptibleSleepCheckInterval;

    auto handler = std::make_unique< IssueHandlers::UninterruptibleSleepHandler >( dbClient );

    std::chrono::seconds timeout( Utils::readConfigIntValue( config, timeoutKey, 180 ) );
    handler->setUninterruptibleSleepTimeout( timeout );

    std::chrono::seconds checkInterval( Utils::readConfigIntValue( config, checkIntervalKey, 15 ) );
    monitor.addHandler( std::move( handler ), checkInterval );
    LOGGER_DEBUG_RAW( "UninterruptibleSleepHandler: timeout: " + cc::num10( timeout.count() ) );
    LOGGER_DEBUG_RAW( "UninterruptibleSleepHandler: checkInterval: " + cc::num10( checkInterval.count() ) );
}

static void addUnresponsiveServicesHandler( IssuesMonitor& monitor, ConfigReader& config, ServiceActivity& service ) {
    const auto timeoutKey = Resources::ConfigKeys::WatcherIssueHandlersUnresponsiveServicesTimeout;
    const auto serviceTimeoutKey = Resources::ConfigKeys::WatcherIssueHandlersUnresponsiveServicesServiceTimeout;
    const auto checkIntervalKey = Resources::ConfigKeys::WatcherIssueHandlersUnresponsiveServicesCheckInterval;

    auto impl = std::make_unique< IssueHandlers::UnresponsiveServicesOpenwrtImpl >( service );
    auto handler = std::make_unique< IssueHandlers::UnresponsiveServicesHandler >( std::move( impl ) );

    std::chrono::seconds timeout( Utils::readConfigIntValue( config, timeoutKey, 600 ) );
    handler->setDefaultHeartbeatTimeout( timeout );

    {  // Read per-service timeout
        auto defaultServiceTimeout = JSON_EMPTY_OBJ;
        defaultServiceTimeout[ Resources::ServiceNames::UpdateManager ] = 12 * 60 * 60;  // 12 hours to download FW

        auto jsonMap = Utils::readConfigObjectValue( config, serviceTimeoutKey, defaultServiceTimeout );
        LOGGER_DEBUG_RAW( "UnresponsiveServices: services timeout: " + cc::j( jsonMap ) );

        std::map< std::string, std::chrono::milliseconds > perServiceTimeout;
        for( auto it = jsonMap.begin(); it != jsonMap.end(); ++it ) {
            std::string serviceName = it.key();
            int value = it.value();
            perServiceTimeout[ serviceName ] = std::chrono::seconds( value );
        }

        handler->setPerProcessHeartbeatTimeout( perServiceTimeout );
    }

    std::chrono::seconds checkInterval( Utils::readConfigIntValue( config, checkIntervalKey, 15 ) );
    monitor.addHandler( std::move( handler ), checkInterval );

    LOGGER_DEBUG_RAW( "UnresponsiveServicesHandler: timeout: " + cc::num10( timeout.count() ) );
    LOGGER_DEBUG_RAW( "UnresponsiveServicesHandler: checkInterval: " + cc::num10( checkInterval.count() ) );
}

static void addZWaveNodeExpiredHandler( IssuesMonitor& monitor, ConfigReader& config, ServiceActivity& service ) {
    const auto timeoutKey = Resources::ConfigKeys::WatcherIssueHandlersZWaveNodeExpiredTimeout;
    const auto checkIntervalKey = Resources::ConfigKeys::WatcherIssueHandlersZWaveNodeExpiredCheckInterval;

    auto impl = std::make_unique< IssueHandlers::ZWaveNodeExpiredOpenwrtImpl >( service );
    auto handler = std::make_unique< IssueHandlers::ZWaveNodeExpiredHandler >( std::move( impl ) );
    std::chrono::seconds timeout( Utils::readConfigIntValue( config, timeoutKey, 3600 ) );
    handler->setZWaveNodeExpirationTimeout( timeout );

    std::chrono::seconds checkInterval( Utils::readConfigIntValue( config, checkIntervalKey, 15 ) );
    monitor.addHandler( std::move( handler ), checkInterval );

    LOGGER_DEBUG_RAW( "ZWaveNodeExpiredHandler: timeout: " + cc::num10( timeout.count() ) );
    LOGGER_DEBUG_RAW( "ZWaveNodeExpiredHandler: checkInterval: " + cc::num10( checkInterval.count() ) );
}

static void addWifiBcdcErrorHandler( IssuesMonitor& monitor,
                                     ConfigReader& config,
                                     IDbClient& dbClient,
                                     SystemLogInterface& systemLog ) {
    const auto checkIntervalKey = Resources::ConfigKeys::WatcherIssueHandlersWifiBcdcErrorCheckInterval;

    auto handler = std::make_unique< IssueHandlers::WifiBcdcErrorHandler >( dbClient, systemLog );

    std::chrono::seconds checkInterval( Utils::readConfigIntValue( config, checkIntervalKey, 30 ) );
    monitor.addHandler( std::move( handler ), checkInterval );

    LOGGER_DEBUG_RAW( "WifiBcdcErrorHandler: checkInterval: " + cc::num10( checkInterval.count() ) );
}

std::unique_ptr< IssuesMonitor > createIssuesMonitor( ConfigReader& config,
                                                      ServiceActivity& service,
                                                      IDbClient& dbClient,
                                                      SystemLogInterface& systemLog ) {
    auto monitor = std::make_unique< IssuesMonitor >();

    addUninterruptibleSleepHandler( *monitor, config, dbClient );
    addUnresponsiveServicesHandler( *monitor, config, service );
    addZWaveNodeExpiredHandler( *monitor, config, service );
    addWifiBcdcErrorHandler( *monitor, config, dbClient, systemLog );

    return monitor;
}

}  // namespace IssuesMonitorFactoryOpenwrt
}  // namespace Watcher
}  // namespace HubPlatform
