#pragma once

#include "zwave/script_events/value_event.h"

#include <znet/znet.h>

namespace HubPlatform {
namespace Zwave {

class ButtonStateUpdated : public ValueEvent {
public:
    ButtonStateUpdated( uint8_t nodeId, uint8_t channelId, uint8_t buttonNum, const std::string& value );

    Json toJson() const final;

private:
    uint8_t m_buttonNum;
    std::string m_value;
};

}  // namespace Zwave
}  // namespace HubPlatform
