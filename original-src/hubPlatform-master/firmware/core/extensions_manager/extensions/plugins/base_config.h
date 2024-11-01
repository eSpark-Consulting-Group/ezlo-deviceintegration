#pragma once

#include "extensions_manager/extensions/extension_dependency.h"
#include "extensions_manager/extensions/plugins/plugin_type.h"
#include "extensions_manager/extensions/plugins/extensions.h"
#include "extensions_manager/common/execution_policy.h"

#include "plugin_utils/plugin_summary.h"
#include "json_utils/json_fwd.h"

#include <string>
#include <memory>

namespace HubPlatform {
namespace ExtensionsManager {

/* @brief base part of a plugin's config */
struct BaseConfig final {
public:
    using Permission = std::string;
    using PluginPermissions = std::vector< Permission >;

    BaseConfig( CommonUtils::PluginSummary pluginSummary, PluginType pluginType, PluginPermissions pluginPermissions );

    static std::unique_ptr< BaseConfig > getBaseConfig( const Json& joConfig );

    void setStartupScript( std::string pluginStartupScript );
    void setTeardownScript( std::string pluginTeardownScript );
    void setInitializeMigrationScript( std::string script );
    void setMigrationScript( std::string script );
    void setFinalizeMigrationScript( std::string script );
    void setSceneBlocksJson( std::string path );
    void setExecutionPolicy( ExecutionPolicy executionPolicy );

public:
    CommonUtils::PluginSummary summary;
    PluginType pluginType;
    std::string startupScript;
    std::string teardownScript;
    std::string initializeMigrationScript;
    std::string migrationScript;
    std::string finalizeMigrationScript;
    std::string sceneBlocksJson;

    ExecutionPolicy executionPolicy = ExecutionPolicy::NewStateEachScriptRun;

    struct Dependencies {
        std::vector< AddonDependency > requiredAddons;
        std::vector< PluginDependency > requiredPlugins;
    } dependencies;

    PluginPermissions permissions;

    std::unique_ptr< Extensions > extensions;
};

}  // namespace ExtensionsManager
}  // namespace HubPlatform
