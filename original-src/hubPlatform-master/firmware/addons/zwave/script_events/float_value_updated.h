#pragma once

#include "zwave/script_events/value_event.h"

namespace HubPlatform {
namespace Zwave {

class FloatValueUpdated : public ValueEvent {
public:
    FloatValueUpdated( uint8_t nodeId,
                       uint8_t classId,
                       uint8_t channelId,
                       const char* type,
                       const char* scale,
                       int32_t value,
                       uint8_t precision );

    Json toJson() const final;

private:
    const char* m_type;
    const char* m_scale;
    int32_t m_value;
    uint8_t m_precision;
};

}  // namespace Zwave
}  // namespace HubPlatform
