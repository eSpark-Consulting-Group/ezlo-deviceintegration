#pragma once

#include "zwave/script_events/value_event.h"

#include <znet/enums/configuration_format.hpp>

namespace HubPlatform {
namespace Zwave {

class ConfigurationValueUpdated : public ValueEvent {
public:
    ConfigurationValueUpdated( uint8_t nodeId,
                               uint8_t channelId,
                               uint16_t parameterNumber,
                               uint8_t parameterSize,
                               uint32_t value,
                               const znet::ConfigurationFormat* format = nullptr );

    Json toJson() const final;

private:
    const uint16_t m_parameterNumber;
    const uint8_t m_parameterSize;
    const znet::ConfigurationFormat* m_format;
    uint32_t m_value;
};

}  // namespace Zwave
}  // namespace HubPlatform
