#pragma once

#include "zwave/script_events/value_event.h"

#include <znet/znet.h>

namespace HubPlatform {
namespace Zwave {

class SoundSwitchConfigurationUpdated : public ValueEvent {
public:
    SoundSwitchConfigurationUpdated( uint8_t nodeId, uint8_t channelId, uint8_t defaultToneId, uint8_t volume );

    Json toJson() const final;

private:
    uint8_t m_defaultToneId;
    uint8_t m_volume;
};

}  // namespace Zwave
}  // namespace HubPlatform
