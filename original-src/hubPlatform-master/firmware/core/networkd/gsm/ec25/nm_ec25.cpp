#include "networkd/gsm/ec25/nm_ec25.h"
#include "networkd/gsm/nm_gsm_model.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "networkd/resources/nm_resources.h"

#include "base_utils/gpio.h"
#include "system_utils/system.h"
#include "base_utils/utils.h"
#include "base_utils/string_utils.h"

#include <stdlib.h>
#include <algorithm>
#include <cctype>
#include <atomic>
#include <thread>
#include <cassert>
#include <mutex>
#include <chrono>
#include <regex>
#include <map>
#include <sstream>
#include <iterator>

#define GPIO_PWRKEY 201
#define GPIO_RESET 100
#define GPIO_STATUS 203

namespace HubPlatform {
namespace Network {
namespace GSM {

// TODO: support only for 1 embedded modem, no external modems can be watchdogged
std::atomic_bool EC25::m_isRebootingEmbedded( false );
std::atomic_bool EC25::m_rebootAnywayEmbedded( false );

namespace {

class StateManager {
    enum class State { On, Off, PoweringOn, PoweringOff, Resetting, Rebooting, Unknown };

public:
    StateManager();
    ~StateManager();
    GSMModem::State reportGSMState() const;
    void powerOn() {
        setState( State::PoweringOn );
    };
    void powerOff() {
        setState( State::PoweringOff );
    };
    void powerReset() {
        setState( State::Resetting );
    };
    void powerReboot() {
        setState( State::Rebooting );
    };

private:
    void threadPowerOff();
    void threadPowerOn();
    void threadReset();
    void threadReboot();

    void setState( State state );
    State getState() const;
    void cyclePin( common::Gpio& pin );

    void waitForPowerOn();
    void waitForPowerOff();
    void waitForSerial();
    void waitForStateTimeout();
    bool detectSerial();

    mutable common::Gpio m_powerPin;
    mutable common::Gpio m_resetPin;
    mutable common::Gpio m_statusPin;

    std::atomic< State > m_state{ State::Unknown };
    std::mutex m_mutex;
    std::thread m_thread;
    std::atomic< bool > m_canOperate{ true };
    std::chrono::steady_clock::time_point m_lastUpdateTime;
};

StateManager::StateManager()
: m_powerPin( GPIO_PWRKEY )
, m_resetPin( GPIO_RESET )
, m_statusPin( GPIO_STATUS ) {
    if( !m_powerPin.setDirection( common::GpioDirection::OUT ) )
        throw GSMModemHardwareFault( "EC25: Failed to set direction for power pin" );
    if( !m_resetPin.setDirection( common::GpioDirection::OUT ) )
        throw GSMModemHardwareFault( "EC25: Failed to set direction for reset pin" );
    if( !m_statusPin.setDirection( common::GpioDirection::IN ) )
        throw GSMModemHardwareFault( "EC25: Failed to set direction for status pin" );
    m_powerPin.setState( common::GpioState::OFF );
    m_resetPin.setState( common::GpioState::OFF );
}

StateManager::~StateManager() {
    try {
        std::lock_guard< std::mutex > lock( m_mutex );
        m_canOperate = false;
        if( m_thread.joinable() )
            m_thread.join();
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR_RAW( cc::error( "EC25: Exception during ModemState destruction: " ) +
                          cc::note( std::string( e.what() ) ) );
    }
}

StateManager::State StateManager::getState() const {
    auto pinState = m_statusPin.getState();
    if( pinState == common::GpioState::OFF )
        return State::On;
    if( pinState == common::GpioState::ON )
        return State::Off;
    throw GSMModemHardwareFault( "Undefined state of EC25 status pin" );
}

void StateManager::setState( State state ) {
    std::lock_guard< std::mutex > lock( m_mutex );
    if( m_thread.joinable() )
        m_thread.join();
    m_state = state;

    if( state == State::PoweringOff )
        m_thread = std::thread( &StateManager::threadPowerOff, this );
    else if( state == State::PoweringOn )
        m_thread = std::thread( &StateManager::threadPowerOn, this );
    else if( state == State::Resetting )
        m_thread = std::thread( &StateManager::threadReset, this );
    else if( state == State::Rebooting )
        m_thread = std::thread( &StateManager::threadReboot, this );
}

void StateManager::cyclePin( common::Gpio& pin ) {
    pin.setState( common::GpioState::ON );
    std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
    pin.setState( common::GpioState::OFF );
}

void StateManager::waitForPowerOn() {
    auto moduleIsOn = [this]() {
        if( false == m_canOperate )
            throw GSMModemHalting( "EC25: Thread force exit requested" );
        return getState() == State::On;
    };
    if( false == waitForCondition( moduleIsOn, std::chrono::seconds( 30 ), std::chrono::milliseconds( 200 ) ) )
        throw GSMModemHangs( "EC25: LTE module power on timeout" );
}

void StateManager::waitForPowerOff() {
    auto moduleIsOff = [this]() {
        if( false == m_canOperate )
            throw GSMModemHalting( "EC25: Thread force exit requested" );
        return getState() == State::Off;
    };
    if( false == waitForCondition( moduleIsOff, std::chrono::seconds( 30 ), std::chrono::milliseconds( 200 ) ) )
        throw GSMModemHangs( "EC25: LTE module power off timeout" );
}

bool StateManager::detectSerial() {
    static const std::string usbDesc = getModemDescription( Model::QUECTEL_EC25 )[ "usbDescription" ];
    static const std::string detectCmd = "lsusb -d " + usbDesc;
    return 0 == System::getCommandOutput( detectCmd.c_str() ).second;
}

void StateManager::waitForSerial() {
    auto serialDetected = [this]() {
        if( false == m_canOperate )
            throw GSMModemHalting( "EC25: Thread force exit requested" );
        return detectSerial();
    };

    if( false == waitForCondition( serialDetected, std::chrono::seconds( 20 ), std::chrono::milliseconds( 1000 ) ) )
        throw GSMModemHangs( "EC25: Module detection failed" );
}

void StateManager::waitForStateTimeout() {
    while( std::chrono::steady_clock::now() - m_lastUpdateTime < std::chrono::seconds( 3 ) ) {
        std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
    }
}

void StateManager::threadPowerOff() {
    try {
        m_state = State::PoweringOff;

        if( getState() == State::Off ) {
            LOGGER_DEBUG_RAW( cc::debug( "Modem already have requested state: " ) + cc::note( "OFF" ) );
            m_state = State::Off;
            return;
        }

        waitForStateTimeout();
        LOGGER_INFO_RAW( cc::info( "Modem Powering Off ..." ) );

        cyclePin( m_powerPin );
        waitForPowerOff();

        m_state = State::Off;
        LOGGER_DEBUG_RAW( cc::debug( "Modem state changed to: OFF" ) );
    }
    catch( std::exception& e ) {
        m_state = State::Unknown;
        LOGGER_ERROR_RAW( cc::error( "Failed to power off modem, " ) + cc::note( e.what() ) );
    }
    m_lastUpdateTime = std::chrono::steady_clock::now();
}

void StateManager::threadPowerOn() {
    try {
        m_state = State::PoweringOn;

        if( getState() == State::On && detectSerial() ) {
            LOGGER_DEBUG_RAW( cc::debug( "Modem already have requested state: " ) + cc::note( "ON" ) );
            m_state = State::On;
            return;
        }

        waitForStateTimeout();
        LOGGER_INFO_RAW( cc::info( "Modem Powering On ..." ) );

        if( getState() == State::Off ) {
            cyclePin( m_powerPin );
            std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
        }
        waitForPowerOn();
        waitForSerial();
        std::this_thread::sleep_for( std::chrono::seconds( 3 ) );

        m_state = State::On;
        LOGGER_DEBUG_RAW( cc::debug( "Modem state changed to: ON" ) );
    }
    catch( std::exception& e ) {
        m_state = State::Unknown;
        LOGGER_ERROR_RAW( cc::error( "Failed to power on modem, " ) + cc::note( e.what() ) );
    }
    m_lastUpdateTime = std::chrono::steady_clock::now();
}

void StateManager::threadReset() {
    try {
        m_state = State::Resetting;

        waitForStateTimeout();
        LOGGER_INFO_RAW( cc::info( "Modem Resetting ..." ) );

        if( getState() == State::Off ) {
            cyclePin( m_powerPin );
        }
        else {
            cyclePin( m_resetPin );
            waitForPowerOff();
        }

        waitForPowerOn();
        waitForSerial();
        std::this_thread::sleep_for( std::chrono::seconds( 2 ) );

        m_state = State::On;
        LOGGER_DEBUG_RAW( cc::debug( "Modem state changed to: ON" ) );
    }
    catch( std::exception& e ) {
        m_state = State::Unknown;
        LOGGER_ERROR_RAW( cc::error( "Failed to reset modem, " ) + cc::note( e.what() ) );
    }
    m_lastUpdateTime = std::chrono::steady_clock::now();
}

void StateManager::threadReboot() {
    try {
        m_state = State::Rebooting;
        waitForStateTimeout();

        LOGGER_INFO_RAW( cc::info( "Rebooting module ( powering off ) ..." ) );
        if( getState() == State::On )
            cyclePin( m_powerPin );

        waitForPowerOff();
        std::this_thread::sleep_for( std::chrono::seconds( 2 ) );

        LOGGER_INFO_RAW( cc::info( "Rebooting module ( powering on  ) ..." ) );
        cyclePin( m_powerPin );
        std::this_thread::sleep_for( std::chrono::seconds( 2 ) );

        waitForPowerOn();
        waitForSerial();
        std::this_thread::sleep_for( std::chrono::seconds( 2 ) );

        m_state = State::On;
        LOGGER_DEBUG_RAW( cc::debug( "Modem state changed to: ON" ) );
    }
    catch( std::exception& e ) {
        m_state = State::Unknown;
        LOGGER_ERROR_RAW( cc::error( "Failed to reboot modem, " ) + cc::note( e.what() ) );
    }
    m_lastUpdateTime = std::chrono::steady_clock::now();
}

GSMModem::State StateManager::reportGSMState() const {
    if( m_state == State::On )
        return GSMModem::State::On;
    if( m_state == State::Off )
        return GSMModem::State::Off;
    return GSMModem::State::Unknown;
}

StateManager& stateManager() {
    static StateManager manager;
    return manager;
}

}  // namespace

EC25::EC25( const NetworkConfigFileManager::MobileConfiguration& configuration,
            const SerialSettings& serialSettings,
            bool embeddedModem,
            bool linkedToPhysicalModem,
            const std::string& interfaceName )
: GSMModem( embeddedModem )
, m_configuration( configuration )
, m_serialSettings( serialSettings )
, m_simLock( m_ATExecutor, interfaceName )
, m_simInserted( false )
, m_interfaceName( interfaceName )
, m_isMonitoringTreadUp( false )
, m_isRebooting( false ) {
    /* for embedded modem we provide a watchdog functionality which may request a modem to be rebooted,
     * so this request should be synchronized with other requests to reboot, the watchdog functionality
     * doesn't use a modem object linked to a real physical modem as other users do, for historical reasons, so
     * such a hack is used */
    if( embeddedModem ) {
        if( linkedToPhysicalModem || m_rebootAnywayEmbedded ) {
            m_isRebootingEmbedded = false;
            m_rebootAnywayEmbedded = false;
        }
        else {
            m_rebootAnywayEmbedded = true;
        }
    }
}

EC25::~EC25() {
    m_isMonitoringTreadUp = false;
    if( m_monitoringThread.joinable() ) {
        m_monitoringThread.join();
    }
}

GSMModem::State EC25::getState() const {
    return stateManager().reportGSMState();
}

bool EC25::isAlive() const {
    if( stateManager().reportGSMState() == GSMModem::State::Off )
        return false;

    auto answer = m_ATExecutor.execute( "at\r" );
    if( answer.find( "OK" ) == std::string::npos ) {
        return false;
    }

    return true;
}

std::string EC25::getDataTTYDevice() const {
    return m_serialSettings.dataTTY;
}

Json EC25::getDeviceInfo() const {
    return JSON( "model" << Model::QUECTEL_EC25 << "embedded" << isEmbedded() );
}

std::string EC25::getGSMModemIMEI() const {
    return m_IMEI;
}

void EC25::executeATInitializeString( const std::string& ATInitialize ) {
    if( ATInitialize.empty() ) {
        LOGGER_DEBUG_RAW( cc::debug( "Try to execute an empty AT initialize string, interface: " ) +
                          cc::note( m_interfaceName ) );
        return;
    }

    auto answer = m_ATExecutor.execute( ATInitialize, std::chrono::milliseconds( 5000 ) );
    if( answer.find( "OK" ) == std::string::npos ) {
        throw GSMModemFailedToRunATInitializeString(
            "EC25 LTE module failed to execute the AT initialize string, answer: " + answer );
    }
}

bool EC25::isPINCodeRequired() const {
    return m_simLock.isPinCodeRequired();
}

bool EC25::isLocked() const {
    return m_simLock.isLocked();
}

void EC25::setSIMLock( const std::string& pinCode, bool lock ) {
    if( lock ) {
        m_simLock.lock( pinCode );
    }
    else {
        m_simLock.unlock( pinCode );
    }
}

void EC25::enterPINCode( const std::string& pinCode ) {
    m_simLock.enterPinCode( pinCode );
}

void EC25::changeSIMPINCode( const std::string& currentPinCode, const std::string& newPinCode ) {
    m_simLock.changePinCode( currentPinCode, newPinCode );
}

void EC25::resetSIMPINCode( const std::string& pukCode, const std::string& newPinCode ) {
    m_simLock.resetPinCode( pukCode, newPinCode );
}

void EC25::reboot() {
    try {
        bool expected = false;
        std::atomic_bool& isRebooting = isEmbedded() ? m_isRebootingEmbedded : m_isRebooting;

        if( !isRebooting.compare_exchange_strong( expected, true ) ) {
            LOGGER_WARN_RAW( cc::warn( "Modem is rebooting already, interface: " ) + cc::note( m_interfaceName ) );
            return;
        }

        LOGGER_WARN_RAW( cc::warn( "Reboot a modem, interface: " ) + cc::note( m_interfaceName ) );
        if( isEmbedded() ) {
            stateManager().powerReboot();
        }
        else {
            rebootExternalModem();
        }
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to reboot a modem, interface " ) + cc::note( m_interfaceName ) +
                          cc::error( ", reason: " ) + cc::note( ex.what() ) );
    }
}

void EC25::rebootExternalModem() {
    auto answer = ATUtils::toStrings( m_ATExecutor.execute( "at+cfun=1,1\r" ) );
    if( ATUtils::isOK( answer, 2 ) ) {
        return;
    }

#if( defined( __HUB_TYPE_H2__ ) || defined( __HUB_TYPE_H2_SECURE__ ) || defined( __HUB_TYPE_G150__ ) )
    const auto busNumber = std::to_string( Resources::GSM::ExternalCellularModemsUSBBusNumber );

    // perform logical power off/on for a usb bus external cellular modems are connected to
    std::system( ( "echo 0 > /sys/bus/usb/devices/usb" + busNumber + "/bConfigurationValue" ).c_str() );
    std::this_thread::sleep_for( std::chrono::seconds( 5 ) );
    std::system( ( "echo 1 > /sys/bus/usb/devices/usb" + busNumber + "/bConfigurationValue" ).c_str() );
#endif
}

void EC25::powerOn() {
    if( isEmbedded() ) {
        stateManager().powerOn();
    }
    else {
        LOGGER_DEBUG_RAW( cc::debug( "Attempt to perform PWR_ON for an external modem, interface: " ) +
                          cc::note( m_interfaceName ) );
    }
}

void EC25::powerOff() {
    if( isEmbedded() ) {
        stateManager().powerOff();
    }
    else {
        LOGGER_DEBUG_RAW( cc::debug( "Attempt to perform PWR_OFF for an external modem, interface: " ) +
                          cc::note( m_interfaceName ) );
    }
}

void EC25::startSession() {
    try {
        m_ATExecutor.setSerialSettings( m_serialSettings );

        m_isMonitoringTreadUp = true;
        m_monitoringThread = std::thread( &EC25::monitoringThread, this );

        setEchoMode( false );
        setupModem();
        retrieveIMEI();
        checkSIMCard();
    }
    catch( const GSMModemRebooting& e ) {
        throw;
    }
    catch( const std::exception& e ) {
        LOGGER_WARN_RAW( cc::warn( "Modem on interface " ) + cc::note( m_interfaceName ) +
                         cc::warn( " failed to start session: " ) + cc::note( e.what() ) );
        reboot();
        throw GSMModemRebooting( "Failed to setup modem session" );
    }
}

void EC25::monitoringThread() {
    while( true ) {
        try {
            LOGGER_DEBUG_RAW( cc::debug( "Check modem accessibility" ) );

            if( !m_ATExecutor.isAccessible() ) {
                LOGGER_WARN_RAW( cc::warn( "Modem on interface " ) + cc::note( m_interfaceName ) +
                                 cc::warn( " doesn't respond, reboot it" ) );
                reboot();
                break;
            }

            if( waitForCondition( [this]() { return !m_isMonitoringTreadUp.load(); },
                                  m_configuration.m_modemMonitoringThreadPollingPeriod,
                                  std::chrono::seconds( 10 ) ) ) {
                break;
            }
        }
        catch( const std::exception& ex ) {
            LOGGER_ERROR_RAW( cc::error( "Failed to check modem accessibility, interface " ) +
                              cc::note( m_interfaceName ) + cc::error( ", reason: " ) + cc::note( ex.what() ) );
        }
    }
}

void EC25::setupModem() {
    bool setupChanged = false;

    setupChanged |= disableIMSSupport();
    setupChanged |= setupDataCentricMode();

    if( isEmbedded() ) {
        setupChanged |= setupAvailableBands();
    }

    if( setupChanged ) {
        reboot();
        throw GSMModemRebooting( "Modem configuration changed" );
    }

    disableVoiceSupport();
}

void EC25::retrieveIMEI() {
    const auto answer = ATUtils::toStrings( m_ATExecutor.execute( "at+gsn\r" ) );
    if( !ATUtils::isOK( answer, 2 ) ) {
        LOGGER_DEBUG_RAW( cc::debug( "Failed to get an LTE modem IMEI, interface: " ) + cc::note( m_interfaceName ) );
        m_IMEI = "unknown";
        return;
    }

    m_IMEI = answer[ 0 ];
}

EC25::SIMInfo EC25::getSIMInfo( bool force ) const {
    std::string imsi, iccid;

    if( !force && m_simInfoPresent ) {
        return m_simInfo;
    }

    try {
        {  // \r\n+QCCID: ICCID\r\n\r\n\nOK
            auto answer = m_ATExecutor.execute( "at+qccid\r" );
            auto ccidPair = ATUtils::AtResponse::extractATParameter( answer, "QCCID" );
            if( !ccidPair.second ) {
                LOGGER_DEBUG_RAW( cc::debug( "Failed to get a SIM ICCID, interface: " ) + cc::note( m_interfaceName ) );
            }
            else if( !ATUtils::isValidICCID( ccidPair.first ) ) {
                LOGGER_DEBUG_RAW( cc::debug( "Failed to validate SIM ICCID: '" + ccidPair.first + "' , interface: " ) +
                                  cc::note( m_interfaceName ) );
            }
            else {
                iccid = ccidPair.first;
            }
        }

        {  // \r\nIMSI\r\n\r\n\nOK
            auto answer = ATUtils::toStrings( m_ATExecutor.execute( "at+cimi\r" ) );
            if( !ATUtils::isOK( answer, 2 ) || answer.at( 0 ).empty() ) {
                LOGGER_DEBUG_RAW( cc::debug( "Failed to get a SIM IMSI, interface: " ) + cc::note( m_interfaceName ) );
            }
            else {
                imsi = answer.at( 0 );
            }
        }

        m_simInfo = SIMInfo{ iccid, imsi, m_simLock.isLocked() };
        m_simInfoPresent = true;
        return m_simInfo;
    }
    catch( const std::exception& ex ) {
        LOGGER_DEBUG_RAW( cc::debug( "Failed to get SIM info, interface: " ) + cc::note( m_interfaceName ) );
        return SIMInfo();
    }
}

EC25::MobileNetworkInfo EC25::getMobileNetworkInfo() const {
    const auto answer = m_ATExecutor.execute( "at+qspn\r" );
    const auto okPos = answer.find( "OK" );

    if( okPos == std::string::npos ) {
        LOGGER_TRACE_RAW( cc::trace( "No mobile network info, interface: " ) + cc::note( m_interfaceName ) );
        return MobileNetworkInfo();
    }

    //+QSPN: "Verizon Wireless","VZW","Hologram",0,"311480"
    //+QSPN: <FNN>,<SNN>,<SPN>,<alphabet(0|1)>,<RPLMN>
    const auto fnnBegin = answer.find( "\"" ) + 1;
    const auto fnnEnd = answer.find( "\"", fnnBegin ) - 1;
    const auto snnBegin = answer.find( ",\"", fnnEnd ) + 2;
    const auto snnEnd = answer.find( "\"", snnBegin ) - 1;
    const auto spnBegin = answer.find( ",\"", snnEnd ) + 2;
    const auto spnEnd = answer.find( "\"", spnBegin ) - 1;
    const auto alphabetBegin = answer.find( ",", spnEnd ) + 1;
    const auto rplmnBegin = answer.find( ",\"", alphabetBegin ) + 2;
    const auto rplmnEnd = answer.find( "\"", rplmnBegin ) - 1;

    if( std::stoi( answer.substr( alphabetBegin, 1 ) ) != 0 ) {
        LOGGER_ERROR( cc::error( "Failed to get a mobile network info, invalid alphabet, interface: " ) +
                      cc::note( m_interfaceName ) );
        return MobileNetworkInfo();
    }

    return MobileNetworkInfo( answer.substr( fnnBegin, fnnEnd - fnnBegin + 1 ),
                              answer.substr( spnBegin, spnEnd - spnBegin + 1 ),
                              answer.substr( rplmnBegin, rplmnEnd - rplmnBegin + 1 ) );
}

EC25::NetworkId EC25::getCurrentNetworkId() const {
    const auto answer = m_ATExecutor.execute( "at+qspn\r" );
    const auto okPos = answer.find( "OK" );

    if( okPos == std::string::npos ) {
        LOGGER_DEBUG_RAW( cc::debug( "No current network id, interface: " ) + cc::note( m_interfaceName ) );
        return "";
    }

    std::stringstream stream( answer );
    NetworkId networkId;

    int i = 0;
    while( stream.good() ) {
        std::string value;

        getline( stream, value, ',' );
        if( i == 4 ) {
            networkId = value;
            break;
        }
        i++;
    }

    auto pos = networkId.find( '"', 1 );
    networkId = networkId.substr( 1, pos - 1 );

    return networkId;
}

bool EC25::isSIMInserted() const {
    return m_simInserted;
}

void EC25::clearFPLMNList() {
    m_ATExecutor.execute( "at+crsm=214,28539,0,0,12,\"FFFFFFFFFFFFFFFFFFFFFFFF\"\r", std::chrono::seconds( 1 ) );
}

void EC25::reRegister( bool clearFPLMN ) {
    LOGGER_INFO_RAW( cc::info( "EC25: re-register network on interface: " ) + cc::note( m_interfaceName ) );
    if( clearFPLMN ) {
        clearFPLMNList();
    }

    deregisterFromNetwork();
    startAutoRegistration();
}

bool EC25::isRegistered() {
    const auto answer = ATUtils::toStrings( m_ATExecutor.execute( "at+creg?\r" ) );
    //+CREG: 0,1
    // OK
    if( !ATUtils::isOK( answer, 2 ) ) {
        LOGGER_DEBUG_RAW( cc::debug( "Failed to get an LTE modem registration status, interface: " ) +
                          cc::note( m_interfaceName ) );
        return false;
    }

    auto status = StringUtils::split( answer[ 0 ], "," ).at( 1 );

    if( status == "1" || status == "5" )  // registered or registered+roaming
        return true;
    return false;
}

bool EC25::activatePDPContext( int contextIndex ) {
    if( contextIndex < 1 || contextIndex > 3 ) {
        return false;
    }

    LOGGER_INFO( cc::info( "Activating PDP context: " + cc::num10( contextIndex ) ) );
    m_ATExecutor.execute( "at+cgact=1," + std::to_string( contextIndex ) + "\r", std::chrono::milliseconds( 300 ) );
    return true;
}

bool EC25::deactivatePDPContext( int contextIndex ) {
    if( contextIndex < 1 || contextIndex > 3 ) {
        return false;
    }

    LOGGER_INFO( cc::info( "Deactivating PDP context: " + cc::num10( contextIndex ) ) );
    m_ATExecutor.execute( "at+cgact=0," + std::to_string( contextIndex ) + "\r", std::chrono::milliseconds( 300 ) );
    return true;
}

bool EC25::removePDPContext( int contextIndex ) {
    if( !deactivatePDPContext( contextIndex ) ) {
        return false;
    }

    LOGGER_INFO( cc::info( "Removing PDP context: " + cc::num10( contextIndex ) ) );
    const auto answer = ATUtils::toStrings( m_ATExecutor.execute( "at+cgdcont=" + std::to_string( contextIndex ) + "\r",
                                                                  std::chrono::milliseconds( 300 ) ) );
    return ATUtils::isOK( answer, 1 );
}

bool EC25::resetPDPContext( int contextIndex ) {
    if( !deactivatePDPContext( contextIndex ) ) {
        return false;
    }

    // Remove PDP context
    const auto removeAnswer = ATUtils::toStrings( m_ATExecutor.execute(
        "at+cgdcont=" + std::to_string( contextIndex ) + "\r", std::chrono::milliseconds( 300 ) ) );

    LOGGER_INFO( cc::info( "Resetting PDP context: " + cc::num10( contextIndex ) ) );
    const auto resetAnswer = ATUtils::toStrings( m_ATExecutor.execute(
        "at+cgdcont=" + std::to_string( contextIndex ) + ",\"IPV4V6\"" + "\r", std::chrono::milliseconds( 300 ) ) );

    // NOTE: Explicit activation of the context does not required,
    // since modem will try to establish data connection and likely will fail.
    // Context will be automatically activated when SIM is registered.
    return ATUtils::isOK( removeAnswer, 1 ) && ATUtils::isOK( resetAnswer, 1 );
}

std::vector< std::string > EC25::getPDPContexts() {
    auto answer = ATUtils::toStrings( m_ATExecutor.execute( "at+cgdcont?\r", std::chrono::milliseconds( 300 ) ) );
    if( answer.size() < 2 || answer.back() != "OK" )
        return {};
    answer.pop_back();
    return answer;
}

void EC25::startScanNetworks() {
    m_ATExecutor.execute( "at+cops=?\r", std::chrono::seconds( 0 ) );
    m_ATExecutor.lock();

    std::this_thread::sleep_for( std::chrono::seconds( 3 ) );
}

std::string EC25::consumeScanNetworksResult() {
    return m_ATExecutor.consume();
}

void EC25::completeScanNetworks() {
    m_ATExecutor.unlock();
}

void EC25::deregisterFromNetwork() {
    m_ATExecutor.execute( "at+cops=2\r", std::chrono::milliseconds( 1000 ) );
}

void EC25::startAutoRegistration() {
    LOGGER_DEBUG_RAW( cc::debug( "EC25: start auto-registration, interface: " ) + cc::note( m_interfaceName ) );
    m_ATExecutor.execute( "at+cops=0\r", std::chrono::seconds( 7 ) );
}

void EC25::startRegisterToNetwork( NetworkId networkId ) {
    LOGGER_DEBUG_RAW( cc::debug( "EC25: start registration to network: " ) + cc::note( networkId ) +
                      cc::debug( ", on interface: " ) + cc::note( m_interfaceName ) );
    if( networkId.empty() ) {
        m_ATExecutor.execute( "at+cops=0\r", std::chrono::seconds( 0 ) );
    }
    else {
        m_ATExecutor.execute( "at+cops=1,2,\"" + networkId + "\"\r", std::chrono::seconds( 0 ) );
    }

    m_ATExecutor.lock();

    std::this_thread::sleep_for( std::chrono::seconds( 10 ) );
}

std::string EC25::consumeRegisterToNetworkResult() {
    return m_ATExecutor.consume();
}

void EC25::completeRegisterToNetwork() {
    m_ATExecutor.unlock();
}

EC25::CRSMCommandResult EC25::executeCRSMCommand( int command,
                                                  int fileId,
                                                  int parameter1,
                                                  int parameter2,
                                                  int parameter3 ) {
    CRSMCommandResult result;

    try {
        const auto answer = ATUtils::toStrings(
            m_ATExecutor.execute( "at+crsm=" + std::to_string( command ) + "," + std::to_string( fileId ) + "," +
                                      std::to_string( parameter1 ) + "," + std::to_string( parameter2 ) + "," +
                                      std::to_string( parameter3 ) + "\r",
                                  std::chrono::milliseconds( 300 ) ) );

        if( ATUtils::isOK( answer, 2 ) ) {
            auto tokens = StringUtils::splitSequence( answer[ 0 ], { "+CRSM: ", ",", ",\"", "\"" } );
            if( tokens.empty() ) {
                tokens = StringUtils::splitSequence( answer[ 0 ], { "+CRSM: ", "," } );
            }

            result.m_statusWord1 = std::stoi( tokens.at( 1 ) );
            result.m_statusWord2 = std::stoi( tokens.at( 2 ) );

            if( tokens.size() > 3 ) {
                result.m_response = tokens.at( 3 );
            }

            result.m_valid = true;
        }
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to execute a crsm command " ) + cc::num10( command ) +
                          cc::error( ", interface: " ) + cc::note( m_interfaceName ) + cc::error( ", reason: " ) +
                          cc::note( ex.what() ) );
    }
    catch( ... ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to execute a crsm command " ) + cc::num10( command ) +
                          cc::error( ", interface: " ) + cc::note( m_interfaceName ) );
    }

    return result;
}

std::string EC25::executeCustomATCommand( const std::string& ATCommand, const std::chrono::milliseconds& timeout ) {
    return m_ATExecutor.execute( ATCommand, timeout );
}

void EC25::executeATCommandOrThrow( const std::string& command,
                                    const std::string& modemHangsExceptionDescription,
                                    const std::chrono::milliseconds& waitForAnswer ) {
    auto answer = m_ATExecutor.execute( command, waitForAnswer );
    if( answer.find( "OK" ) == std::string::npos ) {
        checkATAnswerAndThrow( answer );
        throw GSMModemHangs( modemHangsExceptionDescription );
    }
}

void EC25::checkATAnswerAndThrow( const std::string& ATAnswer ) {
    if( ATAnswer.find( "+CME ERROR: 10" ) != std::string::npos ) {
        throw GSMModemNoSIMCard();
    }
    if( ATAnswer.find( "+CME ERROR: 11" ) != std::string::npos ) {
        throw GSMModemPINCodeRequired();
    }
    if( ATAnswer.find( "+CME ERROR: 12" ) != std::string::npos ) {
        throw GSMModemPUKCodeIsRequired();
    }
    if( ATAnswer.find( "+CME ERROR: 13" ) != std::string::npos ) {
        throw GSMModemSIMCardError();
    }
    if( ATAnswer.find( "+CME ERROR: 16" ) != std::string::npos ) {
        throw GSMModemInvalidPINCode();
    }
    if( ATAnswer.find( "+CME ERROR: 30" ) != std::string::npos ) {
        throw GSMModemNoNetworkService();
    }
    if( ATAnswer.find( "+CME ERROR: 32" ) != std::string::npos ) {
        throw GSMModemOnlyEmergencyCalls();
    }
    if( ATAnswer.find( "+CME ERROR: " ) != std::string::npos || ATAnswer.find( "ERROR" ) != std::string::npos ) {
        throw GSMModemUnknownReason();
    }
}

int EC25::getPINCodeAmountsLeft() const {
    const auto answer = m_ATExecutor.execute( "at+qpinc?\r", std::chrono::milliseconds( 5000 ) );

    auto commaPosition = answer.find( "," );
    if( commaPosition != std::string::npos ) {
        return std::stoi( answer.substr( commaPosition + 1, 1 ) );
    }

    return -1;
}

int EC25::getPUKCodeAmountsLeft() const {
    const auto answer = m_ATExecutor.execute( "at+qpinc?\r", std::chrono::milliseconds( 5000 ) );

    auto commaPosition = answer.find( "," );
    if( commaPosition != std::string::npos ) {
        auto position = answer.find( ",", commaPosition + 1 );
        return std::stoi( answer.substr( position + 1 ) );
    }

    return -1;
}

void EC25::checkSIMCard() {
    SimStatus status = SimStatus::Unknown;
    auto tryAmounts = 5;

    try {
        for( auto i = 0; i < tryAmounts; i++ ) {
            status = getSimStatus();
            if( status != SimStatus::Unknown ) {
                break;
            }

            std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );
        }

        if( status == SimStatus::Unknown ) {
            throw GSMModemSIMCardError( "Unknown status of SIM card" );
        }

        LOGGER_INFO_RAW( cc::info( "SIM status: " + simStatusStr( status ) ) + cc::info( ", interface: " ) +
                         cc::note( m_interfaceName ) );

        if( status == SimStatus::NotInserted ) {
            return;
        }

        std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );

        auto iccid = getSimICCID();
        LOGGER_INFO_RAW( cc::info( "SIM ICCID: " ) + cc::note( iccid ) + cc::info( ", interface: " ) +
                         cc::note( m_interfaceName ) );
    }
    catch( std::runtime_error& e ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to check a SIM card: " ) + cc::note( e.what() ) +
                          cc::error( ", interface: " ) + cc::note( m_interfaceName ) );
        throw;
    }

    m_simInserted = true;
}

/* @brief controls whether modem echoes symbol it receives or not */
void EC25::setEchoMode( bool echoMode ) {
    std::string command = std::string( "ate" ) + ( echoMode ? "1" : "0" ) + '\r';

    LOGGER_DEBUG_RAW( cc::debug( std::string( "Set echo mode to " ) + ( echoMode ? "1" : "0" ) ) +
                      cc::debug( ", interface: " ) + cc::note( m_interfaceName ) );

    executeATCommandOrThrow(
        command, std::string( "Mobile modem failed to switch " ) + ( echoMode ? "on" : "off" ) + " echo mode" );
}

EC25::SimStatus EC25::getSimStatus() {
    static const std::regex re( "QSIMSTAT: [0-1],([0-2])" );
    std::smatch match;

    auto answer = m_ATExecutor.execute( "AT+QSIMSTAT?\r", std::chrono::milliseconds( 300 ) );
    if( !std::regex_search( answer, match, re ) ) {
        return SimStatus::Unknown;
    }

    return static_cast< SimStatus >( std::stoi( match[ 1 ] ) );
}

std::string EC25::simStatusStr( SimStatus status ) {
    static const std::map< SimStatus, std::string > statusMap{ { SimStatus::NotInserted, "not inserted" },
                                                               { SimStatus::Inserted, "inserted" },
                                                               { SimStatus::Unknown, "unknown" } };
    return statusMap.at( status );
}

std::string EC25::getSimICCID() {
    static const std::regex re( "\\D*(\\d{5,24})\\D*" );
    std::smatch match;

    auto answer = m_ATExecutor.execute( "AT+QCCID\r", std::chrono::milliseconds( 300 ) );
    if( !std::regex_match( answer, match, re ) )
        throw GSMModemSIMCardError( "Mobile modem failed to read SIM ICCID" );
    return match[ 1 ];
}

bool EC25::setupAvailableBands() {
    // Enabled bands: 2,4,5,12,13,14,66. Disabled bands: 71 (Due to DRAM noise)
    const std::string allowedBands = "2000000000000381a";

    auto answer = ATUtils::toStrings( m_ATExecutor.execute( "AT+QCFG=\"band\"\r" ) );
    if( ATUtils::isOK( answer, 2 ) ) {
        auto tokens = StringUtils::split( answer[ 0 ], "," );
        if( tokens.at( 2 ).substr( 2 ) == allowedBands )
            return false;
    }

    LOGGER_DEBUG_RAW( cc::debug( "Set available mobile bands: " + allowedBands ) );
    m_ATExecutor.execute( "AT+QCFG=\"band\",0," + allowedBands + ",0,1\r" );
    return true;
}

void EC25::disableVoiceSupport() {
    LOGGER_DEBUG_RAW( cc::debug( "Disabling mobile voice support" ) );  // NOTE: T-Mobile certification requirement
    m_ATExecutor.execute( "AT+QCFG=\"volte_disable\",1\r" );
    m_ATExecutor.execute( "AT+QNVW=5280,0,\"0102000000000000\"\r" );
}

bool EC25::disableIMSSupport() {
    // NOTE: T-Mobile certification requirement
    const std::string imsSupportDisabled = "2";
    const std::string nvramIMSSupportDisabled = "00";

    bool setupRequired = false;

    auto answer = ATUtils::toStrings( m_ATExecutor.execute( "AT+QCFG=\"ims\"\r" ) );
    if( ATUtils::isOK( answer, 2 ) ) {
        auto tokens = StringUtils::split( answer[ 0 ], "," );  // +QCFG: "ims",0,0
        if( tokens.at( 1 ) != imsSupportDisabled )
            setupRequired = true;
    }

    auto nvramAnswer = ATUtils::toStrings( m_ATExecutor.execute( "AT+QNVFR=\"/nv/item_files/ims/IMS_enable\"\r" ) );
    if( ATUtils::isOK( nvramAnswer, 2 ) ) {
        auto tokens = StringUtils::split( nvramAnswer[ 0 ], "+QNVFR: " );  // +QNVFR: 01
        if( tokens.at( 1 ) != nvramIMSSupportDisabled )
            setupRequired = true;
    }

    if( !setupRequired )
        return false;

    LOGGER_DEBUG_RAW( cc::debug( "Disabling IMS support on modem" ) );
    m_ATExecutor.execute( "AT+QCFG=\"ims\",2\r" );
    m_ATExecutor.execute( "AT+QNVFW=\"/nv/item_files/ims/IMS_enable\",00\r" );
    return true;
}

bool EC25::setupDataCentricMode() {
    // NOTE: T-Mobile certification requirement
    const std::string dataCentricMode = "01";

    auto answer =
        ATUtils::toStrings( m_ATExecutor.execute( "AT+QNVFR=\"/nv/item_files/modem/mmode/ue_usage_setting\"\r" ) );
    if( ATUtils::isOK( answer, 2 ) ) {
        auto tokens = StringUtils::split( answer[ 0 ], "+QNVFR: " );  // +QNVFR: 00
        if( tokens.at( 1 ) == dataCentricMode )
            return false;
    }

    LOGGER_DEBUG_RAW( cc::debug( "Setup data centric mode on modem" ) );
    m_ATExecutor.execute( "AT+QNVFW=\"/nv/item_files/modem/mmode/ue_usage_setting\",01\r" );
    return true;
}

EC25::SignalQuality EC25::getSignalQuality( const AccessTechnology& technology ) const {
    AccessTechnology accessTechnology = technology;
    if( accessTechnology.m_accessTechnology == AccessTechnology::Enum::NONE ) {
        accessTechnology = getAccessTechnology();
    }

    if( accessTechnology.m_accessTechnology == AccessTechnology::Enum::NONE ) {
        return SignalQuality();
    }
    if( accessTechnology.isLTE() ) {
        return getLTESignalQuality( accessTechnology );
    }

    return getNoLTESignalQuality( accessTechnology );
}

EC25::SignalQuality EC25::getLTESignalQuality( const AccessTechnology& accessTechnology ) const {
    auto getSignalQualityInfo = [this]( int& rssi, int& rsrp, int& rsrq ) {
        rssi = 99;

        const auto answer = ATUtils::toStrings( m_ATExecutor.execute( "AT+QENG=\"servingcell\"\r" ) );
        if( ATUtils::isOK( answer, 2 ) ) {
            const auto tokens = StringUtils::split( answer[ 0 ], "," );
            if( tokens.size() < 16 ) {
                return;
            }

            rsrp = std::stoi( tokens.at( 13 ) );
            rsrq = std::stoi( tokens.at( 14 ) );
            rssi = std::stoi( tokens.at( 15 ) );
        }
    };

    try {
        int rssi, rsrp, rsrq;

        getSignalQualityInfo( rssi, rsrp, rsrq );
        if( rssi == 99 ) {
            throw GSMModemException( "Not known or not detectable" );
        }

        return SignalQuality( accessTechnology, rssi, rsrp, rsrq );
    }
    catch( const std::exception& ex ) {
        LOGGER_TRACE_RAW( cc::trace( "Failed to detect an LTE signal quality: " ) + cc::note( ex.what() ) +
                          cc::trace( ", interface: " ) + cc::note( m_interfaceName ) );
    }

    return SignalQuality();
}

EC25::SignalQuality EC25::getNoLTESignalQuality( const AccessTechnology& accessTechnology ) const {
    auto getSignalQualityInUnits = [this]() {
        int rssi = 99;

        const auto answer = ATUtils::toStrings( m_ATExecutor.execute( "AT+CSQ\r" ) );
        if( ATUtils::isOK( answer, 2 ) ) {
            const auto tokens = StringUtils::splitSequence( answer[ 0 ], { "+CSQ: ", "," } );
            rssi = std::stoi( ( tokens.size() > 1 ) ? tokens.at( 1 ) : "99" );
        }

        return rssi;
    };

    try {
        const int minRssiDBm = -113;
        const int rssiUnitToDBmStep = 2;
        const int maxRssiUnits = 31;

        int rssiUnits = getSignalQualityInUnits();
        if( rssiUnits == 99 ) {
            throw GSMModemException( "Not known or not detectable" );
        }

        const auto rssiDBm = minRssiDBm + rssiUnitToDBmStep * rssiUnits;

        /*LOGGER_TRACE( cc::trace( "rssi: " ) + cc::note( std::to_string( rssiUnits ) ) + cc::trace( " units, " ) +
                      cc::trace( "rssi: " ) + cc::note( std::to_string( rssiDBm ) ) + cc::trace( " dBm, " ) );*/

        return SignalQuality( accessTechnology, rssiDBm );
    }
    catch( const std::exception& ex ) {
        LOGGER_TRACE_RAW( cc::trace( "Failed to detect a non LTE signal quality: " ) + cc::note( ex.what() ) +
                          cc::trace( ", interface: " ) + cc::note( m_interfaceName ) );
    }

    return SignalQuality();
}

EC25::AccessTechnology EC25::getAccessTechnology() const {
    try {
        const auto answer = ATUtils::toStrings( m_ATExecutor.execute( "AT+QNWINFO\r" ) );
        if( ATUtils::isOK( answer, 2 ) ) {
            const auto tokens = StringUtils::splitSequence( answer[ 0 ], { "+QNWINFO: \"", "\",\"" } );
            auto accessTechnology = ( tokens.size() > 1 ) ? tokens.at( 1 ) : "NONE";

            return AccessTechnology( accessTechnology );
        }
    }
    catch( const std::exception& ex ) {
        LOGGER_TRACE_RAW( cc::trace( "Failed to get a current access technology: " ) + cc::note( ex.what() ) +
                          cc::trace( ", interface: " ) + cc::note( m_interfaceName ) );
    }

    return AccessTechnology();
}

GSMModem::FrequencyBand EC25::getFrequencyBand() const {
    try {
        //+QNWINFO: "FDD LTE","310260","LTE BAND 12",675
        const auto answer = ATUtils::toStrings( m_ATExecutor.execute( "AT+QNWINFO\r" ) );
        if( ATUtils::isOK( answer, 2 ) ) {
            const auto tokens = StringUtils::splitSequence( answer[ 0 ], { "\",\"", "\",\"", "\"," } );
            auto band = ( tokens.size() > 3 ) ? tokens.at( 2 ) : "";
            return band;
        }
    }
    catch( const std::exception& ex ) {
        LOGGER_TRACE_RAW( cc::trace( "Failed to get a current frequency band : " ) + cc::note( ex.what() ) +
                          cc::trace( ", interface: " ) + cc::note( m_interfaceName ) );
    }
    return {};
}

}  // namespace GSM
}  // namespace Network
}  // namespace HubPlatform
