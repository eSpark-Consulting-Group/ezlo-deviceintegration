#pragma once

#include "common_utils/addons/addon_summary.h"
#include "plugin_utils/plugin_summary.h"
#include "base_utils/firmware_version.h"

#include <vector>

namespace HubPlatform {
namespace CommonUtils {

struct HubState final {
    static HubState fromJson( const JsonObject& json );
    JsonObject toJson() const;

    using AddonSummaries = std::vector< AddonSummary >;
    using PluginSummaries = std::vector< PluginSummary >;

    HubPlatform::Firmware::Version firmware;
    AddonSummaries addons;
    PluginSummaries plugins;
    AddonSummaries customPackages;
};

}  // namespace CommonUtils
}  // namespace HubPlatform
