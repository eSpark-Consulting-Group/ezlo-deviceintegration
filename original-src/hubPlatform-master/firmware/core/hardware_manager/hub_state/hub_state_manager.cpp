#include "hardware_manager/hub_state/hub_state_manager.h"
#include "hardware_manager/hub_state/model/hub_state_utils.h"
#include "hardware_manager/remote_interface/hm_remote_interface.h"

#include "cored/model/cored_gateway_processes.h"

#include "json_utils/json_utils.h"
#include "hardware_manager/effect/effect.h"

namespace HubPlatform {
namespace HardwareManager {

HubStateManager::HubStateManager( const Network::RemoteInterface& networkManager,
                                  const CoreDRemoteInterface::CoreDInterface& cored,
                                  IDbClient& dbClient,
                                  HubStateIndicationManager& hubStateEffectManager )
: m_networkManager( networkManager )
, m_cored( cored )
, m_dbClient( dbClient )
, m_hubStateIndicationManager( hubStateEffectManager )
, m_currentHubStateEffect( "<no_effect>" )
, m_processReadyToRun( HubState::Process::Unknown )
// Init external states
, m_internetAvailable( m_networkManager.isInternetAvailable() ) {
    if( cored.isProcessRunning( CoreD::Model::GatewayProcess::AddingDevice ) ) {
        m_runningProcesses.insert( HubState::Process::DeviceAdding );
    }
    if( cored.isProcessRunning( CoreD::Model::GatewayProcess::RemovingDevice ) ) {
        m_runningProcesses.insert( HubState::Process::DeviceRemoving );
    }
    manageHubStateIndication();
}

HubStateManager::~HubStateManager() {
    HubState::Process process = !m_runningProcesses.empty() ? *m_runningProcesses.begin() : HubState::Process::Unknown;

    switch( process ) {
    case HubState::Process::FirmwareUpdate:
    case HubState::Process::FactoryReset:
    case HubState::Process::SoftReset:
        // Leave current effect
        break;
    default:
        m_hubStateIndicationManager.indicateDown();
        break;
    }
}

void HubStateManager::setCloudConnection( HubState::CloudConnection cloudConnection ) {
    if( HubStateUtils::getCloudConnection( m_dbClient ) == cloudConnection ) {
        return;
    }
    LOGGER_DEBUG_RAW( cc::debug( "Setting hub state " ) +
                      cc::note( HubState::toString( HubState::Type::CloudConnection ) ) + cc::debug( " to " ) +
                      cc::note( HubState::toString( cloudConnection ) ) );
    HubStateUtils::setCloudConnection( m_dbClient, cloudConnection );
    manageHubStateIndication();
}

HubState::CloudConnection HubStateManager::getCloudConnection() const {
    return HubStateUtils::getCloudConnection( m_dbClient );
}

void HubStateManager::manageHubStateIndication() {
    std::string newEffect;
    auto suggestNewEffect = [&]( std::string effect ) {
        if( newEffect.empty() ) {
            newEffect = std::move( effect );
        }
    };

    HubState::Process process = m_processReadyToRun;
    if( process == HubState::Process::Unknown && !m_runningProcesses.empty() ) {
        process = *m_runningProcesses.begin();
    }

    switch( process ) {
    case HubState::Process::FirmwareUpdate:
    case HubState::Process::BootloaderUpdate:
        suggestNewEffect( EffectType::UpdateEzloFirmware );
        break;
    case HubState::Process::RecoveryPartitionUpdate:
        suggestNewEffect( EffectType::UpdateEzloFirmware );
        break;
    case HubState::Process::SwitchFirmwareToLuup:
        suggestNewEffect( EffectType::FirmwareLuupSwitch );
        break;
    case HubState::Process::Reboot:
        suggestNewEffect( EffectType::HubReboot );
        break;
    case HubState::Process::FactoryReset:
        suggestNewEffect( EffectType::HubFactoryReset );
        break;
    case HubState::Process::SoftReset:
        suggestNewEffect( EffectType::HubSoftReset );
        break;
    case HubState::Process::DeviceAdding:
        suggestNewEffect( EffectType::DeviceAdding );
        break;
    case HubState::Process::DeviceRemoving:
        suggestNewEffect( EffectType::DeviceRemoving );
        break;
    case HubState::Process::Custom:
        suggestNewEffect( EffectType::CustomEffect );
        m_hubStateIndicationManager.setHubStateCustomEffectSettings( m_customEffectColor, m_customEffectType );
        break;
    default:
        break;
    }

    m_hubStateIndicationManager.setHubStateIndicator( Indicator::Internet, m_internetAvailable );

    auto zwaveState = getZwaveState();
    switch( zwaveState ) {
    case HubState::Zwave::Off:
        m_hubStateIndicationManager.setHubStateIndicator( HardwareManager::Indicator::Zwave, false );
        break;
    case HubState::Zwave::On:
        m_hubStateIndicationManager.setHubStateIndicator( HardwareManager::Indicator::Zwave, true );
        break;
    case HubState::Zwave::Include:
        m_hubStateIndicationManager.setHubStateIndicatorBlinking( HardwareManager::Indicator::Zwave, 500, 500 );
        suggestNewEffect( EffectType::DeviceAdding );
        break;
    case HubState::Zwave::Exclude:
        m_hubStateIndicationManager.setHubStateIndicatorBlinking( HardwareManager::Indicator::Zwave, 150, 150 );
        suggestNewEffect( EffectType::DeviceRemoving );
        break;
    default:
        break;
    }

    auto cloudConnection = getCloudConnection();
    switch( cloudConnection ) {
    case HubState::CloudConnection::NotConnected:
        suggestNewEffect( EffectType::NotConnected );
        m_hubStateIndicationManager.setHubStateIndicator( Indicator::Ready, false );
        break;
    case HubState::CloudConnection::UpdatingControllerInformation:
        suggestNewEffect( EffectType::NotConnected );
        m_hubStateIndicationManager.setHubStateIndicatorBlinking( Indicator::Ready, 1000, 1000 );
        break;
    case HubState::CloudConnection::Connecting:
        suggestNewEffect( EffectType::NotConnected );
        m_hubStateIndicationManager.setHubStateIndicatorBlinking( Indicator::Ready, 150, 150 );
        break;
    case HubState::CloudConnection::Connected: {
        auto batteryState = getBatteryState();
        switch( batteryState ) {
        case HubState::Battery::Unspecified:
        case HubState::Battery::Ok:
            suggestNewEffect( EffectType::DefaultEffect );
            break;
        case HubState::Battery::Discharging:
            suggestNewEffect( EffectType::BatteryDischarging );
            break;
        case HubState::Battery::DischargingLow:
            suggestNewEffect( EffectType::BatteryDischargingLow );
            break;
        case HubState::Battery::DischargingCritical:
            suggestNewEffect( EffectType::BatteryDischargingCritical );
            break;
        }
        m_hubStateIndicationManager.setHubStateIndicator( Indicator::Ready, true );
        break;
    }
    default:
        break;
    }

    if( !newEffect.empty() && newEffect != m_currentHubStateEffect ) {
        m_currentHubStateEffect = newEffect;
        m_hubStateIndicationManager.setHubStateEffect( m_currentHubStateEffect );
    }
}

void HubStateManager::onInternetAvailabilityChanged( bool internetAvailable ) {
    if( m_internetAvailable == internetAvailable ) {
        return;
    }

    LOGGER_INFO_RAW( cc::debug( "Handling hub state " ) +
                     cc::note( HubState::toString( HubState::Type::InternetAvailable ) ) + cc::debug( " being " ) +
                     cc::flag( internetAvailable ) );
    m_internetAvailable = internetAvailable;
    manageHubStateIndication();
}

void HubStateManager::setZwaveState( HubState::Zwave zwaveState ) {
    if( HubStateUtils::getZwaveState( m_dbClient ) == zwaveState ) {
        return;
    }
    LOGGER_DEBUG_RAW( cc::debug( "Setting hub state " ) + cc::note( HubState::toString( HubState::Type::ZwaveState ) ) +
                      cc::debug( " to " ) + cc::note( HubState::toString( zwaveState ) ) );
    HubStateUtils::setZwaveState( m_dbClient, zwaveState );
    manageHubStateIndication();
}

HubState::Zwave HubStateManager::getZwaveState() const {
    return HubStateUtils::getZwaveState( m_dbClient );
}

void HubStateManager::setBatteryState( HubState::Battery batteryState ) {
    if( HubStateUtils::getBatteryState( m_dbClient ) == batteryState ) {
        return;
    }
    LOGGER_DEBUG_RAW( cc::debug( "Setting hub state " ) +
                      cc::note( HubState::toString( HubState::Type::BatteryState ) ) + cc::debug( " to " ) +
                      cc::note( HubState::toString( batteryState ) ) );
    HubStateUtils::setBatteryState( m_dbClient, batteryState );
    manageHubStateIndication();
}

HubState::Battery HubStateManager::getBatteryState() const {
    return HubStateUtils::getBatteryState( m_dbClient );
}

void HubStateManager::onZwaveIdentify() {
    LOGGER_DEBUG_RAW( cc::debug( "Handling " ) + cc::note( "zwave identify" ) + cc::debug( " hub event" ) );
    // TODO: support effects priorities or smth.
    m_hubStateIndicationManager.setEventEffect( EffectType::ZwaveIdentify );
}

void HubStateManager::onProcessReadyToStart( HubState::Process process ) {
    m_processReadyToRun = process;
    manageHubStateIndication();
}

void HubStateManager::onProcessStart() {
    LOGGER_DEBUG_RAW( cc::debug( "Starting process " ) + cc::note( HubState::toString( m_processReadyToRun ) ) );
    m_runningProcesses.insert( m_processReadyToRun );
    m_processReadyToRun = HubState::Process::Unknown;
    manageHubStateIndication();
}

void HubStateManager::onProcessRunning( HubState::Process process ) {
    LOGGER_DEBUG_RAW( cc::debug( "Running process " ) + cc::note( HubState::toString( process ) ) );
    m_runningProcesses.insert( process );
    manageHubStateIndication();
}

void HubStateManager::handleCustom( const Color color, const BlinkingType type ) {
    m_runningProcesses.insert( HubState::Process::Custom );
    m_customEffectColor = color;
    m_customEffectType = type;

    if( m_customEffectColor == Color::None ) {
        LOGGER_ERROR( "Color: " + cc::num( (int) type ) + " is undefined" );
        return;
    }

    if( m_customEffectType == BlinkingType::None ) {
        LOGGER_ERROR( "Blinking type: " + cc::num( (int) type ) + " is undefined" );
        return;
    }

    manageHubStateIndication();
}

void HubStateManager::onProcessStopped( HubState::Process process, HubState::ProcessResult result ) {
    LOGGER_DEBUG_RAW(
        cc::debug( "Stopping process " ) + cc::note( HubState::toString( process ) ) +
        ( result == HubState::ProcessResult::Unspecified ?
              "" :
              cc::debug( " with " ) + cc::note( HubState::toString( result ) ) + cc::debug( " result" ) ) );

    m_runningProcesses.erase( process );
    switch( result ) {
    case HubState::ProcessResult::Success:
        m_hubStateIndicationManager.setEventEffect( EffectType::Ok );
        break;
    case HubState::ProcessResult::Fail:
        m_hubStateIndicationManager.setEventEffect( EffectType::Fail );
        break;
    case HubState::ProcessResult::Unspecified:
    default:
        break;
    }
    manageHubStateIndication();
}

bool HubStateManager::isAnyProcessRunning() const {
    return !m_runningProcesses.empty();
}

void HubStateManager::onHubStateEffectStarted() {
    manageHubStateIndication();
}

}  // namespace HardwareManager
}  // namespace HubPlatform
