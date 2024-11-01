#pragma once

#include "json_utils/json_wrapper.h"
#include "cored/house_modes/house_mode_entry.h"
#include "cored/house_modes/protect_button.h"
#include "service_interfaces/service.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include <set>
#include <vector>

namespace HubPlatform {
namespace Uid {
namespace ResponseBuilder {
namespace HubModesGet {

class IResponseBuilder {
public:
    using Notifications = CoreDHouseModes::HouseModeEntry::Notifications;
    using DisarmedDevices = std::set< std::string >;
    using AlarmsOffDevices = std::set< std::string >;
    using CamerasOffDevices = std::set< std::string >;
    using BypassDevices = std::set< std::string >;

    using AllSecurityDevices = std::vector< std::string >;
    using AllAlarmDevices = std::vector< std::string >;
    using AllCameraDevices = std::vector< std::string >;
    using AllProtectButtons = std::vector< CoreDHouseModes::ProtectButton >;
    using AllProtectDevices = std::vector< std::string >;

    virtual ~IResponseBuilder() = default;

    virtual void setSettings( const std::string& currentHouseModeId,
                              const std::string& nextHouseMode,
                              int timeToLeftForSwitching,
                              int modesSwitchDelay,
                              int alarmOnDelay,
                              Json entryDelay,
                              Json abortWindow,
                              AllSecurityDevices securityDevices,
                              AllAlarmDevices alarmDevices,
                              AllCameraDevices cameraDevices,
                              AllProtectButtons protectButtons,
                              AllProtectDevices protectDevices ) = 0;

    virtual void addHouseMode( const std::string& id,
                               const std::string& name,
                               const std::string& description,
                               bool ignoreSwitchDelay,
                               bool disarmedDefault,
                               bool protect,
                               bool armed,
                               const Notifications& notifications,
                               const DisarmedDevices& disarmedDevices,
                               const AlarmsOffDevices& alarmsOffDevices,
                               const CamerasOffDevices& camerasOffDevices,
                               const BypassDevices& bypassDevices ) = 0;

    virtual JsonObject buildResponse( IDbClient& dbClient ) const = 0;
};

}  // namespace HubModesGet
}  // namespace ResponseBuilder
}  // namespace Uid
}  // namespace HubPlatform
