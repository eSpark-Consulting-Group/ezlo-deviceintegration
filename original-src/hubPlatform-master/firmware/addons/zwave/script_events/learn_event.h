#pragma once

#include "zwave/script_events/statused_event.h"

#include "zwave/resources/zwave_resources.h"

#include "znet/znet_command.hpp"

namespace HubPlatform {
namespace Zwave {

class LearnEvent final : public StatusedEvent {
public:
    explicit LearnEvent( znet::ModeStatus status, znet::ErrorOpt errorDetails )
    : StatusedEvent( Resources::ScriptEvent::Name::Learn, status, std::move( errorDetails ) ) {}
};

}  // namespace Zwave
}  // namespace HubPlatform
