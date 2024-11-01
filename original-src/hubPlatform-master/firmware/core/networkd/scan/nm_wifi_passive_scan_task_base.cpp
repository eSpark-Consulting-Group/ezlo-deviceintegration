#include "networkd/scan/nm_wifi_passive_scan_task_base.h"
#include "networkd/scan/nm_scan_utils.h"

#include "base_utils/prioritized_task_pool.h"

#include "log_utils/ezlo_log.h"
#include "tiny/include/tiny/tiny_consolecolors.h"

#include <thread>
#include <atomic>
#include <future>

namespace HubPlatform {
namespace Network {

namespace {

class WifiScanHandler {
public:
    WifiScanHandler( std::string interfaceName, std::string ssid, int priority ) noexcept
    : m_interfaceName( std::move( interfaceName ) )
    , m_ssid( std::move( ssid ) )
    , m_priority( priority ) {
        static std::atomic_int order{ 0 };
        m_order = ++order;
        m_promise = std::make_shared< std::promise< NetworkMap > >();
    }

    void operator()() {
        try {
            m_promise->set_value( scanWifiToMap( m_interfaceName, m_ssid ) );
        }
        catch( ... ) {
            m_promise->set_exception( std::current_exception() );
        }
    }

    bool operator>( const WifiScanHandler& other ) const {
        return m_priority > other.m_priority && m_order > other.m_order;
    }

    std::future< NetworkMap > getFuture() {
        return m_promise->get_future();
    }

    void stop(){};

private:
    std::string m_interfaceName;
    std::string m_ssid;
    int m_priority = 0;
    int m_order = 0;
    std::shared_ptr< std::promise< NetworkMap > > m_promise;
};

NetworkMap runQueuedWifiScan( std::string interfaceName, std::string ssid, int priority ) {
    static common::PrioritizedTaskPool< WifiScanHandler > queue;
    WifiScanHandler handler( interfaceName, std::move( ssid ), priority );
    auto future = handler.getFuture();
    queue.submit( std::move( handler ) );
    auto result = future.wait_for( std::chrono::seconds( 60 ) );
    if( result != std::future_status::ready ) {
        LOGGER_WARN_RAW( cc::warn( "wifi scan timeout on interface " ) + cc::note( interfaceName ) );
        return {};
    }
    return future.get();
}

// serialize access to a linux network interface
std::mutex& getWifiInterfaceMutex() {
    static std::mutex mutex;
    return mutex;
}

}  // namespace

WifiPassiveScanTaskBase::WifiPassiveScanTaskBase( std::string interfaceName,
                                                  WifiScanResultListener& scanResultListener )
: WifiPassiveScanTask( std::move( interfaceName ), scanResultListener )
, m_scanStartedDuration( std::chrono::seconds( 0 ) ) {}

WifiPassiveScanTaskBase::~WifiPassiveScanTaskBase() {
    stopWorkerAndWait();
}

void WifiPassiveScanTaskBase::start() {
    if( m_isRunning ) {
        // Reset elapsed time of scanner's timeout.
        m_scanStartedDuration = std::chrono::steady_clock::now().time_since_epoch();
        return;
    }
    else {
        if( m_worker.joinable() ) {
            m_worker.join();
        }
    }

    m_worker = std::thread( [this] {
        LOGGER_DEBUG_RAW( cc::debug( "wifi passive scan started on interface " ) + cc::note( m_interfaceName ) );
        m_stopped = false;
        m_isRunning = true;
        std::unique_ptr< decltype( m_isRunning ), void ( * )( decltype( m_isRunning )* ) > guard(
            &m_isRunning, []( decltype( m_isRunning )* val ) { *val = false; } );

        m_scanStartedDuration = std::chrono::steady_clock::now().time_since_epoch();

        beforeScan();
        while( !m_stopped ) {
            doScan();
            sendBroadcast();
            if( m_stopped )
                break;
            std::this_thread::sleep_for( getScanInterval() );

            auto scanDuration = std::chrono::steady_clock::now().time_since_epoch() - m_scanStartedDuration.load();
            if( scanDuration >= getScanTimeout() ) {
                LOGGER_DEBUG_RAW( cc::debug( "wifi passive scan timeout on interface " ) + cc::note( m_interfaceName ) +
                                  cc::debug( ", stopping" ) );
                stop();
            }
        }
        LOGGER_DEBUG_RAW( cc::debug( "wifi passive scan stopped on interface " ) + cc::note( m_interfaceName ) );
        onStopped();
    } );
}

void WifiPassiveScanTaskBase::stop() {
    m_stopped = true;
}

std::chrono::milliseconds WifiPassiveScanTaskBase::getScanInterval() const {
    return std::chrono::seconds( 8 );
}

std::chrono::seconds WifiPassiveScanTaskBase::getScanTimeout() const {
    return std::chrono::seconds( 300 );
}

void WifiPassiveScanTaskBase::beforeScan() {
    m_scanResultListener.onScanStarted( m_interfaceName );
}

void WifiPassiveScanTaskBase::doScan() {
    std::unique_lock< std::mutex > lock( getWifiInterfaceMutex() );

    try {
        auto newlyScanned = runQueuedWifiScan( m_interfaceName, "", ScanPriority::Background );
        // Add newly appeared networks and update already added (signal level may change).
        for( const auto& item : newlyScanned ) {
            m_scanned[ item.first ] = item.second;
        }
    }
    catch( const std::exception& ex ) {
        lock.unlock();
        m_scanResultListener.onScanFailed( m_interfaceName, ex.what() );
    }
}

void WifiPassiveScanTaskBase::sendBroadcast() {
    if( !m_stopped && !m_scanned.empty() ) {
        JsonArray networks;
        for( const auto& it : m_scanned ) {
            networks.push_back( it.second.toJson( false ) );
        }
        m_scanResultListener.onScanProcess( m_interfaceName, std::move( networks ) );
    }

    // Clear scanned networks.
    m_scanned.clear();
}

void WifiPassiveScanTaskBase::onStopped() {
    m_scanned.clear();
    m_scanResultListener.onScanFinished( m_interfaceName );
}

void WifiPassiveScanTaskBase::stopWorkerAndWait() {
    if( m_worker.joinable() ) {
        m_stopped = true;
        m_worker.join();
    }
}

Json WifiPassiveScanTaskBase::manualScan( const std::string& interfaceName, const std::string& ssid ) {
    std::lock_guard< std::mutex > lock( getWifiInterfaceMutex() );

    Json result;
    try {
        JsonArray networks;
        auto scanResult = runQueuedWifiScan( interfaceName, ssid, ScanPriority::Manual );
        for( const auto& item : scanResult ) {
            networks.push_back( item.second.toJson( false ) );
        }
        result[ "networks" ] = std::move( networks );
    }
    catch( const WiFiScanResourceBusyException& ex ) {
        LOGGER_ERROR( cc::error( "wifi scan resource is busy: " ) + ex.what() );
    }
    catch( const WiFiScanException& ex ) {
        LOGGER_ERROR( cc::error( "WiFi scan exception caught: " ) + ex.what() );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Unhandled silent exception: " ) + ex.what() );
    }

    return result;
}

}  // namespace Network
}  // namespace HubPlatform
