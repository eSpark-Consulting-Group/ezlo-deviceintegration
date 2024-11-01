#pragma once

#include "zwave/script_events/value_event.h"

#include <znet/znet_color_switch.hpp>

namespace HubPlatform {
namespace Zwave {

class RgbValueUpdated : public ValueEvent {
public:
    RgbValueUpdated( uint8_t nodeId, uint8_t classId, uint8_t channelId, znet::ColorComponentInfos components );

    Json toJson() const final;

private:
    znet::ColorComponentInfos m_components;
};

}  // namespace Zwave
}  // namespace HubPlatform
