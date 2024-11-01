#pragma once

#include "extensions_manager/extensions/extension_dependency.h"
#include "extensions_manager/extensions/plugins/plugin_type.h"
#include "extensions_manager/common/execution_policy.h"

#include "plugin_utils/plugin_summary.h"
#include "json_utils/json_fwd.h"

#include <string>
#include <memory>

namespace HubPlatform {
namespace ExtensionsManager {

/* @brief plugin extensions */
struct Extensions final {
public:
    struct PluginConfig {
        std::string name;
        std::string description;
    };

    using Configs = std::vector< PluginConfig >;

    struct ExtensionConfig {
        std::string name;
        std::string configFile;
    };

    struct PluginExtension {
        std::string pluginId;
        std::vector< ExtensionConfig > configs;
    };

    using PluginExtensions = std::vector< PluginExtension >;

    static std::unique_ptr< Extensions > getExtensions( const Json& joConfig );

    void addPluginConfig( const PluginConfig& pluginConfig );
    void addPluginExtension( const PluginExtension& pluginExtension );

    Json toJson() const;

public:
    Configs configs;
    PluginExtensions pluginExtensions;
};

}  // namespace ExtensionsManager
}  // namespace HubPlatform
