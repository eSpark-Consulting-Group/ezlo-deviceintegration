#include "hardware_manager/hm_service.h"
#include "hardware_manager/effect/effects.h"
#include "hardware_manager/effect/firmware_down_preset.h"
#include "hardware_manager/remote_interface/hm_remote_interface.h"
#include "hardware_manager/battery/battery_manager_factory.h"
#include "hardware_manager/speaker_wrapper.h"
#include "hardware_manager/resources/hm_resources.h"
#include "hardware_manager/utils/battery.h"
#include "hardware_manager/utils/time.h"

#include "cored/remote_interface/cored_remote_interface.h"

#include "system_utils/environment.h"
#include "common_utils/backup/backup_helper.h"
#include "common_utils/backup/restore_helper.h"
#include "common_utils/lock_helper.h"
#include "common_utils/cpp14.h"
#include "jsonrpc/jsonrpc.h"
#include "database_client/dbqueryparser.h"
#include "hardware_manager/utils/check_time_change.h"
#include "base_utils/timeutils.h"

#if defined( __HUB_TYPE_G150__ ) || defined( __HUB_TYPE_G450__ ) || defined( __HUB_TYPE_G550__ )
#include "hardware_manager/button/vera/vera_buttons_handler.h"
#endif

#include <queue>

const std::chrono::milliseconds ButtonCheckInterval( 50 );
constexpr std::chrono::minutes SpeakerConfigureTryInterval( 5 );
static constexpr size_t RebootAttemptCount( 10 );
static constexpr std::chrono::seconds RebootAttemptTimeout( 5 );
static constexpr std::chrono::minutes RebootTimeout( 5 );
static constexpr std::chrono::minutes HubUptimePrintInterval( 60 );

namespace HubPlatform {
namespace HardwareManager {

namespace {
HubState::Battery getBatteryState( const BatteryInfo& info ) {
    if( !info.isValid() ) {
        return HubState::Battery::Unspecified;
    }
    if( !info.isDischarging() ) {
        return HubState::Battery::Ok;
    }
    if( info.isCritical() ) {
        return HubState::Battery::DischargingCritical;
    }
    if( info.isLow() ) {
        return HubState::Battery::DischargingLow;
    }
    return HubState::Battery::Discharging;
}
}  // namespace

Service::Service()
: Services::Service( GroupsNames::HARDWARE_MANAGER )
, m_taskPool( [this]() {
    if( m_hubStateManager )
        m_hubStateManager->onHubStateEffectStarted();
} )
, m_forceBroadcastTimer( *this, [this]() { forceBroadcast(); } )
, m_isButtonsThreadRunning( true ) {}

Service::~Service() {
    m_taskPool.shutdown();
    m_isButtonsThreadRunning = false;
    if( m_buttonsThread.joinable() ) {
        m_buttonsThread.join();
    }
}

std::shared_ptr< Effect > Service::getEffect( const std::string& effectName ) {
    if( effectName == EffectType::NotConnected )
        return std::make_shared< HubNotConnectedEffect >( m_config->getEmotionLed() );
    if( effectName == EffectType::DefaultEffect )
        return std::make_shared< DefaultColorEffect >( m_config->getEmotionLed() );
    if( effectName == EffectType::DefaultRecovery )
        return std::make_shared< DefaultRecoveryEffect >( m_config->getEmotionLed() );
    if( effectName == EffectType::UpdateEzloFirmware )
        return std::make_shared< UpdateEzloFWEffect >( m_config->getEmotionLed() );
    if( effectName == EffectType::LockEzlo )
        return std::make_shared< LockEzloEffect >( m_config->getEmotionLed() );
    if( effectName == EffectType::CriticalError )
        return std::make_shared< CriticalErrEffect >( m_config->getEmotionLed() );
    if( effectName == EffectType::Ok )
        return std::make_shared< OkColorEffect >( m_config->getEmotionLed() );
    if( effectName == EffectType::Fail )
        return std::make_shared< FailColorEffect >( m_config->getEmotionLed() );
    if( effectName == EffectType::SelectEzlo )
        return std::make_shared< SelectEzloEffect >( m_config->getEmotionLed() );
    if( effectName == EffectType::EzloFound )
        return std::make_shared< EzloFoundEffect >( m_config->getEmotionLed() );
    if( effectName == EffectType::WifiWps )
        return std::make_shared< WifiWpsColorEffect >( m_config->getEmotionLed(), m_config->getWifiLed() );
    if( effectName == EffectType::WifiConnecting )
        return std::make_shared< WifiConnectingEffect >( m_config->getWifiLed() );
    if( effectName == EffectType::FilesUploaded )
        return std::make_shared< FilesUploadedEffect >( m_config->getEmotionLed() );
    if( effectName == EffectType::ConfirmRecovery )
        return std::make_shared< ConfirmRecoveryEffect >( m_config->getEmotionLed() );
    if( effectName == EffectType::HubPowerOff )
        return std::make_shared< HubPowerOffEffect >( m_config->getEmotionLed() );
    if( effectName == EffectType::HubReboot )
        return std::make_shared< HubRebootEffect >( m_config->getEmotionLed() );
    if( effectName == EffectType::HubSoftReset )
        return std::make_shared< HubSoftResetEffect >( m_config->getEmotionLed() );
    if( effectName == EffectType::HubFactoryReset )
        return std::make_shared< HubFactoryResetEffect >( m_config->getEmotionLed() );
    if( effectName == EffectType::DeviceAdding )
        return std::make_shared< DeviceAddingEffect >( m_config->getEmotionLed() );
    if( effectName == EffectType::DeviceRemoving )
        return std::make_shared< DeviceRemovingEffect >( m_config->getEmotionLed() );
    if( effectName == EffectType::FirmwareLuupSwitch )
        return std::make_shared< FirmwareLuupSwitchEffect >(
            m_config->getInternetLed(), m_config->getWifiLed(), m_config->getZwaveLed(), m_config->getServiceLed() );
    if( effectName == EffectType::ZwaveIdentify ) {
        return std::make_shared< ZwaveIdentifyEffect >( m_config->getEmotionLed(), m_config->getZwaveLed() );
    }
    if( effectName == EffectType::BatteryDischarging ) {
        return std::make_shared< BatteryDischargingEffect >( m_config->getEmotionLed() );
    }
    if( effectName == EffectType::BatteryDischargingLow ) {
        return std::make_shared< BatteryDischargingLowEffect >( m_config->getEmotionLed() );
    }
    if( effectName == EffectType::BatteryDischargingCritical ) {
        return std::make_shared< BatteryDischargingCriticalEffect >( m_config->getEmotionLed() );
    }
    if( effectName == EffectType::CustomEffect ) {
        return std::make_shared< CustomEffect >( m_config->getEmotionLed(), m_color, m_blinking );
    }
    return nullptr;
}

Led* Service::getIndicator( const std::string& indicatorName ) {
    if( indicatorName == Indicator::Error ) {
        return &m_config->getErrorRed();
    }
    else if( indicatorName == Indicator::Ready ) {
        return &m_config->getReadyLed();
    }
    else if( indicatorName == Indicator::Internet ) {
        return &m_config->getInternetLed();
    }
    else if( indicatorName == Indicator::Message ) {
        return &m_config->getMessageLed();
    }
    else if( indicatorName == Indicator::Zwave ) {
        return &m_config->getZwaveLed();
    }
    return nullptr;
}

void Service::startEffect( const std::string& effectName, int priority ) {
    std::shared_ptr< Effect > effect = getEffect( effectName );
    if( effect ) {
        m_effectsInQueue.insert( std::pair< std::string, std::shared_ptr< Effect > >( effectName, effect ) );
        m_taskPool.safeSubmit( EffectHandler( effect, priority ) );
    }
    else {
        LOGGER_DEBUG( "effect " + effectName + " not found" );
    }
}

void Service::stopEffect( const std::string& effectName ) {
    auto effects = m_effectsInQueue.equal_range( effectName );
    if( effects.first == effects.second ) {
        LOGGER_TRACE( "effect " + effectName + " not active" );
    }
    for( auto effect = effects.first; effect != effects.second; ++effect ) {
        auto handlerPtr = effect->second;
        handlerPtr->stop();
    }
    m_effectsInQueue.erase( effects.first, effects.second );
}

void Service::setEventEffect( const std::string& effectName ) {
    startEffect( effectName, static_cast< int >( Priority::REALTIME ) );
}

void Service::setHubStateEffect( const std::string& effectName ) {
    LOGGER_TRACE_RAW( cc::debug( "Setting hub state emotion to " ) + cc::note( effectName ) );
    auto effect = getEffect( effectName );
    if( !effect ) {
        LOGGER_DEBUG_RAW( cc::debug( "Attempt to set non-existent hub state effect: " ) + cc::note( effectName ) );
        return;
    }
    m_taskPool.setDefaultTask( EffectHandler( effect, 16 ) );
}

void Service::setHubStateCustomEffectSettings( const Color color, const BlinkingType blinking ) {
    m_color = color;
    m_blinking = blinking;
}

void Service::setHubStateIndicator( const std::string& indicator, bool state ) {
    LOGGER_TRACE_RAW( cc::debug( "Setting hub state indicator " ) + cc::note( indicator ) + cc::debug( " to " ) +
                      cc::flag( state ) );
    auto led = getIndicator( indicator );
    if( !led ) {
        LOGGER_DEBUG_RAW( cc::debug( "Attempt to set non-existent hub state indicator: " ) + cc::note( indicator ) );
        return;
    }
    led->turn( state );
}

void Service::setHubStateIndicatorBlinking( const std::string& indicator, int msOff, int msOn ) {
    LOGGER_TRACE_RAW( cc::debug( "Setting hub state indicator " ) + cc::note( indicator ) + cc::debug( " to blink " ) +
                      cc::num10( msOff ) + cc::debug( "/" ) + cc::num10( msOn ) );
    auto led = getIndicator( indicator );
    if( !led ) {
        LOGGER_DEBUG_RAW( cc::debug( "Attempt to set non-existent hub state indicator: " ) + cc::note( indicator ) );
        return;
    }
    led->turnBlinking( msOff, msOn );
}

void Service::indicateDown() {
    auto& led = m_config->getEmotionLed();
    firmwareDownPreset( led );
}

void Service::handleMethodCall( const JsonObject& json, MessageSubClasses requestType, const std::string& sender ) {
    switch( requestType ) {
    case HUB_POWEROFF: {
        powerOff();
        break;
    }
    case HUB_STATE_SET: {
        auto typeString = json_get_str( json, "type" );
        auto type = HubState::typeFromString( typeString );
        auto valueString = json_get_str( json, "value" );
        switch( type ) {
        case HubState::Type::CloudConnection:
            m_hubStateManager->setCloudConnection( HubState::cloudConnectionFromString( valueString ) );
            break;
        case HubState::Type::ZwaveState:
            m_hubStateManager->setZwaveState( HubState::zwaveStateFromString( valueString ) );
            break;
        case HubState::Type::Unknown:
        default:
            Log::error( cc::error( "Trying to set unknown hub state: " ) + cc::note( typeString ) );
            break;
        }
        break;
    }
    case HUB_PROCESS_RUNNING: {
        auto typeString = json_get_str( json, "type" );
        auto type = HubState::processFromString( typeString );
        if( type == HubState::Process::Unknown ) {
            Log::error( cc::error( "Trying to notify about unknown process running: " ) + cc::note( typeString ) );
            break;
        }
        m_hubStateManager->onProcessRunning( type );
        break;
    }

    case HUB_CUSTOM_LED: {
        auto color = json_get_int( json, "color" );
        auto type = json_get_int( json, "blinkingType" );
        m_hubStateManager->handleCustom( (Color) color, (BlinkingType) type );
        break;
    }

    case HUB_PROCESS_STOPPED: {
        auto typeString = json_get_str( json, "type" );
        auto type = HubState::processFromString( typeString );
        if( type == HubState::Process::Unknown ) {
            Log::error( cc::error( "Trying to notify about unknown process stopped: " ) + cc::note( typeString ) );
            break;
        }
        m_hubStateManager->onProcessStopped( type,
                                             HubState::processResultFromString( json_get_str( json, "result" ) ) );
        break;
    }
    default:
        Services::Service::handleMethodCall( json, requestType, sender );
    }
}

JsonObject Service::handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) {
    switch( requestType ) {
    case SPEAKER_PLAY: {
        auto answer = speakerPlay( request );
        return answer;
    }
    case SPEAKER_STOP: {
        auto answer = speakerStop( request );
        return answer;
    }
    case GLOBAL_LOCK: {
        auto& lockHelper = CommonUtils::LockHelper::global();
        if( m_hubStateManager->isAnyProcessRunning() ) {
            return lockHelper.createFailedResponse( "Some process is running" );
        }
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
        helper.backupDatabase( NetlinkUSBDevicesDBName );
        return helper.getResponse();
    }
    case HUB_REBOOT:
        reboot( false );
        break;
    default:
        return Services::Service::handleRequestFromBus( request, requestType );
    }
    return JSON_EMPTY_OBJ;
}

int Service::restoreBackup( const std::string& tempBackupFolder ) {
    CommonUtils::RestoreHelper helper( getDBClientSafe(), tempBackupFolder );
    helper.restoreDatabase( NetlinkUSBDevicesDBName );
    return helper.getReturnCode();
}

void Service::handleMessageFromBus( const Services::Message& message, const std::string& sender ) {
    switch( message.msgClass ) {
    case EVENT:
        handleEventFromBus( message );
        break;

    default:
        Services::Service::handleMessageFromBus( message, sender );
        break;
    }
}

void Service::handleEventFromBus( const Services::Message& message ) {
    auto toHubStateProcess = []( CoreD::Model::GatewayProcess process ) {
        switch( process ) {
        case CoreD::Model::GatewayProcess::AddingDevice:
            return HubState::Process::DeviceAdding;
        case CoreD::Model::GatewayProcess::RemovingDevice:
            return HubState::Process::DeviceRemoving;
        default:
            return HubState::Process::Unknown;
        }
    };

    auto toHubStateProcessResult = []( CoreD::Model::GatewayProcessResult result ) {
        switch( result ) {
        case CoreD::Model::GatewayProcessResult::Success:
            return HubState::ProcessResult::Success;
        case CoreD::Model::GatewayProcessResult::Fail:
            return HubState::ProcessResult::Fail;
        default:
            return HubState::ProcessResult::Unspecified;
        }
    };

    switch( message.msgSubClass ) {
    case HUB_NETWORK_CONNECTED:
        m_hubStateManager->onInternetAvailabilityChanged( true );
        Utils::syncNTPTime();
        break;

    case HUB_NETWORK_DISCONNECTED:
        m_hubStateManager->onInternetAvailabilityChanged( false );
        break;

    case ZWAVE_IDENTIFY_EVENT:
        m_hubStateManager->onZwaveIdentify();
        break;

    case GATEWAY_PROCESS_STARTED: {
        auto gatewayProcessString = json_get_str( message.data, "process" );
        auto gatewayProcess = CoreD::Model::gatewayProcessFromString( gatewayProcessString );
        auto process = toHubStateProcess( gatewayProcess );

        m_hubStateManager->onProcessRunning( process );
        break;
    }
    case GATEWAY_PROCESS_STOPPED: {
        auto gatewayProcessString = json_get_str( message.data, "process" );
        auto gatewayProcess = CoreD::Model::gatewayProcessFromString( gatewayProcessString );
        auto process = toHubStateProcess( gatewayProcess );

        auto gatewayResultString = json_get_str( message.data, "result" );
        auto gatewayResult = CoreD::Model::gatewayProcessResultFromString( gatewayResultString );
        auto result = toHubStateProcessResult( gatewayResult );

        m_hubStateManager->onProcessStopped( process, result );
        break;
    }
    case HUB_SETTING_UPDATED: {
        auto name = json_get_str( message.data, "name" );
        if( name == Resources::Settings::BatteryPeriodSetting ) {
            onForceBroadcastSettingsChanged();
        }
        break;
    }
    }
}

void Service::setUp() {
    Services::Service::setUp();

    join( GroupsNames::BROADCAST_GROUP );

    m_config = std::make_unique< HWManagerConfig >();
    buttonsTaskStart();

    m_networkManager = std::make_unique< Network::RemoteInterface >( *this );
    m_cored = std::make_unique< CoreDRemoteInterface::CoreDRemoteInterface >( *this );
    m_hubStateManager = std::make_unique< HubStateManager >( *m_networkManager, *m_cored, getDBClient(), *this );

    m_logsHandler = std::make_unique< LogsHandler >( *this, *this, m_config->getPowerLed() );

    m_batteryManager = BatteryManagerFactory::createBatteryManager( *this, *this, getDBClient() );
    if( m_batteryManager ) {
        if( !m_batteryManager->isHardwareBased() ) {
            LOGGER_INFO_RAW( "No hardware based gauge meter, use empirical one (based on ADC values)." );
        }

        m_batteryInfoBroadcaster = std::make_unique< BatteryInfoBroadcaster >(
            getDBClient(), [this]( const Json& broadcast ) { this->sendBroadcast( HUB_INFO_CHANGED, broadcast ); } );
        m_batteryManager->setUp( [this]( const BatteryInfo& info ) {
            if( m_batteryInfoBroadcaster ) {
                m_batteryInfoBroadcaster->makeBroadcastAndSend( info );
            }

            addTask( [this, info]() { sendBroadcast( HUB_BATTERY_INFO_UPDATED, info.toJson() ); } );

            if( m_hubStateManager ) {
                m_hubStateManager->setBatteryState( getBatteryState( info ) );
            }
            if( info.isDrained() ) {
                LOGGER_INFO_RAW( "Battery is drained! Rebooting hub" );
                reboot( true );
            }
        } );
        onForceBroadcastSettingsChanged();
    }

    m_netlinkManager = std::make_unique< NetlinkManager >( getDBClient(), *this );

    m_speaker = std::make_unique< SpeakerWrapper >( *this );
    if( !m_speaker->setUp() ) {
        auto configureTask = createPostponedTask( [this]( Services::PostponedTask& task ) {
            if( m_speaker->setUp() ) {
                task.stop();
            }
            else {
                LOGGER_WARN_RAW( cc::warn( "Cannot setup speaker functionality! Try next time in " ) +
                                 cc::num10( SpeakerConfigureTryInterval.count() ) + cc::warn( " minutes." ) );
            }
        } );

        configureTask->start( SpeakerConfigureTryInterval, SpeakerConfigureTryInterval );
    }

    if( Utils::HasTimerExitOnClose ) {
        LOGGER_INFO( cc::info( "Has timer exit on close flag" ) );

        m_dateTimeWatchdog = std::thread( [this]() {
            LOGGER_INFO_RAW( cc::warn( "Start check time change thread..." ) );
            checkTimeChange();
        } );
    }
    else {
        LOGGER_INFO( cc::info( "Has no timer exit on close flag, check time change not available" ) );
    }
}

void Service::tearDown() {
    if( m_batteryManager ) {
        m_batteryManager->tearDown();
    }
    m_batteryInfoBroadcaster.reset();

    if( m_speaker ) {
        m_speaker->tearDown();
        m_speaker.reset();
    }

    m_config->getWifiLed().turnOff();
    m_config->getReadyLed().turnOff();
    m_config->getErrorRed().turnOff();
    m_config->getMessageLed().turnOff();
    m_config->getExcludeLed().turnOff();
    m_config->getIncludeLed().turnOff();
    m_config->getInternetLed().turnOff();
    // m_config->getPowerLed().turnOff();
    m_config->getZwaveLed().turnOff();
    m_config->getServiceLed().turnOff();
    m_config->getZigbeeLed().turnOff();
    m_config->getBluetoothLed().turnOff();
    m_config->getLpfrLed().turnOff();
    m_config->get3gLed().turnOff();
    m_config->getVoiceLed().turnOff();

    m_taskPool.shutdown();

    if( m_dateTimeWatchdog.joinable() ) {
        m_dateTimeWatchdog.join();
    }

    Services::Service::tearDown();
}

void Service::onResetButtonAction( ResetButtonBase::PressingPicture pressingPicture ) {
    // @note: called from buttonsThread

    switch( pressingPicture ) {
    case ResetButtonBase::PressingPicture::FIND_HUB:
        LOGGER_INFO( "Pressing picture: find hub" );
        sendBroadcast( BUTTON_FIND_HUB, JSON_EMPTY_OBJ, GroupsNames::HARDWARE_BROADCASTS );
        break;
    case ResetButtonBase::PressingPicture::SAVE_LOGS:
        LOGGER_INFO( "Pressing picture: save logs" );
        m_logsHandler->saveLogs();
        break;
    case ResetButtonBase::PressingPicture::WIFI_RESET:
        LOGGER_INFO( "Pressing picture: wifi reset" );
        sendBroadcast( BUTTON_WIFI_RESET, JSON_EMPTY_OBJ, GroupsNames::HARDWARE_BROADCASTS );
        break;
    case ResetButtonBase::PressingPicture::POWEROFF:
        /* theoretically a 'poweroff' functionality can be provided for all hubs,
         * but now allow it only for h2_secure hubs */
#if defined( __HUB_TYPE_H2_SECURE__ )
        LOGGER_INFO( "Pressing picture: power off" );
        sendBroadcast( BUTTON_POWEROFF, JSON_EMPTY_OBJ, GroupsNames::HARDWARE_BROADCASTS );
        powerOff();
        break;
#else
        break;
#endif
    case ResetButtonBase::PressingPicture::REBOOT_READY:
        LOGGER_TRACE( "Pressing picture: reboot ready" );
        m_hubStateManager->onProcessReadyToStart( HubState::Process::Reboot );
        break;
    case ResetButtonBase::PressingPicture::REBOOT:
        LOGGER_INFO( "Pressing picture: reboot" );
        m_hubStateManager->onProcessStart();
        sendBroadcast( BUTTON_REBOOT, JSON_EMPTY_OBJ, GroupsNames::HARDWARE_BROADCASTS );
        reboot( true );
        break;
    case ResetButtonBase::PressingPicture::SOFT_RESET_READY:
        LOGGER_TRACE( "Pressing picture: soft reset ready" );
        m_hubStateManager->onProcessReadyToStart( HubState::Process::SoftReset );
        break;
    case ResetButtonBase::PressingPicture::SOFT_RESET:
        LOGGER_INFO( "Pressing picture: soft reset" );
        m_hubStateManager->onProcessStart();
        sendBroadcast( BUTTON_SOFT_RESET, JSON_EMPTY_OBJ, GroupsNames::HARDWARE_BROADCASTS );
        break;
    case ResetButtonBase::PressingPicture::FACTORY_RESET_READY:
        LOGGER_TRACE( "Pressing picture: factory reset ready" );
        m_hubStateManager->onProcessReadyToStart( HubState::Process::FactoryReset );
        break;
    case ResetButtonBase::PressingPicture::FACTORY_RESET:
        LOGGER_INFO( "Pressing picture: factory reset" );
        m_hubStateManager->onProcessStart();
        sendBroadcast( BUTTON_FACTORY_RESET, JSON_EMPTY_OBJ, GroupsNames::HARDWARE_BROADCASTS );
        break;
    case ResetButtonBase::PressingPicture::CANCEL:
        m_hubStateManager->onProcessReadyToStart();
        break;
    default:
        break;
    }
}

void Service::reboot( bool wait ) {
#if defined( __HUB_TYPE_LOCAL__ ) || defined( __HUB_TYPE_VHUB__ )
    Log::warn( cc::warn( "reboot: local configuration, reboot skipped" ) );
    return;
#else
    m_config->getEmotionLed().turnOff();

    LOGGER_INFO_RAW( "Sending logs before reboot..." );

    Network::RemoteInterface networkRemoteInterface( *this );
    bool isInternetAvailable = networkRemoteInterface.isInternetAvailable();

    std::string logCmd = HubPlatform::Environment::getBinariesFolder() + "/logcmd";
    logCmd += " --add-to-name=reboot-button";

    if( !isInternetAvailable || std::system( ( logCmd + " --idle" ).c_str() ) ) {
        LOGGER_WARN_RAW( "Unable to send logs" );
        LOGGER_INFO_RAW( "Storing logs..." );
        std::system( ( logCmd + " --store-to-postponed" ).c_str() );
    }

    LOGGER_INFO_RAW( "Rebooting..." );

    if( wait ) {
        for( size_t i = 0; i < RebootAttemptCount; ++i ) {
            if( 0 == system( "reboot -f" ) ) {
                std::this_thread::sleep_for( RebootTimeout );
                return;
            }
            std::this_thread::sleep_for( RebootAttemptTimeout );
        }
    }
    else {
        auto rebootCounter = std::make_shared< size_t >( 0 );
        auto task = createPostponedTask( [rebootCounter]( Services::PostponedTask& task ) {
            if( RebootAttemptCount == ( *rebootCounter )++ || 0 == system( "reboot -f" ) ) {
                task.stop();
            }
        } );
        task->start( RebootAttemptTimeout, true );
    }
#endif
}

void Service::powerOff() {
    // TODO: postpone/reject powering off till uninterruptible ha-firmware tasks get finished

    LOGGER_INFO_RAW( "Powering off..." );
    startEffect( EffectType::HubPowerOff, static_cast< int >( Priority::HIGH ) );
    sendBroadcast( HUB_POWEROFF, JSON_EMPTY_OBJ, GroupsNames::HARDWARE_BROADCASTS );

    system( "/opt/firmware/bin/poweroff" );
}

#if defined( __HUB_TYPE_G150__ ) || defined( __HUB_TYPE_G450__ ) || defined( __HUB_TYPE_G550__ )
void Service::buttonsTaskStart() {
    m_resetButton =
        std::make_shared< ResetButton >( std::bind( &Service::onResetButtonAction, this, std::placeholders::_1 ) );
    m_buttonsThread = std::thread( [this]() {
        try {
            VeraButtonsHandler::instance().listen();
            while( m_isButtonsThreadRunning ) {
                VeraButtonsHandler::instance().processButtons();
                m_resetButton->processButton();
                std::this_thread::sleep_for( ButtonCheckInterval );
            }
        }
        catch( const std::exception& ex ) {
            Log::error( ex.what() );
        }
    } );
}

#elif defined( __HUB_TYPE_LOCAL__ ) || defined( __HUB_TYPE_VHUB__ )
void Service::buttonsTaskStart() {}
#else
void Service::buttonsTaskStart() {
    m_resetButton = std::make_shared< ResetButton >(
        std::bind( &Service::onResetButtonAction, this, std::placeholders::_1 ), m_config->getResetButtonNumber() );

    m_buttonsThread = std::thread( [this]() {
        while( m_isButtonsThreadRunning ) {
            m_resetButton->processButton();
            std::this_thread::sleep_for( ButtonCheckInterval );
        }
    } );
}
#endif

void Service::onForceBroadcastSettingsChanged() {
    auto periodSetting = m_cored->findSetting( Resources::Settings::BatteryPeriodSetting );
    auto forceBroadcastPeriod = std::chrono::minutes( periodSetting ? periodSetting->getIntValue() : 0 );
    if( forceBroadcastPeriod.count() ) {
        LOGGER_INFO_RAW( cc::info( "Enabling periodic battery info broadcasts each " ) +
                         cc::num10( forceBroadcastPeriod.count() ) + cc::info( " minute(s)" ) );
        m_forceBroadcastTimer.start( std::chrono::seconds( 0 ), forceBroadcastPeriod );
    }
    else {
        LOGGER_INFO_RAW( cc::info( "Disabling periodic battery info broadcasts" ) );
        m_forceBroadcastTimer.stop();
    }
}

void Service::forceBroadcast() {
    Json broadcast;
    broadcast[ "battery" ] = m_batteryManager->getInfo().toJson();
    json_remove( broadcast[ "battery" ], db::ID_FIELD_NAME );
    sendBroadcast( HUB_INFO_CHANGED, broadcast );
}

void Service::handleHotPlugEvent( const HotPlugEvent& event ) {
    /* give kernel/system time to setup things */
    createPostponedTask( [this, event]( Services::PostponedTask& ) {
        const auto eventJs = event.toJson();
        auto devicePath = json_get_str( eventJs, "devicePath" );
        auto type = json_get_str( eventJs, "type" );

        LOGGER_INFO_RAW( cc::info( "Hotplug event, device: " ) + cc::note( devicePath ) + cc::info( ", type: " ) +
                         cc::note( type ) );
        sendBroadcast( HOTPLUG_EVENT, eventJs, GroupsNames::HARDWARE_BROADCASTS );
    } )
        ->start( std::chrono::seconds( 0 ), false );
}

void Service::handleI2CBusRecovered( const std::string& devicePath ) {
    if( devicePath.find( Resources::LedI2CBus ) != std::string::npos ) {
        LOGGER_INFO_RAW( cc::info( "LED i2c bus was recovered" ) );
        m_hubStateManager->onHubStateEffectStarted();
    }
    else if( devicePath.find( Resources::BatteryI2CBus ) != std::string::npos ) {
        LOGGER_INFO_RAW( cc::info( "Battery i2c bus was recovered" ) );
    }
    else {
        LOGGER_WARN_RAW( cc::warn( "Some unknown i2c bus was recovered" ) );
    }
}

JsonObject Service::addEffect( const JsonObject& request ) {
    std::string command;
    std::string effectName;
    JsonObject answer = JSON_EMPTY_OBJ;
    if( !getField( request, "command", answer, command ) ) {
        return answer;
    }
    if( !getField( request, "effect", answer, effectName ) ) {
        return answer;
    }
    int priority = static_cast< int >( Priority::IDLE );
    getOptionalField( request, "priority", priority );
    LOGGER_INFO( "command: " + command + ", effect: " + effectName + ", priority: " + std::to_string( priority ) );
    if( command == "start" ) {
        if( effectName == EffectType::CustomBlink ) {
            Rgb color;
            int count, delay, gradual;
            if( !getField( request, "red", answer, color.red ) ) {
                return answer;
            }
            if( !getField( request, "green", answer, color.green ) ) {
                return answer;
            }
            if( !getField( request, "blue", answer, color.blue ) ) {
                return answer;
            }
            if( !getField( request, "count", answer, count ) ) {
                return answer;
            }
            if( !getField( request, "delay", answer, delay ) ) {
                return answer;
            }
            if( !getField( request, "gradual", answer, gradual ) ) {
                return answer;
            }
            std::shared_ptr< BlinkEffect > effect = std::make_shared< BlinkEffect >( m_config->getEmotionLed() );
            effect->setBlinkParameters( color, count, delay, gradual );
            m_effectsInQueue.insert( std::pair< std::string, std::shared_ptr< Effect > >( effectName, effect ) );
            m_taskPool.safeSubmit( EffectHandler( effect, priority ) );
        }
        else {
            startEffect( effectName, priority );
        }
    }
    else if( command == "stop" ) {
        stopEffect( effectName );
    }

    return JsonObject();
}

JsonObject Service::setIndicator( const JsonObject& request ) {
    const std::string indicatorField = "indicator";
    std::string indicator;
    JsonObject answer = JSON_EMPTY_OBJ;
    if( !getField( request, indicatorField, answer, indicator ) ) {
        return answer;
    }

    Led* selectedLed = getIndicator( indicator );

    if( !selectedLed ) {
        JsonObject errorMes;
        Errors::getBadRequest( indicatorField, errorMes );
        setErrorObject( answer, errorMes );
        return answer;
    }

    const std::string stateField = "state";
    std::string state;
    if( !getField( request, stateField, answer, state ) ) {
        return answer;
    }
    LOGGER_TRACE( cc::trace( "indicator: " ) + cc::note( indicator ) + cc::trace( ", state: " ) + cc::note( state ) );
    if( state == "off" ) {
        selectedLed->turnOff();
    }
    else if( state == "on" ) {
        selectedLed->turnOn();
    }
    else if( state == "blink" ) {
        int msOff;
        if( !getField( request, "msOff", answer, msOff ) ) {
            return answer;
        }
        int msOn;
        if( !getField( request, "msOn", answer, msOn ) ) {
            return answer;
        }
        selectedLed->turnBlinking( msOff, msOn );
    }
    else {
        JsonObject errorMes;
        Errors::getBadRequest( stateField, errorMes );
        setErrorObject( answer, errorMes );
        return answer;
    }

    return JsonObject();
}

JsonObject Service::speakerPlay( const JsonObject& request ) {
    Json answer = JSON_EMPTY_OBJ;
    if( m_speaker )
        answer = m_speaker->play( request );
    return answer;
}

JsonObject Service::speakerStop( const JsonObject& request ) {
    Json answer = JSON_EMPTY_OBJ;
    if( m_speaker )
        answer = m_speaker->stop( request );
    return answer;
}

void Service::sendExecutePluginScript( const std::string& script,
                                       const Json& scriptParams,
                                       const std::string& operationId,
                                       bool isUserRunScript,
                                       int timeout ) {
    // TODO: Run all plugin scripts only through extensions manager
    auto data = JSON_EMPTY_OBJ;
    data[ "script" ] = script;
    data[ "operationId" ] = operationId;
    data[ "isUserRunScript" ] = isUserRunScript;
    if( !scriptParams.empty() ) {
        data[ "scriptParams" ] = scriptParams;
    }

    sendAsyncRequest( "addon.lua", data, EXECUTE_SCRIPT, []( const Json& ) {} );
}

/** Checks date/time change on a device. Run in a separate thread only. */
void Service::checkTimeChange() {
    while( this->isRunning() ) {
        int result = Utils::checkTimeChange();

        if( result == -1 ) {
            LOGGER_INFO( cc::info( "Check time change: not available in this environment" ) );
            break;
        }
        if( result == 1 ) {
            time_t now = time( nullptr );
            LOGGER_WARN( cc::warn( "Date/Time has changed, new system timestamp is present: " ) +
                         cc::note( TimeUtils::timestampToIsoDate( now ) ) );
            time_t systemUptime = Utils::getSystemUptime();
            std::string readableUptime = TimeUtils::formatUptime( systemUptime );

            LOGGER_WARN( cc::warn( "System uptime: " ) + cc::note( readableUptime ) );

            long uptimeIntervalSeconds =
                std::chrono::duration_cast< std::chrono::seconds >( HubUptimePrintInterval ).count();
            JsonObject uptime =
                JSON( "uptime" << readableUptime << "current" << systemUptime << "interval" << uptimeIntervalSeconds );
            if( m_prevSystemUptime > 0 )
                uptime[ "previous" ] = m_prevSystemUptime;
            Json broadcast = JSON( "systemTime" << now << "uptime" << uptime );
            setSystemUptime( systemUptime );
            this->sendBroadcast( HUB_SYSTEM_DATETIME_CHANGED, broadcast, GroupsNames::BROADCAST_GROUP );
        }
    }
}

void Service::setSystemUptime( time_t systemUptime ) {
    m_prevSystemUptime = systemUptime;
}

}  // namespace HardwareManager
}  // namespace HubPlatform
