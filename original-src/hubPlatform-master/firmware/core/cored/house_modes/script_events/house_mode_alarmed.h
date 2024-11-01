#pragma once

#include <chrono>
#include "cored/house_modes/script_events/base_event.h"

namespace HubPlatform {
namespace CoreDHouseModes {
namespace ScriptEvents {

class HouseModeAlarmed : public BaseEvent {
public:
    HouseModeAlarmed( std::string modeId,
                      std::string deviceId,
                      std::string phase,
                      std::string status,
                      int delay,
                      bool silent,
                      bool chime,
                      std::string soundType );

    Json toJson() const override;

private:
    std::string m_modeId;
    std::string m_deviceId;
    std::string m_phase;
    std::string m_status;
    int m_delay;
    bool m_silent;
    bool m_chime;
    std::string m_soundType;
};

}  // namespace ScriptEvents
}  // namespace CoreDHouseModes
}  // namespace HubPlatform
