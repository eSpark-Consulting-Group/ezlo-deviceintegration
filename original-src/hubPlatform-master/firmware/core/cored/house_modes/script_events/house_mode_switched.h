#pragma once

#include "cored/house_modes/script_events/base_event.h"

namespace HubPlatform {
namespace CoreDHouseModes {
namespace ScriptEvents {

class HouseModeSwitched : public BaseEvent {
public:
    HouseModeSwitched( std::string from, std::string to, std::string status, int delay );

    Json toJson() const override;

private:
    std::string m_from;
    std::string m_to;
    std::string m_status;
    int m_delay;
};

}  // namespace ScriptEvents
}  // namespace CoreDHouseModes
}  // namespace HubPlatform
