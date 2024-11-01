#pragma once

#include "networkd/configuration/nm_config_file_manager.h"
#include "networkd/gsm/nm_gsm.h"

#include <map>
#include <list>
#include <string>
#include <chrono>
#include <utility>

namespace HubPlatform {
namespace Network {

namespace GSM {
class GSMModem;
}

namespace Mobile {

class MobileInterfaceConfiguration;

class NetworkManager final {
public:
    NetworkManager( GSM::GSMModem& modem,
                    const NetworkConfigFileManager::MobileConfiguration& configuration,
                    std::string interfaceName );

    using RegistrationResult = std::pair< bool, std::chrono::seconds >;
    RegistrationResult registerToNetwork();

private:
    struct ScanResult {
        ScanResult();
        ScanResult( std::string networkId, bool isForbidden );

        std::string m_networkId;  // rplmn
        bool m_isForbidden = true;
    };

    using NetworkId = std::string;
    using ScanResults = std::map< NetworkId, ScanResult >;

    void resetInternalData();
    NetworkId getCurrentNetwork() const;
    GSM::GSMModem::AccessTechnology getCurrentAccessTechnology() const;
    RegistrationResult failedToRegister();
    ScanResults processScanResult( const std::string& modemResponse ) const;
    void printScanResult() const;
    std::string getHumanNetworkName( NetworkManager::NetworkId networkId ) const;
    bool isWhiteListProvided() const;
    int getRebootTriger() const;
    void cleanDataPDPContext();

    RegistrationResult handleInit();
    RegistrationResult handleNotRegistered();
    RegistrationResult handleCycleWhiteList();
    RegistrationResult handleScanInProgress();
    RegistrationResult handleTryToRegister();
    RegistrationResult handleTryToAutoRegister();
    RegistrationResult handleRegistered();

private:
    enum class State {
        INIT,
        NOT_REGISTERED,
        CYCLE_WHITE_LIST,
        SCAN_IN_PROGRESS,
        TRY_TO_REGISTER,
        TRY_TO_AUTO_REGISTER,
        REGISTERED
    };

    void changeState( State state );
    static std::string stateToString( State state );

private:
    GSM::GSMModem& m_modem;
    const NetworkConfigFileManager::MobileConfiguration& m_configuration;
    const std::string m_interfaceName;

    State m_state = State::INIT;
    ScanResults m_scanResult;
    std::list< std::string > m_whiteList;
    NetworkId m_registeredNetwork;
    GSM::GSMModem::AccessTechnology m_accessTechnology;
    NetworkId m_registeringNetwork;

    int m_checkRegisterWaitAmounts;
    int m_scanWaitAmounts;
    int m_registerWaitAmounts;
    int m_autoRegisterWaitAmounts;

    // sometimes scan operation fails, by a weird reason, next scan successes, so try several times
    int m_scanRepeatAmounts;
};

}  // namespace Mobile
}  // namespace Network
}  // namespace HubPlatform
