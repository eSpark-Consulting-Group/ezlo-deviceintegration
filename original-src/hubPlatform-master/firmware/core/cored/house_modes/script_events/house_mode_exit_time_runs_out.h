#pragma once

#include "cored/house_modes/script_events/base_event.h"
#include <chrono>
namespace HubPlatform {
namespace CoreDHouseModes {
namespace ScriptEvents {

class HouseModeExitTimeRunsOut : public BaseEvent {
public:
    HouseModeExitTimeRunsOut( bool silent, std::chrono::seconds timestamp );

    Json toJson() const override;

private:
    bool m_silent{ false };
    std::chrono::seconds m_timestamp;
};

}  // namespace ScriptEvents
}  // namespace CoreDHouseModes
}  // namespace HubPlatform
