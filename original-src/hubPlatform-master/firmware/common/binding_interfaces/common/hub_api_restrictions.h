#pragma once

#include <string>

namespace HubPlatform {

enum class HubAPIRestrictions { NO_RESTRICTIONS = 0, NO_ASYNC_RESTRICTION };

std::string getHubAPIRestrictionsString( HubAPIRestrictions luaAPIRestrictions );

}  // namespace HubPlatform
