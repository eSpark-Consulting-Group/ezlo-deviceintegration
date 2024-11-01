#pragma once

#include "service_interfaces/script_event.h"

namespace HubPlatform {
namespace LuaD {
namespace ScriptEvents {

class TokenExpired : public Services::ScriptEvent {
public:
    explicit TokenExpired();
};

}  // namespace ScriptEvents
}  // namespace LuaD
}  // namespace HubPlatform