#pragma once

#include "plugin_utils/plugin_version.h"

#include "json_utils/json_fwd.h"
#include "base_utils/hash_utils.h"

#include <string>

namespace HubPlatform {
namespace CommonUtils {

struct PluginSummary final {
    static PluginSummary fromJson( const JsonObject& json );
    JsonObject toJson() const;
    std::string toString() const;

    bool isValid() const;

    bool operator==( const PluginSummary& pluginSummary ) const;
    bool operator!=( const PluginSummary& pluginSummary ) const;

    std::string id;
    PluginVersion version;
};

}  // namespace CommonUtils
}  // namespace HubPlatform

namespace std {
template <>
struct hash< HubPlatform::CommonUtils::PluginSummary > {
    std::size_t operator()( const HubPlatform::CommonUtils::PluginSummary& summary ) const {
        return common::hash_combine( common::hash_value( summary.id ), common::hash_value( summary.version ) );
    }
};
}  // namespace std
