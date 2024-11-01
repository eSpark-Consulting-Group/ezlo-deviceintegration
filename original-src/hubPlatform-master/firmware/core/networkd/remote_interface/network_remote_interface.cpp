#include "networkd/remote_interface/network_remote_interface.h"
#include "networkd/resources/nm_resources.h"

#include "service_interfaces/groups_names.h"
#include "bus/messagesclasses.h"
#include "service_interfaces/service.h"
#include "database_client/db_tables_names.h"
#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"
#include "jsonrpc/jsonrpc.h"
#include "common_utils/hub_info_helper.h"

#include <regex>
#include <cmath>

namespace HubPlatform {
namespace Network {

RemoteInterface::MobileSettings::MobileSettings() = default;

RemoteInterface::MobileSettings RemoteInterface::MobileSettings::constructMobileSettings( const Json& json ) {
    MobileSettings mobileSettings;

    mobileSettings.m_operatorName = json_get_str( json, "operator" );
    mobileSettings.m_apn = json_get_str( json, "apn" );
    mobileSettings.m_dialNumber = json_get_str( json, "dialNumber" );
    mobileSettings.m_pinCode = json_get_str( json, "pinCode" );
    mobileSettings.m_pdpType = json_get_str( json, "pdpType" );

    return mobileSettings;
}

Json RemoteInterface::MobileSettings::toJson() const {
    return JSON( "operator" << m_operatorName << "apn" << m_apn << "dialNumber" << m_dialNumber << "pinCode"
                            << m_pinCode << "pdpType" << m_pdpType );
}

RemoteInterface::StatisticsPeriod::StatisticsPeriod() = default;
RemoteInterface::StatisticsPeriod::StatisticsPeriod( const Json& period ) {
    auto checker = []( const std::string& date ) {
        if( !date.empty() && date.size() == std::string( "yyyy-mm-dd" ).size() ) {
            int y, m, d;
            if( sscanf( date.c_str(), "%4u-%2u-%2u", &y, &m, &d ) == 3 ) {
                return true;
            }
        }
        return false;
    };

    if( period.is_object() ) {
        auto begin = json_get_str( period, "begin" );
        auto end = json_get_str( period, "end" );

        if( checker( begin ) && checker( end ) ) {
            m_begin = begin;
            m_end = end;
            m_isValid = true;

            return;
        }
    }

    m_isValid = false;
}

Json RemoteInterface::StatisticsPeriod::toJson() const {
    return JSON( "begin" << m_begin << "end" << m_end );
}

bool RemoteInterface::StatisticsPeriod::isValid() const {
    return m_isValid;
}

const std::string& RemoteInterface::StatisticsPeriod::begin() const {
    return m_begin;
}

const std::string& RemoteInterface::StatisticsPeriod::end() const {
    return m_end;
}

RemoteInterface::RemoteInterface( Services::Service& hostService )
: m_hostService( hostService )
, m_statisticSettingsManager( hostService.getDBClient() ) {}

std::vector< Json > RemoteInterface::getNetworkInterfaces() {
    return m_hostService.getDBClient().findAll( TempNetworkInterfaces, JSON_EMPTY_OBJ, nullptr );
}

std::vector< Json > RemoteInterface::getConnectedNetworkInterfaces() {
    std::vector< Json > result;

    auto interfaces = m_hostService.getDBClient().findAll( TempNetworkInterfaces, JSON_EMPTY_OBJ, nullptr );
    for( Json& interface : interfaces ) {
        const std::string status = json_get_str( interface, Resources::InterfaceInfo::Fields::Status );
        if( status == Resources::InterfaceInfo::Statuses::Up ) {
            result.push_back( std::move( interface ) );
        }
    }

    return result;
}

Json RemoteInterface::getActiveNetworkInterface() {
    auto activeInterfaceJson = m_hostService.getDBClient().findOne(
        TempNetworkDynamicInfoDBName, JSON( db::ID_FIELD_NAME << Resources::HubActiveInterfaceId ), nullptr );
    auto interfaceId = json_get_str( activeInterfaceJson, "interfaceId" );
    if( interfaceId.empty() )
        return {};

    auto interface =
        m_hostService.getDBClient().findOne( TempNetworkInterfaces, JSON( db::ID_FIELD_NAME << interfaceId ), nullptr );

    if( json_get_str( interface, "status" ) != "up" ) {
        return {};
    }
    if( json_get_bool( interface, "internetAvailable" ) != true ) {
        return {};
    }
    if( json_get_str( interface, "type" ).empty() ) {
        Log::error( cc::error( "No network type found" ) );
        return {};
    }
    return interface;
}

Json RemoteInterface::getLocalNetworkInterface() {
    auto activeInterfaceJson = m_hostService.getDBClient().findOne(
        TempNetworkDynamicInfoDBName, JSON( db::ID_FIELD_NAME << Resources::HubActiveInterfaceId ), nullptr );
    auto interfaceId = json_get_str( activeInterfaceJson, "interfaceId" );
    if( !interfaceId.empty() ) {
        auto interface = m_hostService.getDBClient().findOne(
            TempNetworkInterfaces, JSON( db::ID_FIELD_NAME << interfaceId ), nullptr );
        auto type = json_get_str( interface, "type" );
        if( type == Resources::EthernetType || type == Resources::WifiType ) {
            return interface;
        }
    }
    else {
        auto interfaces = getNetworkInterfaces();
        for( auto interface : interfaces ) {
            auto status = json_get_str( interface, "status" );
            if( status != "up" ) {
                continue;
            }
            auto type = json_get_str( interface, "type" );
            if( type != Resources::EthernetType && type != Resources::WifiType ) {
                continue;
            }
            return interface;
        }
    }
    return {};
}

bool RemoteInterface::isInterfaceAllowedToReset( const std::string& interfaceId ) const {
    // TODO: currently we don't support several client wifi interfaces
    return isGSMInterface( interfaceId ) || interfaceId == Resources::ClientWifiDeviceName ||
           interfaceId == Resources::ApWifiDeviceName;
}

bool RemoteInterface::isGSMInterface( const std::string& interfaceId ) const {
    /* UI treats a device name (name of a physical linux interface) as an interface name/id */
    return m_hostService.getDBClient().has( GSMManagerInterfacesDBName, JSON( "deviceName" << interfaceId ) );
}

bool RemoteInterface::isInterfaceExist( const std::string& interfaceId ) const {
    /* UI treats a device name (name of a physical linux interface) as an interface name/id */
    return isGSMInterface( interfaceId ) || interfaceId == Resources::ClientWifiDeviceName ||
           interfaceId == Resources::ApWifiDeviceName || interfaceId == Resources::EthernetDeviceName;
}

bool RemoteInterface::isInternetAvailable() const {
    auto result = m_hostService.getDBClient().findOne(
        TempNetworkDynamicInfoDBName, JSON( db::ID_FIELD_NAME << Resources::HubInternetAvailabilityId ), nullptr );
    return json_get_bool( result, "internetAvailable" );
}

void RemoteInterface::resetNetwork( const std::string& interfaceId ) {
    m_hostService.sendMethodCall( GroupsNames::NETWORK, NETWORK_RESET, JSON( "interfaceId" << interfaceId ) );
}

void RemoteInterface::wifiScanStart( const std::string& interfaceId ) {
    m_hostService.sendMethodCall( GroupsNames::NETWORK, NETWORK_WIFI_SCAN_START, JSON( "interfaceId" << interfaceId ) );
}

void RemoteInterface::wifiScanStop( const std::string& interfaceId ) {
    m_hostService.sendMethodCall( GroupsNames::NETWORK, NETWORK_WIFI_SCAN_STOP, JSON( "interfaceId" << interfaceId ) );
}

void RemoteInterface::wifiTryConnect( const std::string& ssid,
                                      const std::string& bssid,
                                      const std::string& encryption,
                                      const std::string& key,
                                      bool force ) {
    m_hostService.sendMethodCall(
        GroupsNames::NETWORK,
        NETWORK_WIFI_TRY_CONNECT,
        JSON( "ssid" << ssid << "bssid" << bssid << "encryption" << encryption << "key" << key << "force" << force ) );
}

void RemoteInterface::modemTryConnect( const std::string& interfaceId, const MobileSettings& mobileSettings ) {
    m_hostService.sendMethodCall( GroupsNames::NETWORK,
                                  NETWORK_MODEM_TRY_CONNECT,
                                  JSON( "interfaceId" << interfaceId << "settings" << mobileSettings.toJson() ) );
}

void RemoteInterface::modemPinCodeSet( const std::string& interfaceId,
                                       const std::string& pinCodeOld,
                                       const std::string& pinCodeNew ) {
    m_hostService.sendMethodCall(
        GroupsNames::NETWORK,
        NETWORK_MODEM_PIN_CODE_SET,
        JSON( "interfaceId" << interfaceId << "pinCodeOld" << pinCodeOld << "pinCodeNew" << pinCodeNew ) );
}

void RemoteInterface::modemPinCodeReset( const std::string& interfaceId,
                                         const std::string& pukCode,
                                         const std::string& pinCodeNew ) {
    m_hostService.sendMethodCall(
        GroupsNames::NETWORK,
        NETWORK_MODEM_PIN_CODE_RESET,
        JSON( "interfaceId" << interfaceId << "pukCode" << pukCode << "pinCodeNew" << pinCodeNew ) );
}

void RemoteInterface::modemSIMLockSet( const std::string& interfaceId, const std::string& pinCode, bool lock ) {
    m_hostService.sendMethodCall( GroupsNames::NETWORK,
                                  NETWORK_MODEM_SIM_LOCK_SET,
                                  JSON( "interfaceId" << interfaceId << "pinCode" << pinCode << "lock" << lock ) );
}

void RemoteInterface::modemClearFPLMN( const std::string& interfaceId ) {
    m_hostService.sendMethodCall(
        GroupsNames::NETWORK, NETWORK_MODEM_CLEAR_FPLMN, JSON( "interfaceId" << interfaceId ) );
}

void RemoteInterface::modemReRegister( const std::string& interfaceId, bool clearFPLM ) {
    m_hostService.sendMethodCall( GroupsNames::NETWORK,
                                  NETWORK_MODEM_REREGISTER,
                                  JSON( "interfaceId" << interfaceId << "clearFPLM" << clearFPLM ) );
}

StatisticsSettingsManager::StatisticsSettings RemoteInterface::getStatisticsSettings() {
    return m_statisticSettingsManager.getSetting();
}

Json RemoteInterface::setStatisticsSettings( const StatisticsSettingsManager::StatisticsSettings& settings ) {
    return m_hostService.sendRequest( GroupsNames::NETWORK, settings.toJson(), NETWORK_STATISTICS_SETTINGS_SET );
}

void RemoteInterface::getStatistics( const StatisticsPeriod& period ) {
    m_hostService.sendMethodCall( GroupsNames::NETWORK, NETWORK_STATISTICS_GET, period.toJson() );
}

Json RemoteInterface::setEnablePolicy( const std::string& interfaceId, const std::string& enablePolicy ) {
    return m_hostService.sendRequest( GroupsNames::NETWORK,
                                      JSON( "interfaceId" << interfaceId << "enabled" << enablePolicy ),
                                      NETWORK_ENABLED_SET,
                                      std::chrono::seconds( 15 ) );  // interface reconfiguration may take some time...
}

Json RemoteInterface::setCustomSettings( const std::string& interfaceId, const Json& settings ) {
    return m_hostService.sendRequest(
        GroupsNames::NETWORK, JSON( "interfaceId" << interfaceId << "settings" << settings ), NETWORK_SETTINGS_SET );
}

void RemoteInterface::sshTunnelSupportEnabledSet( bool enabled, int endTime ) {
    m_hostService.sendRequest( GroupsNames::NETWORK,
                               JSON( "enabled" << enabled << "endTime" << endTime ),
                               HUB_SSH_TUNNEL_SUPPORT_ENABLED_SET,
                               std::chrono::seconds{ 15 } );
}

Json RemoteInterface::getSshTunnelSupportInfo() {
    auto supportInfo = JSON_EMPTY_OBJ;
    auto& dbClient = m_hostService.getDBClient();

    auto settingsQuery = JSON( "_id"
                               << "supportTunnelSettings" );
    auto settingsJson = dbClient.findOne( NetworkCloudSshTunnelsSettingsDBName, settingsQuery, nullptr );
    int endTime = json_get_int( settingsJson, "endTime", 0 );

    if( endTime > 0 ) {
        supportInfo[ "enabled" ] = true;

        std::vector< char > buffer( 30, 0 );
        std::time_t time = endTime;
        std::tm* tm = std::localtime( &time );
        strftime( buffer.data(), buffer.size(), "%Y-%m-%dT%T", tm );
        supportInfo[ "endTime" ] = std::string( buffer.data() );
    }
    else
        supportInfo[ "enabled" ] = false;
    auto dataQuery = JSON( "_id"
                           << "cloud.tunnel_support_coordinator" );
    auto data = dbClient.findOne( TempNetworkCloudSshTunnelsDBName, dataQuery, nullptr );
    auto status = json_get_str( data, "status" );
    supportInfo[ "status" ] = ( !status.empty() ) ? status : "disconnected";

    return supportInfo;
}

void RemoteInterface::requestADCValue( GetADCValueCallback cb, int adcPort ) {
    Services::Service::ReceiveCallback receiveCb = [cb]( const JsonObject& message ) {
        if( hasError( message ) ) {
            return cb( nanf( "Nan" ) );
        }

        auto result = json_get_str( message, "result" );
        static const std::regex re( "QADC: 1,(.*)" );
        std::smatch match;

        if( !std::regex_search( result, match, re ) ) {
            return cb( nanf( "Nan" ) );
        }

        cb( std::stof( match[ 1 ] ) );
    };

    m_hostService.sendAsyncRequest( GroupsNames::NETWORK,
                                    JSON( "ATCommand"
                                          << "at+qadc=" + std::to_string( adcPort ) + "\r"
                                          << "timeout" << 300 ),
                                    EXECUTE_AT_COMMAND_EMBEDDED,
                                    receiveCb,
                                    std::chrono::minutes( 2 ) );
}

void RemoteInterface::communicationTest( const std::string& interface,
                                         const std::string& operationId,
                                         std::chrono::seconds timeout,
                                         const std::function< void( const JsonObject& message ) >& callback ) {
    m_hostService.sendAsyncRequest(
        GroupsNames::NETWORK,
        JSON( "interface" << interface << "operationId" << operationId << "timeout" << timeout.count() ),
        NETWORK_COMMUNICATION_TEST,
        [callback]( const JsonObject& message ) { callback( message ); },
        std::chrono::minutes( 2 ) );
}

bool RemoteInterface::isCommunicationTestStarted() {
    auto statusObject =
        m_hostService.getDBClient().findOne( TempNetworkManualCommunicationTestStatus, JSON_EMPTY_OBJ, nullptr );
    return json_get_bool( statusObject, "started" );
}

}  // namespace Network
}  // namespace HubPlatform
