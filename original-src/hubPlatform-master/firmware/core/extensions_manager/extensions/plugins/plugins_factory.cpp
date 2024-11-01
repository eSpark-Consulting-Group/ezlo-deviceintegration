#include "extensions_manager/extensions/plugins/plugins_factory.h"

#include "extensions_manager/extensions/plugins/gateway/gateway_config.h"
#include "extensions_manager/extensions/plugins/gateway/gateway_plugin.h"
#include "extensions_manager/extensions/plugins/extension/extension_plugin.h"
#include "extensions_manager/extensions/plugins/library/library_plugin.h"
#include "extensions_manager/extensions/plugins/base_config.h"

#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"

#include <tiny/tiny_consolecolors.h>

namespace HubPlatform {
namespace ExtensionsManager {

PluginsFactory::PluginsFactory( HubPlatform::ExtensionsManager::Environment& environment )
: m_environment( environment ) {}

std::unique_ptr< Plugin > PluginsFactory::createPlugin( const Json& joConfig ) {
    auto baseConfigInfo = BaseConfig::getBaseConfig( joConfig );
    if( !baseConfigInfo ) {
        Log::error( cc::error( "Plugin: fail to get a base part of the plugin's config" ) );
        return nullptr;
    }

    switch( baseConfigInfo->pluginType ) {
    case PluginType::FUNCTIONAL_UNIT: {
        return std::make_unique< Plugin >( m_environment, std::move( baseConfigInfo ) );
    }
    case PluginType::EXTENTION: {
        return std::make_unique< ExtensionPlugin >( m_environment, std::move( baseConfigInfo ) );
    }
    case PluginType::GATEWAY: {
        auto gatewayConfigInfo = GatewayConfig::getGatewayConfig( joConfig );
        if( !gatewayConfigInfo ) {
            Log::error( cc::error( "Plugin: fail to get a gateway part of the plugin's config" ) );
            return nullptr;
        }

        return std::make_unique< GatewayPlugin >(
            m_environment, std::move( baseConfigInfo ), std::move( gatewayConfigInfo ) );
    }
    case PluginType::LIBRARY: {
        auto libraryConfigInfo = LibraryConfig::getLibraryConfig( joConfig );
        if( !libraryConfigInfo ) {
            Log::error( cc::error( "Plugin: fail to get a library part of the plugin's config" ) );
            return nullptr;
        }
        return std::make_unique< LibraryPlugin >(
            m_environment, std::move( baseConfigInfo ), std::move( libraryConfigInfo ) );
    }
    default:
        Log::error( cc::error( "Plugin: an unsupported plugin type" ) );
        break;
    }

    return nullptr;
}

}  // namespace ExtensionsManager
}  // namespace HubPlatform
