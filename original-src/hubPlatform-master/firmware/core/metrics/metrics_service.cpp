#include "defaults.h"
#include "metrics_service.h"
#include "config_reader/config_keys.h"
#include "custom_reporter.h"
#include "system_utils/system.h"
#include "cored/remote_interface/cored_remote_interface.h"

namespace HubPlatform {

namespace Metrics {

Service::Service()
: Services::Service( GroupsNames::METRICS )
, m_internalMetrics( std::make_shared< InternalMetrics >() )
, m_receiver( m_internalMetrics ) {}

void Service::setUp() {
    Services::Service::setUp();
    LOGGER_TRACE( "Service setup" );

    ConfigReader configReader;

    try {
        if( configReader.hasValue( ConfigKeys::MetricsCloudUrl ) ) {
            m_cloudUrl = configReader.getString( ConfigKeys::MetricsCloudUrl );
        }

        if( configReader.hasValue( ConfigKeys::MetricsCloudPorts ) ) {
            auto portsArrayPath = ConfigReader::Path( configReader, ConfigKeys::MetricsCloudPorts );
            auto ports = ConfigReader::getStringArray( portsArrayPath );
            for( const auto& port : ports ) {
                m_cloudPorts.push_back( std::stoi( port ) );
            }
        }

        if( configReader.hasValue( ConfigKeys::MetricsReporterServerCrt ) ) {
            m_serverCrtFile = configReader.getString( ConfigKeys::MetricsReporterServerCrt );
        }

        if( configReader.hasValue( ConfigKeys::MetricsReporterClientCrt ) ) {
            m_clientCrtFile = configReader.getString( ConfigKeys::MetricsReporterClientCrt );
        }

        if( configReader.hasValue( ConfigKeys::MetricsReporterClientKey ) ) {
            m_clientKeyFile = configReader.getString( ConfigKeys::MetricsReporterClientKey );
        }

        if( configReader.hasValue( ConfigKeys::MetricsReporterCustomServices ) ) {
            auto customServicesArrayPath =
                ConfigReader::Path( configReader, ConfigKeys::MetricsReporterCustomServices );
            auto customServices = ConfigReader::getJsonArray( customServicesArrayPath );
            for( auto& service : customServices ) {
                auto name = json_get_str( service, "service" );
                auto feature = json_get_str( service, "feature" );
                m_serviceFeatureMap[ name ] = feature;
            }
        }
    }
    catch( std::exception& ex ) {
        Log::error( cc::error( "Unable found config option " ) + cc::error( ": " ) + cc::attention( ex.what() ) );
    }

    setConfigIntOption( configReader,
                        ConfigKeys::MetricsSendIntervalNetworkDefault,
                        m_sendIntervalNetworkDefault,
                        DefaultSendIntervalNetworkDefault );

    setConfigIntOption(
        configReader, ConfigKeys::MetricsReporterGetInterval, m_getRequestInterval, DefaultGetInterval );

    setConfigIntOption( configReader,
                        ConfigKeys::MetricsSendIntervalNetworkAnomaly,
                        m_sendIntervalNetworkAnomaly,
                        DefaultSendIntervalNetworkAnomaly );

    m_promiseDataLoop = std::make_shared< std::promise< void > >();

    m_getMetricsLoopThread = std::thread( [this] {
        LOGGER_INFO_RAW( "Start metrics loop thread" );

        m_receiver.getSender()->setSendInterval( m_sendIntervalNetworkDefault );
        m_receiver.getSender()->setServerUrlAndPort( m_cloudUrl, m_cloudPorts );
        m_receiver.getSender()->setCertifications( m_serverCrtFile, m_clientCrtFile, m_clientKeyFile );

        LOGGER_TRACE_RAW( "Receiver: startGetDataLoop in another thread." );

        m_receiver.startGetDataLoop( m_getRequestInterval );

        LOGGER_TRACE_RAW( "Exit from startGetDataLoop" );

        m_promiseDataLoop->set_value();

        LOGGER_TRACE_RAW( "Promise value set, exit from thread" );
    } );

    m_watchdogThread = std::thread( [this] {
        Log::info( "Start metrics watchdog loop thread" );

        while( m_receiver.isRunning() ) {
            Log::info( "Before get metrics" );
            if( auto* metrics = m_receiver.getInternalMetrics() ) {
                Log::info( "Before metrics->attempts" );
                auto busAttempts = metrics->bus().attempts();
                std::time_t latestAttempt = metrics->bus().attemptAt();

                const auto BusAttemptInterval = 600;
                time_t now = std::time( nullptr );
                Log::info( "Latest bus request attempt was " + std::to_string( now - latestAttempt ) +
                           "s ago, attempts: " + std::to_string( busAttempts ) );
                if( busAttempts > 0 && latestAttempt < now - BusAttemptInterval ) {
                    Log::warn( "Latest bus attempt was more than " + std::to_string( BusAttemptInterval ) +
                               " seconds ago. Recovering bus." );
                }
            }

            Log::info( "Before wait for condition" );
            const auto RepeatInterval = std::chrono::seconds( 30 );
            {
                std::unique_lock< std::mutex > lock( m_intervalMutex );
                m_watchdogCondition.wait_for( lock, RepeatInterval, [this] { return !m_receiver.isRunning(); } );
            }
            LOGGER_TRACE_RAW( "Continue watchdog loop" );
        }

        LOGGER_TRACE_RAW( "Watchdog thread: promise value set, exit from thread" );
    } );

    createPostponedTask( [this]( Services::PostponedTask& ) { prepareCustomServicesReporter(); } )
        ->start( std::chrono::seconds( 10 ), false );
    LOGGER_TRACE( "Service setup: complete" );
}

void Service::tearDown() {
    LOGGER_INFO( "Service teardown" );

    m_receiver.stopGetDataLoop();

    m_watchdogCondition.notify_all();

    m_promiseDataLoop->get_future().wait_for( m_quitLoopDurationSec );
    if( m_getMetricsLoopThread.joinable() )
        m_getMetricsLoopThread.join();

    if( m_watchdogThread.joinable() )
        m_watchdogThread.join();

    auto serviceInfo = getServiceInfo();
    LOGGER_INFO_RAW( "Service info before teardown: " + cc::j( serviceInfo ) );

    Services::Service::tearDown();
    LOGGER_TRACE( "Service teardown: complete" );
}

void Service::setConfigIntOption( ConfigReader& configReader,
                                  const char* optionPath,
                                  int& setValue,
                                  int defaultValue ) {
    try {
        setValue = configReader.getInt( optionPath );
    }
    catch( std::exception& ex ) {
        setValue = defaultValue;
        Log::error( cc::error( "Unable found config option " ) + cc::note( optionPath ) + cc::error( ": " ) +
                    cc::attention( ex.what() ) );
        Log::error( cc::error( "Default value is set: " ) + cc::num( defaultValue ) );
    }
}

// Extract to system_utils
bool Service::isServiceRunning( const std::string& serviceName ) {
    auto pid = CustomReporter::fetchPidByProcessName( serviceName );
    return pid > 0;
}

void Service::prepareCustomServicesReporter() {
    LOGGER_INFO_RAW( "Initialize external service reporters" );

    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( *this );

    for( auto& serviceFeaturePair : m_serviceFeatureMap ) {
        const auto serviceName = serviceFeaturePair.first;

        LOGGER_DEBUG_RAW( "External service: " + serviceName );

        const std::string& feature = serviceFeaturePair.second;
        if( feature.empty() || remoteInterface.checkFeatureStatus( feature ) ) {
            if( isServiceRunning( serviceName ) ) {
                Service::addReporter( std::make_unique< CustomReporter >( serviceName ) );
                LOGGER_DEBUG_RAW( "Custom Reporter added for service: " + serviceName );
            }
            else {
                LOGGER_INFO( serviceName + " is not running" );
            }
        }
    }

    LOGGER_TRACE_RAW( "External service reporter: finished" );
}

JsonObject Service::getServiceInfo() {
    Json info = JSON_EMPTY_OBJ;
    info[ "metrics" ] = m_internalMetrics->toJson();
    return info;
}

}  // namespace Metrics
}  // namespace HubPlatform
