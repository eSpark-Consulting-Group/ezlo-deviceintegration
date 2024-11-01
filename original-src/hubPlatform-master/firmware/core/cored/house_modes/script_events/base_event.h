#pragma once

#include "service_interfaces/script_event.h"
#include <string>

namespace HubPlatform {
namespace CoreDHouseModes {
namespace ScriptEvents {

class BaseEvent : public Services::ScriptEvent {
public:
    BaseEvent( const std::string& event );
};

}  // namespace ScriptEvents
}  // namespace CoreDHouseModes
}  // namespace HubPlatform
