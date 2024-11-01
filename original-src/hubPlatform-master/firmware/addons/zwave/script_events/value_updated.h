#pragma once

#include "zwave/script_events/value_event.h"

#include <znet/znet.h>
#include <znet/optional.hpp>

#include <chrono>

namespace HubPlatform {
namespace Zwave {

class ValueUpdated : public ValueEvent {
public:
    ValueUpdated( uint8_t nodeId,
                  uint8_t classId,
                  uint8_t channelId,
                  uint8_t currentValue,
                  znet::utils::optional< uint8_t > targetValue,
                  znet::utils::optional< std::chrono::seconds > targetDuration );

    Json toJson() const final;

private:
    uint8_t m_currentValue;
    znet::utils::optional< uint8_t > m_targetValue;
    znet::utils::optional< std::chrono::seconds > m_targetDuration;
};

}  // namespace Zwave
}  // namespace HubPlatform
