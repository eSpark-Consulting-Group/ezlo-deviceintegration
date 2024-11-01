#include "networkd/network_manager.h"
#include "networkd/resources/nm_resources.h"
#include "networkd/routes/nm_routes_manager_factory.h"
#include "networkd/monitoring/nm_ping_statistics.h"
#include "networkd/monitoring/nm_aux_servers.h"
#include "networkd/monitoring/nm_monitor_utils.h"
#include "networkd/monitoring/nm_ping_hosts.h"
#include "provision_data_utils/provision_data_manager.h"
#include "database_client/db_tables_names.h"
#include "database_client/dbqueryparser.h"
#include "system_utils/system.h"
#include "base_utils/utils.h"
#include "common_utils/exception_utils.h"
#include "json_utils/json_utils.h"
#include "jsonrpc/error_codes.h"

#include <future>

namespace HubPlatform {
namespace Network {
namespace {
constexpr std::chrono::seconds RoutesCheckPeriod{ 10 };
constexpr std::chrono::seconds ApInterfaceCheckPeriod{ 60 };
constexpr std::chrono::seconds InterfacesCheckPeriod{ 200 };
constexpr std::chrono::minutes PrintPingStatsPeriod{ 10 };
constexpr std::chrono::seconds SyncCameraStreamsDelay{ 30 };
constexpr std::chrono::seconds SyncCameraStreamsPeriod{ 120 };
}  // namespace

NetworkManager::NetworkManager( IDbClient& dbClient,
                                const ProvisionDataUtils::ProvisionData& provisionData,
                                NetworkConfigManager& configManager,
                                const NetworkConfigFileManager& configFileManager,
                                Services::TaskLoop& taskLoop,
                                NetworkChangeListener& networkChangeListener,
                                WifiScanResultListener& wifiScanResultListener,
                                const CoreDRemoteInterface::CoreDInterface& core,
                                ConfigReader& configReader )
: m_dbClient( dbClient )
, m_configManager( configManager )
, m_configFileManager( configFileManager )
, m_taskLoop( taskLoop )
, m_networkChangeListener( networkChangeListener )
, m_core( core )
, m_configReader( configReader )
, m_routesCheckTimer( m_taskLoop, [this]() { checkAndFixRoutes(); } )
, m_apInterfaceCheckTimer( m_taskLoop, [this]() { checkIfApInterfaceShouldBeUp(); } )
, m_fixNetworkTimer( m_taskLoop, [this]() { fixNetwork(); } )
, m_interfaceCheckTimer( m_taskLoop, [this]() { checkAndFixInterfaceStates(); } )
, m_forceBroadcastTimer( m_taskLoop, [this]() { forceBroadcast(); } )
, m_printPingStatsTimer( m_taskLoop, [this]() { printPingStats(); } )
, m_syncCameraStreamsTimer( m_taskLoop, [this]() { syncCameraStreams(); } )
, m_routesManager( RoutesManagerFactory::createRouteManager() )
, m_externalDataProvider( std::make_unique< ExternalDataProvider >() )
, m_wifiPassiveScanner( wifiScanResultListener )
, m_ethernetInterface( m_configManager,
                       InterfaceMonitorFactory( taskLoop, configReader, *m_externalDataProvider ),
                       *this )
, m_clientWifiInterface( m_taskLoop,
                         m_dbClient,
                         provisionData,
                         m_configManager,
                         m_wifiPassiveScanner,
                         InterfaceMonitorFactory( taskLoop, configReader, *m_externalDataProvider ),
                         *this )
, m_apWifiInterface( m_dbClient,
                     provisionData,
                     m_configManager,
                     InterfaceMonitorFactory( taskLoop, configReader, *m_externalDataProvider ),
                     *this )
, m_gsmManager( dbClient,
                provisionData,
                m_configManager,
                m_configFileManager.getMobileConfiguration(),
                InterfaceMonitorFactory( taskLoop, configReader, *m_externalDataProvider ),
                *this,
                taskLoop )
, m_wifiConnectionService( *this ) {
    updateAdditionalPingServers();
    if( getActiveInterface().empty() ) {
        setActiveInterface( Resources::EthernetDeviceName );
    }
    if( !m_dbClient.has( TempNetworkDynamicInfoDBName,
                         JSON( db::ID_FIELD_NAME << Resources::HubInternetAvailabilityId ) ) ) {
        m_dbClient.insert(
            TempNetworkDynamicInfoDBName,
            JSON( db::ID_FIELD_NAME << Resources::HubInternetAvailabilityId << "internetAvailable" << false ) );
    }

    onNewInterfaceInfo( m_ethernetInterface.getInfo() );
    onNewInterfaceInfo( m_clientWifiInterface.getInfo() );
    onNewInterfaceInfo( m_apWifiInterface.getInfo() );

    {
        auto transaction = m_configManager.startWirelessConfigurationTransaction();

        m_configManager.prepareEnvironment();

        m_ethernetInterface.startMonitoring();

        if( m_clientWifiInterface.hasSettings() && m_clientWifiInterface.getEnablePolicy() != EnablePolicy::Off ) {
            m_clientWifiInterface.enable( true );

            if( m_apWifiInterface.getEnablePolicy() != EnablePolicy::Off ) {
                m_apWifiInterface.enable( true );
            }
            else {
                m_apWifiInterface.disable( true );
                // If we have no ethernet connection, give some time for client Wi-Fi to start
                m_apInterfaceCheckTimer.start( ApInterfaceCheckPeriod );
            }
        }
        else {
            m_clientWifiInterface.disable( true );

            if( m_apWifiInterface.getEnablePolicy() != EnablePolicy::Off || !isNonMobileInternetAvailable() ) {
                m_apWifiInterface.enable( true );
            }
            else {
                m_apWifiInterface.disable( true );
            }
        }

        manageConnectionService();

        /* openwrt network manager (netifd) has no time to get to a state where it can handle requests we make
         * using a 'wifi' command (to ask it to apply our settings to network device drivers),
         * so put a delay here to give it a time */
        std::this_thread::sleep_for( std::chrono::seconds( 3 ) );
    }

    m_interfaceCheckTimer.start( InterfacesCheckPeriod );

    m_printPingStatsTimer.start( PrintPingStatsPeriod );
    m_syncCameraStreamsTimer.start( SyncCameraStreamsDelay, SyncCameraStreamsPeriod );

    m_taskLoop
        .createPostponedTask( [=]( Services::PostponedTask& ) {
            // give physical modems time to "get over it" between reboots
            getGSMManager().start();
            getGSMManager().restoreDynamicInterfaces();
        } )
        ->start( std::chrono::seconds( 15 ), false );

    // NOTE: sometimes on failed attempt to connect to WiFi if backup WiFi is also bad,
    //       we do not get event that internet availability changed, but rotes become broken.
    //       So just in case periodically check and fix routes
    m_routesCheckTimer.start( std::chrono::seconds( 0 ), RoutesCheckPeriod );

    onForceBroadcastSettingsChanged();
}

GSMManager& NetworkManager::getGSMManager() {
    return m_gsmManager;
}

const GSMManager& NetworkManager::getGSMManager() const {
    return m_gsmManager;
}

void NetworkManager::connectToWiFi( const std::string& ssid,
                                    const std::string& bssid,
                                    const std::string& encryption,
                                    const std::string& key,
                                    bool force,
                                    bool temporaryDisconnectAp ) {
    using namespace Resources::InterfaceInfo;

    if( m_clientWifiInterface.getEnablePolicy() == EnablePolicy::Off ) {
        LOGGER_WARN_RAW(
            cc::warn( "Client wifi (STA) interface is turned off, reject the request to establish a connection" ) );
        return;
    }

    auto wifiDevice = Resources::ClientWifiDeviceName;

    LOGGER_INFO( cc::info( "Trying to detect wireless networks on interface: " ) + cc::note( wifiDevice ) );
    const auto scannedNetwork = m_wifiPassiveScanner.scanAndGetNetwork( wifiDevice, ssid, bssid );
    if( scannedNetwork.isValid() ) {
        if( !bssid.empty() && bssid != scannedNetwork.m_bssid ) {
            LOGGER_WARN_RAW( cc::warn( "Wireless network bssid was changed from " ) + cc::note( bssid ) +
                             cc::warn( " to " ) + cc::note( scannedNetwork.m_bssid ) );
        }

        LOGGER_INFO_RAW( cc::info( "Will try to connect to a wireless network with the next parameters, ssid " ) +
                         cc::note( scannedNetwork.m_ssid ) + cc::info( ", bssid " ) +
                         cc::note( scannedNetwork.m_bssid ) + cc::info( ", channel " ) +
                         cc::num10( scannedNetwork.m_channel ) );
    }
    else {
        LOGGER_WARN_RAW( cc::warn( "There are no visible wireless networks with a specified ssid nearby" ) );
    }

    const bool isApEnabled = m_apWifiInterface.isEnabled();
    if( temporaryDisconnectAp ||
        ( m_apWifiInterface.getEnablePolicy() == EnablePolicy::Off && isNonMobileInternetAvailable() ) ) {
        LOGGER_INFO( cc::info( "Temporarily disabling wifi access point..." ) );
        m_apWifiInterface.disable( true );
    }

    m_clientWifiInterface.connect( scannedNetwork.m_channel, ssid, scannedNetwork.m_bssid, encryption, key, force );

    if( temporaryDisconnectAp && isApEnabled ) {
        if( m_apWifiInterface.getEnablePolicy() != EnablePolicy::Off || !isNonMobileInternetAvailable() ) {
            LOGGER_INFO( cc::info( "Enabling wifi access point..." ) );
            m_apWifiInterface.enable();
        }
    }
    manageConnectionService();
}

bool NetworkManager::resetInterface( const std::string& interfaceId ) {
    if( interfaceId.empty() ) {  // no interface - reset all network settings
        resetWiFi();
        getGSMManager().resetGSMInterfaces();
    }
    else if( interfaceId == Resources::ClientWifiDeviceName ) {
        m_clientWifiInterface.reset();
        manageConnectionService();
    }
    else if( interfaceId == Resources::ApWifiDeviceName ) {
        m_apWifiInterface.reset();
        /* TODO: wifi ap goes up if we have no connection to a LAN, should we prevent this
         *       if we have added a hub to an account and have an access to the hub via JSON RPC API?
         *       (we may have an LTE connection) */
        if( m_apWifiInterface.getEnablePolicy() != EnablePolicy::Off || !isNonMobileInternetAvailable() ) {
            m_apWifiInterface.enable();
        }
        else {
            m_apWifiInterface.disable();
        }

        manageConnectionService();
    }
    else {
        try {
            getGSMManager().getGSMInterface( interfaceId ).reset();
        }
        // this may happen if interface was removed after we checked its existence in ha-uid.handleResetRequest
        catch( const InvalidGSMInterface& ex ) {
            Log::error( cc::error( "Failed to reset non-existing gsm interface, " ) + cc::note( ex.what() ) );
        }
        catch( ... ) {
            Log::error( cc::error( "Failed to reset non-existing gsm interface" ) );
        }
    }
    return true;
}

bool NetworkManager::resetWiFi() {
    auto transaction = m_configManager.startWirelessConfigurationTransaction();
    if( !m_clientWifiInterface.reset() )
        return false;

    if( !m_apWifiInterface.reset() )
        return false;

    if( m_apWifiInterface.getEnablePolicy() != EnablePolicy::Off || !isNonMobileInternetAvailable() ) {
        m_apWifiInterface.enable();
    }
    else {
        m_apWifiInterface.disable();
    }

    manageConnectionService();
    return true;
}

bool NetworkManager::startScan( const std::string& interfaceId ) {
    LOGGER_INFO( cc::info( "Starting wifi scan..." ) );
    using namespace Resources::InterfaceInfo;
    createScanTask( interfaceId );
    return true;
}

void NetworkManager::createScanTask( const std::string& interface ) {
    LOGGER_INFO( cc::info( "Creating wifi scan task for interface: " ) + cc::note( interface ) );
    if( m_currentScanningInterface != interface ) {
        m_wifiPassiveScanner.stopScan( m_currentScanningInterface );
        m_currentScanningInterface = interface;
    }
    m_wifiPassiveScanner.startScan( m_currentScanningInterface );
}

std::string NetworkManager::getActiveWifiDevice() {
    auto& client = m_clientWifiInterface;
    auto& ap = m_apWifiInterface;

    if( client.isEnabled() && client.isUp() )
        return client.getName();
    if( ap.isEnabled() && ap.isUp() )
        return ap.getName();
    return {};
}

bool NetworkManager::stopScan( const std::string& /*interfaceId*/ ) {
    LOGGER_INFO( cc::info( "Stopping wifi scan..." ) );
    m_wifiPassiveScanner.stopScan( m_currentScanningInterface );
    if( m_apWifiInterface.getEnablePolicy() == EnablePolicy::Off && isNonMobileInternetAvailable() ) {
        LOGGER_INFO( cc::info( "Disabling wifi access point after the scan..." ) );
        m_apWifiInterface.disable();
    }
    return true;
}

Errors::Error NetworkManager::setEnablePolicy( const std::string& interfaceId, const std::string& enablePolicy ) {
    auto policy = enablePolicyFromString( enablePolicy, EnablePolicy::Auto );
    LOGGER_INFO( cc::info( "Setting enable policy for interface: " ) + cc::note( interfaceId ) +
                 cc::info( ", policy: " ) + cc::note( enablePolicy ) );

    // TODO: add correct support for Auto enable policy for wan interfaces

    if( interfaceId == Resources::EthernetDeviceName ) {
        if( !m_ethernetInterface.setEnablePolicy( policy ) )
            return Errors::NETWORK_OPERATION_PROHIBITED;
        return Errors::OK;
    }
    else if( interfaceId == Resources::ApWifiDeviceName ) {
        if( !m_apWifiInterface.setEnablePolicy( policy ) )
            return Errors::NETWORK_OPERATION_PROHIBITED;

        if( policy == EnablePolicy::Off && isNonMobileInternetAvailable() ) {
            LOGGER_INFO( cc::info( "Disabling wifi access point via enable policy..." ) );
            m_apWifiInterface.disable();
        }
        else {
            LOGGER_INFO( cc::info( "Enabling wifi access point via enable policy..." ) );
            m_apWifiInterface.enable();
        }

        manageConnectionService();
        return Errors::OK;
    }
    else if( interfaceId == Resources::ClientWifiDeviceName ) {
        if( !m_clientWifiInterface.setEnablePolicy( policy ) )
            return Errors::NETWORK_OPERATION_PROHIBITED;

        if( policy == EnablePolicy::Off ) {
            LOGGER_INFO( cc::info( "Disabling wifi client via enable policy..." ) );
            m_clientWifiInterface.disable();
        }
        else {
            if( m_clientWifiInterface.hasSettings() ) {
                LOGGER_INFO( cc::info( "Enabling wifi client via enable policy..." ) );
                m_clientWifiInterface.enable();
            }
            else {
                LOGGER_INFO( cc::info( "Settings is not present for wifi client" ) );
            }
        }

        manageConnectionService();
        return Errors::OK;
    }
    else {
        try {
            auto& interface = getGSMManager().getGSMInterface( interfaceId );
            if( !interface.setEnablePolicy( policy ) )
                return Errors::NETWORK_OPERATION_PROHIBITED;
            return Errors::OK;
        }
        catch( ... ) {
            return Errors::NETWORK_INVALID_INTERFACE;
        }
    }

    return Errors::NETWORK_INVALID_INTERFACE;
}

Errors::Error NetworkManager::setCustomSettings( const std::string& interfaceId, const Json& settings ) {
    bool result = false;

    LOGGER_DEBUG( cc::debug( "Set custom settings for interface: " ) + cc::note( interfaceId ) +
                  cc::debug( ", settings: " ) + cc::j( settings ) );
    forEachNetworkInterface( [&]( NetworkInterface& interface ) {
        if( interface.getName() == interfaceId ) {
            result = interface.setCustomSettings( settings );
        }
    } );

    if( result ) {
        if( interfaceId == std::string( Resources::ApWifiDeviceName ) &&
            m_apWifiInterface.getEnablePolicy() == EnablePolicy::On ) {
            LOGGER_INFO( cc::info( "Enabling wifi access point interface" ) );
            // restart ap with new settings
            m_apWifiInterface.enable( true );
        }
    }

    return result ? Errors::OK : Errors::NETWORK_SETTINGS_SET_FAILED;
}

void NetworkManager::onForceBroadcastSettingsChanged() {
    auto periodSetting = m_core.findSetting( Resources::Settings::ModemSignalQualityPeriod );
    auto forceBroadcastPeriod = std::chrono::minutes( periodSetting ? periodSetting->getIntValue() : 0 );
    if( forceBroadcastPeriod.count() ) {
        LOGGER_INFO_RAW( cc::info( "Enabling periodic modem signal quality broadcasts each " ) +
                         cc::num10( forceBroadcastPeriod.count() ) + cc::info( " minute(s)" ) );
        m_forceBroadcastTimer.start( std::chrono::seconds( 0 ), forceBroadcastPeriod );
    }
    else {
        LOGGER_INFO_RAW( cc::info( "Disabling periodic modem signal quality broadcasts" ) );
        m_forceBroadcastTimer.stop();
    }
}

void NetworkManager::manageConnectionService() {
    if( !m_clientWifiInterface.hasSettings() && ( m_clientWifiInterface.getEnablePolicy() != EnablePolicy::Off ) &&
        ( ( m_apWifiInterface.getEnablePolicy() != EnablePolicy::Off ) || !isNonMobileInternetAvailable() ) ) {
        LOGGER_INFO( cc::info( "Starting wifi connection service for wifi client" ) );
        m_wifiConnectionService.start();
    }
    else {
        LOGGER_INFO( cc::info( "Stopping wifi connection service for wifi client" ) );
        m_wifiConnectionService.stop();
    }
}

bool NetworkManager::isWifiInternetAvailable() const {
    return m_clientWifiInterface.isInternetAvailable();
}

bool NetworkManager::isInternetAvailable() const {
    auto internetAvailableJson = m_dbClient.findOne(
        TempNetworkDynamicInfoDBName, JSON( db::ID_FIELD_NAME << Resources::HubInternetAvailabilityId ), nullptr );
    return json_get_bool( internetAvailableJson, "internetAvailable" );
}

bool NetworkManager::isNonMobileInternetAvailable() const {
    return m_ethernetInterface.isInternetAvailable() || m_clientWifiInterface.isInternetAvailable();
}

NetworkChangeListener::NetworkInfo NetworkManager::getActiveNetwork( std::vector< Json > interfaces ) const {
    std::sort( interfaces.begin(), interfaces.end(), []( const Json& a, const Json& b ) {
        return json_get_int( a, "priority" ) < json_get_int( b, "priority" );
    } );

    NetworkChangeListener::NetworkInfo network;
    for( const auto& interface : interfaces ) {
        auto interfaceName = json_get_str( interface, "_id" );

        if( json_get_str( interface, "status" ) != "up" ) {
            continue;
        }
        if( !json_get( interface, std::vector< Json >{ "ipv4", "ip" }, network.ipv4 ) ) {
            Log::error( cc::error( "No network ip address found, interface: " ) + cc::note( interfaceName ) );
            continue;
        }
        if( !json_get( interface, "type", network.type ) ) {
            Log::error( cc::error( "No network type found" ) );
            continue;
        }
        if( !json_get( interface, "_id", network.interfaceId ) ) {
            Log::error( cc::error( "No network interface id found" ) );
            continue;
        }
        return network;
    }
    return {};
}

/** Attempts routes changes.
 * The method is called periodically from the fix routes timer.
 * Notifies listener when the device has changed, emits one or two events:
 *      - notifies false on active interface change, always;
 *      - if changed to another existing device, then notifies true;
 *  Notifies about active device change.
 *
 * @params notifyOnRoutesChange - if true, calls network change listener's method @sa onInternetAvailabilityChanged
 *
 * */
void NetworkManager::checkAndFixRoutes( bool notifyOnRoutesChange ) {
    // TODO: Research routing problem deeper. Try to fix routing before notifying
    //       about internet availability change, because it may fix internet

    auto previousInterface = getActiveInterface();
    // NOTE: 'noop' flag for the wifi interface means this interface is not suitable for scanning
    auto interfaceResult = System::getCommandOutputWithErrorReport( "ip addr show", true /*capture stderr*/ );
    auto routesResult = System::getCommandOutputWithErrorReport( "ip route show", true /*capture stderr*/ );

    auto device = m_routesManager->checkAndFixRoutes( getNetworkInterfacesVector( false ) );
    if( getActiveInterface() != device ) {
        LOGGER_INFO( cc::info( "Previous active interface: " ) + cc::note( getActiveInterface() ) );
        LOGGER_INFO( cc::info( "Previous interfaces: code: " ) + cc::num10( interfaceResult.exitCode ) +
                     cc::info( ", output:\n" ) + cc::note( interfaceResult.output ) );
        LOGGER_INFO( cc::info( "Previous routes: code: " ) + cc::num10( routesResult.exitCode ) +
                     cc::info( ", output:\n" ) + cc::note( routesResult.output ) );
        // NOTE: maybe we should also notify when we fix routes even if default route was not changed at the end
        Log::info( cc::info( "Default route changed to " ) + cc::note( device.empty() ? "none" : device ) );
        setActiveInterface( device );
        if( notifyOnRoutesChange ) {
            LOGGER_INFO( cc::info( "Set internet availability to: " ) + cc::note( "false" ) );
            m_networkChangeListener.onInternetAvailabilityChanged( false );
            if( !device.empty() ) {
                LOGGER_INFO( cc::info( "Set internet availability to: " ) + cc::note( "true" ) );
                m_networkChangeListener.onInternetAvailabilityChanged( true );
            }
        }

        notifyActiveInterfaceChanged( device );

        auto interfaceResult = System::getCommandOutputWithErrorReport( "ip addr show", true /*capture stderr*/ );
        LOGGER_INFO( cc::info( "Updated interfaces: code: " ) + cc::num10( interfaceResult.exitCode ) +
                     cc::info( ", output:\n" ) + cc::note( interfaceResult.output ) );
        auto routesResult = System::getCommandOutputWithErrorReport( "ip route show", true /*capture stderr*/ );
        LOGGER_INFO( cc::info( "Updated routes: code: " ) + cc::num10( routesResult.exitCode ) +
                     cc::info( ", output:\n" ) + cc::note( routesResult.output ) );
    }
}

void NetworkManager::setActiveInterface( const std::string& activeInterface ) {
    auto query = JSON( db::ID_FIELD_NAME << Resources::HubActiveInterfaceId << "interfaceId" << activeInterface );
    m_dbClient.insert( TempNetworkDynamicInfoDBName, query );
}

std::string NetworkManager::getActiveInterface() {
    auto activeInterfaceJson = m_dbClient.findOne(
        TempNetworkDynamicInfoDBName, JSON( db::ID_FIELD_NAME << Resources::HubActiveInterfaceId ), nullptr );
    return json_get_str( activeInterfaceJson, "interfaceId" );
}

void NetworkManager::notifyActiveInterfaceChanged( const std::string& activeDevice ) {
    forEachNetworkInterface( [&activeDevice]( NetworkInterface& interface ) {
        if( interface.getName() == activeDevice ) {
            LOGGER_INFO( cc::info( "Set network interface: " ) + cc::note( interface.getName() ) + cc::info( " as " ) +
                         cc::note( "'active'" ) );
            interface.onInterfaceActive( true );
        }
        else {
            LOGGER_INFO( cc::info( "Set network interface: " ) + cc::note( interface.getName() ) + cc::info( " as " ) +
                         cc::note( "'not-active'" ) );
            interface.onInterfaceActive( false );
        }
    } );
}

void NetworkManager::forEachNetworkInterface( const std::function< void( NetworkInterface& ) >& callback ) {
    callback( m_ethernetInterface );
    callback( m_clientWifiInterface );
    callback( m_apWifiInterface );

    m_gsmManager.foreachGSMInterface( [&callback]( GSMInterface& gsmInterface ) { callback( gsmInterface ); } );
}

std::vector< Json > NetworkManager::getNetworkInterfacesVector( bool requireWifiApInterface ) const {
    auto interfaces = std::vector< Json >( { m_ethernetInterface.getInfo(), m_clientWifiInterface.getInfo() } );

    if( requireWifiApInterface ) {
        interfaces.push_back( m_apWifiInterface.getInfo() );
    }

    std::vector< Json > gsmDynamicInterfacesInfo;
    auto getGSMDynamicInterfacesInfo = [&gsmDynamicInterfacesInfo]( const GSMInterface& gsmInterface ) {
        gsmDynamicInterfacesInfo.push_back( gsmInterface.getInfo() );
    };

    getGSMManager().foreachGSMInterface( getGSMDynamicInterfacesInfo );
    interfaces.insert( interfaces.end(), gsmDynamicInterfacesInfo.begin(), gsmDynamicInterfacesInfo.end() );

    return interfaces;
}

void NetworkManager::onInterfaceInternetAvailabilityChanged( bool internetAvailable ) {
    // NOTE: It may take some time for UID to understand that it needs
    //       to reconnect because default route changed, so poke it here
    auto prevInternetAvailableJson = m_dbClient.findOne(
        TempNetworkDynamicInfoDBName, JSON( db::ID_FIELD_NAME << Resources::HubInternetAvailabilityId ), nullptr );
    bool prevInternetAvailable = json_get_bool( prevInternetAvailableJson, "internetAvailable" );

    LOGGER_INFO( cc::info( "Interface internet availability changed to: " ) + cc::flag( internetAvailable ) );

    checkAndFixRoutes( prevInternetAvailable || internetAvailable );

    if( prevInternetAvailable == internetAvailable ) {
        // This can only happen when this interface got internet
        // but we already have internet by other interface, nothing to do
        LOGGER_INFO( cc::info( "Internet availability not changed from previous state" ) );
        return;
    }

    bool overallInternetAvailable =
        m_ethernetInterface.isInternetAvailable() || m_clientWifiInterface.isInternetAvailable();

    onNonMobileInternetAvailabilityChanged( overallInternetAvailable );

    auto checkGSMDynamicInterfacesInternet = [&overallInternetAvailable]( const GSMInterface& gsmInterface ) {
        if( gsmInterface.isInternetAvailable() ) {
            overallInternetAvailable = true;
        }
    };

    getGSMManager().foreachGSMInterface( checkGSMDynamicInterfacesInternet );

    if( prevInternetAvailable == overallInternetAvailable ) {
        // This can only happen when this interface lost internet
        // but we already have internet by other interface, nothing to do
        LOGGER_INFO( cc::info( "Internet available on other interfaces. Nothing to do." ) );
        return;
    }

    Log::info( cc::info( "Internet is now " ) + cc::note( overallInternetAvailable ? "available" : "not available" ) );
    m_dbClient.update( TempNetworkDynamicInfoDBName,
                       JSON( db::ID_FIELD_NAME << Resources::HubInternetAvailabilityId ),
                       JSON( "$set" << JSON( "internetAvailable" << overallInternetAvailable ) ) );

    m_networkChangeListener.onInternetAvailabilityChanged( overallInternetAvailable );
}

void NetworkManager::onNonMobileInternetAvailabilityChanged( bool internetAvailable ) {
    m_fixNetworkTimer.stop();
    if( !internetAvailable ) {
        m_fixNetworkTimer.start( std::chrono::seconds( 30 ), false );
    }
    m_apInterfaceCheckTimer.stop();
    m_apInterfaceCheckTimer.start( std::chrono::seconds( 60 ), false );
}

void NetworkManager::fixNetwork() {
    if( isNonMobileInternetAvailable() ) {
        return;
    }
    Log::info( cc::info( "Trying to fix network" ) );
    updateAdditionalPingServers();
    System::getCommandOutput( "/etc/init.d/firewall restart" );
}

void NetworkManager::forceBroadcast() {
    using namespace Resources::InterfaceInfo;
    m_gsmManager.foreachGSMInterface( [this]( GSMInterface& gsmInterface ) {
        auto info = gsmInterface.getInfo();
        if( !info[ Fields::SignalQuality ].empty() ) {
            auto notificationInterfaceInfo = JSON_EMPTY_OBJ;
            notificationInterfaceInfo[ Fields::Id ] = info[ Fields::Id ];
            notificationInterfaceInfo[ Fields::SignalQuality ] = info[ Fields::SignalQuality ];

            Json notification;
            notification[ "interfaces" ] = JSON_ARRAY( notificationInterfaceInfo );
            notification[ "syncNotification" ] = false;
            m_networkChangeListener.onNetworkInfoChanged( std::move( notification ) );
        }
        else {
            LOGGER_TRACE_RAW( cc::trace( "Modem signal quality not available for: " ) +
                              cc::note( gsmInterface.getDeviceName() ) );
        }
    } );
}

void NetworkManager::checkIfApInterfaceShouldBeUp() {
    if( m_apWifiInterface.getEnablePolicy() != EnablePolicy::Off || !isNonMobileInternetAvailable() ) {
        LOGGER_INFO( cc::info( "WiFi access point should be up: enabling..." ) );
        m_apWifiInterface.enable();
    }
    else {
        LOGGER_INFO( cc::info( "WiFi access point should be down: disabling..." ) );
        m_apWifiInterface.disable();
    }

    manageConnectionService();
}

void NetworkManager::updateAdditionalPingServers() {
    auto extractAddress = []( std::string url ) {
        auto address = std::move( url );
        if( !address.empty() ) {
            static const std::string protoMark = "://";
            auto protoMarkPos = address.find( protoMark );
            if( protoMarkPos != std::string::npos ) {
                address.erase( 0, protoMarkPos + protoMark.size() );
            }
        }
        if( !address.empty() ) {
            auto portMarkPos = address.find( ':' );
            if( portMarkPos != std::string::npos ) {
                address.erase( portMarkPos );
            }
        }
        if( !address.empty() ) {
            auto resourceMarkPos = address.find( '/' );
            if( resourceMarkPos != std::string::npos ) {
                address.erase( resourceMarkPos );
            }
        }
        return address;
    };

    auto& manager = ProvisionDataUtils::getManager();
    std::vector< std::string > servers;
    LOGGER_DEBUG( cc::debug( "Read provisioning data" ) );
    manager.readData();
    auto provisionServer = extractAddress( manager.getProvisionData().provisionServer );
    if( !provisionServer.empty() ) {
        servers.push_back( provisionServer );
    }
    std::string cloudServerFullAddress = manager.getProvisionData().cloudServer;
    auto cloudServer = extractAddress( cloudServerFullAddress );
    Log::info( cc::info( "Cloud Server extracted from provisioning: " ) + cloudServer + cc::info( ", full address: " ) +
               cloudServerFullAddress );

    if( !cloudServer.empty() ) {
        servers.push_back( cloudServer );
        registerAdditionalServers( Resources::ServerRoles::CloudCoordinator, cloudServer, cloudServerFullAddress );
    }
    // TODO: move all ping resources to provision data.
    std::string nmaServer;
    auto joCacheCloud = m_dbClient.findOne( HaTempCacheDBName, JSON( "cache-cloud-info" << 1 ), nullptr );
    LOGGER_DEBUG_RAW( "Cached cloud info: " + cc::j( joCacheCloud ) );

    std::string nmaServerFullUrl;
    if( !joCacheCloud.empty() ) {
        nmaServerFullUrl = json_get_str( joCacheCloud, "cache-nma-url" );
        nmaServer = extractAddress( nmaServerFullUrl );
    }
    if( !nmaServer.empty() ) {
        Log::info( cc::info( "Nma Server extracted from cache: " ) + nmaServer + cc::info( ", full address: " ) +
                   nmaServerFullUrl );
        servers.push_back( nmaServer );
        registerAdditionalServers( Resources::ServerRoles::NmaServer, nmaServer, "" );
    }
    setAdditionalServers( servers );
}

namespace {
Json generateAddRemoveJsonBroadcast( const std::string& deviceName, const std::string& status ) {
    using namespace Resources::InterfaceInfo;
    return JSON( db::ID_FIELD_NAME << deviceName << Fields::Status << status );
}
}  // namespace

void NetworkManager::onNewInterfaceInfo( Json info ) {
    using namespace Resources::InterfaceInfo;

    auto interfaces = m_dbClient.findAll( TempNetworkInterfaces, JSON_EMPTY_OBJ, nullptr );
    LOGGER_TRACE_RAW( cc::trace( "Old network interfaces: " ) + cc::j( interfaces ) );
    auto oldNetwork = getActiveNetwork( std::move( interfaces ) );
    LOGGER_TRACE_RAW( cc::debug( "Old IP address: " ) + cc::note( oldNetwork.ipv4 ) );

    if( Fields::Id != db::ID_FIELD_NAME ) {
        info[ db::ID_FIELD_NAME ] = info[ Fields::Id ];
    }

    const auto deviceName = json_get_str( info, db::ID_FIELD_NAME );
    const auto status = json_get_str( info, Fields::Status );
    const auto type = json_get_str( info, Fields::Type );

    /* handle special cases: interface added/removed */
    if( !status.empty() && ( status == "added" || status == "removed" ) ) {
        if( status == "added" ) {
            m_dbClient.insert( TempNetworkInterfaces, JSON( db::ID_FIELD_NAME << deviceName ) );
        }
        else {
            m_dbClient.remove( TempNetworkInterfaces, JSON( db::ID_FIELD_NAME << deviceName ) );
        }

        Json notification;
        notification[ "interfaces" ] = JSON_ARRAY( generateAddRemoveJsonBroadcast( deviceName, status ) );
        notification[ "syncNotification" ] = false;
        m_networkChangeListener.onNetworkInfoChanged( std::move( notification ) );
    }
    else {
        auto prevNotification =
            m_dbClient.findOne( TempNetworkInterfaces, JSON( db::ID_FIELD_NAME << deviceName ), nullptr );
        auto diff = jsonDiff( prevNotification, info );
        if( !diff.empty() ) {
            m_dbClient.update( TempNetworkInterfaces, JSON_EMPTY_OBJ, info, true );

            diff[ Fields::Id ] = json_get_str( info, Fields::Id );

            LOGGER_TRACE_RAW( cc::trace( "Interface " ) + cc::note( json_get_str( info, Fields::Id ) ) +
                              cc::trace( " changed: " ) + cc::j( diff ) );

            auto it = diff.find( Fields::Status );
            if( it != diff.end() && it.value().is_string() ) {
                auto status = it.value().get< std::string >();
                Log::info( cc::info( "Interface " ) + cc::note( json_get_str( info, Fields::Id ) ) +
                           cc::info( " went " ) + cc::note( status ) );
                m_networkChangeListener.onInterfaceStatusChanged( deviceName, type, status == Statuses::Up );
            }

            it = diff.find( Fields::InternetAvailable );
            if( it != diff.end() && it.value().is_boolean() ) {
                bool internetAvailable = it.value().get< bool >();
                Log::info( cc::info( "Interface " ) +
                           cc::note( json_get_str( info, Fields::Id ) + " " + ( internetAvailable ? "got" : "lost" ) ) +
                           cc::info( " internet" ) );

                onInterfaceInternetAvailabilityChanged( internetAvailable );
            }

            if( !prevNotification.empty() ) {
                Json notification;
                notification[ "interfaces" ] = JSON_ARRAY( std::move( diff ) );
                Json& interfaces = notification[ "interfaces" ];
                bool syncNotification = false;
                syncNotification |= json_contains( interfaces, Fields::Ipv4 );
                syncNotification |= json_contains( interfaces, Fields::Ipv6 );
                syncNotification |= json_contains( interfaces, Fields::Status );
                syncNotification |= json_contains( interfaces, Fields::Mask );
                notification[ "syncNotification" ] = syncNotification;

                m_networkChangeListener.onNetworkInfoChanged( std::move( notification ) );
            }
        }
    }
    interfaces = m_dbClient.findAll( TempNetworkInterfaces, JSON_EMPTY_OBJ, nullptr );
    LOGGER_INFO( cc::info( "New network interfaces: " ) + cc::j( interfaces ) );
    auto newNetwork = getActiveNetwork( std::move( interfaces ) );
    LOGGER_INFO( cc::info( "New IP address: " ) + cc::note( newNetwork.ipv4 ) );
    if( newNetwork.ipv4 != oldNetwork.ipv4 ) {
        LOGGER_INFO( cc::info( "Update IP address: " ) + cc::note( newNetwork.ipv4 ) + cc::info( " and type: " ) +
                     cc::note( newNetwork.type ) );
        m_networkChangeListener.onCurrentNetworkChanged( newNetwork );
    }
}

void NetworkManager::onNewInterfaceAdded( const std::string& interfaceName ) {
    onNewInterfaceInfo( generateAddRemoveJsonBroadcast( interfaceName, "added" ) );
}

void NetworkManager::onInterfaceRemoved( const std::string& interfaceName ) {
    onNewInterfaceInfo( generateAddRemoveJsonBroadcast( interfaceName, "removed" ) );
}

bool NetworkManager::scanWifi( std::string& jsonResult ) {
    // NOTE: this handler is asynchronous
    auto device = Resources::ClientWifiDeviceName;
    LOGGER_INFO( cc::info( "Start manual wifi scan on device: " ) + cc::note( device ) );
    Json scanResult = m_wifiPassiveScanner.manualScan( device );
    if( scanResult.empty() ) {
        LOGGER_WARN( cc::warn( "Empty manual wifi scan results on device: " ) + cc::note( device ) );
        return false;
    }

    JsonArray& networks = scanResult[ "networks" ];
    if( networks.is_array() ) {
        for( auto& network : networks ) {
            network[ "open" ] = json_get_str( network, Resources::Scan::WifiSecurityKey ) == "open";
            json_remove( network, Resources::Scan::BssidKey );
            json_remove( network, Resources::Scan::ChannelKey );
            json_remove( network, Resources::Scan::WifiSecurityKey );
        }
        scanResult[ "status" ] = "success";
    }

    jsonResult = scanResult.dump();
    return true;
}

void NetworkManager::connect( std::string ssid, std::string bssid, std::string encryption, std::string key ) {
    // NOTE: this handler is asynchronous
    m_taskLoop.addTask( [=]() { connectToWiFi( ssid, bssid, encryption, key, true ); } );
}

void NetworkManager::temporaryShutDownAP() {
    // NOTE: this handler is asynchronous
    m_taskLoop.addTask( [=]() {
        LOGGER_INFO( cc::info( "Temporary shutdown wifi access point: disabling..." ) );
        m_apWifiInterface.disable();
        m_taskLoop
            .createPostponedTask( [=]( Services::PostponedTask& ) {
                if( m_apWifiInterface.getEnablePolicy() != EnablePolicy::Off || !isInternetAvailable() ) {
                    LOGGER_INFO( cc::info( "Enabling wifi access point after it was temporary disabled" ) );
                    m_apWifiInterface.enable();
                }
                manageConnectionService();
            } )
            ->start( std::chrono::seconds( 20 ), false );
    } );
}

void NetworkManager::restart() {
    // NOTE: this handler is asynchronous
    m_taskLoop.addTask( [=]() { resetWiFi(); } );
}

void NetworkManager::checkAndFixInterfaceStates() {
    LOGGER_TRACE_RAW( cc::debug( "Try to check and fix interface states" ) );
    bool reloadWifi = false;

    if( m_clientWifiInterface.isEnabled() ) {
        if( !m_clientWifiInterface.isUp() ) {
            LOGGER_WARN_RAW( cc::warn( "Interface " ) + cc::note( m_clientWifiInterface.getName() ) +
                             cc::warn( " is enabled but not UP" ) );
            reloadWifi = true;
        }
        else if( m_clientWifiInterface.getIpAddress().empty() ) {
            LOGGER_WARN_RAW( cc::warn( "Interface " ) + cc::note( m_clientWifiInterface.getName() ) +
                             cc::warn( " is enabled but not connected" ) );
            bool apIsUp = m_apWifiInterface.isUp();
            LOGGER_DEBUG_RAW( cc::debug( "Wifi client recovery: Access point: " ) +
                              cc::note( ( apIsUp ) ? "is up" : "not up" ) );

            if( !apIsUp ) {
                reloadWifi = true;
            }
            else {
                // NOTE: AP can block the client by using different channel.
                // This issue is common when network has many SSID's with the same value.
                std::string ssid;
                bool hasSSID = false;
                std::tie( ssid, hasSSID ) = m_clientWifiInterface.getSSID();
                if( !hasSSID ) {
                    LOGGER_ERROR_RAW( cc::error( "Wifi client recovery: Client does not have SSID in settings" ) );
                    // Client has invalid settings, nothing we can do
                }
                else {
                    // Check if SSID is present
                    LOGGER_DEBUG_RAW( cc::debug( "Wifi client recovery: Run wifi scan for SSID: " ) +
                                      cc::note( ssid ) );
                    // NOTE: Current implementation of the scanner will run multiple scans until it found SSID
                    const auto network =
                        m_wifiPassiveScanner.scanAndGetNetwork( m_clientWifiInterface.getName(), ssid, "" );
                    if( network.m_channel >= 0 && network.m_ssid == ssid ) {
                        LOGGER_DEBUG_RAW(
                            cc::debug( "Wifi client recovery: Network is found. Disabling blocking access point." ) );
                        // NOTE: On H2 hubs, access point will block client if it is run on different frequency that AP.
                        // Since we don't use BSSID anymore, there is no way to guess to which client is willing to
                        // connect.

                        m_apInterfaceCheckTimer.stop();
                        m_apInterfaceCheckTimer.start( std::chrono::seconds( 60 ), ApInterfaceCheckPeriod );
                        m_apWifiInterface.disable();

                        reloadWifi = true;
                    }
                    else {
                        LOGGER_DEBUG_RAW( cc::debug( "Wifi client recovery: Network not found in scan result" ) );
                    }
                }
            }
        }
        else if( false == m_clientWifiInterface.isInternetAvailable() ) {
            const std::chrono::seconds uptime(
                json_get_int( getInterfacesUptime()[ m_clientWifiInterface.getName() ], "uptime", 0 ) );
            if( uptime > std::chrono::minutes( 10 ) ) {
                LOGGER_WARN_RAW(
                    cc::warn( "Wifi client recovery: WiFi client is connected but does not have the internet" ) );
                reloadWifi = true;
            }
        }
    }

    if( m_apWifiInterface.isEnabled() ) {
        if( m_apWifiInterface.getIpAddress().empty() ) {
            LOGGER_WARN_RAW( cc::warn( "Interface " ) + cc::note( m_apWifiInterface.getName() ) +
                             cc::warn( " is enabled but not UP" ) );
            reloadWifi = true;
        }
    }

#if defined( __HUB_TYPE_H2__ ) || defined( __HUB_TYPE_H2_SECURE__ )
    if( reloadWifi ) {
        LOGGER_INFO_RAW( cc::info( "Reloading wireless interfaces" ) );
        // TODO add '/etc/init.d/network reload' if 'wifi' command does not help
        auto errorReport = System::getCommandOutputWithErrorReport( "wifi" );
        if( errorReport.exitCode != 0 ) {
            LOGGER_WARN_RAW( cc::warn( "Failed to reload wireless interfaces: \"wifi\" command failed with code: " ) +
                             cc::num10( errorReport.exitCode ) + cc::warn( ", err: " ) +
                             cc::note( errorReport.output ) );
        }
    }
#endif
}

Json NetworkManager::getUINetworkConfiguration() const {
    return m_ethernetInterface.getUINetworkConfiguration();
}

ExternalDataProvider& NetworkManager::getExternalData() {
    return *m_externalDataProvider;
}

void NetworkManager::onCloudConnected() {
    updateAdditionalPingServers();
}

/** Returned schema: object: ( <deviceName>, { uptime: int64, interface: string, up: bool }
 * Returns uptime for interfaces that have "device" property and uptime greater than zero..
 */
JsonObject NetworkManager::getInterfacesUptime() {
    const auto fetchInterfacesList = "ubus call network.interface dump";
    std::string callResult;
    int exitCode;
    std::tie( callResult, exitCode ) = System::getCommandOutput( fetchInterfacesList );

    if( exitCode != 0 ) {
        return JSON_EMPTY_OBJ;
    }

    Json interfacesStatusDump;
    JsonObject result = JSON_EMPTY_OBJ;

    try {
        if( !callResult.empty() ) {
            parseJson( callResult, interfacesStatusDump );
        }

        JsonArray interfaces = JSON_EMPTY_OBJ;
        if( json_get_arr( interfacesStatusDump, "interface", interfaces ) ) {
            for( auto itemIt = interfaces.begin(); itemIt != interfaces.end(); ++itemIt ) {
                const auto& interface = *itemIt;
                auto name = json_get_str( interface, "interface" );
                auto device = json_get_str( interface, "device" );
                int64_t uptime = json_get_int64( interface, "uptime", 0 );
                bool isUp = json_get_bool( interface, "up" );

                if( !device.empty() && uptime > 0 ) {
                    result[ device ] = JSON( "up" << isUp << "interface" << name << "uptime" << uptime );
                }
            }
        }
        LOGGER_TRACE( "Interfaces transformed : " + result.dump() );
    }
    catch( const std::exception& e ) {
        LOGGER_WARN(
            cc::warn( "Unable to parse interfaces uptime, error: " + cc::note( CommonUtils::whatError( e ) ) ) );
    }

    return result;
}

JsonArray NetworkManager::getPingStatisticsJson() {
    auto stats = JSON_EMPTY_ARR;

    std::vector< ServiceMetricsData::PingStatisticsData > data;
    data.reserve( 4 );

    data.emplace_back( m_ethernetInterface.getPingStatistics().getPingStatisticsData() );
    data.emplace_back( m_clientWifiInterface.getPingStatistics().getPingStatisticsData() );
    m_gsmManager.foreachGSMInterface( [&data]( GSMInterface& gsmInterface ) {
        data.push_back( gsmInterface.getPingStatistics().getPingStatisticsData() );
    } );

    for( auto& element : data ) {
        if( !element.interfaceName.empty() ) {
            stats.push_back( element.toJson() );
        }
    }
    return stats;
}

void NetworkManager::printPingStats() {
    m_ethernetInterface.getPingStatistics().print();

    if( m_clientWifiInterface.isEnabled() ) {
        m_clientWifiInterface.getPingStatistics().print();
    }

    m_gsmManager.foreachGSMInterface( []( GSMInterface& gsmInterface ) { gsmInterface.getPingStatistics().print(); } );
}

void NetworkManager::syncCameraStreams() {
    auto result = m_dbClient.findAll( CameraStreamsDBName, JSON_EMPTY_OBJ, nullptr );
    auto cameras = cameraStreamsToPingHosts( result );
    updateCameraPingHosts( std::move( cameras ) );
}

}  // namespace Network
}  // namespace HubPlatform
