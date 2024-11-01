#pragma once

#include "service_interfaces/script_event.h"

namespace HubPlatform {

class DiscoveryStopped : public Services::ScriptEvent {
public:
    DiscoveryStopped()
    : Services::ScriptEvent( "discovery_stopped" ) {}
};

}  // namespace HubPlatform
