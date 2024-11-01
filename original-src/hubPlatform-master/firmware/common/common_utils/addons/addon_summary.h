#pragma once

#include "common_utils/addons/addon_version.h"

#include "json_utils/json_fwd.h"

#include <string>

namespace HubPlatform {
namespace CommonUtils {

struct AddonSummary final {
    static AddonSummary fromJson( const JsonObject& json );
    std::string toString() const;
    JsonObject toJson() const;

    bool isValid() const;

    std::string id;
    AddonVersion version;
};

}  // namespace CommonUtils
}  // namespace HubPlatform
