#pragma once

#include "networkd/gsm/nm_gsm_mobile_network_manager.h"
#include "networkd/gsm/nm_gsm_connection_manager.h"

#include <chrono>
#include <utility>

class IDbClient;

namespace HubPlatform {

namespace Services {
class TaskLoop;
class TaskLoopTimer;
}  // namespace Services

namespace Network {

class GSMInterface;

namespace GSM {
class GSMModem;
class SettingsProvider;
}  // namespace GSM

namespace Mobile {

class InterfaceStateManager final {
    friend ConnectionManager;

public:
    InterfaceStateManager( IDbClient& dbClient,
                           Services::TaskLoop& taskLoop,
                           GSMInterface& interface,
                           const NetworkConfigFileManager::MobileConfiguration& configuration );
    void start( const std::weak_ptr< GSMInterface >& weakSelf );

    void scheduleRun( const std::chrono::seconds& relativeStartTime = std::chrono::seconds( 0 ) );

    void onConnectionInterrupted( bool probablyFalsePositive = false );
    void onConnectionRestored();

private:
    enum class State {
        MODEM_UP,
        MODEM_HUNGUP,
        MODEM_DOWN,
        SIM_PRESENT,
        SIM_NOT_PRESENT,
        PIN_REQUIRED,
        CHECK_REGISTRATION
    };

    void run();

    void handleModemUp();
    void handleSIMPresent();
    void handleSIMNotPresent();
    void handlePINRequired();
    void handleCheckRegistration();
    void handleModemHungUp();
    void handleModemDown();

    std::pair< bool, std::chrono::seconds > registerToNetwork();
    void reRegister( bool cleanFPLMN = false );
    void rebootModem();

    void changeState( State state );
    static std::string stateToString( State state );

private:
    std::weak_ptr< GSMInterface > m_weakSelf;

    IDbClient& m_dbClient;
    Services::TaskLoop& m_taskLoop;
    GSMInterface& m_interface;
    GSM::GSMModem& m_modem;
    GSM::SettingsProvider& m_apnSettingsProvider;
    Services::TaskLoopTimer& m_modemInfoPollingTimer;

    const std::string m_interfaceName;
    Mobile::NetworkManager m_networkManager;
    ConnectionManager m_connectionManager;

    State m_state = State::MODEM_UP;
    bool m_isRegistered = false;
};

}  // namespace Mobile
}  // namespace Network
}  // namespace HubPlatform
