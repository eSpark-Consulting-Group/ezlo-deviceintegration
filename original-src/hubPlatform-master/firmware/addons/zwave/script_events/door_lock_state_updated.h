#pragma once

#include "zwave/script_events/value_event.h"
#include <znet/znet_door_condition.hpp>

#include <znet/znet_utils.hpp>

#include <chrono>

namespace HubPlatform {
namespace Zwave {

class DoorLockStateUpdated : public ValueEvent {
public:
    using Seconds = std::chrono::seconds;
    using SecondsOpt = znet::utils::optional< Seconds >;
    DoorLockStateUpdated( const uint8_t nodeId,
                          const uint8_t channelId,
                          const znet::DoorLock::Mode mode,
                          const uint8_t insideHandlesMode,
                          const uint8_t outsideHandlesMode,
                          const znet::DoorCondition condition,
                          const SecondsOpt lockTimeout,
                          const znet::DoorLock::ModeOpt targetMode,
                          const SecondsOpt targetModeTimeout );

    Json toJson() const final;

private:
    znet::DoorLock::Mode m_mode;
    uint8_t m_insideHandlesMode;
    uint8_t m_outsideHandlesMode;
    znet::DoorCondition m_condition;
    SecondsOpt m_lockTimeout;
    znet::DoorLock::ModeOpt m_targetMode;
    SecondsOpt m_targetModeTimeout;
};

}  // namespace Zwave
}  // namespace HubPlatform
