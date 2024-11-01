#include "2gig/2gig_devices_reachability_manager.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

namespace HubPlatform {
namespace TwoGig {

constexpr std::chrono::minutes DevicesReachabilityManagerImpl::ms_DefaultUnreachableInterval;

DevicesReachabilityManagerImpl::DevicesReachabilityManagerImpl(::TwoGig::Client& client,
                                                               ::TwoGig::DevicesRepository& devicesRepository,
                                                               std::chrono::steady_clock::duration unreachableInterval )
: m_client( client )
, m_devices( devicesRepository )
, m_unreachableInterval( unreachableInterval ) {
    init();
}

DevicesReachabilityManagerImpl::~DevicesReachabilityManagerImpl() = default;

void DevicesReachabilityManagerImpl::checkAllDevicesReachableStates() {
    yamc::shared_lock< yamc::alternate::shared_mutex > sharedLocker( m_cacheMutex );
    Log::debug( cc::debug( "Start all node's reachability check" ) );

    auto now = std::chrono::steady_clock::now();
    for( auto const& nodeInfo : m_nodeToLatsUpdateTime ) {
        if( ( now - nodeInfo.second ) > m_unreachableInterval ) {
            Log::info( cc::info( "Mark node as unreachable: " + cc::num10( nodeInfo.first ) ) );

            m_nodesToIgnoreTrueReachable.erase( nodeInfo.first );
            m_client.onDeviceReachableChanged( nodeInfo.first, false );
        }
    }
}

void DevicesReachabilityManagerImpl::getDevicesUpdateTimes( JsonObject& twoGigDeviceUpdateTimes ) try {
    yamc::shared_lock< yamc::alternate::shared_mutex > sharedLocker( m_cacheMutex );
    for( auto const& device : m_nodeToLatsUpdateTime ) {
        twoGigDeviceUpdateTimes[ std::to_string( device.first ) ] =
            JSON( "last_update"
                  << std::chrono::time_point_cast< std::chrono::seconds >( device.second ).time_since_epoch().count()
                  << "reachable"
                  << ( m_nodesToIgnoreTrueReachable.find( device.first ) != m_nodesToIgnoreTrueReachable.end() ) );
    }
}
catch( std::exception& e ) {
    LOGGER_ERROR( "Exception caught, " + cc::error( e.what() ) );
}

void DevicesReachabilityManagerImpl::onDeviceAdded( const std::vector< uint32_t >& txIds ) {
    yamc::shared_lock< yamc::alternate::shared_mutex > sharedLocker( m_cacheMutex );
    auto now = std::chrono::steady_clock::now();

    for( const auto& txId : txIds ) {
        m_nodeToLatsUpdateTime[ txId ] = now;
    }
}

void DevicesReachabilityManagerImpl::onDeviceRemoved( const std::vector< uint32_t >& txIds ) {
    yamc::shared_lock< yamc::alternate::shared_mutex > sharedLocker( m_cacheMutex );
    for( const auto& txId : txIds ) {
        m_nodeToLatsUpdateTime.erase( txId );
        m_nodesToIgnoreTrueReachable.erase( txId );
    }
}

void DevicesReachabilityManagerImpl::onDeviceReceivedData( uint32_t txId ) {
    yamc::shared_lock< yamc::alternate::shared_mutex > sharedLocker( m_cacheMutex );
    m_nodeToLatsUpdateTime[ txId ] = std::chrono::steady_clock::now();
    if( m_nodesToIgnoreTrueReachable.find( txId ) == m_nodesToIgnoreTrueReachable.end() ) {
        Log::info( cc::info( "Mark node as reachable: " + cc::num10( txId ) ) );

        m_nodesToIgnoreTrueReachable.emplace( txId );
        m_client.onDeviceReachableChanged( txId, true );
    }
}

void DevicesReachabilityManagerImpl::init() {
    yamc::shared_lock< yamc::alternate::shared_mutex > sharedLocker( m_cacheMutex );
    auto now = std::chrono::steady_clock::now();
    m_devices.forEachDevice( [this, &now]( uint32_t txId ) { m_nodeToLatsUpdateTime[ txId ] = now; } );
}

}  // namespace TwoGig
}  // namespace HubPlatform
