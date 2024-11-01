#pragma once

#include "service_interfaces/script_event.h"

namespace HubPlatform {
namespace LuaD {
namespace ScriptEvents {

class TokenUpdated : public Services::ScriptEvent {
public:
    explicit TokenUpdated();
};

}  // namespace ScriptEvents
}  // namespace LuaD
}  // namespace HubPlatform