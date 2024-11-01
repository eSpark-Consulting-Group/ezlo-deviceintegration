#include "binding_interfaces/common/hub_api_restrictions.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

#include <string>
#include <map>

namespace HubPlatform {

std::string getHubAPIRestrictionsString( HubAPIRestrictions luaAPIRestrictions ) {
    static const std::map< HubAPIRestrictions, std::string > luaAPIRestrictionsMap = {
        { HubAPIRestrictions::NO_RESTRICTIONS, "NO_RESTRICTIONS" },
        { HubAPIRestrictions::NO_ASYNC_RESTRICTION, "NO_ASYNC_RESTRICTION" }
    };

    const auto it = luaAPIRestrictionsMap.find( luaAPIRestrictions );
    if( it != luaAPIRestrictionsMap.end() ) {
        return it->second;
    }

    return "UNKNOWN_RESTRICTION";
}

}  // namespace HubPlatform
