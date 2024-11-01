#include "networkd/network_service.h"
#include "networkd/resources/nm_resources.h"
#include "networkd/monitoring/nm_interface_traffic_monitor.h"
#include "networkd/utils/nm_utils.h"
#include "http/http_events.h"
#include "http/http_options.h"
#include "cloud_storage/controller_token.h"
#if defined( __HUB_TYPE_LOCAL__ ) || defined( __HUB_TYPE_VHUB__ )
#include "networkd/configuration/nm_dummy_config_manager.h"
#else
#include "networkd/configuration/nm_openwrt_config_manager.h"
#endif

#include "cored/remote_interface/cored_interface_factory.h"
#include "service_interfaces/groups_names.h"
#include "bus/messagesclasses.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include "jsonrpc/jsonrpc.h"
#include "provision_data_utils/provision_data_manager.h"
#include "common_utils/backup/backup_helper.h"
#include "common_utils/backup/restore_helper.h"
#include "common_utils/lock_helper.h"
#include "common_utils/hub_info_helper.h"
#include "common_utils/exception_utils.h"
#include "system_utils/system.h"
#include "system_utils/filesystem.h"
#include "database_client/dbqueryparser.h"
#include "system_utils/environment.h"
#include "base_utils/utils.h"
#include "base_utils/return_action.h"
#include "hub_settings/hub_settings_reader.h"
#include "cloud_storage/controller_token.h"
#include "service_metrics_data/hub_errors_table/hub_errors_table.h"
#include "network_reporter.h"
#include "networkd/monitoring/nm_aux_servers.h"

#include <cassert>
#include <map>
#include <utility>

namespace HubPlatform {
namespace Network {

const SshTunnel::Identity Service::HttpServerIdentity = "firmware_http_server";

Service::Service()

: Services::Service( GroupsNames::NETWORK ) {}

void Service::setUp() {
    createDatabase();
    m_tunnelManager = std::make_unique< SshTunnel::TunnelManagerImpl >();
    m_sshCameraTunnel = std::make_unique< SshTunnel::SshCameraTunnel >( *this, *m_tunnelManager );
    m_sshSupportTunnel = std::make_unique< SshTunnel::SshSupportTunnel >( *this, *m_tunnelManager );
    m_statisticsSettingsManager = std::make_unique< StatisticsSettingsManager >( getDBClient() );

    Services::Service::setUp();
    join( GroupsNames::HARDWARE_BROADCASTS );  // separate group for broadcasts only
    join( GroupsNames::DEVICE_GROUP );
    join( GroupsNames::CAMERAS_BROADCASTS );
    join( GroupsNames::BROADCAST_GROUP );

    Hub::SettingsReader reader( getDBClient() );
    if( auto pingTimeoutSetting = reader.findSetting( Resources::Settings::PingTimeout ) ) {
        setPingTimeout( std::chrono::seconds( pingTimeoutSetting->getIntValue() ) );
    }

#if defined( __HUB_TYPE_LOCAL__ ) || defined( __HUB_TYPE_VHUB__ )
    m_networkConfigManager = std::make_unique< DummyNetworkConfigManager >();
#else
    m_networkConfigManager = std::make_unique< OpenwrtNetworkConfigManager >(
        ProvisionDataUtils::getManager().getProvisionData(), getDBClient() );
#endif
    m_networkConfigFileManager = std::make_unique< NetworkConfigFileManager >();
    m_cored = HubPlatform::CoreDRemoteInterface::makeInstance( *this );
    m_configReaderPtr = std::make_shared< ConfigReader >();
    m_networkManager = std::make_shared< NetworkManager >( getDBClient(),
                                                           ProvisionDataUtils::getManager().getProvisionData(),
                                                           *m_networkConfigManager,
                                                           *m_networkConfigFileManager,
                                                           *this,
                                                           *this,
                                                           *this,
                                                           *m_cored,
                                                           *m_configReaderPtr );

    Service::Service::setReporter( std::make_unique< NetworkReporter >( getName(), *this, m_networkManager ) );

    m_sshCameraTunnel->initTunnels();
    m_sshCameraTunnel->setOnStatusChangedCallback(
        std::bind( &Service::onCameraTunnelStatusChanged, this, std::placeholders::_1, std::placeholders::_2 ) );
    m_sshTunnelTypes[ HttpServerIdentity ] = ServiceMetricsData::SshTunnelData::SshTunnelType::TYPE_HTTP_SERVER;

    m_sshSupportTunnel->initTunnels();
    m_sshSupportTunnel->createSupportTunnel();
    m_manualCommunicationTest =
        std::make_unique< CommunicationTest::ManualCommunicationTest >( *this, m_networkManager );

    sendPostponedLogs();
    extractKernelCrashLogs();
}

void Service::tearDown() {
    LOGGER_TRACE( cc::trace( "started" ) );
    m_sshSupportTunnel->tearDown();
    discardAllSshTunnelStops();

    m_networkManager.reset();
    m_cored.reset();
    m_networkConfigFileManager.reset();
    m_networkConfigManager.reset();
    m_statisticsSettingsManager.reset();
    m_sshSupportTunnel.reset();
    m_sshCameraTunnel.reset();
    m_tunnelManager.reset();

    Services::Service::tearDown();
    LOGGER_TRACE( cc::trace( "finished" ) );
}

void Service::handleMethodCall( const JsonObject& request, MessageSubClasses requestType, const std::string& sender ) {
    try {
        tryHandleMethodCall( request, requestType, sender );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Uncaught exception: method call request failed: " ) +
                      cc::note( CommonUtils::whatError( ex ) ) );
    }
}

void Service::tryHandleMethodCall( const JsonObject& request,
                                   MessageSubClasses requestType,
                                   const std::string& sender ) {
    switch( requestType ) {
    case NETWORK_RESET: {
        auto interfaceId = json_get_str( request, "interfaceId" );
        Log::info( cc::info( "Got request to reset network settings, interface id: " ) + cc::note( interfaceId ) );

        m_networkManager->resetInterface( interfaceId );
        break;
    }
    case NETWORK_WIFI_SCAN_START: {
        LOGGER_TRACE_RAW( cc::trace( "Got request to start scanning for wireless networks" ) );
        const std::string interfaceId = json_get_str( request, "interfaceId" );
        assert( !interfaceId.empty() );
        m_networkManager->startScan( interfaceId );
        break;
    }
    case NETWORK_WIFI_SCAN_STOP: {
        LOGGER_TRACE_RAW( cc::trace( "Got request to stop scanning for wireless networks" ) );
        const std::string interfaceId = json_get_str( request, "interfaceId" );
        assert( !interfaceId.empty() );
        m_networkManager->stopScan( interfaceId );
        break;
    }
    case NETWORK_WIFI_TRY_CONNECT: {
        auto ssid = json_get_str( request, "ssid" );
        Log::info( cc::info( "Got request to connect to " ) + cc::note( ssid ) + cc::info( " wireless network" ) );
        auto bssid = json_get_str( request, "bssid" );
        auto encryption = json_get_str( request, "encryption" );
        auto key = json_get_str( request, "key" );
        auto force = json_get_bool( request, "force" );
        assert( !ssid.empty() );
        assert( !encryption.empty() );

        // NOTE: Since bssid support was removed (ELET-117),
        // client can connect to any router's band, thus we need to always
        // disable hub's access point to prevent it from blocking the client
        // https://confluence.mios.com/display/ELFD/Wifi+client+and+access+point+coexistence
        const bool temporaryDisableAp = true;
        m_networkManager->connectToWiFi( ssid, bssid, encryption, key, force, temporaryDisableAp );
        break;
    }
    case NETWORK_STATISTICS_GET: {
        RemoteInterface::StatisticsPeriod period( request );

        LOGGER_DEBUG_RAW( cc::debug( "Got request to get statistics" ) );

        handleStatisticsGet( period );
        break;
    }
    }

    handleMobileMethodCalls( request, requestType, sender );
}

void Service::handleMobileMethodCalls( const JsonObject& request,
                                       MessageSubClasses requestType,
                                       const std::string& /*sender*/ ) {
    switch( requestType ) {
    case NETWORK_MODEM_TRY_CONNECT: {
        auto interfaceId = json_get_str( request, "interfaceId" );
        auto mobileSettings = RemoteInterface::MobileSettings::constructMobileSettings( request[ "settings" ] );

        auto mobileSettingsJs = mobileSettings.toJson();
        if( !mobileSettings.m_pinCode.empty() ) {
            mobileSettingsJs[ "pinCode" ] = "****";
        }
        LOGGER_INFO_RAW( cc::info( "Got request to connect to mobile network for an interface " ) +
                         cc::note( interfaceId ) + cc::info( " with the next settings: " ) +
                         cc::note( mobileSettingsJs.dump() ) );

        try {
            auto& gsmManager = m_networkManager->getGSMManager();
            auto& gsmInterface = gsmManager.getGSMInterface( interfaceId );

            if( !mobileSettings.m_pinCode.empty() ) {
                if( gsmInterface.isPinCodeRequired() ) {
                    gsmInterface.enterPinCode( mobileSettings.m_pinCode );
                }
                else {
                    LOGGER_WARN_RAW( cc::warn( "SIM PIN code is specified when it's not needed" ) );
                    throw GSM::GSMOperationNotAllowed();
                }
            }

            gsmInterface.setUserSettings( mobileSettings.m_apn,
                                          mobileSettings.m_dialNumber,
                                          mobileSettings.m_pinCode,
                                          mobileSettings.m_operatorName,
                                          mobileSettings.m_pdpType );

            gsmInterface.tryUp();
        }
        catch( const std::exception& ex ) {
            const auto error = GSMManager::handleGSMErrors( ex );

            LOGGER_ERROR_RAW( cc::error( "Failed to establish connection for an interface " ) +
                              cc::note( interfaceId ) + cc::error( ": " ) + cc::note( error.dump() ) );
            sendBroadcast( NETWORK_MODEM_TRY_CONNECT_FAILED, error );
        }
        break;
    }
    case NETWORK_MODEM_PIN_CODE_SET: {
        auto interfaceId = json_get_str( request, "interfaceId" );
        auto pinCodeOld = json_get_str( request, "pinCodeOld" );
        auto pinCodeNew = json_get_str( request, "pinCodeNew" );

        LOGGER_INFO_RAW( cc::info( "Got request to set a new SIM PIN code for an interface " ) +
                         cc::note( interfaceId ) );

        try {
            auto& gsmManager = m_networkManager->getGSMManager();
            auto& gsmInterface = gsmManager.getGSMInterface( interfaceId );

            gsmInterface.changePinCode( pinCodeNew, pinCodeOld );

            LOGGER_INFO_RAW( cc::info( "SIM PIN code for an interface " ) + cc::note( interfaceId ) +
                             cc::info( " has been changed" ) );
            sendBroadcast( NETWORK_MODEM_PIN_CODE_SET, JSON_EMPTY_OBJ );
        }
        catch( const std::exception& ex ) {
            const auto error = GSMManager::handleGSMErrors( ex );

            LOGGER_ERROR_RAW( cc::error( "Failed to set a new SIM PIN code for an interface " ) +
                              cc::note( interfaceId ) + cc::error( ": " ) + cc::note( error.dump() ) );
            sendBroadcast( NETWORK_MODEM_PIN_CODE_SET_FAILED, error );
        }
        break;
    }
    case NETWORK_MODEM_PIN_CODE_RESET: {
        auto interfaceId = json_get_str( request, "interfaceId" );
        auto pukCode = json_get_str( request, "pukCode" );
        auto pinCodeNew = json_get_str( request, "pinCodeNew" );

        LOGGER_INFO_RAW( cc::info( "Got request to reset a SIM PIN code for an interface " ) +
                         cc::note( interfaceId ) );

        try {
            auto& gsmManager = m_networkManager->getGSMManager();
            auto& gsmInterface = gsmManager.getGSMInterface( interfaceId );

            gsmInterface.resetPinCode( pukCode, pinCodeNew );

            LOGGER_INFO_RAW( cc::info( "SIM PIN code for an interface " ) + cc::note( interfaceId ) +
                             cc::info( " has been reset" ) );
            sendBroadcast( NETWORK_MODEM_PIN_CODE_RESET, JSON_EMPTY_OBJ );
        }
        catch( const std::exception& ex ) {
            const auto error = GSMManager::handleGSMErrors( ex );

            LOGGER_ERROR_RAW( cc::error( "Failed to reset SIM PIN code for an interface " ) + cc::note( interfaceId ) +
                              cc::error( ": " ) + cc::note( error.dump() ) );
            sendBroadcast( NETWORK_MODEM_PIN_CODE_RESET_FAILED, error );
        }
        break;
    }
    case NETWORK_MODEM_SIM_LOCK_SET: {
        auto interfaceId = json_get_str( request, "interfaceId" );
        auto pinCode = json_get_str( request, "pinCode" );
        auto lock = json_get_bool( request, "lock" );

        LOGGER_INFO_RAW( cc::info( "Got request to turn " ) + cc::note( lock ? "on" : "off" ) +
                         cc::info( " SIM Lock for an interface " ) + cc::note( interfaceId ) );

        try {
            auto& gsmManager = m_networkManager->getGSMManager();
            auto& gsmInterface = gsmManager.getGSMInterface( interfaceId );

            gsmInterface.setSIMLock( pinCode, lock );

            LOGGER_INFO_RAW( cc::info( "SIM Lock for an interface " ) + cc::note( interfaceId ) +
                             cc::info( " has been turned " ) + cc::note( lock ? "on" : "off" ) );
        }
        catch( const std::exception& ex ) {
            const auto error = GSMManager::handleGSMErrors( ex );

            LOGGER_ERROR_RAW( cc::error( "Failed to change SIM Lock for an interface " ) + cc::note( interfaceId ) +
                              cc::error( ": " ) + cc::note( error.dump() ) );
            sendBroadcast( NETWORK_MODEM_SIM_LOCK_SET_FAILED, error );
        }
        break;
    }
    case NETWORK_MODEM_REREGISTER: {
        auto interfaceId = json_get_str( request, "interfaceId" );
        auto clearFPLMN = json_get_bool( request, "clearFPLMN", false );

        LOGGER_INFO_RAW( cc::info( "Got request to perform network re-registration, interface " ) +
                         cc::note( interfaceId ) );

        try {
            auto& gsmManager = m_networkManager->getGSMManager();
            auto& gsmInterface = gsmManager.getGSMInterface( interfaceId );

            gsmInterface.reRegister( clearFPLMN );

            LOGGER_INFO_RAW( cc::info( "Network re-registration happened, interface " ) + cc::note( interfaceId ) );
        }
        catch( const std::exception& ex ) {
            const auto error = GSMManager::handleGSMErrors( ex );

            LOGGER_ERROR_RAW( cc::error( "Failed to perform network re-registration, interface " ) +
                              cc::note( interfaceId ) + cc::error( ", reason " ) + cc::note( error.dump() ) );
            sendBroadcast( NETWORK_MODEM_REREGISTRATION_FAILED, error );
        }
        break;
    }
    case NETWORK_MODEM_CLEAR_FPLMN: {
        auto interfaceId = json_get_str( request, "interfaceId" );

        LOGGER_INFO_RAW( cc::info( "Got request to clear FPLMN, interface " ) + cc::note( interfaceId ) );

        try {
            auto& gsmManager = m_networkManager->getGSMManager();
            auto& gsmInterface = gsmManager.getGSMInterface( interfaceId );

            gsmInterface.clearFPLMN();

            LOGGER_INFO_RAW( cc::info( "FPLMN was cleared, interface " ) + cc::note( interfaceId ) );
        }
        catch( const std::exception& ex ) {
            const auto error = GSMManager::handleGSMErrors( ex );

            LOGGER_ERROR_RAW( cc::error( "Failed to clear FPLMN, interface " ) + cc::note( interfaceId ) +
                              cc::error( ", reason " ) + cc::note( error.dump() ) );
            sendBroadcast( NETWORK_MODEM_CLEAR_FPLMN_FAILED, error );
        }
        break;
    }
    }
}

JsonObject Service::handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) {
    try {
        return tryHandleRequestFromBus( request, requestType );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( "Uncaught exception: failed to handle event from bus: " + cc::j( request ) +
                      ", error: " + CommonUtils::whatError( ex ) );
        Json response;
        setError( response, Errors::INTERNAL_UNHANDLED_EXCEPTION_ERROR, ex.what() );
        return response;
    }
}

JsonObject Service::tryHandleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) {
    switch( requestType ) {
    case NETWORK_ENABLED_SET: {
        // TODO: move/copy checking for ability to set enable policy to remote interface
        auto interfaceId = json_get_str( request, "interfaceId" );
        auto enabled = json_get_str( request, "enabled" );

        Log::info( cc::info( "Got request to set interface " ) + cc::note( interfaceId ) + ( " enabled to " ) +
                   cc::note( enabled ) );

        auto error = m_networkManager->setEnablePolicy( interfaceId, enabled );
        if( error != Errors::OK ) {
            Json result;
            setError( result, error );
            return result;
        }

        return {};
    }
    case NETWORK_SETTINGS_SET: {
        JsonObject settings;
        auto interfaceId = json_get_str( request, "interfaceId" );
        json_get_obj( request, "settings", settings );

        LOGGER_DEBUG_RAW( cc::debug( "Got request to set custom settings for interface " ) + cc::note( interfaceId ) +
                          cc::debug( " , custom settings: " ) + cc::j( settings ) );

        return handleCustomSettingsSet( interfaceId, settings );
    }
    case NETWORK_STATISTICS_SETTINGS_SET: {
        StatisticsSettingsManager::StatisticsSettings settings( request );

        LOGGER_DEBUG_RAW( cc::debug( "Got request to set statistics settings: " ) + cc::j( settings.toJson() ) );

        return handleStatisticSettingsSet( settings );
    }
    case HUB_SSH_TUNNEL_SUPPORT_ENABLED_SET: {
        handleSshTunnelSupportEnabledSet( request );
        return {};
    }
    case GLOBAL_LOCK: {
        auto& lockHelper = CommonUtils::LockHelper::global();
        lockHelper.tryLock( request );
        return lockHelper.getResponse();
    }
    case GLOBAL_UNLOCK: {
        auto& lockHelper = CommonUtils::LockHelper::global();
        lockHelper.unlock( request );
        return lockHelper.getResponse();
    }
    case BACKUP_CREATE: {
        CommonUtils::BackupHelper helper( getDBClient(), request );
        const auto& provisionData = ProvisionDataUtils::getManager().getProvisionData();
        for( auto& tableFieldPair : Resources::BackupTableFieldPairs ) {
            if( !helper.backupDatabase( tableFieldPair.first, provisionData, tableFieldPair.second ) ) {
                return helper.getResponse();
            }
        }
        return helper.getResponse();
    }
    case REQUEST_CLOUD_STORAGE_TOKEN: {
        return getCloudStorageToken();
    }
    case NETWORK_COMMUNICATION_TEST: {
        auto interface = json_get_str( request, "interface" );
        auto operationId = json_get_str( request, "operationId" );
        auto timeout = json_get_int( request, "timeout" );
        return m_manualCommunicationTest->process( interface, operationId, timeout );
    }
    default:
        return Services::Service::handleRequestFromBus( request, requestType );
    }
}

bool Service::handleAsyncRequestFromBus( const JsonObject& request,
                                         MessageSubClasses requestType,
                                         const std::string& uuid,
                                         const std::string& sender ) {
    switch( requestType ) {
    case EXECUTE_AT_COMMAND_EMBEDDED: {
        handleExecuteATCommandEmbedded( request, uuid, sender );
    } break;

    default:
        return false;
    }

    return true;
}

void Service::handleExecuteATCommandEmbedded( const JsonObject& request,
                                              const std::string& uuid,
                                              const std::string& sender ) {
    std::string ATCommand;

    try {
        ATCommand = json_get_str( request, "ATCommand" );
        auto timeout = json_get_int( request, "timeout" );
        LOGGER_TRACE_RAW( cc::trace( "Got request to execute an AT command " ) +
                          cc::note( Utils::purgeCRSymbol( ATCommand ) ) + cc::trace( " on an embedded modem" ) );

        auto response = handleExecuteATCommandRequestEmbedded( ATCommand, std::chrono::milliseconds( timeout ) );
        sendAsyncResponse( sender, response, uuid );
    }
    catch( const std::exception& ex ) {
        JsonObject response;

        LOGGER_ERROR_RAW( cc::error( "Unable to execute an AT command " ) + cc::note( ATCommand ) +
                          cc::error( " on an embedded modem, " ) + cc::note( CommonUtils::whatError( ex ) ) );

        setError( response, Errors::FAILED_TO_EXECUTE_AT_COMMAND_EMBEDDED );
        sendAsyncResponse( sender, response, uuid );
    }
}

int Service::restoreBackup( const std::string& tempBackupFolder ) {
    CommonUtils::RestoreHelper helper( getDBClientSafe(), tempBackupFolder );
    const auto& provisionData = ProvisionDataUtils::getManager().getProvisionData();
    for( auto& tableFieldPair : Resources::BackupTableFieldPairs ) {
        if( !helper.restoreDatabase( tableFieldPair.first, provisionData, tableFieldPair.second ) ) {
            return helper.getReturnCode();
        }
    }
    return helper.getReturnCode();
}

void Service::handleMessageFromBus( const Services::Message& message, const std::string& sender ) {
    if( message.msgClass != EVENT ) {
        Services::Service::handleMessageFromBus( message, sender );
        return;
    }

    switch( message.msgSubClass ) {
    case BUTTON_WIFI_RESET:
        Log::info( cc::info( "Got request to reset wireless network settings via button" ) );
        m_networkManager->resetWiFi();
        break;
    case HOTPLUG_EVENT:
        LOGGER_DEBUG_RAW( cc::debug( "Got hotplug event: \n" + message.data.dump( 1 ) ) );
        addTask( [this, message]() { handleHotplugEvents( message.data ); } );
        break;
    case CAMERAS_BROADCAST_START_RTSP_STREAMING: {
        handleCamerasStartRtspStreamingBroadcast( message.data );
        break;
    }
    case CAMERAS_BROADCAST_STOP_RTSP_STREAMING:
        handleCamerasStopRtspStreamingBroadcast( message.data );
        break;
    case HUB_SETTING_UPDATED: {
        auto name = json_get_str( message.data, "name" );
        if( name == Resources::Settings::ModemSignalQualityPeriod ) {
            m_networkManager->onForceBroadcastSettingsChanged();
        }
        else if( name == Resources::Settings::PingTimeout ) {
            int value = json_get_int( message.data, "value" );
            if( value > 0 )
                setPingTimeout( std::chrono::seconds( value ) );
        }
        break;
    }
    case BT_INCOMMING_CONNECTION_ESTABLISHED: {
        auto hostMac = json_get_str( message.data, "hostMac" );
        auto peerMac = json_get_str( message.data, "peerMac" );

        LOGGER_DEBUG_RAW( cc::debug( "A new incoming bluetooth connection is established from a " ) +
                          cc::note( peerMac ) + cc::debug( " device" ) );

        /* TODO: accept an fd passed by a ha-bluetoothd/ha-hardware_manager */
        onBtIncomingConnectionEstablished( hostMac, peerMac );
        break;
    }
    case NMA_CLIENT_CONNECTED: {
        handleNmaClientConnected( message.data );
        m_networkManager->onCloudConnected();
        break;
    }
    case NMA_CLIENT_DISCONNECTED: {
        handleNmaClientDisconnected( message.data );
        break;
    }
    default:
        // Do nothing
        break;
    }
}

/*
 * {
 *   "deviceClass": "usb" | "ethernet",
 *   "type": "add" | "remove",
 *   "devicePath": string
 * }
 */
void Service::handleHotplugEvents( const Json& hotplugEvent ) {
    auto readUsbDescription = []( const std::string& eventType, const std::string& devicePath ) {
        if( eventType != "add" )
            return System::getUSBDescription( devicePath );

        System::USBDescription description;
        waitForCondition(
            [&devicePath, &description]() {
                description = System::getUSBDescription( devicePath );
                return !description.idProduct.empty() && !description.idVendor.empty();
            },
            std::chrono::seconds( 7 ),
            std::chrono::seconds( 1 ) );
        return description;
    };

    const auto deviceClass = json_get_str( hotplugEvent, "deviceClass" );
    const auto eventType = json_get_str( hotplugEvent, "type" );
    const auto devicePath = "/sys" + json_get_str( hotplugEvent, "devicePath" );

    if( deviceClass == "usb" ) {
        System::USBDescription usbDesc = readUsbDescription( eventType, devicePath );
        LOGGER_DEBUG_RAW( cc::debug( "Hotplug event for a usb device, device: " ) + cc::note( devicePath ) +
                          cc::debug( ", idVendor: " ) + cc::note( usbDesc.idVendor ) + cc::debug( ", idProduct: " ) +
                          cc::note( usbDesc.idProduct ) );

        auto& gsmManager = m_networkManager->getGSMManager();

        if( eventType == "add" ) {
            if( gsmManager.isMobileModem( devicePath ) ) {
                LOGGER_INFO_RAW( cc::info( "New mobile modem discovered, device: " ) + cc::note( devicePath ) );
                gsmManager.addModemEventToQueue( eventType, devicePath );
            }
        }
        else if( eventType == "remove" ) {
            gsmManager.addModemEventToQueue( eventType, devicePath );
        }
    }
}

void Service::handleCamerasStartRtspStreamingBroadcast( const Json& message ) {
    LOGGER_TRACE( cc::j( message ) );
    std::string localRtspUrl = json_get_str( message, "localRtspUrl" );
    if( localRtspUrl.empty() ) {
        LOGGER_ERROR( cc::error( "empty localRtspUrl" ) );
        return;
    }
    auto streamIdPos = localRtspUrl.find_last_of( "/" );
    auto portPos = localRtspUrl.find_last_of( ":" );
    if( streamIdPos == std::string::npos || portPos == std::string::npos ) {
        LOGGER_ERROR( cc::error( "incorrect localRtspUrl" ) );
        return;
    }
    auto identity = json_get_str( message, "streamId" );
    auto localPort = std::stoi( localRtspUrl.substr( portPos + 1, streamIdPos - 1 ) );

    auto identities = m_sshCameraTunnel->getIdentities();
    auto it = identities.find( HttpServerIdentity );
    if( it == identities.end() ) {
        if( isSshTunnelStopScheduled( HttpServerIdentity ) ) {
            discardSshTunnelStop( HttpServerIdentity );
        }
        else {
            m_sshCameraTunnel->createCameraTunnel( HttpServerIdentity, HttpServerPort );
            m_sshTunnelTypes[ HttpServerIdentity ] = ServiceMetricsData::SshTunnelData::SshTunnelType::TYPE_HTTP_SERVER;
        }
    }

    if( isSshTunnelStopScheduled( identity ) ) {
        discardSshTunnelStop( identity );
    }
    else {
        m_sshCameraTunnel->createCameraTunnel( identity, localPort );
        m_sshTunnelTypes[ identity ] = ServiceMetricsData::SshTunnelData::SshTunnelType::TYPE_CAMERA;
    }
}

void Service::handleCamerasStopRtspStreamingBroadcast( const Json& message ) {
    LOGGER_TRACE( cc::j( message ) );
    auto identity = json_get_str( message, "streamId" );

    scheduleSshTunnelStop( identity );

    auto identities = m_sshCameraTunnel->getIdentities();
    if( identities.size() == 1 && *identities.begin() == HttpServerIdentity ) {
        scheduleSshTunnelStop( HttpServerIdentity );
    }
}

void Service::handleSshTunnelSupportEnabledSet( const Json& message ) {
    bool enabled = message[ "enabled" ];
    int endTime = message[ "endTime" ];

    if( enabled )
        m_sshSupportTunnel->createSupportTunnel( endTime );
    else
        m_sshSupportTunnel->deleteSupportTunnel();
}

Json Service::handleCustomSettingsSet( const std::string& interfaceId, const Json& settings ) {
    auto error = m_networkManager->setCustomSettings( interfaceId, settings );
    if( error != Errors::OK ) {
        Json result;
        setError( result, error );
        return result;
    }

    return JSON_EMPTY_OBJ;
}

Json Service::handleStatisticSettingsSet( const StatisticsSettingsManager::StatisticsSettings& settings ) {
    m_statisticsSettingsManager->setSetting( settings );
    return JSON_EMPTY_OBJ;
}

void Service::handleStatisticsGet( const RemoteInterface::StatisticsPeriod& period ) {
    // TODO: provide statistics for the specified period

    using InterfaceName = std::string;
    std::map< InterfaceName, InterfaceTrafficMonitor::TrafficStatistic > trafficStats;
    auto result = JSON_EMPTY_OBJ;
    auto interfaces = JSON_EMPTY_ARR;

    m_networkManager->forEachNetworkInterface( [&trafficStats]( const NetworkInterface& interface ) {
        try {
            trafficStats.emplace( std::make_pair( interface.getName(), interface.getTrafficStatistics() ) );
        }
        catch( const std::exception& ex ) {
            LOGGER_ERROR( cc::error( "traffic statistics fetching failed: " ) +
                          cc::note( CommonUtils::whatError( ex ) ) );
        }
    } );

    for( const auto& elm : trafficStats ) {
        interfaces.push_back( JSON( "name" << elm.first << "input" << elm.second.m_input / ( 1024.0 * 1024 ) << "output"
                                           << elm.second.m_output / ( 1024.0 * 1024 ) ) );
    }

    result[ "units" ] = "MB";
    result[ "period" ] = period.toJson();
    result[ "interfaces" ] = interfaces;

    sendBroadcast( NETWORK_STATISTICS_GET, result );
}

void Service::onNetworkInfoChanged( Json diff ) {
    sendBroadcast( HUB_NETWORK_CHANGED, diff );
    if( m_manualCommunicationTest ) {
        m_manualCommunicationTest->onNetworkChanged( diff );
    }
}

void Service::onInternetAvailabilityChanged( bool internetAvailable ) {
    if( internetAvailable ) {
        LOGGER_TRACE( cc::trace( "Internet is available" ) );
    }
    else {
        LOGGER_TRACE( cc::trace( "Internet is NOT available" ) );
    }

    sendBroadcast( internetAvailable ? HUB_NETWORK_CONNECTED : HUB_NETWORK_DISCONNECTED, {} );

    m_sshCameraTunnel->setInternetAvailability( internetAvailable );  // should be after broadcast sending

    if( internetAvailable ) {
        sendPostponedLogs();

        stopBleAp();
    }
    else {
        /* we have no other way to access the Internet, so we bring up a ble AP (GATT Server) and wait till somebody
         * , who knows password, connects, ha-networkd will be notified about this and will create a new virtual ble
         * network interface to "use" it as an Internet provider */
        startBleAp();
    }
}

void Service::onInterfaceStatusChanged( const std::string& interfaceId,
                                        const std::string& interfaceType,
                                        bool isConnected ) {
    sendBroadcast( NETWORK_INTERFACE_STATUS_CHANGED,
                   JSON( "interfaceId" << interfaceId << "type" << interfaceType << "connected" << isConnected ) );
}

void Service::onCurrentNetworkChanged( const NetworkInfo& network ) {
    sendBroadcast( NETWORK_BROADCAST_IP_ADDRESS_CHANGED,
                   JSON( "ipv4" << network.ipv4 << "type" << network.type << "interfaceId" << network.interfaceId ) );
    m_sshCameraTunnel->onIpAddressChanged();
}

void Service::onScanStarted( const std::string& interfaceName ) {
    Json broadcastData;
    broadcastData[ "interfaceId" ] = interfaceName;
    broadcastData[ "status" ] = "started";
    sendBroadcast( NETWORK_WIFI_SCAN_PROGRESS, broadcastData );
}

void Service::onScanProcess( const std::string& interfaceName, JsonArray&& networks ) {
    Json broadcastData;
    broadcastData[ "interfaceId" ] = interfaceName;
    broadcastData[ "networks" ] = std::move( networks );
    broadcastData[ "status" ] = "process";
    sendBroadcast( NETWORK_WIFI_SCAN_PROGRESS, broadcastData );
}

void Service::onScanFinished( const std::string& interfaceName ) {
    Json broadcastData;
    broadcastData[ "interfaceId" ] = interfaceName;
    broadcastData[ "status" ] = "finished";
    sendBroadcast( NETWORK_WIFI_SCAN_PROGRESS, broadcastData );
}

void Service::onScanFailed( const std::string& interfaceName, const std::string& errorDescription ) {
    Json broadcastData;
    broadcastData[ "interfaceId" ] = interfaceName;
    broadcastData[ "status" ] = "failed";
    setError( broadcastData, Errors::WIFI_SCAN_FAILED, errorDescription );
    sendBroadcast( NETWORK_WIFI_SCAN_PROGRESS, broadcastData );
}

void Service::onCameraTunnelStatusChanged( const SshTunnel::Identity& identity, SshTunnel::Status status ) {
    if( identity != HttpServerIdentity ) {
        const std::string statusStr = SshTunnel::toString( status );
        if( !statusStr.empty() ) {
            sendBroadcast( NETWORK_CAMERA_CLOUD_TUNNEL_STATUS_CHANGED,
                           JSON( "streamId" << identity << "status" << statusStr ) );
        }
    }
}

void Service::sendPostponedLogs() {
    std::lock_guard< std::mutex > lock( m_sendPostponedLogsMutex );

    if( m_sendPostponedLogsTimer )
        m_sendPostponedLogsTimer->stop();

    int attempts = 0;
    m_sendPostponedLogsTimer = createPostponedTask( [this, attempts]( Services::PostponedTask& ) mutable {
        if( ++attempts > 3 ) {
            LOGGER_WARN_RAW( cc::warn( "Exceeded maximum number of attempts for sending postponed logs" ) );
            if( m_sendPostponedLogsTimer ) {
                m_sendPostponedLogsTimer->stop();
            }
            return;
        }
        LOGGER_DEBUG_RAW( cc::debug( "Try to send postponed logs" ) );

        Network::RemoteInterface networkRemoteInterface( *this );
        bool isInternetAvailable = networkRemoteInterface.isInternetAvailable();

        if( !isInternetAvailable ) {
            LOGGER_DEBUG_RAW( cc::debug( "Unable to send postponed logs: internet is not available" ) );
            return;
        }

        std::string command = HubPlatform::Environment::getBinariesFolder() + "/logcmd --upload-postponed";

        if( std::system( command.c_str() ) ) {
            LOGGER_WARN_RAW( cc::warn( "Failed to send postponed logs" ) );
            return;
        }

        LOGGER_DEBUG_RAW( cc::debug( "Task for sending postponed logs finished" ) );

        if( m_sendPostponedLogsTimer ) {
            m_sendPostponedLogsTimer->stop();
        }
    } );

    m_sendPostponedLogsTimer->start( std::chrono::seconds( 30 ), std::chrono::minutes( 5 ) );
}

void Service::extractKernelCrashLogs() {
#if !defined( __HUB_TYPE_H2__ ) && !defined( __HUB_TYPE_H2_SECURE__ )
    return;
#endif
    createPostponedTask( [this]( Services::PostponedTask& ) {
        Network::RemoteInterface networkRemoteInterface( *this );
        bool isInternetAvailable = networkRemoteInterface.isInternetAvailable();
        auto& dbClient = getDBClient();

        std::thread( [isInternetAvailable, &dbClient] {
            LOGGER_INFO_RAW( cc::info( "Thread for Kernel crash logs extraction started" ) );
            ReturnAction onReturn(
                []() { LOGGER_INFO_RAW( cc::info( "Thread for Kernel crash logs extraction finished" ) ); } );

            const std::string pstoreDirectory = "/sys/fs/pstore";
            const std::string mountCmd = "/bin/mount -o noatime -t pstore pstore " + pstoreDirectory;
            const std::string umountCmd = "/bin/umount " + pstoreDirectory;

            // pstore is not supported
            if( !filesystem::isExist( pstoreDirectory ) )
                return;

            ReturnAction umountAction( [&umountCmd] { std::system( umountCmd.c_str() ); } );
            std::system( mountCmd.c_str() );

            auto crashLogs = filesystem::findAllFiles( pstoreDirectory );
            if( crashLogs.empty() )
                return;

            LOGGER_WARN_RAW( cc::warn( "Kernel crash logs found!" ) );

            ServiceMetricsData::HubErrorsTable table( dbClient );
            table.incrementError( ServiceMetricsData::HubErrorsTable::ErrorType::KernelCrash );

            std::string kernelLogsDir = filesystem::createRandomFolder( HubPlatform::Environment::getTempDataFolder() );
            ReturnAction removeKernelLogsDir( [&kernelLogsDir] { filesystem::deleteFolder( kernelLogsDir ); } );

            for( auto& logName : crashLogs ) {
                std::string srcPath = pstoreDirectory + "/" + logName;
                std::string dstPath = kernelLogsDir + "/" + logName + ".log";
                filesystem::copyFile( srcPath, dstPath );
            }

            bool isLogsProcessed = false;
            if( isInternetAvailable ) {
                std::string uploadCommand =
                    HubPlatform::Environment::getBinariesFolder() +
                    "/logcmd --upload --no-system-logs --add-to-name=kernel_crash --log-dir=" + kernelLogsDir;
                isLogsProcessed = !std::system( uploadCommand.c_str() );
                if( !isLogsProcessed ) {
                    LOGGER_WARN_RAW( cc::warn( "Failed to send kernel crash logs" ) );
                }
            }

            if( !isLogsProcessed ) {
                std::string storeCommand =
                    HubPlatform::Environment::getBinariesFolder() +
                    "/logcmd --store-to-postponed --no-system-logs --add-to-name=kernel_crash --log-dir=" +
                    kernelLogsDir;
                isLogsProcessed = !std::system( storeCommand.c_str() );
                if( !isLogsProcessed ) {
                    LOGGER_WARN_RAW( cc::warn( "Failed to store kernel crash logs" ) );
                }
            }

            if( isLogsProcessed ) {
                filesystem::clearFolder( pstoreDirectory );
            }
            LOGGER_INFO_RAW( cc::info( "Thread for Kernel crash logs extraction finished" ) );
        } )
            .detach();  // Do not lock task look no matter what
    } )
        ->start( std::chrono::seconds( 90 ), false );
}

void Service::onBtIncomingConnectionEstablished( const std::string& hostMac, const std::string& peerMac ) {
    /*TODO: create a virtual ble network interface */
}

void Service::startBleAp() {
    sendMethodCall( "addon_bluetooth", BT_START_SERVICE_AP, JSON_EMPTY_OBJ );
}

void Service::stopBleAp() {
    sendMethodCall( "addon_bluetooth", BT_STOP_SERVICE_AP, JSON_EMPTY_OBJ );
}

bool Service::isSshTunnelStopScheduled( const SshTunnel::Identity& identity ) {
    std::lock_guard< std::mutex > lock( m_sshTunnelStopTasksMutex );

    return m_sshTunnelStopTasks.count( identity );
}

void Service::scheduleSshTunnelStop( const SshTunnel::Identity& identity ) {
    std::lock_guard< std::mutex > lock( m_sshTunnelStopTasksMutex );

    if( m_sshTunnelStopTasks.count( identity ) != 0 ) {
        LOGGER_WARN_RAW( cc::warn( "Attempt to schedule a camera ssh tunnel " ) + cc::note( identity ) +
                         cc::warn( " to stop, while it's being already scheduled to stop" ) );
        return;
    }

    auto postponedRemoveSshTunnelTask = createPostponedTask( [this, identity]( Services::PostponedTask& ) {
        LOGGER_DEBUG_RAW( cc::debug( "Execute a postponed request to stop a camera ssh tunnel " ) +
                          cc::note( identity ) );
        discardSshTunnelStop( identity );
        m_sshCameraTunnel->removeTunnel( identity );
        m_sshTunnelTypes.erase( identity );
    } );
    postponedRemoveSshTunnelTask->start( m_sshTunnelStopDelay, false );

    LOGGER_DEBUG_RAW( cc::debug( "Schedule a postponed request to stop an ssh tunnel " ) + cc::note( identity ) );

    m_sshTunnelStopTasks.emplace( identity, std::move( postponedRemoveSshTunnelTask ) );
}

void Service::discardSshTunnelStop( const SshTunnel::Identity& identity ) {
    std::lock_guard< std::mutex > lock( m_sshTunnelStopTasksMutex );

    if( m_sshTunnelStopTasks.count( identity ) == 0 ) {
        return;
    }

    LOGGER_DEBUG_RAW( cc::debug( "Discard a postponed request to stop an ssh tunnel " ) + cc::note( identity ) );

    m_sshTunnelStopTasks[ identity ]->stop( StoppingMode::NoWaiting );
    m_sshTunnelStopTasks.erase( identity );
}

void Service::discardAllSshTunnelStops() {
    std::lock_guard< std::mutex > lock( m_sshTunnelStopTasksMutex );

    for( auto& tunnelTask : m_sshTunnelStopTasks ) {
        tunnelTask.second->stop( StoppingMode::NoWaiting );
    }

    m_sshTunnelStopTasks.clear();
}

JsonObject Service::getCloudStorageToken() {
    using namespace Common::CloudStorageInterface;
    ControllerToken controllerToken;
    ControllerToken::fetchDbRecord( getDBClient(), controllerToken );

    JsonObject response;
    response[ "token" ] = controllerToken.getTokenValue();
    response[ "expires" ] = controllerToken.getExpirationTime().count();
    response[ "xapp" ] = controllerToken.getXAppValue();
    response[ "endpoints" ] = controllerToken.getEndpoints().toJson();
    response[ "controller_uuid" ] = HubInfoHelper::getControllerUUID();

    return response;
}

JsonObject Service::handleExecuteATCommandRequestEmbedded( const std::string& ATCommand,
                                                           const std::chrono::milliseconds& timeout ) {
    JsonObject result;

    if( m_networkManager->getGSMManager().noEmbeddedInterfaces() ) {
        LOGGER_WARN_RAW( cc::warn( "No embedded modem to execute custom AT command " ) + cc::note( ATCommand ) );
        return JSON_EMPTY_OBJ;
    }

    m_networkManager->getGSMManager().foreachGSMInterface( [&]( GSMInterface& interface ) {
        auto& genericModem = interface.getGenericModem();
        if( genericModem.isEmbedded() ) {
            result[ "result" ] = genericModem.executeCustomATCommand( ATCommand, timeout );
        }
    } );

    return result;
}

void Service::handleNmaClientConnected( const JsonObject& message ) {
    Log::info( cc::info( "Nma Client Connected: " ) + cc::j( message ) );

    m_networkManager->getExternalData().setNmaClientConnectionState( message, true );
}

void Service::handleNmaClientDisconnected( const JsonObject& message ) {
    Log::info( cc::info( "Nma Client Disconnected: " ) + cc::j( message ) );

    m_networkManager->getExternalData().setNmaClientConnectionState( message, false );
}

JsonObject Service::getServiceInfo() {
    return JSON( "interfacesUptime" << m_networkManager->getInterfacesUptime() << "pingStatistics"
                                    << m_networkManager->getPingStatisticsJson() );
}

bool updateTunnelMetricsData( const std::string& identity,
                              SshTunnel::SshTunnel& tunnel,
                              ServiceMetricsData::SshTunnelData& metrics ) {
    using Metrics = ServiceMetricsData::SshTunnelData;
    const std::map< SshTunnel::SshTunnel::TunnelState, Metrics::SshTunnelStatus > tunnelStatusMap = {
        { SshTunnel::SshTunnel::TunnelState::Running, Metrics::SshTunnelStatus::STATUS_OPEN },
        { SshTunnel::SshTunnel::TunnelState::Stopped, Metrics::SshTunnelStatus::STATUS_CLOSED },
    };

    const std::map< SshTunnel::Status, Metrics::SshTunnelConnectionStatus > connectionStatusMap = {
        { SshTunnel::Status::Connected, Metrics::SshTunnelConnectionStatus::STATUS_CONNECTED },
        { SshTunnel::Status::Rejected, Metrics::SshTunnelConnectionStatus::STATUS_CONNECTION_REFUSED },
    };

    SshTunnel::TunnelData tunnelData;
    SshTunnel::SshTunnel::TunnelState tunnelState;
    if( !tunnel.getTunnelData( identity, tunnelData, tunnelState ) ) {
        return false;
    }

    metrics.id = identity;
    tunnel.getTunnelPid( identity, metrics.pid );
    metrics.sshTunnelStatus = tunnelStatusMap.at( tunnelState );
    metrics.serverUrl = tunnelData.data.serverAddress;
    metrics.serverPort = tunnelData.data.serverPort;
    metrics.remotePort = tunnelData.data.remotePort;
    metrics.localPort = tunnelData.data.ownPort;

    std::chrono::steady_clock::time_point creationTime{};
    if( tunnel.getTunnelCreationTime( identity, creationTime ) ) {
        if( std::chrono::duration_cast< std::chrono::seconds >( creationTime.time_since_epoch() ).count() > 0 ) {
            auto creationTimeElapseTime = std::chrono::steady_clock::now() - creationTime;
            metrics.startTime = std::chrono::duration_cast< std::chrono::seconds >(
                                    std::chrono::system_clock::now().time_since_epoch() - creationTimeElapseTime )
                                    .count();
        }
    }

    std::chrono::milliseconds uptime;
    if( tunnel.getTunnelUptime( identity, uptime ) ) {
        metrics.uptime = std::chrono::duration_cast< std::chrono::seconds >( uptime ).count();
    }

    SshTunnel::SshTunnel::StatusStatistics statusStatistics;
    tunnel.getStatusStatistics( identity, statusStatistics );
    for( auto& element : statusStatistics ) {
        auto& status = element.first;
        auto it = connectionStatusMap.find( status );
        if( it != connectionStatusMap.cend() ) {
            auto& connectionStatus = it->second;
            metrics.connectionStatistics[ connectionStatus ] = element.second;
        }
    }
    return true;
}

JsonArray Service::dumpSshTunnelsMetricsData() {
    try {
        std::vector< ServiceMetricsData::SshTunnelData > tunnels;
        if( m_sshSupportTunnel ) {
            auto identities = m_sshSupportTunnel->getIdentities();
            for( auto& identity : identities ) {
                ServiceMetricsData::SshTunnelData metrics;
                if( updateTunnelMetricsData( identity, *m_sshSupportTunnel, metrics ) ) {
                    metrics.sshTunnelType = ServiceMetricsData::SshTunnelData::SshTunnelType::TYPE_SUPPORT;
                    tunnels.emplace_back( std::move( metrics ) );
                }
            }
        }

        if( m_sshCameraTunnel ) {
            auto identities = m_sshCameraTunnel->getIdentities();
            for( auto& identity : identities ) {
                ServiceMetricsData::SshTunnelData metrics;
                if( updateTunnelMetricsData( identity, *m_sshCameraTunnel, metrics ) ) {
                    auto it = m_sshTunnelTypes.find( identity );
                    if( it != m_sshTunnelTypes.cend() ) {
                        metrics.sshTunnelType = it->second;
                    }
                    tunnels.emplace_back( std::move( metrics ) );
                }
            }
        }

        auto netstatRecords = Utils::getNetstatRecords();
        for( auto& tunnel : tunnels ) {
            if( tunnel.pid <= 0 )
                continue;
            auto pidStr = std::to_string( tunnel.pid );
            for( auto& record : netstatRecords ) {
                if( record.pid == pidStr ) {
                    tunnel.tcpState = record.state;
                    break;
                }
            }
        }

        auto tunnelsArray = JSON_EMPTY_ARR;
        for( auto& tunnel : tunnels ) {
            tunnelsArray.push_back( tunnel.toJson() );
        }

        return tunnelsArray;
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( cc::error( "SSH tunnels metrics dump exception: " ) + cc::note( CommonUtils::whatError( e ) ) );
    }
    return JSON_EMPTY_ARR;
}

}  // namespace Network
}  // namespace HubPlatform
