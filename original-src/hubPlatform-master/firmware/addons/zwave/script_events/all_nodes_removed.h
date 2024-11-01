#pragma once

#include "zwave/resources/zwave_resources.h"

#include "service_interfaces/script_event.h"

namespace HubPlatform {
namespace Zwave {

class AllNodesRemoved : public Services::ScriptEvent {
public:
    explicit AllNodesRemoved()
    : Services::ScriptEvent( Resources::ScriptEvent::Name::AllNodesRemoved ) {}
};

}  // namespace Zwave
}  // namespace HubPlatform
