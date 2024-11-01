#pragma once

#include "zwave/script_events/value_event.h"

#include <znet/anti_theft_info.hpp>

namespace HubPlatform {
namespace Zwave {

class AntiTheftStateUpdated : public ValueEvent {
public:
    AntiTheftStateUpdated( uint8_t nodeId, uint8_t channelId, uint8_t classId, const znet::AntiTheftInfoOpt& infoOpt );

    Json toJson() const final;

private:
    znet::AntiTheftInfoOpt m_infoOpt;
};

}  // namespace Zwave
}  // namespace HubPlatform
