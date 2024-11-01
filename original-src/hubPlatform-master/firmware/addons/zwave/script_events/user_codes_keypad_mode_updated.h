#pragma once

#include "zwave/script_events/value_event.h"

namespace HubPlatform {
namespace Zwave {

class UserCodesKeypadModeUpdated : public ValueEvent {
public:
    UserCodesKeypadModeUpdated( uint8_t nodeId, uint8_t channelId, std::string mode );

    Json toJson() const override;

private:
    std::string m_mode;
};

}  // namespace Zwave
}  // namespace HubPlatform
