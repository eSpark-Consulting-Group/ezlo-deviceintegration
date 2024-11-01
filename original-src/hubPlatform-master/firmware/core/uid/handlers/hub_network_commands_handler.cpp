#include "uid/handlers/hub_network_commands_handler.h"
#include "uid/ui_service.h"
#include "service_interfaces/service.h"
#include "database_client/dbqueryparser.h"

#include "base_utils/acl.h"

#include <algorithm>

namespace HubPlatform {
namespace Uid {

HubNetworkCommandsHandler::HubNetworkCommandsHandler( Services::Service& hostService )
: m_networkdInterface( hostService ) {}

void HubNetworkCommandsHandler::invoke( const std::string& method, rpc::Message& rpcMessage ) {
    using RequestHandler = void ( HubNetworkCommandsHandler::* )( rpc::Message & rpcMessage );
    using RequestHandlersMap = std::unordered_map< std::string, RequestHandler >;
    static const RequestHandlersMap requestHandlers = {
        { "get", &HubNetworkCommandsHandler::handleGetRequest },
        { "settings.set", &HubNetworkCommandsHandler::handleSettingsSetRequest },
        { "reset", &HubNetworkCommandsHandler::handleResetRequest },
        { "wifi.scan.start", &HubNetworkCommandsHandler::handleWifiScanStartRequest },
        { "wifi.scan.stop", &HubNetworkCommandsHandler::handleWifiScanStopRequest },
        { "wifi.try_connect", &HubNetworkCommandsHandler::handleWifiTryConnectRequest },
        { "modem.try_connect", &HubNetworkCommandsHandler::handleModemTryConnectRequest },
        { "modem.pin_code.set", &HubNetworkCommandsHandler::handleModemPinCodeSetRequest },
        { "modem.pin_code.reset", &HubNetworkCommandsHandler::handleModemPinCodeResetRequest },
        { "modem.sim_lock.set", &HubNetworkCommandsHandler::handleModemSimLockSetRequest },
        { "modem.clear_fplmn", &HubNetworkCommandsHandler::handleModemClearFPLMNRequest },
        { "modem.reregister", &HubNetworkCommandsHandler::handleModemReRegisterRequest },
        { "statistics.settings.set", &HubNetworkCommandsHandler::handleStatisticsSettingsSetRequest },
        { "statistics.settings.get", &HubNetworkCommandsHandler::handleStatisticsSettingsGetRequest },
        { "statistics.get", &HubNetworkCommandsHandler::handleStatisticsGetRequest },
        { "enabled.set", &HubNetworkCommandsHandler::handleEnabledSetRequest },
        { "communication.test", &HubNetworkCommandsHandler::handleCommunicationTest },
    };

    const auto it = requestHandlers.find( method );
    if( it != requestHandlers.end() ) {
        const auto handler = it->second;
        ( this->*handler )( rpcMessage );
        return;
    }

    JsonObject& joAnswer = rpcMessage.getAnswer();
    setError( joAnswer, Errors::UNKNOWN_METHOD );
}

void HubNetworkCommandsHandler::handleGetRequest( rpc::Message& rpcMessage ) {
    auto interfaces = m_networkdInterface.getNetworkInterfaces();
    if( interfaces.empty() ) {
        setError( rpcMessage.getAnswer(), Errors::NETWORK_INFO_GETTING_ERROR );
        return;
    }

    rpcMessage.getAnswer()[ "result" ][ "interfaces" ] = interfaces;
}

void HubNetworkCommandsHandler::handleSettingsSetRequest( rpc::Message& rpcMessage ) {
    JsonObject settings;
    auto interfaceId = json_get_str( rpcMessage.getParams(), "interfaceId" );

    if( !interfaceId.empty() && !m_networkdInterface.isInterfaceExist( interfaceId ) ) {
        setError( rpcMessage.getAnswer(), Errors::NETWORK_INVALID_INTERFACE );
        return;
    }

    if( !getField( rpcMessage.getParams(), "settings", rpcMessage.getAnswer(), settings ) ) {
        return;
    }

    auto response = m_networkdInterface.setCustomSettings( interfaceId, settings );
    setErrorIfHas( rpcMessage.getAnswer(), response );
}

void HubNetworkCommandsHandler::handleResetRequest( rpc::Message& rpcMessage ) {
    auto interfaceId = json_get_str( rpcMessage.getParams(), "interfaceId" );

    if( !interfaceId.empty() && !m_networkdInterface.isInterfaceAllowedToReset( interfaceId ) ) {
        setError( rpcMessage.getAnswer(), Errors::NETWORK_INVALID_INTERFACE );
        return;
    }

    m_networkdInterface.resetNetwork( interfaceId );
    rpcMessage.setResult( JSON_EMPTY_OBJ );
}

void HubNetworkCommandsHandler::handleWifiScanStartRequest( rpc::Message& rpcMessage ) {
    std::string interfaceId;
    if( !getField( rpcMessage.getParams(), "interfaceId", rpcMessage.getAnswer(), interfaceId ) ) {
        return;
    }
    m_networkdInterface.wifiScanStart( interfaceId );
    rpcMessage.setResult( JSON_EMPTY_OBJ );
}

void HubNetworkCommandsHandler::handleWifiScanStopRequest( rpc::Message& rpcMessage ) {
    std::string interfaceId;
    if( !getField( rpcMessage.getParams(), "interfaceId", rpcMessage.getAnswer(), interfaceId ) ) {
        return;
    }
    m_networkdInterface.wifiScanStop( interfaceId );
    rpcMessage.setResult( JSON_EMPTY_OBJ );
}

void HubNetworkCommandsHandler::handleWifiTryConnectRequest( rpc::Message& rpcMessage ) {
    std::string interfaceId;
    const bool force = json_get_bool( rpcMessage.getParams(), "force", false );

    JsonObject network;
    if( !getField( rpcMessage.getParams(), "network", rpcMessage.getAnswer(), network ) ) {
        return;
    }

    std::string ssid;
    if( !getField( network, "ssid", rpcMessage.getAnswer(), ssid ) ) {
        return;
    }

    std::string encryption;
    if( !getField( network, "encryption", rpcMessage.getAnswer(), encryption ) ) {
        return;
    }
    std::string bssid = json_get_str( network, "bssid" );
    std::string key = json_get_str( network, "key" );

    m_networkdInterface.wifiTryConnect( ssid, bssid, encryption, key, force );
    rpcMessage.setResult( JSON_EMPTY_OBJ );
}

void HubNetworkCommandsHandler::handleModemTryConnectRequest( rpc::Message& rpcMessage ) {
    auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();

    std::string interfaceId;

    if( !getField( params, "interfaceId", answer, interfaceId ) ) {
        return;
    }

    if( !m_networkdInterface.isGSMInterface( interfaceId ) ) {
        setError( answer, Errors::NETWORK_INVALID_INTERFACE );
        return;
    }

    const auto mobileSettings = Network::RemoteInterface::MobileSettings::constructMobileSettings( params );

    if( false == mobileSettings.m_pdpType.empty() ) {
        auto& pdpType = mobileSettings.m_pdpType;
        if( pdpType != "IPv4" && pdpType != "IPv6" ) {
            setError( answer, Errors::MODEM_BAD_INTERNET_PARAMETERS, "Unknown PDP Type" );
            return;
        }
    }

    m_networkdInterface.modemTryConnect( interfaceId, mobileSettings );
    rpcMessage.setResult( JSON_EMPTY_OBJ );
}

void HubNetworkCommandsHandler::handleModemPinCodeSetRequest( rpc::Message& rpcMessage ) {
    auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();

    std::string interfaceId;

    if( !getField( params, "interfaceId", answer, interfaceId ) ) {
        return;
    }

    if( !m_networkdInterface.isGSMInterface( interfaceId ) ) {
        setError( answer, Errors::NETWORK_INVALID_INTERFACE );
        return;
    }

    std::string pinCodeOld;
    if( !getField( params, "pinCodeOld", answer, pinCodeOld ) ) {
        return;
    }

    std::string pinCodeNew;
    if( !getField( params, "pinCodeNew", answer, pinCodeNew ) ) {
        return;
    }

    if( pinCodeOld.empty() || pinCodeNew.empty() ) {
        setError( answer, Errors::MODEM_INVALID_PIN );
        return;
    }

    m_networkdInterface.modemPinCodeSet( interfaceId, pinCodeOld, pinCodeNew );
    rpcMessage.setResult( JSON_EMPTY_OBJ );
}

void HubNetworkCommandsHandler::handleModemPinCodeResetRequest( rpc::Message& rpcMessage ) {
    auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();

    std::string interfaceId;
    if( !getField( params, "interfaceId", answer, interfaceId ) ) {
        return;
    }

    if( !m_networkdInterface.isGSMInterface( interfaceId ) ) {
        setError( answer, Errors::NETWORK_INVALID_INTERFACE );
        return;
    }

    std::string pukCode;
    if( !getField( params, "pukCode", answer, pukCode ) ) {
        return;
    }

    std::string pinCodeNew;
    if( !getField( params, "pinCodeNew", answer, pinCodeNew ) ) {
        return;
    }

    if( pukCode.empty() || pinCodeNew.empty() ) {
        setError( answer, Errors::MODEM_INVALID_PIN );
        return;
    }

    m_networkdInterface.modemPinCodeReset( interfaceId, pukCode, pinCodeNew );
    rpcMessage.setResult( JSON_EMPTY_OBJ );
}

void HubNetworkCommandsHandler::handleModemSimLockSetRequest( rpc::Message& rpcMessage ) {
    auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();

    std::string interfaceId;
    if( !getField( params, "interfaceId", answer, interfaceId ) ) {
        return;
    }

    if( !m_networkdInterface.isGSMInterface( interfaceId ) ) {
        setError( answer, Errors::NETWORK_INVALID_INTERFACE );
        return;
    }

    std::string pinCode;
    if( !getField( params, "pinCode", answer, pinCode ) ) {
        return;
    }

    bool lock;
    if( !getField( params, "lock", answer, lock ) ) {
        return;
    }

    if( pinCode.empty() ) {
        setError( answer, Errors::MODEM_INVALID_PIN );
        return;
    }

    m_networkdInterface.modemSIMLockSet( interfaceId, pinCode, lock );
    rpcMessage.setResult( JSON_EMPTY_OBJ );
}

void HubNetworkCommandsHandler::handleModemClearFPLMNRequest( rpc::Message& rpcMessage ) {
    auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();

    std::string interfaceId;
    if( !getField( params, "interfaceId", answer, interfaceId ) ) {
        return;
    }

    if( !m_networkdInterface.isGSMInterface( interfaceId ) ) {
        setError( answer, Errors::NETWORK_INVALID_INTERFACE );
        return;
    }

    m_networkdInterface.modemClearFPLMN( interfaceId );
    rpcMessage.setResult( JSON_EMPTY_OBJ );
}

void HubNetworkCommandsHandler::handleModemReRegisterRequest( rpc::Message& rpcMessage ) {
    auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();

    std::string interfaceId;
    if( !getField( params, "interfaceId", answer, interfaceId ) ) {
        return;
    }

    if( !m_networkdInterface.isGSMInterface( interfaceId ) ) {
        setError( answer, Errors::NETWORK_INVALID_INTERFACE );
        return;
    }

    auto clearFPLMN = json_get_bool( params, "clearFPLMN", false );

    m_networkdInterface.modemReRegister( interfaceId, clearFPLMN );
    rpcMessage.setResult( JSON_EMPTY_OBJ );
}

void HubNetworkCommandsHandler::handleStatisticsSettingsGetRequest( rpc::Message& rpcMessage ) {
    const auto statisticSettings = m_networkdInterface.getStatisticsSettings();

    const auto result = JSON( "storageSize" << statisticSettings.m_storageSize );

    rpcMessage.setResult( result );
}

void HubNetworkCommandsHandler::handleStatisticsSettingsSetRequest( rpc::Message& rpcMessage ) {
    using namespace Network;
    auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();

    if( params.empty() ) {
        JsonObject errorMessage;
        Errors::getWrongParams( "params", errorMessage );
        setErrorObject( answer, errorMessage );
        return;
    }

    const auto storageSize = json_get_int( params, "storageSize", -1 );

    StatisticsSettingsManager::StatisticsSettings setting;
    setting.m_storageSize = storageSize;

    if( setting.empty() ) {
        JsonObject errorMessage;
        Errors::getWrongParams( "params", errorMessage );
        setErrorObject( answer, errorMessage );
        return;
    }

    auto response = m_networkdInterface.setStatisticsSettings( setting );
    setErrorIfHas( answer, response );
}

void HubNetworkCommandsHandler::handleStatisticsGetRequest( rpc::Message& rpcMessage ) {
    using namespace Network;
    auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();

    RemoteInterface::StatisticsPeriod statisticPeriod;

    if( json_contains( params, "period" ) ) {
        statisticPeriod = RemoteInterface::StatisticsPeriod( params[ "period" ] );
        if( !statisticPeriod.isValid() ) {
            JsonObject errorMessage;
            Errors::getWrongParams( "period", errorMessage );
            setErrorObject( answer, errorMessage );
            return;
        }
    }

    m_networkdInterface.getStatistics( statisticPeriod );
    rpcMessage.setResult( JSON_EMPTY_OBJ );
}

// "params": {
//     "interfaceId" : "wlan0",
//     "enabled" : "auto"
// }

void HubNetworkCommandsHandler::handleEnabledSetRequest( rpc::Message& rpcMessage ) {
    auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();

    std::string interfaceId;

    if( !getField( params, "interfaceId", answer, interfaceId ) ) {
        return;
    }

    std::string enablePolicy;
    if( !getField( params, "enabled", answer, enablePolicy ) ) {
        return;
    }

    auto response = m_networkdInterface.setEnablePolicy( interfaceId, enablePolicy );
    setErrorIfHas( answer, response );
}

bool HubNetworkCommandsHandler::getAndValidateInterfaceField( const Json& message, Json& answer, std::string& result ) {
    auto field = "interface";
    json_get( message, field, result );

    if( !result.empty() ) {
        auto interfaces = m_networkdInterface.getNetworkInterfaces();
        for( auto interface : interfaces ) {
            if( json_get_str( interface, "_id" ) == result )
                return true;
        }
    }

    JsonObject error_message;
    Errors::getBadRequest( field, error_message );
    setErrorObject( answer, error_message );
    return false;
}

bool HubNetworkCommandsHandler::isActiveNetworkInterface( const std::string& interfaceId ) {
    auto interfaces = m_networkdInterface.getConnectedNetworkInterfaces();
    for( auto& interface : interfaces ) {
        if( json_get_str( interface, "_id" ) == interfaceId )
            return true;
    }

    return false;
}

void HubNetworkCommandsHandler::handleCommunicationTest( rpc::Message& rpcMessage ) {
    auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();

    if( m_isCommunicationManualTestStarted == true || m_networkdInterface.isCommunicationTestStarted() ) {
        setError( answer, Errors::NETWORK_TEST_ALREADY_STARTED );
        return;
    }

    m_isCommunicationManualTestStarted = true;

    std::string interface;
    if( !getAndValidateInterfaceField( params, answer, interface ) ) {
        setError( answer, Errors::NETWORK_INVALID_INTERFACE );
        return;
    }

    std::string operationId;
    getOptionalField( params, "operationId", operationId );

    int timeout = 30;
    getOptionalField( params, "timeout", timeout );

    if( !isActiveNetworkInterface( interface ) ) {
        setError( answer, Errors::NETWORK_INTERFACE_NOT_UP );
        return;
    }

    m_networkdInterface.communicationTest(
        interface, operationId, std::chrono::seconds( timeout ), [&answer, this]( const JsonObject& message ) {
            setErrorIfHas( answer, message );
            m_isCommunicationManualTestStarted = false;
        } );
}

}  // namespace Uid
}  // namespace HubPlatform
