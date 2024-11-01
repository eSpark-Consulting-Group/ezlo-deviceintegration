#pragma once

#include "service_interfaces/service.h"
#include "cored/house_mode_proc.h"
#include "service_interfaces/script_event.h"
#include "json_utils/json_utils.h"

#include <memory>
#include <string>

namespace HubPlatform {
namespace CoreD {

class HouseModeAlarms {
    using SetItemValue = std::function< void( const JsonObject& ) >;
    using Emit = std::function< void( const Services::ScriptEvent& ) >;
    using PostponedTask = std::function< Services::PostponedTaskPtr( Services::PostponedTaskFunction ) >;
    using Timestamp = unsigned long;

public:
    HouseModeAlarms( Services::Service& service, SetItemValue setItemValue );
    void houseModeSwitchFromArmedToDisarmedMode( const JsonObject& message );
    void checkSecurityThreat( const std::string& itemId );
    void cancelEntryDelay();
    JsonObject skipEntryDelay();
    JsonObject setEntryDelay( const JsonObject& params );
    JsonObject resetEntryDelay();
    JsonObject turnOffLocalAlarm( const JsonObject& params );
    JsonObject forceDisarm( const JsonObject& params );

    void setTestMode( bool testMode );

private:
    using TaskItem = std::pair< Services::PostponedTaskPtr, Timestamp >;
    using PostponedTasks = std::map< std::string, std::vector< TaskItem > >;
    using DeviceID = std::string;

    bool getDeviceArmedStatusByItem( const JsonObject& item );
    void addDeviceToHouseModesAlarmedList( const JsonObject& json, const std::string& deviceId, int delay );
    void deleteDeviceFromHouseModesAlarmedList( const std::string& id );
    void clearPostponedTasks( const std::string& exceptedId = "" );

    void sendEntryDelayBeginBroadcast( const std::string& deviceId, int delay );
    void sendEntryDelayDoneBroadcast( const std::string& deviceId, int delay );
    void sendEntryDelayCanceledBroadcast();
    void sendEntryDelaySkipBroadcast();
    void sendMainBeginBroadcast( const std::string& deviceId, int delay );
    void sendMainDoneBroadcast( const std::string& deviceId, int delay );
    void sendBypassDoneBroadcast( const std::string& deviceId, int delay );

    void turnOffAlarms( const std::vector< std::string >& alarmItemNames );
    void notifyAllCameras( const std::vector< std::string >& cameraItemNames,
                           CoreDHouseModes::HouseModeEntry& currentHouseMode );
    void notifyAllAlarms( const std::vector< std::string >& alarmItemNames,
                          CoreDHouseModes::HouseModeEntry& currentHouseMode );

    void runSecurityThreatNotificationsWithBroadcast( const JsonObject& json,
                                                      const DeviceID& deviceId,
                                                      std::function< void() > emitBroadcast );
    void clearAllPendingSecurityEventsAndCountdown();
    void turningOffLocalAlarms( std::string& modeId );
    bool isProtectAlarmEvent( const std::string& itemName, const std::string& itemValue );
    bool isSecurityThreatEvent( bool deviceArmed, const std::string& itemName, bool itemValue );

private:
    std::unique_ptr< ConfigReader > m_configReader;

    std::mutex m_postponedMutex;
    PostponedTasks m_postponedTasks;

    Services::Service& m_service;
    SetItemValue m_setItemValue;
    std::string m_curentPhaseDeviceId;
    std::mutex m_mutex;
    bool m_silentAlarm{ false };
    bool m_testMode{ false };
    bool m_chime{ false };
    std::string m_soundType;
};

}  // namespace CoreD
}  // namespace HubPlatform
