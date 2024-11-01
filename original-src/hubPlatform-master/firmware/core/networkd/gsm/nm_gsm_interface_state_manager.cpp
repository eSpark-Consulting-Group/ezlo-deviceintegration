#include "networkd/gsm/nm_gsm_interface_state_manager.h"
#include "networkd/interfaces/nm_gsm_interface.h"
#include "networkd/resources/nm_resources.h"
#include "networkd/gsm/nm_gsm_modem_active_simcards_table.h"

#include "log_utils/ezlo_log.h"
#include "tiny/include/tiny/tiny_consolecolors.h"
#include "service_interfaces/task_loop.h"
#include "service_interfaces/task_loop_timer.h"
#include "database_client/idbclient.h"

#include <cassert>
#include <chrono>
#include <utility>
#include <tuple>

namespace HubPlatform {
namespace Network {
namespace Mobile {

namespace {
const auto initColdStartTimeout = std::chrono::seconds( 45 );
const auto initQuickStartTimeout = std::chrono::seconds( 5 );
const auto isRegisteredFailedTimeout = std::chrono::minutes( 3 );
}  // namespace

InterfaceStateManager::InterfaceStateManager( IDbClient& dbClient,
                                              Services::TaskLoop& taskLoop,
                                              GSMInterface& interface,
                                              const NetworkConfigFileManager::MobileConfiguration& configuration )
: m_dbClient( dbClient )
, m_taskLoop( taskLoop )
, m_interface( interface )
, m_modem( *m_interface.m_gsmModem )
, m_apnSettingsProvider( m_interface.m_apnSettingsProvider )
, m_modemInfoPollingTimer( m_interface.m_modemInfoPollingTimer )
, m_interfaceName( m_interface.getInterfaceName() )
, m_networkManager( m_modem, configuration, m_interfaceName )
, m_connectionManager( m_interface, *this, m_taskLoop, configuration ) {}

void InterfaceStateManager::start( const std::weak_ptr< GSMInterface >& weakSelf ) {
    m_weakSelf = weakSelf;
    m_connectionManager.start( m_weakSelf );
    if( m_modem.getState() == GSMModem::State::On ) {
        scheduleRun( initQuickStartTimeout );
    }
    else {
        scheduleRun( initColdStartTimeout );
    }
}

void InterfaceStateManager::scheduleRun( const std::chrono::seconds& relativeStartTime ) {
    auto weakSelf = m_weakSelf;
    m_taskLoop
        .createPostponedTask( [this, weakSelf]( Services::PostponedTask& ) {
            auto interface = weakSelf.lock();
            if( !interface ) {
                LOGGER_WARN_RAW( cc::warn( "Mobile interface already removed before scheduleRun" ) );
                return;
            }

            try {
                run();
            }
            catch( const std::exception& e ) {
                LOGGER_ERROR_RAW( cc::error( "Failed to run state machine, interface: " ) +
                                  cc::note( m_interfaceName ) + cc::error( ", " ) + cc::note( e.what() ) );

                changeState( State::MODEM_HUNGUP );
                handleModemHungUp();
            }
            catch( ... ) {
                LOGGER_WARN_RAW( cc::warn( "Unknown exception was caught during running mobile state manager" ) );
            }
        } )
        ->start( relativeStartTime, false );
    LOGGER_TRACE_RAW( cc::trace( "Scheduled interface state task on mobile interface: " ) +
                      cc::note( m_interfaceName ) + cc::trace( ", with delay: " ) +
                      cc::num10( relativeStartTime.count() ) + cc::trace( " seconds" ) );
}

void InterfaceStateManager::onConnectionInterrupted( bool probablyFalsePositive ) {
    m_connectionManager.onConnectionInterrupted( probablyFalsePositive );
}

void InterfaceStateManager::onConnectionRestored() {
    m_connectionManager.onConnectionRestored();
}

void InterfaceStateManager::run() {
    switch( m_state ) {
    case State::MODEM_UP:
        handleModemUp();
        break;
    case State::SIM_PRESENT:
        handleSIMPresent();
        break;
    case State::SIM_NOT_PRESENT:
        handleSIMNotPresent();
        break;
    case State::PIN_REQUIRED:
        handlePINRequired();
        break;
    case State::CHECK_REGISTRATION:
        handleCheckRegistration();
        break;
    case State::MODEM_HUNGUP:
        handleModemHungUp();
        break;
    case State::MODEM_DOWN:
        handleModemDown();
        break;
    default:
        assert( false );
    }
}

void InterfaceStateManager::handleModemUp() {
    m_apnSettingsProvider.init();

    if( !m_modem.isSIMInserted() ) {
        changeState( State::SIM_NOT_PRESENT );
        handleSIMNotPresent();

        return;
    }

    changeState( State::SIM_PRESENT );
    handleSIMPresent();
}

void InterfaceStateManager::handleSIMPresent() {
    const int primaryPDPContext = 1;
    const auto imei = m_modem.getGSMModemIMEI();
    const auto iccid = m_modem.getSIMInfo().m_iccid;

    auto simTable = GSMModemActiveSimcardsTable( m_dbClient );
    auto previousICCID = simTable.getICCID( imei );
    if( previousICCID != iccid ) {
        LOGGER_INFO_RAW( cc::info( "New SIM detected, change settings for modem: " ) + cc::note( imei ) );
        if( m_modem.isRegistered() ) {
            LOGGER_INFO_RAW( cc::info( "Modem is already registered, skip PDP context reset, modem interface: " ) +
                             cc::note( m_interfaceName ) );
        }
        else {
            m_modem.deregisterFromNetwork();
            LOGGER_INFO_RAW( cc::info( "Old PDP contexts: " ) + cc::j( Json( m_modem.getPDPContexts() ) ) );
            m_modem.resetPDPContext( primaryPDPContext );
            LOGGER_INFO_RAW( cc::info( "New PDP contexts: " ) + cc::j( Json( m_modem.getPDPContexts() ) ) );
        }
        simTable.storeSimcard( imei, iccid );
    }

    {
        const std::string primaryContext = "+CGDCONT: 1";
        auto pdpContexts = m_modem.getPDPContexts();
        bool contextFound = false;
        for( auto& context : pdpContexts ) {
            if( context.find( primaryContext ) != std::string::npos ) {
                contextFound = true;
                break;
            }
        }
        if( !contextFound ) {
            LOGGER_WARN( cc::warn( "Primary PDP context not found. Resetting PDP context for modem interface: " ) +
                         cc::note( m_interfaceName ) );
            m_modem.deregisterFromNetwork();
            m_modem.resetPDPContext( primaryPDPContext );
            pdpContexts = m_modem.getPDPContexts();
        }
        LOGGER_INFO_RAW( cc::info( "Modem PDP context list for interface " ) + cc::note( m_interfaceName ) +
                         cc::note( ": " ) + cc::j( Json( pdpContexts ) ) );
    }

    if( m_modem.isPINCodeRequired() && !m_interface.enterPinCodeAutomatically() ) {
        changeState( State::PIN_REQUIRED );
        handlePINRequired();

        return;
    }

    LOGGER_DEBUG_RAW( cc::debug( "SIM Lock is turned " ) + cc::note( ( m_modem.isLocked() ? "on" : "off" ) ) +
                      cc::debug( ", interface: " ) + cc::note( m_interfaceName ) );

    changeState( State::CHECK_REGISTRATION );
    handleCheckRegistration();
}

void InterfaceStateManager::handleCheckRegistration() {
    m_modemInfoPollingTimer.stop();

    auto result = registerToNetwork();
    if( !result.first ) {
        if( m_isRegistered ) {
            m_isRegistered = false;
            LOGGER_WARN_RAW( cc::warn( "Lost registration on a mobile interface, interface: " ) +
                             cc::note( m_interfaceName ) );

            m_interface.onRegistrationInterrupted();
        }

        scheduleRun( result.second );

        return;
    }
    else {
        m_isRegistered = true;
        m_interface.onRegistrationEstablished();

        return;
    }
}

void InterfaceStateManager::handleSIMNotPresent() {
    LOGGER_WARN_RAW( cc::warn( "SIM card isn't present, interface: " ) + cc::note( m_interfaceName ) );
    m_interface.notifyError( Errors::MODEM_SIM_NO_PRESENT );
}

void InterfaceStateManager::handlePINRequired() {
    LOGGER_WARN_RAW( cc::warn( "SIM PIN code is required, interface: " ) + cc::note( m_interfaceName ) );
    m_interface.notifyError( Errors::MODEM_SIM_PIN_CODE_IS_REQUIRED );
}

void InterfaceStateManager::handleModemHungUp() {
    try {
        LOGGER_WARN_RAW( cc::warn( "Mobile modem hang up, interface: " ) + cc::note( m_interfaceName ) );
        m_modem.reboot();

        changeState( State::MODEM_DOWN );
    }
    catch( std::exception& e ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to handle modem hung up on interface: " ) + cc::note( m_interfaceName ) +
                          cc::error( ", " ) + cc::note( e.what() ) );
        changeState( State::MODEM_HUNGUP );
    }

    m_interface.notifyError( Errors::MODEM_HUNG );
}

void InterfaceStateManager::handleModemDown() {
    LOGGER_INFO_RAW( cc::info( "Mobile modem is down, interface: " ) + cc::note( m_interfaceName ) );
}

std::pair< bool, std::chrono::seconds > InterfaceStateManager::registerToNetwork() {
    std::chrono::seconds timeout( isRegisteredFailedTimeout );
    bool isRegistered = false;

    try {
        std::tie( isRegistered, timeout ) = m_networkManager.registerToNetwork();
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to check is interface registered, interface: " ) +
                          cc::note( m_interfaceName ) + cc::error( ", reason " ) + cc::note( e.what() ) );
    }
    catch( ... ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to check is interface registered, interface: " ) +
                          cc::note( m_interfaceName ) );
    }

    return { isRegistered, timeout };
}

void InterfaceStateManager::reRegister( bool cleanFPLMN ) {
    m_modem.reRegister( cleanFPLMN );
}

void InterfaceStateManager::rebootModem() {
    m_modem.reboot();
}

void InterfaceStateManager::changeState( State state ) {
    m_state = state;
    LOGGER_INFO_RAW( cc::info( "Modem state changed to: " ) + cc::note( stateToString( m_state ) ) +
                     cc::info( ", interface: " ) + cc::note( m_interfaceName ) );
}

std::string InterfaceStateManager::stateToString( State state ) {
    static const std::map< State, std::string > map{
        { State::MODEM_UP, "ModemUp" },
        { State::MODEM_HUNGUP, "ModemHungup" },
        { State::MODEM_DOWN, "ModemDown" },
        { State::SIM_PRESENT, "SimPresent" },
        { State::SIM_NOT_PRESENT, "SimNotPresent" },
        { State::PIN_REQUIRED, "PinRequired" },
        { State::CHECK_REGISTRATION, "CheckRegistration" },
    };

    auto it = map.find( state );
    return ( it != map.cend() ) ? it->second : "Unknown";
}

}  // namespace Mobile
}  // namespace Network
}  // namespace HubPlatform
