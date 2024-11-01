#include "watcher/service.h"

#include "watcher/resources/watcher_resources.h"
#include "watcher/issues_monitor/issues_monitor_factory_openwrt.h"
#include "watcher/utils/utils.h"

#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include "database_client/idbclient.h"
#include "base_utils/timeutils.h"

#include <cassert>

namespace HubPlatform {
namespace Watcher {

static Service*& serviceContext() {
    static Service* service = nullptr;
    return service;
}

void signalHandler( int signal ) {
    if( signal == SIGINT || signal == SIGTERM ) {
        HubPlatform::Watcher::setWatcherExitSignalStatus( true );
    }
    else {
        HubPlatform::Watcher::setWatcherUnhandledSignal( true );
    }
}

void stopUBusThread( std::shared_ptr< Service >& watcherObj ) {
    if( watcherObj != nullptr ) {
        watcherObj->stopWatcherUloop();
        if( watcherObj->ubusRunThread.joinable() ) {
            watcherObj->ubusRunThread.join();
        }
        watcherObj.reset();
        watcherObj = nullptr;
    }
}

threads::UniqueSafeWrapper< bool >& g_watcherExitSignalStatus() {
    static threads::UniqueSafeWrapper< bool > value( false );
    return value;
}

threads::UniqueSafeWrapper< bool >& g_watcherUnhandledSignal() {
    static threads::UniqueSafeWrapper< bool > signal( false );
    return signal;
}

void setWatcherExitSignalStatus( bool status ) {
    g_watcherExitSignalStatus().set( status );
}
bool getWatcherExitSignalStatus() {
    bool status;
    g_watcherExitSignalStatus().get( status );
    return status;
}
void setWatcherUnhandledSignal( bool status ) {
    g_watcherExitSignalStatus().set( status );
}
bool getWatcherUnhandledStatus() {
    bool status;
    g_watcherExitSignalStatus().get( status );
    return status;
}

Service::Service() {
    m_serviceStartupTime = std::chrono::steady_clock::now();

    // NOTE: ubus handlers do not accept user data, so statics is required
    serviceContext() = this;

    m_taskLoopThread = std::thread( [this]() { m_taskLoop.run(); } );

    m_messagePolicy[ SERVER_ID ] = { .name = "id", .type = BLOBMSG_TYPE_INT32 };
    m_messagePolicy[ SERVICE_MSG ] = { .name = "msg", .type = BLOBMSG_TYPE_STRING };

    m_serverMethods[ 0 ] = {
        "send_status", ( ubus_handler_t )( &Service::ubusSendStatusHandler ), 0, 0, m_messagePolicy, 0
    };
    m_serverMethods[ 1 ] = { "get_info", ( ubus_handler_t )( &Service::ubusGetInfoHandler ), 0, 0, m_messagePolicy, 0 };

    m_serverObjectType = { serverName, 0, m_serverMethods, 2 };

    m_dbClient = createDbClient();

    m_issuesMonitor =
        IssuesMonitorFactoryOpenwrt::createIssuesMonitor( m_configReader, m_service, *m_dbClient, m_systemLog );
    m_issuesMonitorThread = std::thread( [this]() { m_issuesMonitor->run(); } );
    m_systemLog.start( "logread ; logread -f" );

    const auto printIntervalKey = Resources::ConfigKeys::WatcherSettingsPeriodicReportInterval;
    std::chrono::seconds printInterval( Utils::readConfigIntValue( m_configReader, printIntervalKey, 600 ) );
    auto printTask = m_taskLoop.createPostponedTask( [this]( Services::PostponedTask& ) {
        LOGGER_INFO_RAW( cc::info( "Metrics periodic report: " ) + cc::j( getWatcherMetrics() ) );
    } );
    printTask->start( std::chrono::seconds( 10 ), printInterval );

    // print service uptime and logs statistics
    m_taskLoop
        .createPostponedTask( [this]( Services::PostponedTask& ) {
            auto currentTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast< std::chrono::seconds >( currentTime - m_serviceStartupTime );
            std::string readableUptime = TimeUtils::formatUptime( duration.count() );
            Log::info( cc::info( "Service uptime: " ) + cc::note( readableUptime ) );

            Log::printLogCounter();
        } )
        ->start( std::chrono::minutes( 60 ), true );
}

Service::~Service() {
    m_taskLoop.stop();
    if( m_taskLoopThread.joinable() )
        m_taskLoopThread.join();

    m_systemLog.stop();
    m_issuesMonitor->stop();
    if( m_issuesMonitorThread.joinable() )
        m_issuesMonitorThread.join();
}

int Service::ubusGetInfoHandler( struct ubus_context* ctx,
                                 struct ubus_object* obj,
                                 struct ubus_request_data* req,
                                 const char* method,
                                 struct blob_attr* msg ) {
    try {
        auto bufferBlob = std::make_unique< blob_buf >();

        JsonObject serviceReportJson;
        std::string systemReportString;

        Service* context = serviceContext();
        if( !context ) {
            LOGGER_FATAL( "Service context is not initialized" );
            std::exit( -1 );  // really?
        }

        context->getWatcherInfoRef().sentReports++;
        context->getWatcherInfoRef().lastSentReport = std::time( nullptr );

        context->m_service.getServiceReportData( serviceReportJson );
        context->m_system.getSystemReportData( systemReportString );

        blob_buf_init( bufferBlob.get(), 0 );
        blobmsg_add_json_from_string( bufferBlob.get(), serviceReportJson.dump().c_str() );
        blobmsg_add_json_from_string( bufferBlob.get(), systemReportString.c_str() );

        ubus_send_reply( ctx, req, bufferBlob->head );

        blob_buf_free( bufferBlob.get() );
        return UBUS_STATUS_OK;
    }
    catch( ... ) {
        Log::error( cc::error( "Could not collect hub information!" ) );
        return UBUS_STATUS_UNKNOWN_ERROR;
    }
}

int Service::ubusSendStatusHandler( struct ubus_context* ctx,
                                    struct ubus_object* obj,
                                    struct ubus_request_data* req,
                                    const char* method,
                                    struct blob_attr* msg ) {
    try {
        Service* context = serviceContext();
        if( !context ) {
            LOGGER_FATAL( "Service context is not initialized" );
            std::exit( -1 );
        }

        context->getWatcherInfoRef().receivedReports++;
        context->getWatcherInfoRef().lastReceivedReport = std::time( nullptr );

        blob_attr* serverBuffer[ __MESSAGE_POLICY_MAX ];
        std::string msgStr;
        JsonObject jsonInfo;

        blobmsg_parse( context->m_messagePolicy,
                       ARRAY_SIZE( context->m_messagePolicy ),
                       serverBuffer,
                       blob_data( msg ),
                       blob_len( msg ) );

        if( serverBuffer[ SERVICE_MSG ] != nullptr ) {
            msgStr = static_cast< char* >( blobmsg_data( serverBuffer[ SERVICE_MSG ] ) );
        }

        if( msgStr.empty() ) {
            Log::error( cc::error( "Could not get service info!" ) );
            return UBUS_STATUS_INVALID_ARGUMENT;
        }

        parseJson( msgStr, jsonInfo );
        if( jsonInfo.find( Watcher::Resources::ServiceActivity::service ) == jsonInfo.end() ) {
            Log::error( cc::error( "Could not get service information!" ) );
            return UBUS_STATUS_INVALID_ARGUMENT;
        }

        context->m_service.addServiceInfo( jsonInfo );
        return UBUS_STATUS_OK;
    }
    catch( ... ) {
        Log::error( cc::error( "Could not collect hub information!" ) );
        return UBUS_STATUS_UNKNOWN_ERROR;
    }
}

void Service::stopWatcherUloop() {
    m_uBusObject->breakUloop();
}

// TODO: This makes watcher unresponsive, fix it.
/*
int Service::getWatcherConnection() {
    int connectionStatus = m_uBusObject->checkConnection( serverName );
    return connectionStatus;
}
*/

// ubus_lookup_id() causes blocking so using std::system for now.
int Service::getWatcherUbusConnectionStatus() {
    int status = -100;
    int waitFor = 0;
    // Sometimes command returns -1, better to check a couple of times
    do {
        status = std::system( "ubus list watcher >/dev/null 2>/dev/null" );
        status = WEXITSTATUS( status );
        std::this_thread::sleep_for( std::chrono::milliseconds( WAIT_FOR ) );

    } while( status != 0 && ++waitFor < 5 );
    return status;
}

void Service::uBusRun() {
    m_serverObject.name = serverName;
    m_serverObject.type = &m_serverObjectType;
    m_serverObject.methods = Service::m_serverMethods;
    m_serverObject.n_methods = ARRAY_SIZE( Service::m_serverMethods );

    m_uBusObject = std::make_unique< Ubus::UBusImpl >();

    signal( SIGPIPE, SIG_IGN );

    if( !m_uBusObject->connect( nullptr ) ) {
        Log::error( cc::error( "Could not connect to the uBus socket!" ) );
        return;
    }

    // handle = this;
    m_uBusObject->addUBusObject( Service::m_serverObject );
}

Json Service::getWatcherMetrics() {
    auto toAge = []( std::time_t time ) -> std::string {
        return ( time ) ? TimeUtils::formatUptime( std::time( nullptr ) - time ) : "";
    };

    Json metrics;

    Json serviceReportData;
    m_service.getServiceReportData( serviceReportData );

    auto serviceReportsIt = serviceReportData.find( Watcher::Resources::ServiceActivity::service );
    if( serviceReportsIt != serviceReportData.end() ) {
        const Json& serviceReports = *serviceReportsIt;

        Json services;
        for( auto reportIt = serviceReports.cbegin(); reportIt != serviceReports.cend(); ++reportIt ) {
            Json j;

            int reportDate = json_get_int( reportIt.value(), Watcher::Resources::ServiceActivity::reportDate, 0 );
            j[ "reportDate" ] = reportDate;
            j[ "reportAge" ] = toAge( reportDate );

            j[ "respawnCount" ] = json_get_int( reportIt.value(), Watcher::Resources::ServiceActivity::respawncount );
            j[ "runningStatus" ] =
                json_get_bool( reportIt.value(), Watcher::Resources::ServiceActivity::serviceRunningStatus );
            j[ "serviceStatus" ] = json_get_str( reportIt.value(), Watcher::Resources::ServiceActivity::serviceStatus );
            j[ "uptime" ] = json_get_int( reportIt.value(), Watcher::Resources::ServiceActivity::serviceUptime );

            services[ reportIt.key() ] = std::move( j );
        }

        metrics[ "services" ] = std::move( services );
    }

    auto& watcherInfo = metrics[ "watcherInfo" ];
    watcherInfo[ "receivedReports" ] = int( m_watcherInfo.receivedReports );
    watcherInfo[ "lastReceivedReportDate" ] = int( m_watcherInfo.lastReceivedReport );
    watcherInfo[ "lastReceivedReportAge" ] = toAge( m_watcherInfo.lastReceivedReport );

    watcherInfo[ "sentReports" ] = int( m_watcherInfo.sentReports );
    watcherInfo[ "lastSentReportDate" ] = int( m_watcherInfo.lastSentReport );
    watcherInfo[ "lastSentReportAge" ] = toAge( m_watcherInfo.lastSentReport );

    return metrics;
}

Service::WatcherInfo& Service::getWatcherInfoRef() {
    return m_watcherInfo;
}

}  // namespace Watcher
}  // namespace HubPlatform
