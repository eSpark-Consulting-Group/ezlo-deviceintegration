#include "networkd/gsm/nm_gsm_mobile_network_manager.h"
#include "networkd/gsm/nm_gsm.h"

#include "log_utils/ezlo_log.h"
#include "tiny/include/tiny/tiny_consolecolors.h"
#include "json_utils/json_utils.h"

#include <cassert>
#include <sstream>
#include <utility>

namespace HubPlatform {
namespace Network {
namespace Mobile {

namespace {
const int checkRegisterWaitAmounts = 2;
const int scanWaitAmounts = 5;
const int registerWaitAmounts = 5;
const int autoRegisterWaitAmounts = 5;
const int scanRepeatAmounts = 2;

const auto failedToRegisterFirstTimeout = std::chrono::seconds( 30 );
const auto failedToRegisterSecondTimeout = std::chrono::minutes( 1 );

const auto checkScanResultTimeout = std::chrono::seconds( 45 );
const auto checkRegistrationResultTimeout = std::chrono::seconds( 45 );
}  // namespace

NetworkManager::ScanResult::ScanResult() = default;
NetworkManager::ScanResult::ScanResult( std::string networkId, bool isForbidden )
: m_networkId( std::move( networkId ) )
, m_isForbidden( isForbidden ) {}

NetworkManager::NetworkManager( GSM::GSMModem& modem,
                                const NetworkConfigFileManager::MobileConfiguration& configuration,
                                std::string interfaceName )
: m_modem( modem )
, m_configuration( configuration )
, m_interfaceName( std::move( interfaceName ) ) {
    resetInternalData();
}

NetworkManager::RegistrationResult NetworkManager::registerToNetwork() {
    switch( m_state ) {
    case State::INIT:
        return handleInit();
    case State::NOT_REGISTERED:
        return handleNotRegistered();
    case State::CYCLE_WHITE_LIST:
        return handleCycleWhiteList();
    case State::SCAN_IN_PROGRESS:
        return handleScanInProgress();
    case State::TRY_TO_REGISTER:
        return handleTryToRegister();
    case State::TRY_TO_AUTO_REGISTER:
        return handleTryToAutoRegister();
    case State::REGISTERED:
        return handleRegistered();
    default:
        assert( false );
    }

    return { true, std::chrono::seconds( 0 ) };
}

NetworkManager::RegistrationResult NetworkManager::handleInit() {
    resetInternalData();

    // On the very first init call, try to start registration if needed
    if( m_registeringNetwork.empty() || getCurrentNetwork().empty() ) {
        if( !m_modem.isRegistered() )
            m_modem.reRegister();
    }

    changeState( State::NOT_REGISTERED );
    return handleNotRegistered();
}

NetworkManager::RegistrationResult NetworkManager::handleNotRegistered() {
    const auto networkId = getCurrentNetwork();

    if( networkId.empty() ) {
        if( m_checkRegisterWaitAmounts < getRebootTriger() ) {
            LOGGER_INFO( cc::info( "Mobile network registration timeout, modem will be rebooted, interface: " ) +
                         cc::note( m_interfaceName ) );
            cleanDataPDPContext();
            m_modem.reboot();

            return { false, m_configuration.m_finallyFailedToRegisterTimeout };
        }
        if( m_checkRegisterWaitAmounts < 0 ) {
            --m_checkRegisterWaitAmounts;

            LOGGER_DEBUG_RAW( cc::debug( "Mobile network not registered, try re-register, interface: " ) +
                              cc::note( m_interfaceName ) );
            m_modem.clearFPLMNList();
            m_modem.reRegister();

            return { false, m_configuration.m_finallyFailedToRegisterTimeout };
        }
        if( m_checkRegisterWaitAmounts == 0 ) {
            --m_checkRegisterWaitAmounts;
            LOGGER_TRACE_RAW( cc::trace( "Mobile network registration second timeout, interface: " ) +
                              cc::note( m_interfaceName ) );

            m_modem.clearFPLMNList();
            return { false, failedToRegisterSecondTimeout };
        }
        else {
            --m_checkRegisterWaitAmounts;
            LOGGER_TRACE_RAW( cc::trace( "Mobile network registration first timeout, interface: " ) +
                              cc::note( m_interfaceName ) );
            return { false, failedToRegisterFirstTimeout };
        }
    }

    LOGGER_INFO_RAW( cc::info( "Registered mobile network: " ) + cc::note( networkId ) + cc::info( ", interface: " ) +
                     cc::note( m_interfaceName ) );

    if( !isWhiteListProvided() || ( !m_whiteList.empty() && networkId == m_whiteList.front() ) ) {
        m_registeringNetwork = networkId;

        changeState( State::REGISTERED );
        return handleRegistered();
    }

    changeState( State::CYCLE_WHITE_LIST );
    return handleCycleWhiteList();
}

NetworkManager::RegistrationResult NetworkManager::handleCycleWhiteList() {
    if( m_scanResult.empty() ) {
        m_modem.startScanNetworks();

        changeState( State::SCAN_IN_PROGRESS );
        return handleScanInProgress();
    }

    for( auto whiteListIt = m_whiteList.begin(); whiteListIt != m_whiteList.end(); ) {
        const auto networkId = *whiteListIt;
        whiteListIt = m_whiteList.erase( whiteListIt );

        auto foundNetworkIt = m_scanResult.find( networkId );
        if( foundNetworkIt != m_scanResult.end() ) {
            m_registeringNetwork = networkId;

            if( m_registeringNetwork == getCurrentNetwork() ) {
                changeState( State::REGISTERED );
                return handleRegistered();
            }

            LOGGER_INFO_RAW( cc::info( "Try to register to a " ) +
                             cc::note( getHumanNetworkName( m_registeringNetwork ) ) +
                             cc::info( " mobile network, interface: " ) + cc::note( m_interfaceName ) );

            m_modem.clearFPLMNList();
            m_modem.startRegisterToNetwork( m_registeringNetwork );

            changeState( State::TRY_TO_REGISTER );
            return handleTryToRegister();
        }
    }

    if( m_configuration.m_autoRegistration ) {
        m_modem.clearFPLMNList();
        m_modem.startRegisterToNetwork();

        LOGGER_DEBUG_RAW( cc::debug( "Try to auto register, interface: " ) + cc::note( m_interfaceName ) );

        changeState( State::TRY_TO_AUTO_REGISTER );
        return handleTryToAutoRegister();
    }

    return failedToRegister();
}

NetworkManager::RegistrationResult NetworkManager::handleScanInProgress() {
    auto scanResult = m_modem.consumeScanNetworksResult();
    auto scanFailed = scanResult.find( "ERROR" ) != scanResult.npos;

    if( scanResult.empty() || scanFailed ) {
        if( m_scanWaitAmounts <= 0 || scanFailed ) {
            m_modem.completeScanNetworks();

            if( m_scanRepeatAmounts <= 0 ) {
                throw GSM::GSMModemHangs( "Failed to scan networks" );
            }

            m_scanRepeatAmounts--;

            changeState( State::CYCLE_WHITE_LIST );
            return handleCycleWhiteList();
        }
        m_scanWaitAmounts--;

        return { false, checkScanResultTimeout };
    }

    m_modem.completeScanNetworks();

    m_scanResult = processScanResult( scanResult );
    printScanResult();

    changeState( State::CYCLE_WHITE_LIST );
    return handleCycleWhiteList();
}

NetworkManager::RegistrationResult NetworkManager::handleTryToRegister() {
    auto registerResult = m_modem.consumeRegisterToNetworkResult();
    auto foundError = registerResult.find( "ERROR" ) != registerResult.npos;
    bool registerFailed = false;

    if( foundError ) {
        m_modem.completeRegisterToNetwork();
        if( m_registeringNetwork != getCurrentNetwork() ) {
            registerFailed = true;
        }
    }

    if( registerResult.empty() || registerFailed ) {
        if( m_registerWaitAmounts <= 0 || registerFailed ) {
            m_registerWaitAmounts = registerWaitAmounts;
            m_modem.completeRegisterToNetwork();

            changeState( State::CYCLE_WHITE_LIST );
            return handleCycleWhiteList();
        }
        m_registerWaitAmounts--;

        return { false, checkRegistrationResultTimeout };
    }

    m_modem.completeRegisterToNetwork();

    changeState( State::REGISTERED );
    return handleRegistered();
}

NetworkManager::RegistrationResult NetworkManager::handleTryToAutoRegister() {
    auto registerResult = m_modem.consumeRegisterToNetworkResult();
    auto foundError = registerResult.find( "ERROR" ) != registerResult.npos;
    bool registerFailed = false;

    if( foundError ) {
        m_modem.completeRegisterToNetwork();
        if( getCurrentNetwork().empty() ) {
            registerFailed = true;
        }
    }

    if( registerResult.empty() || registerFailed ) {
        if( m_autoRegisterWaitAmounts <= 0 || registerFailed ) {
            m_autoRegisterWaitAmounts = autoRegisterWaitAmounts;
            m_modem.completeRegisterToNetwork();

            return failedToRegister();
        }
        m_autoRegisterWaitAmounts--;

        return { false, checkRegistrationResultTimeout };
    }

    m_modem.completeRegisterToNetwork();
    m_registeringNetwork = getCurrentNetwork();

    changeState( State::REGISTERED );
    return handleRegistered();
}

NetworkManager::RegistrationResult NetworkManager::handleRegistered() {
    auto currentNetwork = getCurrentNetwork();
    auto currentAccessTechnology = getCurrentAccessTechnology();

    if( m_registeredNetwork.empty() ) {
        m_registeredNetwork = m_registeringNetwork;
        m_accessTechnology = currentAccessTechnology;
        LOGGER_INFO_RAW( cc::info( "Registered on a " ) + cc::note( getHumanNetworkName( m_registeredNetwork ) ) +
                         cc::info( "/" ) + cc::note( m_accessTechnology.toString() ) +
                         cc::info( " mobile network, interface: " ) + cc::note( m_interfaceName ) );
    }

    if( m_registeredNetwork != currentNetwork || m_accessTechnology != currentAccessTechnology ) {
        if( !currentNetwork.empty() ) {
            m_registeredNetwork = currentNetwork;
            m_accessTechnology = currentAccessTechnology;

            LOGGER_INFO_RAW( cc::info( "Re-registered on a " ) +
                             cc::note( getHumanNetworkName( m_registeredNetwork ) ) + cc::info( "/" ) +
                             cc::note( m_accessTechnology.toString() ) + cc::info( " mobile network, interface: " ) +
                             cc::note( m_interfaceName ) );

            if( isWhiteListProvided() ) {
                changeState( State::INIT );
                return handleInit();
            }
        }
        else {
            LOGGER_WARN_RAW( cc::warn( "Registration on a " ) + cc::note( getHumanNetworkName( m_registeredNetwork ) ) +
                             cc::warn( "/" ) + cc::note( m_accessTechnology.toString() ) +
                             cc::warn( " mobile network was lost, interface: " ) + cc::note( m_interfaceName ) );

            changeState( State::INIT );
            return handleInit();
        }
    }

    return { true, std::chrono::seconds( 0 ) };
}

/* finally failed to register, start waiting till we can register,
 * trying to register periodically */
NetworkManager::RegistrationResult NetworkManager::failedToRegister() {
    LOGGER_WARN_RAW( cc::warn( "Failed to register to any mobile network, interface: " ) +
                     cc::note( m_interfaceName ) );

    changeState( State::INIT );
    return { false, m_configuration.m_finallyFailedToRegisterTimeout };
}

void NetworkManager::resetInternalData() {
    m_checkRegisterWaitAmounts = checkRegisterWaitAmounts;
    m_scanWaitAmounts = scanWaitAmounts;
    m_registerWaitAmounts = registerWaitAmounts;
    m_autoRegisterWaitAmounts = autoRegisterWaitAmounts;
    m_scanRepeatAmounts = scanRepeatAmounts;
    m_registeredNetwork.clear();
    m_accessTechnology = GSM::GSMModem::AccessTechnology();
    m_registeringNetwork.clear();
    m_scanResult.clear();

    m_whiteList = std::list< std::string >( m_configuration.m_whiteOperatorsList.begin(),
                                            m_configuration.m_whiteOperatorsList.end() );
}

NetworkManager::ScanResults NetworkManager::processScanResult( const std::string& modemResponse ) const {
    ScanResults networksList;
    bool isForbidden = false;
    std::string networkId;

    const auto scanResult = ',' + modemResponse.substr( sizeof( "+COPS:" ) );

    std::size_t closerPos = 0;
    while( true ) {
        auto openerPos = scanResult.find( ",(", closerPos );
        closerPos = scanResult.find( "),", closerPos + 1 );

        if( openerPos == scanResult.npos || closerPos == scanResult.npos ) {
            break;
        }

        auto networkString = scanResult.substr( openerPos + 2, closerPos - openerPos - 2 );

        if( networkString.empty() || networkString.size() < 6 ) {
            continue;
        }

        std::stringstream stream( networkString );

        int i = 0;
        while( stream.good() ) {
            std::string value;

            getline( stream, value, ',' );
            if( i == 0 ) {
                if( value == "0" || value == "3" ) {
                    isForbidden = true;
                }
            }
            else if( i == 3 ) {
                networkId = value.substr( 1, value.size() - 2 );  // drop "" symbols
            }

            i++;
        }

        networksList.emplace( std::make_pair( networkId, ScanResult{ networkId, isForbidden } ) );
    }

    return networksList;
}

void NetworkManager::printScanResult() const {
    auto js = JSON_EMPTY_ARR;

    for( const auto& element : m_scanResult ) {
        js.push_back( JSON( "rplmn" << element.second.m_networkId << "isForbidden" << element.second.m_isForbidden ) );
    }

    LOGGER_DEBUG_RAW( cc::debug( "Scanned mobile networks, interface: " ) + cc::note( m_interfaceName ) +
                      cc::debug( " : " ) + cc::j( js ) );
}

std::string NetworkManager::getHumanNetworkName( NetworkManager::NetworkId networkId ) const {
    const static std::map< NetworkId, std::string > networkNames = {
        { "310260", "T-Mobile" }, { "310410", "AT&T" }, { "311480", "Verizon" },
        { "25501", "MTS" },       { "25506", "Life" },  { "25503", "Kievstar" }
    };

    auto it = networkNames.find( networkId );
    if( it != networkNames.end() ) {
        return it->second;
    }

    return networkId;
}

NetworkManager::NetworkId NetworkManager::getCurrentNetwork() const {
    return m_modem.getCurrentNetworkId();
}

GSM::GSMModem::AccessTechnology NetworkManager::getCurrentAccessTechnology() const {
    return m_modem.getAccessTechnology();
}

bool NetworkManager::isWhiteListProvided() const {
    return !m_configuration.m_whiteOperatorsList.empty();
}

int NetworkManager::getRebootTriger() const {
    const auto timeout = std::chrono::duration_cast< std::chrono::seconds >( std::chrono::minutes( 15 ) ).count();

    auto period =
        std::chrono::duration_cast< std::chrono::seconds >( m_configuration.m_finallyFailedToRegisterTimeout ).count();

    return -( timeout / ( period + 1 ) + 1 );
}

void NetworkManager::cleanDataPDPContext() {
    /* 1-st context is used for data link */
    if( !m_modem.resetPDPContext( 1 ) ) {
        LOGGER_WARN_RAW( cc::warn( "Failed to reset a data PDP context, interface: " ) + cc::note( m_interfaceName ) );
    }
}

void NetworkManager::changeState( State state ) {
    m_state = state;
    LOGGER_INFO_RAW( cc::info( "Mobile network state changed to: " ) + cc::note( stateToString( m_state ) ) +
                     cc::info( ", interface: " ) + cc::note( m_interfaceName ) );
}

std::string NetworkManager::stateToString( State state ) {
    static const std::map< State, std::string > map{
        { State::INIT, "Init" },
        { State::NOT_REGISTERED, "NotRegistered" },
        { State::CYCLE_WHITE_LIST, "CycleWhiteList" },
        { State::SCAN_IN_PROGRESS, "ScanInProgress" },
        { State::TRY_TO_REGISTER, "TryToRegister" },
        { State::REGISTERED, "Registered" },
    };

    auto it = map.find( state );
    return ( it != map.cend() ) ? it->second : "Unknown";
}

}  // namespace Mobile
}  // namespace Network
}  // namespace HubPlatform
