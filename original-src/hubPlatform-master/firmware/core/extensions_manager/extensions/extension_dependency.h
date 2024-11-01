#pragma once

#include "base_utils/semantic_version.h"

#include "json_utils/json_fwd.h"

#include "base_utils/hash_utils.h"

#include <string>

namespace HubPlatform {
namespace CommonUtils {
struct AddonSummary;
struct PluginSummary;
}  // namespace CommonUtils
}  // namespace HubPlatform

namespace HubPlatform {
namespace ExtensionsManager {

using RequiredVersion = Common::SemanticVersion< std::uint32_t >;

template < class SummaryType >
struct ExtensionDependency final {
    static ExtensionDependency< SummaryType > fromJson( const JsonObject& json );
    std::string toString() const;

    bool isValid() const;
    bool isSatisfiedBy( const SummaryType& addonSummary ) const;

    bool operator==( const ExtensionDependency& extensionDependency ) const;

    std::string id;
    RequiredVersion version;
};

using AddonDependency = ExtensionDependency< CommonUtils::AddonSummary >;
using PluginDependency = ExtensionDependency< CommonUtils::PluginSummary >;

}  // namespace ExtensionsManager
}  // namespace HubPlatform

namespace std {
template <>
template < class SummaryType >
struct hash< HubPlatform::ExtensionsManager::ExtensionDependency< SummaryType > > {
    std::size_t operator()(
        const HubPlatform::ExtensionsManager::ExtensionDependency< SummaryType >& dependency ) const {
        return common::hash_combine( common::hash_value( dependency.id ), common::hash_value( dependency.version ) );
    }
};
}  // namespace std
