#pragma once

#include "hardware_manager/hm_messenger.h"
#include "hardware_manager/config/hw_manager_config.h"
#include "hardware_manager/effect/effect_handler.h"
#include "hardware_manager/button/reset_button.h"
#include "hardware_manager/battery/battery_manager.h"
#include "hardware_manager/battery/battery_info_broadcaster.h"
#include "hardware_manager/netlink_manager/netlink_manager.h"
#include "hardware_manager/speaker_wrapper.h"
#include "hardware_manager/hub_state/hub_state_manager.h"
#include "hardware_manager/hub_state/hub_state_indication_manager.h"
#include "hardware_manager/logs_handler/logs_handler.h"

#include "service_interfaces/service.h"
#include "service_interfaces/task_loop_timer.h"
#include "base_utils/prioritized_task_pool.h"

#include <map>

namespace HubPlatform {
namespace HardwareManager {

class Service final : public Services::Service,
                      public Messenger,
                      public INetlinkEventsHandler,
                      public HubStateIndicationManager {
public:
    Service();
    ~Service() override;

public:
    std::shared_ptr< Effect > getEffect( const std::string& effectName );
    Led* getIndicator( const std::string& indicatorName );
    void sendExecutePluginScript( const std::string& script,
                                  const Json& scriptParams,
                                  const std::string& operationId = "",
                                  bool isUserRunScript = false,
                                  int timeout = 60 ) override;

    int restoreBackup( const std::string& tempBackupFolder ) override;

private:
    void handleMethodCall( const JsonObject& request,
                           MessageSubClasses requestType,
                           const std::string& sender ) override;
    JsonObject handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) override;
    void handleMessageFromBus( const Services::Message& message, const std::string& sender ) override;
    void handleEventFromBus( const Services::Message& message );
    void setUp() override;
    void tearDown() override;

private:
    void setEventEffect( const std::string& effectName ) override;
    void setHubStateEffect( const std::string& effectName ) override;
    void setHubStateIndicator( const std::string& indicator, bool state ) override;
    void setHubStateIndicatorBlinking( const std::string& indicator, int msOff, int msOn ) override;
    void indicateDown() override;
    void setHubStateCustomEffectSettings( const Color color, const BlinkingType blinking ) override;

private:
    void startEffect( const std::string& effectName, int priority );
    void stopEffect( const std::string& effectName );
    void buttonsTaskStart();

    void onForceBroadcastSettingsChanged();
    void forceBroadcast();

    void onResetButtonAction( ResetButtonBase::PressingPicture pic );
    void reboot( bool wait );
    void powerOff();

    /* these functions get called on behalf of a Netlink's manager thread */
    void handleHotPlugEvent( const HotPlugEvent& event ) override;
    void handleI2CBusRecovered( const std::string& devicePath ) override;

    JsonObject addEffect( const JsonObject& request );
    JsonObject setIndicator( const JsonObject& request );

    JsonObject speakerPlay( const JsonObject& request );
    JsonObject speakerStop( const JsonObject& request );

    void checkTimeChange();
    void setSystemUptime( time_t value );

private:
    std::unique_ptr< HWManagerConfig > m_config;
    common::PrioritizedTaskPool< EffectHandler > m_taskPool;
    std::multimap< std::string, std::shared_ptr< Effect > > m_effectsInQueue;
    Services::TaskLoopTimer m_forceBroadcastTimer;
    std::thread m_buttonsThread;
    std::atomic_bool m_isButtonsThreadRunning;
    std::shared_ptr< ResetButton > m_resetButton;
    std::unique_ptr< BatteryManager > m_batteryManager;
    std::unique_ptr< BatteryInfoBroadcaster > m_batteryInfoBroadcaster;
    std::unique_ptr< NetlinkManager > m_netlinkManager;
    std::unique_ptr< SpeakerWrapper > m_speaker;
    std::unique_ptr< Network::RemoteInterface > m_networkManager;
    std::unique_ptr< CoreDRemoteInterface::CoreDRemoteInterface > m_cored;
    std::unique_ptr< HubStateManager > m_hubStateManager;
    std::unique_ptr< LogsHandler > m_logsHandler;

    std::thread m_dateTimeWatchdog;
    time_t m_prevSystemUptime{ 0 };

    Color m_color = Color::None;
    BlinkingType m_blinking = BlinkingType::None;
};

}  // namespace HardwareManager
}  // namespace HubPlatform
