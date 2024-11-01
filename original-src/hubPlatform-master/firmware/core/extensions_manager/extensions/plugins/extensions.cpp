#include "extensions_manager/extensions/plugins/extensions.h"
#include "extensions_manager/resources/em_resources.h"

#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace ExtensionsManager {

void Extensions::addPluginConfig( const PluginConfig& pluginConfig ) {
    configs.push_back( pluginConfig );
}

void Extensions::addPluginExtension( const PluginExtension& pluginExtension ) {
    pluginExtensions.push_back( pluginExtension );
}

Json Extensions::toJson() const {
    Json result;
    if( !pluginExtensions.empty() ) {
        result[ Resources::PluginRecord::ExtensionsRecord::Plugins ] = JsonArray();
        for( const auto& pluginExtension : pluginExtensions ) {
            JsonObject pluginExtensionJo;
            pluginExtensionJo[ Resources::PluginRecord::ExtensionsRecord::Id ] = pluginExtension.pluginId;
            pluginExtensionJo[ Resources::PluginRecord::ExtensionsRecord::Configs ] = JsonArray();
            for( const auto& config : pluginExtension.configs ) {
                pluginExtensionJo[ Resources::PluginRecord::ExtensionsRecord::Configs ].push_back(
                    JSON( Resources::PluginRecord::ExtensionsRecord::Name
                          << config.name << Resources::PluginRecord::ExtensionsRecord::Config << config.configFile ) );
            }

            result[ Resources::PluginRecord::ExtensionsRecord::Plugins ].push_back( std::move( pluginExtensionJo ) );
        }
    }

    if( !configs.empty() ) {
        result[ Resources::PluginRecord::ExtensionsRecord::Configs ] = JsonArray();
        for( const auto& config : configs ) {
            result[ Resources::PluginRecord::ExtensionsRecord::Configs ].push_back( JSON(
                Resources::PluginRecord::ExtensionsRecord::Name
                << config.name << Resources::PluginRecord::ExtensionsRecord::Description << config.description ) );
        }
    }

    return result;
}

std::unique_ptr< Extensions > Extensions::getExtensions( const Json& joConfig ) {
    if( joConfig.count( "extensions" ) == 0 ) {
        return nullptr;
    }

    auto extensions = std::make_unique< Extensions >();

    JsonArray jaConfigs;
    if( json_get_arr( joConfig[ "extensions" ], "configs", jaConfigs ) ) {
        for( const auto& elm : jaConfigs ) {
            PluginConfig config;
            config.name = json_get_str( elm, "name" );
            config.description = json_get_str( elm, "description" );
            extensions->addPluginConfig( config );
        }
    }

    JsonArray jaExtensions;
    if( json_get_arr( joConfig[ "extensions" ], "plugins", jaExtensions ) ) {
        for( const auto& elm : jaExtensions ) {
            PluginExtension extension;
            extension.pluginId = json_get_str( elm, "id" );

            JsonArray jaExtensionConfigs;
            if( json_get_arr( elm, "configs", jaExtensionConfigs ) ) {
                for( const auto& elmCfg : jaExtensionConfigs ) {
                    ExtensionConfig config;
                    config.name = json_get_str( elmCfg, "name" );
                    config.configFile = json_get_str( elmCfg, "config" );
                    extension.configs.push_back( config );
                }
            }

            extensions->addPluginExtension( extension );
        }
    }

    return extensions;
}

}  // namespace ExtensionsManager
}  // namespace HubPlatform
