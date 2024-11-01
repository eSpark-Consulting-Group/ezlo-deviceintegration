#pragma once

#include <thread>
#include <atomic>

#include "cored/house_modes/house_mode_entry.h"
#include "cored/house_modes/protect_service_enum.h"
#include "json_utils/json_fwd.h"
#include "config_reader/config_reader.h"

class IDbClient;

namespace HubPlatform {

namespace UT {
class HouseModesProcessorFixture;
}

namespace CoreD {

class CoreService;

class HouseModeProcessor {
    friend UT::HouseModesProcessorFixture;

public:
    HouseModeProcessor( CoreService& service, IDbClient& dbClient );
    ~HouseModeProcessor();

    JsonObject handleSetHouseMode( const std::string& nextHouseModeId );
    void cancelSwitchingHouseMode();

    JsonObject handleAddDisarmedDevice( const std::string& houseModeId, const std::string& deviceId );
    JsonObject handleRemoveDisarmedDevice( const std::string& houseModeId,
                                           const std::string& deviceId,
                                           bool changeArmedState );

    JsonObject handleSetNotifications( const std::string& houseModeId,
                                       bool notifyAll,
                                       const std::vector< std::string >& userIds );

    JsonObject handleAddAlarmsOffDevice( const std::string& modeId, const std::string& deviceId );
    JsonObject handleRemoveAlarmsOffDevice( const std::string& modeId, const std::string& deviceId );

    JsonObject handleAddCamerasOffDevice( const std::string& modeId, const std::string& deviceId );
    JsonObject handleRemoveCamerasOffDevice( const std::string& modeId, const std::string& deviceId );

    JsonObject handleAddBypassDevices( const std::string& modeId, const std::vector< std::string >& deviceIds );
    JsonObject handleRemoveBypassDevices( const std::string& modeId, const std::vector< std::string >& deviceIds );

    JsonObject handleSetDisarmedDefault( const std::string& houseModeId, bool disarmedDefault );
    JsonObject handleSetProtect( const std::string& houseModeId, bool protect );
    JsonObject handleSetProtectWithTimestamp( const std::string& houseModeId,
                                              bool protect,
                                              std::chrono::milliseconds timestamp );
    JsonObject handleProtectButtonsSet( const std::string& deviceId, CoreDHouseModes::ProtectServiceEnum service );
    JsonObject handleAddProtectDevices( const std::vector< std::string >& deviceIds );
    JsonObject handleRemoveProtectDevices( const std::vector< std::string >& deviceIds );

    JsonObject handleAddDevice( const std::string& deviceId, const JsonObject& device );
    JsonObject handleModifyDevice( const std::string& deviceId, const JsonObject& device );

    using AddDeviceToHouseModesCallback =
        std::function< Json( const std::string& deviceId, const JsonObject& device ) >;
    JsonObject handleAddItem( const std::string& deviceId,
                              const std::string& itemId,
                              const JsonObject& item,
                              AddDeviceToHouseModesCallback addToHouseModes );
    JsonObject handleModifyItem( const std::string& deviceId,
                                 const std::string& itemId,
                                 const JsonObject& item,
                                 const JsonObject& itemPrevValue );
    JsonObject handleRemoveItem( const JsonObject& item );

    static bool isSecurityThreatItem( const JsonObject& item );

    std::string houseModeIdToArmedWhenField( const std::string& houseModeId );

private:
    bool setHouseMode( const std::string& houseModeId );
    HouseModeProcessor( CoreService& service, IDbClient& dbClient, ConfigReader&& configReader );

    void switchThreadFunction( const std::string& houseModeId,
                               const CoreDHouseModes::HouseModeEntry& nextHouseModeId,
                               int delay );

    Json fetchDeviceById( const std::string& deviceId ) const;
    bool hasDeviceById( const std::string& deviceId ) const;
    bool updateDisarmedDevice( const std::string& houseModeId );

    std::vector< std::string > filterExistingDevicesIds( const std::vector< std::string >& deviceIds,
                                                         Json& answer ) const;

    void wait();

    static bool isCameraMakeRecordingItem( const JsonObject& item );
    using CamerasOffMap = std::map< std::string, bool >;
    CamerasOffMap fetchCamerasOffOptions( const std::string& deviceId );

    void removeSecurityDevice( const std::string& deviceId );

    bool isBypassAllowedForDevice( const std::string& id ) const;

private:
    IDbClient& m_dbClient;
    CoreService& m_hostService;
    ConfigReader m_configReader;

    std::thread m_switchingThread;
    std::atomic_bool m_isSwitchingNow;
    bool m_silentAlarm{ false };
};

}  // namespace CoreD
}  // namespace HubPlatform
