#include "extensions_manager/extensions/plugins/plugin_extension_priorities_manager.h"
#include "extensions_manager/em_environment.h"

#include "extensions_manager/resources/em_resources.h"

#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"
#include "jsonrpc/jsonrpc.h"

#include "database_client/idbclient.h"

#include <tiny/tiny_consolecolors.h>
#include <unordered_set>

namespace HubPlatform {
namespace ExtensionsManager {

PluginExtensionPrioritiesManager::PluginExtensionPrioritiesManager( Environment& environment )
: m_environment( environment ) {}

void PluginExtensionPrioritiesManager::registerPlugin( const Plugin& plugin ) {
    const auto& extensions = plugin.getBaseConfig().extensions;
    if( !extensions || extensions->pluginExtensions.empty() ) {
        return;
    }

    auto const& pluginId = plugin.getBaseConfig().summary.id;

    for( const auto& extension : extensions->pluginExtensions ) {
        auto extensionsJson = m_environment.getDbClient().findOne(
            Resources::DBTableNames::PluginsExtensionsPriorities, JSON( "_id" << extension.pluginId ), nullptr );
        if( extensionsJson.empty() ) {
            extensionsJson[ "_id" ] = extension.pluginId;
            extensionsJson[ "extensions" ] = JsonArray();
        }

        bool found = false;
        for( const auto& extensionName : extensionsJson[ "extensions" ] ) {
            if( extensionName == pluginId ) {
                found = true;
                break;
            }
        }

        if( !found ) {
            extensionsJson[ "extensions" ].push_back( pluginId );
            m_environment.getDbClient().update(
                Resources::DBTableNames::PluginsExtensionsPriorities, JSON_EMPTY_OBJ, extensionsJson, true );
        }
    }
}

void PluginExtensionPrioritiesManager::unregisterPlugin( const Plugin& plugin ) {
    const auto& extensions = plugin.getBaseConfig().extensions;
    if( !extensions || extensions->pluginExtensions.empty() ) {
        return;
    }

    auto const& pluginId = plugin.getBaseConfig().summary.id;

    for( const auto& extension : extensions->pluginExtensions ) {
        auto extensionsJson = m_environment.getDbClient().findOne(
            Resources::DBTableNames::PluginsExtensionsPriorities, JSON( "_id" << extension.pluginId ), nullptr );
        if( extensionsJson.empty() ) {
            continue;
        }

        bool found = false;
        JsonArray newExtensions;
        for( const auto& extensionName : extensionsJson[ "extensions" ] ) {
            if( extensionName != pluginId ) {
                newExtensions.push_back( extensionName );
            }
            else {
                found = true;
            }
        }

        if( !found ) {
            extensionsJson[ "extensions" ] = std::move( newExtensions );
            m_environment.getDbClient().update( Resources::DBTableNames::PluginsExtensionsPriorities,
                                                JSON( "_id" << extension.pluginId ),
                                                extensionsJson );
        }
    }
}

std::vector< std::string > PluginExtensionPrioritiesManager::getPluginExtensions( const std::string& pluginName ) {
    auto extensions = m_environment.getDbClient().findOne(
        Resources::DBTableNames::PluginsExtensionsPriorities, JSON( "_id" << pluginName ), nullptr );

    auto extensionsListIt = extensions.find( "extensions" );

    if( extensionsListIt != extensions.end() && extensionsListIt->is_array() && !extensionsListIt->empty() ) {
        return *extensionsListIt;
    }
    else {
        return std::vector< std::string >();
    }
}

void PluginExtensionPrioritiesManager::changePluginExtensionsPriorities( const std::string& pluginName,
                                                                         const std::vector< std::string >& list ) {
    JsonArray result;
    std::unordered_set< std::string > addedExtensions;

    auto addExtensions = [&result, &addedExtensions]( const std::vector< std::string >& extensions ) {
        for( const auto& extensionName : extensions ) {
            if( addedExtensions.insert( extensionName ).second ) {
                result.push_back( extensionName );
            }
        }
    };

    addExtensions( list );

    auto extensions = m_environment.getDbClient().findOne(
        Resources::DBTableNames::PluginsExtensionsPriorities, JSON( "_id" << pluginName ), nullptr );

    if( extensions.count( "extensions" ) ) {
        addExtensions( extensions[ "extensions" ] );
    }

    extensions[ "_id" ] = pluginName;
    extensions[ "extensions" ] = std::move( result );

    m_environment.getDbClient().update(
        Resources::DBTableNames::PluginsExtensionsPriorities, JSON_EMPTY_OBJ, extensions, true );
}

}  // namespace ExtensionsManager
}  // namespace HubPlatform
