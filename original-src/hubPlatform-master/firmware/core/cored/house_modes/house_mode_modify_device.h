#pragma once

#include "cored/house_modes/house_mode_entry.h"
#include "cored/db_modifier/idb_modifier.h"

namespace HubPlatform {

namespace Services {
class Service;
}

namespace CoreDHouseModes {

struct DeviceEntry {
    std::string m_modeId;
    bool m_disarmed{ false };
    bool m_alarmOff{ false };
    bool m_cameraOff{ false };
    bool m_disarmedDefault{ true };
    HouseModeEntry::DeviceIds m_disarmedDeviceIds;
    HouseModeEntry::DeviceIds m_alarmsOffDeviceIds;
    HouseModeEntry::DeviceIds m_camerasOffDeviceIds;

public:
    DeviceEntry() = default;

    explicit DeviceEntry( std::string modeId )
    : m_modeId( std::move( modeId ) ) {}

    Json toJson() const {
        return { { "modeId", m_modeId },
                 { "disarmed", m_disarmed },
                 { "alarmOff", m_alarmOff },
                 { "cameraOff", m_cameraOff },
                 { "disarmedDefault", m_disarmedDefault } };
    }
};

class DeviceHouseModeSettingsModifier final {
public:
    DeviceHouseModeSettingsModifier( Services::Service& hostService,
                                     CoreD::IDbModifier& dbModifier,
                                     std::string deviceId,
                                     JsonObject device )
    : m_hostService( hostService )
    , m_dbModifier( dbModifier )
    , m_deviceId( std::move( deviceId ) )
    , m_device( std::move( device ) ) {}

    bool modifyDevice();

private:
    IDbClient& getDBClient();
    std::map< std::string, DeviceEntry > buildDeviceHouseModesOptions();
    std::map< std::string, DeviceEntry > buildHouseModesEntries();

    bool addAlarmOffDevice( const std::string& modeId );
    bool removeAlarmOffDevice( const std::string& modeId );
    bool addDisarmedDevice( const std::string& modeId, bool changeArmedState );
    bool removeDisarmedDevice( const std::string& modeId, bool changeArmedState );
    bool addCameraOffDevice( const std::string& modeId );
    bool removeCameraOffDevice( const std::string& modeId );

private:
    Services::Service& m_hostService;
    CoreD::IDbModifier& m_dbModifier;
    std::string m_deviceId;
    JsonObject m_device;
    bool m_isModified = false;
};

}  // namespace CoreDHouseModes
}  // namespace HubPlatform