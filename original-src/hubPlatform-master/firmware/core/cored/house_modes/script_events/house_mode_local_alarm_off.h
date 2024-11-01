#pragma once

#include <chrono>
#include "cored/house_modes/script_events/base_event.h"

namespace HubPlatform {
namespace CoreDHouseModes {
namespace ScriptEvents {

class HouseModeLocalAlarmOff : public BaseEvent {
public:
    HouseModeLocalAlarmOff( std::string& modeId, std::chrono::seconds timestamp );

    Json toJson() const override;

private:
    std::string m_modeId;
    std::chrono::seconds m_timestamp;
};

}  // namespace ScriptEvents
}  // namespace CoreDHouseModes
}  // namespace HubPlatform
