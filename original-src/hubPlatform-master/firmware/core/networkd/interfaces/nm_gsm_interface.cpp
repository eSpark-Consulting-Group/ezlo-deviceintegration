#include "networkd/interfaces/nm_gsm_interface.h"
#include "json_utils/json_fwd.h"
#include "networkd/monitoring/nm_ping_statistics.h"
#include "networkd/resources/nm_resources.h"
#include "networkd/gsm/nm_gsm_interface_number_manager.h"
#include "networkd/configuration/nm_chat_script.h"

#include "json_utils/json_utils.h"
#include "jsonrpc/jsonrpc.h"
#include "service_interfaces/postponed_task.h"
#include "system_utils/system.h"

#include <cassert>

namespace HubPlatform {
namespace Network {

namespace {
const auto apnShiftTimeout = std::chrono::seconds( 30 );
}  // namespace

using namespace GSM;

GSMInterface::GSMInterface( IDbClient& dbClient,
                            const ProvisionDataUtils::ProvisionData& provisionData,
                            NetworkConfigManager& configManager,
                            const InterfaceMonitorFactory& interfaceMonitorFactory,
                            InterfaceInfoListener& interfaceInfoListener,
                            Services::TaskLoop& taskLoop,
                            std::unique_ptr< Mobile::InterfaceNumber > interfaceNumber,
                            std::unique_ptr< GSM::GSMModem > gsmModem,
                            const NetworkConfigFileManager::MobileConfiguration& configuration )
: m_interfaceNumber( std::move( interfaceNumber ) )
, m_interfaceName( Resources::MobileBaseInterfaceName + std::to_string( m_interfaceNumber->getInterfaceNumber() ) )
, m_deviceName( Resources::MobileBaseDeviceName + std::to_string( m_interfaceNumber->getInterfaceNumber() ) )
, m_priority( Resources::MobileBasePriority + m_interfaceNumber->getInterfaceNumber() )
, m_metric( Resources::MobileBaseMetric + m_interfaceNumber->getInterfaceNumber() )
, m_configuration( configuration )
, m_configManager( configManager )
, m_settings( dbClient, m_deviceName, EnablePolicy::Auto )
, m_monitor( interfaceMonitorFactory.createInterfaceMonitor( *this,
                                                             m_interfaceName,
                                                             true,
                                                             m_configuration.m_pingPeriodNotActive ) )
, m_interfaceInfoListener( interfaceInfoListener )
, m_taskLoop( taskLoop )
, m_gsmModem( std::move( gsmModem ) )
, m_stateManager( dbClient, m_taskLoop, *this, m_configuration )
, m_modemInfoPollingTimer( taskLoop, [this]() { captureModemInfo(); } )
, m_apnSettingsProvider( m_interfaceName, dbClient, *m_gsmModem, provisionData, configuration )
, m_trafficMonitor( m_interfaceName ) {
    m_interfaceInfoListener.onNewInterfaceAdded( m_interfaceName );
    m_configManager.configureMobileInterface( m_interfaceName );

    startMonitoring();
}

GSMInterface::~GSMInterface() {
    stopMonitoring();
    m_interfaceInfoListener.onInterfaceRemoved( m_interfaceName );
}

void GSMInterface::start( const std::weak_ptr< GSMInterface >& weakSelf ) {
    m_weakSelf = weakSelf;

    m_monitor->manualUpdate();
    m_stateManager.start( m_weakSelf );
}

bool GSMInterface::enterPinCodeAutomatically() {
    try {
        if( isPinCodeRequired() ) {
            LOGGER_DEBUG_RAW( cc::debug( "Try to enter a SIM PIN code automatically, interface: " ) +
                              cc::note( m_interfaceName ) );

            const auto setting = m_apnSettingsProvider.getDatabaseUserSetting();
            if( setting ) {
                enterPinCode( setting.m_pinCode );
                LOGGER_DEBUG_RAW( cc::debug( "SIM PIN code was entered automatically, interface: " ) +
                                  cc::note( m_interfaceName ) );

                return true;
            }
            else {
                LOGGER_DEBUG_RAW( cc::debug( "No SIM PIN code stored, interface: " ) + cc::note( m_interfaceName ) );
            }
        }
        else {
            return true;
        }
    }
    catch( ... ) {
        LOGGER_DEBUG_RAW( cc::debug( "Failed to enter a SIM PIN code automatically, interface: " ) +
                          cc::note( m_interfaceName ) );
    }

    return false;
}

Json GSMInterface::getInfo() const {
    return getInfo( m_monitor->getInfo() );
}

bool GSMInterface::isUp() const {
    auto ret = system( ( "ip a | grep " + m_deviceName + " > /dev/null" ).c_str() );
    if( ret > 0 ) {
        return false;
    }
    else {
        return true;
    }
}

void GSMInterface::setUserSettings( const std::string& apnName,
                                    const std::string& dialNumber,
                                    const std::string& pinCode,
                                    const std::string& operatorName,
                                    const std::string& pdpType ) {
    // TODO: implement mobile operator selection using a provided operatorName
    m_apnSettingsProvider.setDatabaseUserSetting(
        GSM::SettingsProvider::Setting( apnName, dialNumber, pinCode, operatorName, pdpType ) );
}

void GSMInterface::setUserSettingsPinCode( const std::string& pinCode ) {
    setUserSettings( "", "", pinCode, "", "" );
}

void GSMInterface::tryUp() {
    m_stateManager.scheduleRun();
}

bool GSMInterface::scheduleUp() {
    if( getEnablePolicy() == EnablePolicy::Off ) {
        LOGGER_WARN_RAW( cc::warn( "Attempt to up a disabled mobile interface, interface: " ) +
                         cc::note( m_interfaceName ) );
        return false;
    }

    if( m_requestedToStop ) {
        LOGGER_WARN_RAW(
            cc::warn( "Mobile interface is scheduled to be stopped, reject an attempt to up it, interface: " ) +
            cc::note( m_interfaceName ) );
        return false;
    }

    if( isUp() ) {
        LOGGER_DEBUG_RAW(
            cc::debug( "Attempt to configure/up an already configured/up mobile interface, interface: " ) +
            cc::note( m_interfaceName ) );
    }

    auto weakSelf = m_weakSelf;
    m_taskLoop
        .createPostponedTask( [weakSelf]( Services::PostponedTask& ) {
            auto interface = weakSelf.lock();
            if( !interface ) {
                LOGGER_WARN_RAW( cc::warn( "GSM interface already removed before scheduleUp" ) );
                return;
            }

            try {
                interface->up();
            }
            catch( const std::exception& e ) {
                LOGGER_ERROR_RAW( cc::error( "Failed to establish a mobile connection, interface: " ) +
                                  cc::note( interface->getInterfaceName() ) + cc::error( ", " ) +
                                  cc::note( e.what() ) );
            }
            catch( ... ) {
                LOGGER_WARN_RAW( cc::warn( "Unknown exception was caught during GSM interface scheduleUp" ) );
            }
        } )
        ->start( apnShiftTimeout, false );

    return true;
}

void GSMInterface::up() {
    const auto setting = m_apnSettingsProvider.getNextSetting();
    if( !setting ) {
        handleHopelessAttemptToUp();
        return;
    }

    stopMonitoring();

    const auto configuration = makeSystemNetworkConfiguration( setting );

    LOGGER_DEBUG_RAW( cc::debug( "Configure a mobile connection for a " ) + cc::note( m_interfaceName ) +
                      cc::debug( " interface using the configuration: " ) + cc::j( configuration.toJson() ) );

    if( configuration.m_ipv6 ) {
        LOGGER_DEBUG_RAW( cc::debug( "Set connection monitor protocol version to IPv6 for interface: " ) +
                          cc::note( m_interfaceName ) );
        m_monitor->setIPVersion( IPVersion::IPv6 );
    }

    m_configManager.configureMobileInterface( m_interfaceName, configuration );
    m_monitor->manualUpdate();

    if( !m_connectionEstablished ) {
        onConnectionFailed();
        LOGGER_ERROR_RAW( cc::error( "Failed to establish a mobile connection, signal quality: " ) +
                          cc::note( getInstantSignalQuality().dump() ) + cc::error( ", interface: " ) +
                          cc::note( m_interfaceName ) + cc::error( ", trying another settings" ) );
    }
    else {
        onConnectionEstablished( setting );
        LOGGER_DEBUG_RAW( cc::debug( "Mobile connection has been established, interface: " ) +
                          cc::note( m_interfaceName ) );
    }

    startMonitoring();
}

MobileConfiguration GSMInterface::makeSystemNetworkConfiguration( const SettingsProvider::Setting& setting ) const {
    MobileConfiguration mobileConfiguration;

    mobileConfiguration.m_apn = setting.m_apnName;
    mobileConfiguration.m_device = m_gsmModem->getDataTTYDevice();
    mobileConfiguration.m_pppName = m_deviceName;
    mobileConfiguration.m_dialNumber = setting.m_dialNumber;
    mobileConfiguration.m_ipv6 = setting.m_pdpType == "IPv6";
    mobileConfiguration.m_metric = std::to_string( m_metric );

    return mobileConfiguration;
}

void GSMInterface::onConnectionEstablished( SettingsProvider::Setting setting ) {
    m_successfulSetting = std::move( setting );
    m_isFinallyFailed = false;

    // notify NetworkManager about new mobile settings
    notifyError( Errors::OK );
}

void GSMInterface::onConnectionFailed() {
    /* in case of a fail there are no changes visible by a monitor, so generate a newInterfaceInfo event manually */
    notifyError( Errors::NETWORK_COULD_NOT_CONNECT );
    down();

    scheduleUp();
}

void GSMInterface::onRegistrationEstablished() {
    m_modemInfoPollingTimer.start( std::chrono::seconds( 0 ), m_configuration.m_modemInfoPollingPeriod );

    m_apnSettingsProvider.reconstructSettings();
    scheduleUp();
}

void GSMInterface::onRegistrationInterrupted() {
    const int primaryPDPContext = 1;
    notifyError( Errors::MODEM_NO_REGISTRATION );

    LOGGER_WARN_RAW( cc::warn( "Mobile network initialization routine was interrupted on interface: " ) +
                     cc::note( m_interfaceName ) + cc::warn( ", rebooting modem..." ) );

    m_modemInfoPollingTimer.stop();
    m_gsmModem->resetPDPContext( primaryPDPContext );
    m_gsmModem->clearFPLMNList();
    rebootModem();
}

/* we did our best - tried all settings and failed to establish a connection, no reason to continue, but
 * we keep trying for cases when fails were caused by a mobile operator itself
 * (some technical issues, payment past...) to be able to establish a connection
 * when such problems get resolved without a need to reboot a hub */
void GSMInterface::handleHopelessAttemptToUp() {
    if( !m_isFinallyFailed ) {
        m_isFinallyFailed = true;

        LOGGER_ERROR_RAW( cc::error( "Finally failed to establish a mobile connection, interface: " ) +
                          cc::note( m_interfaceName ) +
                          cc::error( " after trying all available configurations, start to wait external help..." ) );
    }

    down();
    m_stateManager.onConnectionInterrupted();
}

void GSMInterface::down() {
    m_configManager.configureMobileInterface( m_interfaceName );
    m_successfulSetting = GSM::SettingsProvider::Setting();

    std::this_thread::sleep_for( std::chrono::seconds( 1 ) );  // sleep to allow the monitor to get changes
    m_monitor->manualUpdate();

    LOGGER_DEBUG_RAW( cc::debug( "Mobile interface has been turned off, interface: " ) + cc::note( m_interfaceName ) );

    stopMonitoring();
}

bool GSMInterface::isPinCodeRequired() const {
    return m_gsmModem->isPINCodeRequired();
}

void GSMInterface::enterPinCode( const std::string& pinCode ) {
    m_gsmModem->enterPINCode( pinCode );
    setUserSettingsPinCode( pinCode );
    getSIMInfo( true );
}

void GSMInterface::changePinCode( const std::string& pinCodeNew, const std::string& pinCodeOld ) {
    m_gsmModem->changeSIMPINCode( pinCodeOld, pinCodeNew );
    setUserSettingsPinCode( pinCodeNew );
}

void GSMInterface::setSIMLock( const std::string& pinCode, bool lock ) {
    m_gsmModem->setSIMLock( pinCode, lock );
    getSIMInfo( true );
}

void GSMInterface::resetPinCode( const std::string& pukCode, const std::string& pinCodeNew ) {
    m_gsmModem->resetSIMPINCode( pukCode, pinCodeNew );
    setUserSettingsPinCode( pinCodeNew );
    getSIMInfo( true );
}

EnablePolicy GSMInterface::getEnablePolicy() const {
    return m_settings.getEnablePolicy();
}

bool GSMInterface::setEnablePolicy( EnablePolicy policy ) {
    if( !m_settings.setEnablePolicy( policy ) )
        return true;

    onNewInterfaceInfo( getInfo() );

    if( policy == EnablePolicy::Off ) {
        m_requestedToStop = true;
        down();
    }
    else {
        m_requestedToStop = false;
        m_stateManager.scheduleRun();
    }

    return true;
}

void GSMInterface::reset() {
    m_apnSettingsProvider.resetDatabaseUserSetting();
    m_settings.reset();

    /* as gsm interfaces follow automatic configuration policy, we should try to up the interface
     * here, but now without user settings */
    m_stateManager.scheduleRun();

    LOGGER_DEBUG_RAW( cc::debug( "Current settings has been reset, interface: " ) + cc::note( m_interfaceName ) );
}

void GSMInterface::rebootModem() {
    m_gsmModem->reboot();
}

void GSMInterface::clearFPLMN() {
    m_gsmModem->clearFPLMNList();
}

void GSMInterface::reRegister( bool clearFPLMN ) {
    m_gsmModem->reRegister( clearFPLMN );
}

void GSMInterface::pause() {
    system( ( "ubus call network.interface." + m_interfaceName + " down" ).c_str() );
}

void GSMInterface::restore() {
    system( ( "ubus call network.interface." + m_interfaceName + " up" ).c_str() );
}

void GSMInterface::startMonitoring() {
    m_monitor->configure();
    m_monitor->start();
}

void GSMInterface::stopMonitoring() {
    m_monitor->stop();
}

std::string GSMInterface::getGSMModemIMEI() const {
    return m_gsmModem->getGSMModemIMEI();
}

Json GSMInterface::getSIMInfo( bool realHwRequest ) const {
    const auto& simInfo = m_gsmModem->getSIMInfo( realHwRequest );
    if( !simInfo ) {
        return JSON_EMPTY_OBJ;
    }

    return JSON( "iccid" << simInfo.m_iccid << "imsi" << simInfo.m_imsi << "locked" << simInfo.m_isSIMLocked );
}

bool GSMInterface::isSIMInserted() const {
    return m_gsmModem->isSIMInserted();
}

void GSMInterface::captureModemInfo() {
    const auto accessTechnology = m_gsmModem->getAccessTechnology();
    m_modemInfoProxy.setAccessTechnology( accessTechnology );
    m_modemInfoProxy.setSignalQuality( m_gsmModem->getSignalQuality( accessTechnology ) );
    m_modemInfoProxy.setFrequencyBand( m_gsmModem->getFrequencyBand() );

    onNewInterfaceInfo( m_monitor->getInfo() );
}

Json GSMInterface::getSignalQuality() const {
    return m_modemInfoProxy.getStabilizedSignalQuality();
}

GSMModem::AccessTechnology GSMInterface::getAccessTechnology() const {
    return m_modemInfoProxy.getCachedAccessTechnology();
}

Json GSMInterface::getInstantSignalQuality() const {
    return Mobile::ModemInfoProxy::getSignalQuality( m_gsmModem->getSignalQuality() );
}

/* TODO: add a severity/level flag for a hub.network.get request */
Json GSMInterface::getDebugInformation() const {
    std::string cmdResult;
    int interfaceNumber = -1;
    int result;

    try {
        std::tie( cmdResult, result ) = System::getCommandOutput(
            ( "ip a | awk '/" + m_deviceName + ":/ { print substr( $1, 0, length($1) -1 ) }'" ).c_str() );
        if( !result && !cmdResult.empty() ) {
            interfaceNumber = std::stoi( cmdResult );
        }
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to get debug information, interface: " ) + cc::note( m_interfaceName ) +
                          cc::error( ", reason: " ) + cc::note( ex.what() ) );
    }
    catch( ... ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to get debug information, interface: " ) + cc::note( m_interfaceName ) );
    }

    return JSON( "interfaceNumber" << interfaceNumber );
}

Json GSMInterface::getDeviceInfo() const {
    return m_gsmModem->getDeviceInfo();
}

Json GSMInterface::getModemInfo() const {
    using namespace Resources::InterfaceInfo;

    auto modemInfo = JSON_EMPTY_OBJ;

    modemInfo[ Fields::SIMInfo ] = getSIMInfo();

    if( m_successfulSetting ) {
        const auto js = m_successfulSetting.toJson();
        for( auto it = js.begin(); it != js.end(); it++ ) {
            modemInfo[ it.key() ] = it.value();
        }
    }

    modemInfo[ Fields::AccessTechnology ] = getAccessTechnology().toString();
    modemInfo[ Fields::FrequencyBand ] = m_modemInfoProxy.getCachedFrequencyBand();

    return modemInfo;
}

Json GSMInterface::getInfo( Json dynamicInfo ) const {
    using namespace Resources::InterfaceInfo;

    dynamicInfo[ Fields::Id ] = m_deviceName;
    dynamicInfo[ "modem" ] = getModemInfo();
    dynamicInfo[ Fields::Type ] = Resources::MobileType;
    dynamicInfo[ Fields::Priority ] = m_priority;
    dynamicInfo[ Fields::Metric ] = m_metric;
    dynamicInfo[ Fields::Network ] = Resources::WanNetwork;
    dynamicInfo[ Fields::EnablePolicy ] = toString( getEnablePolicy() );
    dynamicInfo[ Fields::IMEI ] = getGSMModemIMEI();
    dynamicInfo[ Fields::SignalQuality ] = getSignalQuality();
    dynamicInfo[ Fields::DebugInformation ] = getDebugInformation();

    if( m_error != Errors::OK ) {
        setError( dynamicInfo, m_error );
    }

    return dynamicInfo;
}

void GSMInterface::notifyError( Errors::Error error ) {
    m_error = error;
    onNewInterfaceInfo( m_monitor->getInfo() );
}

bool GSMInterface::isInternetAvailable() const {
    return m_monitor->isInternetAvailable();
}

std::string GSMInterface::getInterfaceName() const {
    return m_interfaceName;
}

std::string GSMInterface::getDeviceName() const {
    return m_deviceName;
}

void GSMInterface::onNewInterfaceInfo( Json dynamicInfo ) {
    using namespace Resources::InterfaceInfo;

    const auto isUp = json_get_str( dynamicInfo, Fields::Status ) == "up";
    const auto isInternetAvailable = json_get_bool( dynamicInfo, Fields::InternetAvailable );

    if( isUp && isInternetAvailable ) {
        if( !m_connectionEstablished ) {
            m_connectionEstablished = true;
            m_stateManager.onConnectionRestored();
        }
    }

    /* netifd doesn't control a real state of a mobile connection, so we use a ping to
     * determine that a mobile connection was interrupted */
    if( !isUp || ( isUp && !isInternetAvailable ) ) {
        if( m_connectionEstablished ) {
            /* sometimes a ppp connection gets lost, it gets restored automatically
             * if it's possible to restore a ppp connection at all */
            m_connectionEstablished = false;
            m_stateManager.onConnectionInterrupted( true );
        }
    }

    m_interfaceInfoListener.onNewInterfaceInfo( getInfo( std::move( dynamicInfo ) ) );
}

const char* GSMInterface::getName() const {
    return m_interfaceName.data();
}

void GSMInterface::onInterfaceActive( bool active ) {
    if( m_active == active ) {
        return;
    }

    m_active = active;

    if( m_active ) {
        m_monitor->setPingPeriod( m_configuration.m_pingPeriodActive );
    }
    else {
        m_monitor->setPingPeriod( m_configuration.m_pingPeriodNotActive );
    }
}

InterfaceTrafficMonitor::TrafficStatistic GSMInterface::getTrafficStatistics() const {
    return m_trafficMonitor.getTrafficStatistics();
}

GenericModem& GSMInterface::getGenericModem() {
    return dynamic_cast< GenericModem& >( *m_gsmModem );
}

const GenericModem& GSMInterface::getGenericModem() const {
    return dynamic_cast< const GenericModem& >( *m_gsmModem );
}

Json GSMInterface::getReporterModemInfo() const {
    JsonObject modemInfo = JSON( "interface_name" << m_interfaceName << "signal_quality" << getInstantSignalQuality()
                                                  << "frequency_band" << m_modemInfoProxy.getCachedFrequencyBand() );

    return modemInfo;
}

const PingStatistics& GSMInterface::getPingStatistics() const {
    return m_monitor->pingStatistics();
}

}  // namespace Network
}  // namespace HubPlatform
