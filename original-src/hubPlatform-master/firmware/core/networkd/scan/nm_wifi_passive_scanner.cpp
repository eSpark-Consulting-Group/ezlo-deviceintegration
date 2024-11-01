#include "networkd/scan/nm_wifi_passive_scanner.h"
#include "networkd/scan/nm_scan_utils.h"
#if defined( __HUB_TYPE_G150__ ) || defined( __HUB_TYPE_G450__ ) || defined( __HUB_TYPE_G550__ )
#include "networkd/scan/nm_vera_wifi_passive_scan_task.h"
#elif defined( __HUB_TYPE_LOCAL__ ) || defined( __HUB_TYPE_VHUB__ )
#include "networkd/scan/nm_dummy_wifi_passive_scan_task.h"
#else
#include "networkd/scan/nm_wifi_passive_scan_task_base.h"
#endif
#include "networkd/resources/nm_resources.h"

#include "log_utils/ezlo_log.h"
#include "tiny/include/tiny/tiny_consolecolors.h"

#include "common_utils/cpp14.h"
#include "config_reader/config_reader.h"
#include <thread>

namespace HubPlatform {
namespace Network {

WifiPassiveScanner::WifiPassiveScanner( WifiScanResultListener& scanResultListener )
: m_scanResultListener( scanResultListener ) {}

void WifiPassiveScanner::startScan( const std::string& interfaceName ) {
    std::unique_lock< std::mutex > lock( m_mutex );
    LOGGER_INFO( cc::info( "Starting automatic wifi scan on interface: " ) + cc::note( interfaceName ) );

    auto it = m_scannerTasks.find( interfaceName );
    if( it == m_scannerTasks.end() ) {
        lock.unlock();
#if defined( __HUB_TYPE_G150__ ) || defined( __HUB_TYPE_G450__ ) || defined( __HUB_TYPE_G550__ )
        auto task = std::make_unique< VeraWifiPassiveScanTask >( interfaceName, m_scanResultListener );
#elif defined( __HUB_TYPE_LOCAL__ ) || defined( __HUB_TYPE_VHUB__ )
        auto task = std::make_unique< DummyWifiPassiveScanTask >( interfaceName, m_scanResultListener );
#else
        auto task = std::make_unique< WifiPassiveScanTaskBase >( interfaceName, m_scanResultListener );
#endif
        task->start();

        lock.lock();
        m_scannerTasks.emplace( interfaceName, std::move( task ) );
    }
    else {
        it->second->start();
    }
}

void WifiPassiveScanner::stopScan( const std::string& interfaceName ) {
    std::lock_guard< std::mutex > lock( m_mutex );

    LOGGER_INFO( cc::info( "Stopping automatic wifi scan on interface: " ) + cc::note( interfaceName ) );
    auto it = m_scannerTasks.find( interfaceName );
    if( it != m_scannerTasks.end() ) {
        it->second->stop();
    }
}

JsonArray WifiPassiveScanner::manualScan( const std::string& interfaceName, const std::string& ssid ) {
    LOGGER_INFO( cc::info( "Starting manual wifi scan on interface: " ) + cc::note( interfaceName ) +
                 cc::info( ", ssid: " ) + cc::note( ssid ) );
#if defined( __HUB_TYPE_G150__ ) || defined( __HUB_TYPE_G450__ ) || defined( __HUB_TYPE_G550__ )
    return VeraWifiPassiveScanTask::manualScan( interfaceName, ssid );
#elif defined( __HUB_TYPE_LOCAL__ ) || defined( __HUB_TYPE_VHUB__ )
    return DummyWifiPassiveScanTask::manualScan( interfaceName, ssid );
#else
    return WifiPassiveScanTaskBase::manualScan( interfaceName, ssid );
#endif
}

// TODO: make this functionality asynchronous
NetworkItem WifiPassiveScanner::scanAndGetNetwork( const std::string& interfaceName,
                                                   const std::string& ssid,
                                                   const std::string& bssid ) {
    /* sometimes a scan result doesn't contain a network with a specified ssid, so try several times;
     * even specifying a ssid scanning may return several networks regardless of a platform... */
    const int maxSleepDuration = 5;
    const int maxSleepDelta = 10;
    std::chrono::seconds sleepDuration = std::chrono::seconds( 1 );
    std::chrono::seconds sleepDelta = std::chrono::seconds( 2 );

    ConfigReader reader;
    using namespace Resources::Scan::ConfigKeys;
    if( reader.hasValue( NetworkWifiScanInterval ) ) {
        sleepDuration = std::chrono::seconds( std::min( reader.getInt( NetworkWifiScanInterval ), maxSleepDuration ) );
    }
    if( reader.hasValue( NetworkWifiScanDelta ) )
        sleepDelta = std::chrono::seconds( std::min( reader.getInt( NetworkWifiScanDelta ), maxSleepDelta ) );

    LOGGER_DEBUG_RAW( cc::debug( "Scan networks on interface: " ) + cc::note( interfaceName ) );
    for( auto i = 0; i < Resources::Scan::TryAttempts; i++ ) {
        const auto networkScan = manualScan( interfaceName, ssid );

        LOGGER_DEBUG_RAW( cc::debug( "Available wireless networks: " ) + cc::j( networkScan ) );

        auto network = getScannedNetwork( networkScan, ssid, bssid );
        if( network.isValid() && network.m_ssid == ssid ) {
            return network;
        }

        const bool isLastAttempt = i + 1 == Resources::Scan::TryAttempts;
        if( !isLastAttempt ) {
            LOGGER_DEBUG( cc::debug( "Attempt: " ) + cc::num10( i + 1 ) +
                          cc::debug( " to scan wireless network: sleep for " ) + cc::num10( sleepDuration.count() ) +
                          cc::debug( " seconds" ) );
            std::this_thread::sleep_for( sleepDuration );
            sleepDuration += sleepDelta;
        }
    }

    LOGGER_WARN_RAW( cc::warn( "Failed to find the '" ) + cc::note( ssid ) + cc::warn( "' wireless network" ) );

    return {};
}

}  // namespace Network
}  // namespace HubPlatform
