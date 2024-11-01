#pragma once

#include "service_interfaces/script_event.h"

namespace HubPlatform {

class DiscoveryStarted : public Services::ScriptEvent {
public:
    DiscoveryStarted()
    : Services::ScriptEvent( "discovery_started" ) {}
};

}  // namespace HubPlatform
