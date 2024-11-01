#include "common_utils/hub_state.h"

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace CommonUtils {

HubState HubState::fromJson( const JsonObject& stateJson ) {
    HubState hubState;

    auto versionString = json_get_str( stateJson, "firmware" );
    if( !versionString.empty() ) {
        hubState.firmware = Firmware::Version( versionString );
    }

    JsonArray addons;
    json_get_arr( stateJson, "addons", addons );
    for( auto& addon : addons ) {
        hubState.addons.push_back( CommonUtils::AddonSummary::fromJson( addon ) );
    }

    JsonArray plugins;
    json_get_arr( stateJson, "plugins", plugins );
    for( auto& plugin : plugins ) {
        hubState.plugins.push_back( CommonUtils::PluginSummary::fromJson( plugin ) );
    }

    JsonArray customPackages;
    json_get_arr( stateJson, "customPackages", customPackages );
    for( auto& customPackage : customPackages ) {
        hubState.customPackages.push_back( CommonUtils::AddonSummary::fromJson( customPackage ) );
    }

    return hubState;
}

JsonObject HubState::toJson() const {
    JsonObject stateJson = JSON_EMPTY_OBJ;
    if( firmware.isValid() ) {
        stateJson[ "firmware" ] = firmware.toString();
    }

    if( !addons.empty() ) {
        JsonArray addonsJson;
        for( auto& addon : addons ) {
            addonsJson.push_back( addon.toJson() );
        }
        stateJson[ "addons" ] = addonsJson;
    }

    if( !plugins.empty() ) {
        JsonArray pluginsJson;
        for( auto& plugin : plugins ) {
            pluginsJson.push_back( plugin.toJson() );
        }
        stateJson[ "plugins" ] = pluginsJson;
    }

    if( !customPackages.empty() ) {
        JsonArray customPackagesJson;
        for( auto& customPackage : customPackages ) {
            customPackagesJson.push_back( customPackage.toJson() );
        }
        stateJson[ "customPackages" ] = customPackagesJson;
    }

    return stateJson;
}

}  // namespace CommonUtils
}  // namespace HubPlatform
