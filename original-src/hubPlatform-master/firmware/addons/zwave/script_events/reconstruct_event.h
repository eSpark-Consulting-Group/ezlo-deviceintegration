#pragma once

#include "zwave/script_events/statused_event.h"

#include "zwave/resources/zwave_resources.h"

#include "znet/znet_command.hpp"

namespace HubPlatform {
namespace Zwave {

class ReconstructEvent final : public StatusedEvent {
public:
    explicit ReconstructEvent( znet::ModeStatus status, znet::ErrorOpt errorDetails )
    : StatusedEvent( Resources::ScriptEvent::Name::Reconstruct, status, std::move( errorDetails ) ) {}
};

}  // namespace Zwave
}  // namespace HubPlatform
