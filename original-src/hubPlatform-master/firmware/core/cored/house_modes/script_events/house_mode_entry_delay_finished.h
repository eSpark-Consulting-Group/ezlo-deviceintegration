#pragma once

#include "cored/house_modes/script_events/base_event.h"
#include "cored/house_modes/resources/house_mode_resources.h"
#include <chrono>
namespace HubPlatform {
namespace CoreDHouseModes {
namespace ScriptEvents {

class HouseModeEntryDelayFinished : public BaseEvent {
public:
    HouseModeEntryDelayFinished( bool silent,
                                 bool chime,
                                 std::string soundType,
                                 std::chrono::seconds timestamp,
                                 Resources::EntryDelayStatus status );

    Json toJson() const override;
    std::string enumToString() const;

private:
    bool m_silent{ false };
    bool m_chime{ false };
    std::string m_soundType;
    std::chrono::seconds m_timestamp;
    Resources::EntryDelayStatus m_status;
};

}  // namespace ScriptEvents
}  // namespace CoreDHouseModes
}  // namespace HubPlatform