#pragma once

#include "zwave/resources/zwave_resources.h"

#include "service_interfaces/script_event.h"

namespace HubPlatform {
namespace Zwave {

class UnknownNodeRemoved final : public Services::ScriptEvent {
public:
    UnknownNodeRemoved()
    : Services::ScriptEvent( Resources::ScriptEvent::Name::UnknownNodeRemoved ) {}
};

}  // namespace Zwave
}  // namespace HubPlatform
