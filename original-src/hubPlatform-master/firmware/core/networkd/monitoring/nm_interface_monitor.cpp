#include "networkd/monitoring/nm_interface_monitor.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include "networkd/resources/nm_resources.h"
#include <tiny/tiny_consolecolors.h>

namespace HubPlatform {
namespace Network {

InterfaceMonitor::InterfaceMonitor( Services::TaskLoop& taskLoop,
                                    InterfaceInfoListener& interfaceInfoListener,
                                    std::string interfaceName,
                                    ConfigReader& configReader,
                                    ExternalDataProvider& externalDataProvider,
                                    bool pingEnabled,
                                    std::chrono::seconds pingPeriod,
                                    IPVersion ipVersion )
: m_taskLoop( taskLoop )
, m_interfaceChangeListener( interfaceInfoListener )
, m_interfaceName( std::move( interfaceName ) )
, m_pingEnabled( pingEnabled )
, m_pingPeriod( pingPeriod )
, m_ipVersion( ipVersion )
, m_configReader( configReader )
, m_externalDataProvider( externalDataProvider ) {}

void InterfaceMonitor::setPingPeriod( std::chrono::seconds pingPeriod ) {
    m_pingPeriod = pingPeriod;
}

void InterfaceMonitor::setIPVersion( IPVersion ipVersion ) {
    m_ipVersion = ipVersion;
}

// NOTE: Reliable only if the ha-uid is alive!
void ExternalDataProvider::setNmaClientConnectionState( Json state, bool isConnected ) {
    LOGGER_DEBUG_RAW( cc::debug( "State: " ) + cc::j( state ) + cc::debug( ", isConnected: " ) +
                      cc::flag( isConnected ) );

    m_nmaConnectionStatus = std::move( state );
    m_isConnected = isConnected;
    if( isConnected ) {
        onNmaConnected();
    }
    else {
        onNmaDisconnected();
    }
}

void ExternalDataProvider::onNmaDisconnected() {
    m_lastConnected = 0;
    auto lastMonotonicConnected = m_lastMonotonicConnected;
    m_lastMonotonicConnected = 0;

    auto disconnectedAt = json_get_int( m_nmaConnectionStatus, Resources::NmaConnectedState::monotonicNow );

    if( lastMonotonicConnected > 0 && disconnectedAt > lastMonotonicConnected ) {
        appendNmaConnectedDuration( std::chrono::seconds( disconnectedAt - lastMonotonicConnected ) );
        Log::info( "NMA connection duration: " + cc::num10( getNmaConnectedDuration().count() ) +
                   ", reconnects count: " + cc::num10( nmaReconnectsCount() ) );
    }
}

void ExternalDataProvider::onNmaConnected() {
    m_lastConnected = json_get_int( m_nmaConnectionStatus, Resources::NmaConnectedState::now );
    m_lastMonotonicConnected = json_get_int( m_nmaConnectionStatus, Resources::NmaConnectedState::monotonicNow );
    m_nmaReconnects++;
}

bool ExternalDataProvider::isNmaConnected( Json* status ) {
    if( status )
        *status = m_nmaConnectionStatus;
    return m_isConnected;
}

std::chrono::seconds ExternalDataProvider::getNmaConnectedDuration() const {
    return m_nmaConnectedDuration;
}

size_t ExternalDataProvider::nmaReconnectsCount() const {
    return m_nmaReconnects;
}

void ExternalDataProvider::appendNmaConnectedDuration( std::chrono::seconds period ) {
    m_nmaConnectedDuration += period;
}

}  // namespace Network
}  // namespace HubPlatform
