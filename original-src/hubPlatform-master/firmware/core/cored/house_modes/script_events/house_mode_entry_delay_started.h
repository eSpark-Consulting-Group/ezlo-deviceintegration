#pragma once

#include "cored/house_modes/script_events/base_event.h"
#include <chrono>
namespace HubPlatform {
namespace CoreDHouseModes {
namespace ScriptEvents {

class HouseModeEntryDelayStarted : public BaseEvent {
public:
    HouseModeEntryDelayStarted( bool silent, bool chime, std::string soundType, std::chrono::seconds timestamp );

    Json toJson() const override;

private:
    bool m_silent{ false };
    bool m_chime{ false };
    std::string m_soundType;
    std::chrono::seconds m_timestamp;
};

}  // namespace ScriptEvents
}  // namespace CoreDHouseModes
}  // namespace HubPlatform
