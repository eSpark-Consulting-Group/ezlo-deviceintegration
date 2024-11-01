#include "networkd/monitoring/nm_openwrt_interface_monitor.h"
#include "networkd/monitoring/nm_monitor_utils.h"
#include "networkd/monitoring/nm_ping.h"
#include "networkd/monitoring/nm_ping_hosts.h"
#include "networkd/monitoring/nm_tcp_check.h"
#include "networkd/resources/nm_resources.h"

#include "json_utils/json_utils.h"
#include "base_utils/ezlodb.h"
#include "database_client/db_flags_names.h"

#include <thread>
#include <utility>

namespace HubPlatform {
namespace Network {
namespace {
const auto UpdatePeriod = std::chrono::seconds( 1 );
}  // namespace

OpenwrtInterfaceMonitor::OpenwrtInterfaceMonitor( Services::TaskLoop& taskLoop,
                                                  InterfaceInfoListener& interfaceChangeListener,
                                                  std::string interfaceName,
                                                  ConfigReader& configReader,
                                                  ExternalDataProvider& externalDataProvider,
                                                  bool pingEnabled,
                                                  std::chrono::seconds pingPeriod,
                                                  IPVersion ipVersion )
: InterfaceMonitor( taskLoop,
                    interfaceChangeListener,
                    std::move( interfaceName ),
                    configReader,
                    externalDataProvider,
                    pingEnabled,
                    pingPeriod,
                    ipVersion )
, m_updateTimer( m_taskLoop, [this]() { update(); } )
, m_nextPingTime( std::chrono::steady_clock::now() ) {
    m_updateTimer.setPeriodicMode( Services::PostponedTask::PeriodicMode::FixedIdleDuration );
}

void OpenwrtInterfaceMonitor::start() {
    m_updateTimer.start( std::chrono::seconds( 0 ), UpdatePeriod );
}

void OpenwrtInterfaceMonitor::stop() {
    m_updateTimer.stop();

    auto& pingHealth = m_pingStatistics.getPingHealthExternal();
    LOGGER_INFO( cc::info( "Stopping monitor. Print ping stats: hits: " ) + cc::num10( pingHealth.hits ) );
    if( pingHealth.hits > 0 )
        m_pingStatistics.print();
}

bool OpenwrtInterfaceMonitor::isActive() {
    return m_updateTimer.isActive();
}

void OpenwrtInterfaceMonitor::runPingTest( Json& actualInfo ) {
    using namespace Resources::InterfaceInfo;

    if( m_pingEnabled && actualInfo[ Fields::Status ] == Statuses::Up ) {
        if( json_get_str( m_interfaceInfo, Fields::Status ) != Statuses::Up ||
            std::chrono::steady_clock::now() >= m_nextPingTime ) {
            m_nextPingTime = std::chrono::steady_clock::now() + m_pingPeriod;

            const std::string& interfaceName = actualInfo[ Fields::Id ];
            const bool stopOnFirstHit = true;

            auto externalHosts = getPublicPingHosts( interfaceName, m_ipVersion );
            bool externalPingStatus = ping( interfaceName, externalHosts, stopOnFirstHit, &m_pingStatistics );

            auto localHosts = getLocalPingHosts( interfaceName, m_ipVersion );
            bool localPingStatus = ping( interfaceName, localHosts, !stopOnFirstHit, &m_pingStatistics );

            // Return previous behaviour, before EFWR-4347 (2024-02-02): return false on failed ping immediately.
            actualInfo[ Fields::InternetAvailable ] = externalPingStatus;
        }
        else {
            actualInfo[ Fields::InternetAvailable ] = m_interfaceInfo[ Fields::InternetAvailable ];
        }
    }
    else {
        actualInfo[ Fields::InternetAvailable ] = false;
    }
}

/** Update internet availability on the interface.
 *      Step 1: Run ping tests on the interface;
 *      Step 2: If Step 1 failed, run TCP tests on the interface
 *      Step 3: If Step 2 failed, run cloud connection test, if cloud connection is started on this interface
 *
 *  Should be synchronous task to return actual interface monitor state to getInfo method.
 * */
void OpenwrtInterfaceMonitor::update( bool silent ) {
    try {
        using namespace Resources::InterfaceInfo;

        std::lock_guard< std::recursive_mutex > lock( m_interfaceInfoMutex );

        auto actualInfo = getActualNetworkIfaceConfig( m_interfaceName );
        auto interfaceName = json_get_str( actualInfo, Fields::Id );
        bool pingScheduled = isNextPingTime();

        const bool isPingEnabled = m_pingEnabled && actualInfo[ Fields::Status ] == Statuses::Up;

        runPingTest( actualInfo );

        bool doCloudTest = false;

        bool internetAvailable = isInternetAvailable( actualInfo );
        if( isPingEnabled && !internetAvailable && pingScheduled ) {
            runTcpTest( actualInfo );

            internetAvailable = isInternetAvailable( actualInfo );

            doCloudTest = !internetAvailable;
        }

        if( doCloudTest ) {
            runCloudConnectionTest( actualInfo );
        }

        const auto diff = jsonDiff( m_interfaceInfo, actualInfo );
        const bool isInfoChanged = exchangeInterfaceInfo( actualInfo );
        if( isInfoChanged && !silent ) {
            LOGGER_DEBUG( cc::debug( "isInfoChanged, diff with previous interface state: " ) + cc::j( diff ) );
            m_interfaceChangeListener.onNewInterfaceInfo( m_interfaceInfo );
        }
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Exception in interfaceMonitor::update:  " ) + ex.what() );
    }
}

const Json& OpenwrtInterfaceMonitor::manualUpdate() {
    std::lock_guard< std::recursive_mutex > lock( m_interfaceInfoMutex );

    m_nextPingTime = std::chrono::steady_clock::now();
    update();
    return m_interfaceInfo;
}

void OpenwrtInterfaceMonitor::setPingPeriod( std::chrono::seconds pingPeriod ) {
    std::lock_guard< std::recursive_mutex > lock( m_interfaceInfoMutex );

    m_nextPingTime = std::chrono::steady_clock::now();
    InterfaceMonitor::setPingPeriod( pingPeriod );
}

/*
Examples:
{
    "_id": "wlan0",
    "hwaddr": "8C:F7:10:E4:ED:90",
    "ipv4": {
        ...
    },
    "status": "up",
    "internetAvailable", true
}
{
    "status": "down",
    "internetAvailable", false
}
*/
const Json& OpenwrtInterfaceMonitor::getInfo() {
    std::lock_guard< std::recursive_mutex > lock( m_interfaceInfoMutex );

    if( m_interfaceInfo.empty() ) {
        m_nextPingTime = std::chrono::steady_clock::now();
        update( true );
    }

    return m_interfaceInfo;
}

bool OpenwrtInterfaceMonitor::isInternetAvailable() {
    return isInternetAvailable( getInfo() );
}

namespace {
void printTcpStat( Statistics& tcpStat ) {
    Log::info( cc::info( "TCP Check statistics: succeeded: " ) + cc::num10( tcpStat.succeeded() ) + ", failed: " +
               cc::num10( tcpStat.failed() ) + ", fail sequence: " + cc::num10( tcpStat.sequentialFailures() ) );
}
}  // namespace

void OpenwrtInterfaceMonitor::runTcpTest( Json& actualInfo ) {
    using namespace Resources::InterfaceInfo;

    if( json_get_str( actualInfo, Fields::Status ) != Statuses::Up )
        return;

    auto interfaceName = json_get_str( actualInfo, "_id" );

    HttpMethodOptions options;
    options.configure( m_httpMethodConfig );

    TcpInternetAvailabilityCheck tcpChecker;
    tcpChecker.setOptions( options );
    bool tcpCheck = tcpChecker.checkCloudAvailability( interfaceName, m_ipVersion, &m_tcpCheckStatistics );

    if( m_tcpCheckStatistics.total() % 20 == 0 ) {
        printTcpStat( m_tcpCheckStatistics );
    }

    actualInfo[ Fields::InternetAvailable ] = tcpCheck;
}

void OpenwrtInterfaceMonitor::runCloudConnectionTest( Json& actualInfo ) {
    using namespace Resources::InterfaceInfo;
    if( json_get_str( actualInfo, Fields::Status ) != Statuses::Up )
        return;

    const bool isCloudConnected = Db::getFlag( DbFlags::NmaClientConnected );

    actualInfo[ Fields::InternetAvailable ] = isCloudConnected;
}

/** Checks if cloud connected, OR if cloud connected on a specific interface */
bool OpenwrtInterfaceMonitor::isCloudConnected( const std::string& interfaceName ) {
    Json status;
    bool isConnected = m_externalDataProvider.isNmaConnected( &status );

    if( isConnected ) {
        auto activeInterface = json_get_str( status, "activeInterface" );

        if( !activeInterface.empty() && !interfaceName.empty() ) {
            if( activeInterface == interfaceName ) {
                Log::info( cc::info( "Cloud connected on the same interface! Interface Name: " ) + activeInterface );
                return true;
            }
            else {
                Log::info( cc::info( "Cloud connected, but on another interface: " ) + activeInterface );
                return false;
            }
        }
    }

    return isConnected;
}

bool OpenwrtInterfaceMonitor::configure() {
    const char* const NetworkInternetAvailabilityHttpMethodKey = "network/internetAvailability/httpMethod";
    m_httpMethodConfig =
        ConfigReader::getJsonObject( ConfigReader::Path( m_configReader, NetworkInternetAvailabilityHttpMethodKey ) );

    LOGGER_INFO( cc::info( "Http method config: " ) + cc::j( m_httpMethodConfig ) );

    return true;
}

const PingStatistics& OpenwrtInterfaceMonitor::pingStatistics() {
    return m_pingStatistics;
}

bool OpenwrtInterfaceMonitor::isNextPingTime() const {
    using namespace Resources::InterfaceInfo;

    if( json_get_str( m_interfaceInfo, Fields::Status ) != Statuses::Up ||
        std::chrono::steady_clock::now() >= m_nextPingTime ) {
        return true;
    }
    return false;
}

bool OpenwrtInterfaceMonitor::isInternetAvailable( const Json& actualInfo ) {
    using namespace Resources::InterfaceInfo;
    return json_get_bool( actualInfo, Fields::InternetAvailable, false );
}

bool OpenwrtInterfaceMonitor::exchangeInterfaceInfo( const Json& actualInfo ) {
    if( m_interfaceInfo != actualInfo ) {
        m_interfaceInfo = actualInfo;
        return true;
    }
    return false;
}

}  // namespace Network
}  // namespace HubPlatform
