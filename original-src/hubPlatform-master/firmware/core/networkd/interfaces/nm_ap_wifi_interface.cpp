#include "networkd/interfaces/nm_ap_wifi_interface.h"
#include "networkd/monitoring/nm_monitor_utils.h"
#include "networkd/resources/nm_resources.h"
#include "networkd/utils/nm_utils.h"

#include "json_utils/json_utils.h"
#include "jsonrpc/error_codes.h"
#include "system_utils/system.h"

namespace HubPlatform {
namespace Network {

ApWifiInterface::ApWifiInterface( IDbClient& dbClient,
                                  const ProvisionDataUtils::ProvisionData& provisionData,
                                  NetworkConfigManager& configManager,
                                  const InterfaceMonitorFactory& interfaceMonitorFactory,
                                  InterfaceInfoListener& interfaceInfoListener )
: m_provisionData( provisionData )
, m_configManager( configManager )
, m_interfaceInfoListener( interfaceInfoListener )
, m_settings( dbClient, Resources::ApWifiDeviceName, EnablePolicy::Off )
, m_monitor( interfaceMonitorFactory.createInterfaceMonitor( *this, Resources::ApInterfaceName, false ) )
, m_trafficMonitor( Resources::ApWifiDeviceName ) {}

Json ApWifiInterface::getInfo() const {
    return getInfo( m_monitor->getInfo() );
}

Json ApWifiInterface::getInfo( Json dynamicInfo ) const {
    using namespace Resources::InterfaceInfo;

    Json interfaceInfo = std::move( dynamicInfo );

    if( interfaceInfo.find( Fields::Ipv4 ) == interfaceInfo.end() ) {
        auto network = getNetworkConfig();
        interfaceInfo[ Fields::Ipv4 ] = network[ Fields::Ipv4 ];
    }

    interfaceInfo[ "wifi" ] = getWirelessConfig();
    interfaceInfo[ "type" ] = Resources::WifiType;
    interfaceInfo[ Fields::Priority ] = Resources::WifiApPriority;
    interfaceInfo[ Fields::Metric ] = Resources::WifiApMetric;
    interfaceInfo[ Fields::Network ] = Resources::LanNetwork;
    interfaceInfo[ Fields::EnablePolicy ] = toString( getEnablePolicy() );

    json_remove( interfaceInfo, Fields::InternetAvailable );

    // TODO: duplicate mac address from other interface

    // NOTE: Override interface id, originally it is not predictable, which is confusing
    interfaceInfo[ "_id" ] = Resources::ApWifiDeviceName;

    return interfaceInfo;
}

std::string ApWifiInterface::getIpAddress() {
    using namespace Resources::InterfaceInfo;
    auto info = getInfo();

    std::string ip = json_get_str( info[ Fields::Ipv4 ], Fields::Ip );
    if( !ip.empty() )
        return ip;

    ip = json_get_str( info[ Fields::Ipv6 ], Fields::Ip );
    if( !ip.empty() )
        return ip;

    return {};
}

const char* ApWifiInterface::getName() const {
    return Resources::ApWifiDeviceName;
}

bool ApWifiInterface::enable( bool force ) {
    if( m_enabled && !force )
        return true;

    LOGGER_INFO_RAW( cc::info( "Enabling " ) + cc::note( Resources::ApWifiDeviceName ) + cc::info( " interface" ) );
    m_enabled = true;
    m_monitor->start();

    auto wirelessConfig = getWirelessConfig();
    wirelessConfig[ "mode" ] = Resources::ApType;

    return m_configManager.configureAPWifiInterface( Resources::ApInterfaceName,
                                                     makeSystemNetworkConfiguration( wirelessConfig ) );
}

bool ApWifiInterface::disable( bool force ) {
    if( !m_enabled && !force )
        return true;

    LOGGER_INFO_RAW( cc::info( "Disabling " ) + cc::note( Resources::ApWifiDeviceName ) + cc::info( " interface" ) );
    m_enabled = false;
    auto result = m_configManager.configureAPWifiInterface( Resources::ApInterfaceName );
    if( result ) {
        notifyChanges();
    }

    return result;
}

bool ApWifiInterface::isEnabled() const {
    return m_enabled;
}

bool ApWifiInterface::isUp() const {
    auto result = Utils::getSystemInterfaces( { getName() } );
    if( result.empty() )
        return false;
    return result.front().isUp();
}

bool ApWifiInterface::reset() {
    m_settings.reset();
    notifyChanges();

    return true;
}

Json ApWifiInterface::getWirelessConfig() const {
    auto wirelessConfig = getUIWirelessConfiguration();

    if( wirelessConfig.count( "network" ) == 0 ) {
        wirelessConfig = getDefaultWirelessConfig();
    }

    applyCustomSettings( wirelessConfig );

    return wirelessConfig;
}

void ApWifiInterface::applyCustomSettings( Json& wirelessConfig ) const {
    auto apSettings = m_settings.getAPSettings();
    if( !apSettings.empty() ) {
        if( !apSettings.m_apName.empty() ) {
            wirelessConfig[ "network" ][ "ssid" ] = std::move( apSettings.m_apName );
        }
    }
}

Json ApWifiInterface::getDefaultWirelessConfig() const {
    JsonObject network;
    network[ "ssid" ] = m_provisionData.defaultWifiSsid;
    // network["ssid"] = "Atom" + m_provisionData.defaultWifiSsid.substr( m_provisionData.defaultWifiSsid.find( '_' ) );
    network[ "key" ] = m_provisionData.defaultWifiPassword;
    network[ "encryption" ] = Resources::DefaultApEncryption;

    JsonObject wifi;
    wifi[ "network" ] = std::move( network );
    wifi[ "mode" ] = Resources::ApType;
    wifi[ "channel" ] = Resources::DefaultApWifiChannel;
    wifi[ "region" ] = Resources::DefaultWifiRegion;

    return wifi;
}

Json ApWifiInterface::getNetworkConfig() const {
    auto network = getUINetworkConfiguration();
    if( network.empty() ) {
        network = getDefaultNetworkConfig();
    }
    return network;
}

Json ApWifiInterface::getDefaultNetworkConfig() const {
    JsonObject ipv4;
    ipv4[ "mode" ] = "dhcp";

    JsonObject interface;
    interface[ "ipv4" ] = std::move( ipv4 );

    return interface;
}

void ApWifiInterface::onNewInterfaceInfo( Json dynamicInfo ) {
    m_interfaceInfoListener.onNewInterfaceInfo( getInfo( std::move( dynamicInfo ) ) );
}

EnablePolicy ApWifiInterface::getEnablePolicy() const {
    return m_settings.getEnablePolicy();
}

bool ApWifiInterface::setEnablePolicy( EnablePolicy policy ) {
    if( policy == EnablePolicy::Auto ) {
        return false;
    }

    if( m_settings.setEnablePolicy( policy ) )
        onNewInterfaceInfo( getInfo() );
    return true;
}

bool ApWifiInterface::setCustomSettings( Json customSettings ) {
    auto apSettings = APInterfaceSettings::getAPSettings( std::move( customSettings ) );
    auto result = m_settings.setAPSettings( std::move( apSettings ) );
    if( result ) {
        notifyChanges();
    }

    return result;
}

void ApWifiInterface::notifyChanges() {
    onNewInterfaceInfo( m_monitor->getInfo() );
}

APWifiConfiguration ApWifiInterface::makeSystemNetworkConfiguration( const Json& config ) const {
    APWifiConfiguration configuration;

    auto channel = json_get_str( config, "channel" );
    configuration.m_channel = channel.empty() ? 0 : std::stoi( channel );

    if( json_contains( config, "network" ) ) {
        configuration.m_ssid = json_get_str( config[ "network" ], "ssid" );
        configuration.m_encryption = json_get_str( config[ "network" ], "encryption" );
        configuration.m_key = json_get_str( config[ "network" ], "key" );
    }

    return configuration;
}

Json ApWifiInterface::getUINetworkConfiguration() const {
    using namespace Resources::InterfaceInfo;

    auto configuration = m_configManager.getAPWifiConfiguration( Resources::ApInterfaceName );
    return JSON( Fields::Ipv4 << configuration.m_mode.toJson() );
}

Json ApWifiInterface::getUIWirelessConfiguration() const {
    using namespace Resources::InterfaceInfo;

    auto configuration = m_configManager.getAPWifiConfiguration( Resources::ApInterfaceName );
    Json config;
    Json& networkConfig = config[ Fields::Network ];

    if( configuration.isEmtpy() ) {
        return JSON_EMPTY_OBJ;
    }

    config[ "region" ] = Resources::DefaultWifiRegion;
    config[ "channel" ] = std::to_string( configuration.m_channel );
    config[ "mode" ] = Resources::ApType;

    networkConfig[ "ssid" ] = configuration.m_ssid;
    networkConfig[ "key" ] = configuration.m_key;
    networkConfig[ "encryption" ] = configuration.m_encryption;

    return config;
}

InterfaceTrafficMonitor::TrafficStatistic ApWifiInterface::getTrafficStatistics() const {
    return m_trafficMonitor.getTrafficStatistics();
}

}  // namespace Network
}  // namespace HubPlatform
