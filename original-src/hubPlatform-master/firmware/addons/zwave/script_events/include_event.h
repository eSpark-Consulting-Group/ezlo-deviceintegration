#pragma once

#include "zwave/script_events/statused_event.h"

#include "zwave/resources/zwave_resources.h"

#include "znet/znet_command.hpp"

namespace HubPlatform {
namespace Zwave {

class IncludeEvent final : public StatusedEvent {
public:
    explicit IncludeEvent( znet::ModeStatus status, znet::ErrorOpt errorDetails )
    : StatusedEvent( Resources::ScriptEvent::Name::Include, status, std::move( errorDetails ) ) {}
};

}  // namespace Zwave
}  // namespace HubPlatform
