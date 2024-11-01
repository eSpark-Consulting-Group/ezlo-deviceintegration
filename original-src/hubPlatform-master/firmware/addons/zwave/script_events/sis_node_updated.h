#pragma once

#include "service_interfaces/script_event.h"

#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

class SisNodeUpdated : public Services::ScriptEvent {
public:
    SisNodeUpdated()
    : Services::ScriptEvent( Resources::ScriptEvent::Name::SisNodeUpdated ) {}
};

}  // namespace Zwave
}  // namespace HubPlatform
