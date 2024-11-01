#include "networkd/interfaces/nm_ethernet_interface.h"
#include "networkd/resources/nm_resources.h"
#include "networkd/monitoring/nm_ping_statistics.h"

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace Network {

EthernetInterface::EthernetInterface( NetworkConfigManager& configManager,
                                      const InterfaceMonitorFactory& interfaceMonitorFactory,
                                      InterfaceInfoListener& interfaceInfoListener )
: m_configManager( configManager )
, m_interfaceInfoListener( interfaceInfoListener )
, m_monitor( interfaceMonitorFactory.createInterfaceMonitor( *this, Resources::EthernetInterfaceName, true ) )
, m_trafficMonitor( Resources::EthernetDeviceName ) {}

EthernetInterface::~EthernetInterface() {
    stopMonitoring();
}

void EthernetInterface::startMonitoring() {
    m_monitor->configure();
    m_monitor->start();
}

void EthernetInterface::stopMonitoring() {
    m_monitor->stop();
}

Json EthernetInterface::getInfo() const {
    return getInfo( m_monitor->getInfo() );
}

Json EthernetInterface::getInfo( Json dynamicInfo ) const {
    using namespace Resources::InterfaceInfo;

    Json interfaceInfo = std::move( dynamicInfo );

    if( interfaceInfo.find( Fields::Ipv4 ) == interfaceInfo.end() ) {
        interfaceInfo[ Fields::Ipv4 ] = getUINetworkConfiguration();
    }

    interfaceInfo[ "type" ] = Resources::EthernetType;
    interfaceInfo[ Fields::Priority ] = Resources::EthernetPriority;
    interfaceInfo[ Fields::Metric ] = Resources::EthernetMetric;
    interfaceInfo[ Fields::Network ] = Resources::WanNetwork;
    interfaceInfo[ Fields::EnablePolicy ] = toString( getEnablePolicy() );

    // NOTE: Override interface id, originally it is not predictable, which is confusing
    interfaceInfo[ "_id" ] = Resources::EthernetDeviceName;

    return interfaceInfo;
}

const char* EthernetInterface::getName() const {
    return Resources::EthernetDeviceName;
}

bool EthernetInterface::isInternetAvailable() const {
    return m_monitor->isInternetAvailable();
}

void EthernetInterface::onNewInterfaceInfo( Json dynamicInfo ) {
    m_interfaceInfoListener.onNewInterfaceInfo( getInfo( std::move( dynamicInfo ) ) );
}

EnablePolicy EthernetInterface::getEnablePolicy() const {
    return EnablePolicy::On;
}

bool EthernetInterface::setEnablePolicy( EnablePolicy policy ) {
    return false;
}

Json EthernetInterface::getUINetworkConfiguration() const {
    auto configuration = m_configManager.getEthernetConfiguration( Resources::EthernetInterfaceName );
    return configuration.m_mode.toJson();
}

InterfaceTrafficMonitor::TrafficStatistic EthernetInterface::getTrafficStatistics() const {
    return m_trafficMonitor.getTrafficStatistics();
}

const PingStatistics& EthernetInterface::getPingStatistics() const {
    return m_monitor->pingStatistics();
}

}  // namespace Network
}  // namespace HubPlatform
