#include "extensions_manager/extensions/plugins/base_config.h"

#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace ExtensionsManager {

BaseConfig::BaseConfig( CommonUtils::PluginSummary pluginSummary,
                        PluginType pluginType,
                        PluginPermissions pluginPermissions )
: summary( std::move( pluginSummary ) )
, pluginType( pluginType )
, permissions( std::move( pluginPermissions ) ) {}

void BaseConfig::setStartupScript( std::string pluginStartupScript ) {
    startupScript = std::move( pluginStartupScript );
}

void BaseConfig::setTeardownScript( std::string pluginTeardownScript ) {
    teardownScript = std::move( pluginTeardownScript );
}

void BaseConfig::setInitializeMigrationScript( std::string script ) {
    initializeMigrationScript = std::move( script );
}

void BaseConfig::setMigrationScript( std::string script ) {
    migrationScript = std::move( script );
}

void BaseConfig::setFinalizeMigrationScript( std::string script ) {
    finalizeMigrationScript = std::move( script );
}

void BaseConfig::setSceneBlocksJson( std::string path ) {
    sceneBlocksJson = std::move( path );
}

void BaseConfig::setExecutionPolicy( ExecutionPolicy newExecutionPolicy ) {
    executionPolicy = newExecutionPolicy;
}

std::unique_ptr< BaseConfig > BaseConfig::getBaseConfig( const Json& joConfig ) {
    // Reading required fields
    auto summary = CommonUtils::PluginSummary::fromJson( joConfig );
    if( !summary.isValid() ) {
        Log::error( cc::error( "Plugin: invalid plugin summary" ) );
        return nullptr;
    }

    try {
        jsonCheckString( joConfig, "type" );
    }
    catch( ... ) {
        Log::error( cc::error( "Plugin: invalid 'type' field" ) );
        return nullptr;
    }

    auto pluginType = stringToPluginType( json_get_str( joConfig, "type" ) );
    if( pluginType == PluginType::NONE ) {
        Log::error( cc::error( "Plugin: invalid plugin type" ) );
        return nullptr;
    }

    PluginPermissions permissions;
    JsonArray jaPermissions = JSON_EMPTY_ARR;

    if( json_get_arr( joConfig, "permissions", jaPermissions ) ) {
        for( const auto& elm : jaPermissions )
            if( elm.is_string() )
                permissions.push_back( json_get_str( elm ) );
    }
    else {
        Log::error( cc::error( "Plugin: fail to fetch permissions" ) );
        return nullptr;
    }

    std::string executionPolicyString;
    auto executionPolicy = ExecutionPolicy::NewStateEachScriptRun;
    if( json_get( joConfig, "executionPolicy", executionPolicyString ) ) {
        executionPolicy = toExecutionPolicy( executionPolicyString );
        if( executionPolicy == ExecutionPolicy::Unknown ) {
            LOGGER_WARN( cc::note( "executionPolicy" ) + cc::warn( " is defined with wrong value: '" ) +
                         cc::note( executionPolicyString ) + cc::warn( "'. " ) + cc::note( "newStateEachScriptRun" ) +
                         cc::warn( " policy will be used." ) );
        }
    }
    else {
        LOGGER_DEBUG( cc::note( "executionPolicy" ) + cc::debug( " is not defined. " ) +
                      cc::note( "newStateEachScriptRun" ) + cc::debug( " policy will be used." ) );
    }

    auto baseConfig = std::make_unique< BaseConfig >( std::move( summary ), pluginType, std::move( permissions ) );

    if( baseConfig == nullptr ) {
        Log::error( cc::error( "Plugin: fail to construct a BaseConfig" ) );
        return nullptr;
    }

    // Reading optional fields
    baseConfig->setStartupScript( json_get_str( joConfig, "startup" ) );
    baseConfig->setTeardownScript( json_get_str( joConfig, "teardown" ) );
    baseConfig->setInitializeMigrationScript( json_get_str( joConfig, "initialize_migration" ) );
    baseConfig->setMigrationScript( json_get_str( joConfig, "migration" ) );
    baseConfig->setFinalizeMigrationScript( json_get_str( joConfig, "finalize_migration" ) );
    baseConfig->setSceneBlocksJson( json_get_str( joConfig, "sceneBlocks" ) );
    baseConfig->setExecutionPolicy( executionPolicy );

    JsonObject joDependencies = JSON_EMPTY_OBJ;
    if( json_get( joConfig, "dependencies", joDependencies ) ) {
        JsonArray jaAddonDependencies = JSON_EMPTY_ARR;
        if( json_get_arr( joDependencies, "addons", jaAddonDependencies ) )
            for( JsonObject& joAddonDependency : jaAddonDependencies )
                if( joAddonDependency.is_object() ) {
                    auto extensionDependency = AddonDependency::fromJson( joAddonDependency );
                    if( extensionDependency.isValid() )
                        baseConfig->dependencies.requiredAddons.push_back( std::move( extensionDependency ) );
                    else
                        Log::warn( cc::warn( "Plugin: skipping invalid addon dependency" ) );
                }

        JsonArray jaPluginDependencies = JSON_EMPTY_ARR;
        if( json_get_arr( joDependencies, "plugins", jaPluginDependencies ) )
            for( JsonObject& joPluginDependency : jaPluginDependencies )
                if( joPluginDependency.is_object() ) {
                    auto extensionDependency = PluginDependency::fromJson( joPluginDependency );
                    if( extensionDependency.isValid() )
                        baseConfig->dependencies.requiredPlugins.push_back( std::move( extensionDependency ) );
                    else
                        Log::warn( cc::warn( "Plugin: skipping invalid addon dependency" ) );
                }
    }

    baseConfig->extensions = Extensions::getExtensions( joConfig );

    return baseConfig;
}

}  // namespace ExtensionsManager
}  // namespace HubPlatform
