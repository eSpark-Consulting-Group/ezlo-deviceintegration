#include "extensions_manager/extensions/extensions_graph.h"
#include "extensions_manager/em_environment.h"

#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"

#include <tiny/tiny_consolecolors.h>

namespace HubPlatform {
namespace ExtensionsManager {

ExtensionsGraph::ExtensionsGraph( Environment& environment )
: m_environment( environment ) {}

Addon& ExtensionsGraph::registerAddon( const std::string& addonId ) {
    auto addonPtr = std::make_unique< Addon >( getEnvironment(), addonId );
    auto& addon = *addonPtr;

    m_registeredAddons.emplace( addonId, std::move( addonPtr ) );

    return addon;
}

void ExtensionsGraph::unregisterAddon( Addon& addon ) {
    onAddonStopped( addon.getSummary() );

    // Teardown plugins that depend on this addon
    auto it = m_addonSatisfiedADs.find( &addon );
    if( it != m_addonSatisfiedADs.end() ) {
        for( auto& dependency : it->second ) {
            for( auto pluginPtr : m_ADRequiringPlugins[ dependency ] ) {
                teardownPlugin( *pluginPtr );
            }
        }
    }

    for( auto& dependency : m_addonSatisfiedADs[&addon ] ) {
        m_ADSatisfyingAddon.erase( dependency );
    }

    m_addonSatisfiedADs.erase( &addon );
    m_registeredAddons.erase( addon.getSummary().id );
}

void ExtensionsGraph::onAddonStarted( const CommonUtils::AddonSummary& addonSummary, int64_t setupId ) {
    auto optAddon = findRegisteredAddon( addonSummary.id );
    if( !optAddon ) {
        Log::error( cc::error( "Started addon is not registered: " ) + cc::note( addonSummary.toString() ) );
        optAddon = &registerAddon( addonSummary.id );
    }
    optAddon->setSummary( addonSummary );
    if( !optAddon->startup( setupId ) )
        return;

    // Check if there are dependencies this addon satisfies
    auto it = m_addonSatisfiedADs.find( optAddon );
    if( it == m_addonSatisfiedADs.end() ) {
        for( auto& dependencyPair : m_ADRequiringPlugins ) {
            checkIfAddonSatisfiesDependency( *optAddon, dependencyPair.first );
        }
    }

    // Start plugins that had only this dependency unsatisfied
    std::unordered_set< Plugin* > processedPlugins;
    it = m_addonSatisfiedADs.find( optAddon );
    if( it != m_addonSatisfiedADs.end() ) {
        for( auto& dependency : it->second ) {
            for( auto pluginPtr : m_ADRequiringPlugins[ dependency ] ) {
                tryStartPlugin( *pluginPtr, processedPlugins );
            }
        }
    }
}

bool ExtensionsGraph::canStartPlugin( Plugin& plugin ) const {
    for( auto& dependency : plugin.getBaseConfig().dependencies.requiredAddons ) {
        auto it = m_ADSatisfyingAddon.find( dependency );
        if( it == m_ADSatisfyingAddon.end() ) {
            LOGGER_DEBUG_RAW( cc::debug( "Can not start plugin " ) +
                              cc::note( plugin.getBaseConfig().summary.toString() ) +
                              cc::debug( " right now, because it's dependency on addon " ) +
                              cc::note( dependency.toString() ) + cc::debug( " is not yet satisfied" ) );
            return false;
        }

        if( it->second->getStatus() != Addon::Status::Started ) {
            LOGGER_DEBUG_RAW( cc::debug( "Can not start plugin " ) +
                              cc::note( plugin.getBaseConfig().summary.toString() ) +
                              cc::debug( " right now, because addon " ) + cc::note( dependency.toString() ) +
                              cc::debug( " it depends on is not yet started" ) );
            return false;
        }
    }
    for( auto& dependency : plugin.getBaseConfig().dependencies.requiredPlugins ) {
        auto it = m_PDSatisfyingPlugin.find( dependency );
        if( it == m_PDSatisfyingPlugin.end() ) {
            LOGGER_DEBUG_RAW( cc::debug( "Can not start plugin " ) +
                              cc::note( plugin.getBaseConfig().summary.toString() ) +
                              cc::debug( " right now, because it's dependency on plugin " ) +
                              cc::note( dependency.toString() ) + cc::debug( " is not yet satisfied" ) );
            return false;
        }

        if( it->second->getStatus() != Plugin::Status::Started ) {
            LOGGER_DEBUG_RAW( cc::debug( "Can not start plugin " ) +
                              cc::note( plugin.getBaseConfig().summary.toString() ) +
                              cc::debug( " right now, because plugin " ) + cc::note( dependency.toString() ) +
                              cc::debug( " it depends on is not yet started" ) );
            return false;
        }
    }

    return true;
}

void ExtensionsGraph::tryStartPlugin( Plugin& plugin, std::unordered_set< Plugin* >& processedPlugins ) {
    std::vector< Plugin* > pluginsStack;
    pluginsStack.push_back( &plugin );

    while( !pluginsStack.empty() ) {
        auto& currentPlugin = *pluginsStack.back();
        pluginsStack.pop_back();

        if( !canStartPlugin( currentPlugin ) )
            continue;

        if( !processedPlugins.insert( &currentPlugin ).second )
            continue;

        currentPlugin.init();
        currentPlugin.migrate();
        currentPlugin.startup();

        // Schedule checking plugins that depend on this one
        addDependentPlugins( pluginsStack, currentPlugin );
    }
}

void ExtensionsGraph::teardownPlugin( Plugin& plugin ) {
    std::vector< Plugin* > pluginsStack = getPluginsTeardownOrder( plugin );
    // Teardown plugins in correct order
    for( int i = pluginsStack.size() - 1; i >= 0; --i ) {
        auto& currentPlugin = *pluginsStack[ i ];
        JsonObject operationInfo = { { "operation", "teardown" } };
        currentPlugin.teardown( operationInfo );
    }
}

void ExtensionsGraph::uninstallPlugin( Plugin& plugin ) {
    std::vector< Plugin* > pluginsStack = getPluginsTeardownOrder( plugin );
    for( int i = pluginsStack.size() - 1; i >= 0; --i ) {
        auto& currentPlugin = *pluginsStack[ i ];
        JsonObject operationInfo = { { "operation", "uninstall" } };
        currentPlugin.teardown( operationInfo );
        currentPlugin.uninstall();
    }
}

std::vector< Plugin* > ExtensionsGraph::getPluginsTeardownOrder( Plugin& plugin ) {
    std::unordered_set< Plugin* > processedPlugins;

    // Get correct plugins' teardown order
    std::vector< Plugin* > pluginsStack;
    pluginsStack.push_back( &plugin );

    for( int i = 0; i < pluginsStack.size(); ++i ) {
        auto& currentPlugin = *pluginsStack[ i ];

        if( !processedPlugins.insert( &currentPlugin ).second )
            continue;

        if( currentPlugin.getStatus() != Plugin::Status::Started )
            continue;

        // Schedule checking plugins that depend on this one
        addDependentPlugins( pluginsStack, currentPlugin );
    }
    return pluginsStack;
}

void ExtensionsGraph::addDependentPlugins( PluginPtrs& plugins, const Plugin& plugin ) const {
    auto it = m_pluginSatisfiedPDs.find( &plugin );
    if( it == m_pluginSatisfiedPDs.end() )
        return;

    auto& pluginSatisfiedPDs = it->second;
    for( auto& dependency : pluginSatisfiedPDs ) {
        auto it = m_PDRequiringPlugins.find( dependency );
        if( it == m_PDRequiringPlugins.end() )
            continue;

        auto& PDRequiringPlugins = it->second;
        for( auto& nextPluginPtr : PDRequiringPlugins ) {
            // Add dependent plugin
            plugins.push_back( nextPluginPtr );
        }
    }
}

void ExtensionsGraph::onAddonStopped( const CommonUtils::AddonSummary& addonSummary ) {
    auto optAddon = findRegisteredAddon( addonSummary.id );
    if( !optAddon ) {
        Log::error( cc::error( "Stopped addon is not registered: " ) + cc::note( addonSummary.toString() ) );
        return;
    }

    // NOTE: It is hard to teardown plugins that depend on this addon here
    //       as addon is supposed to be already stopped right now.

    // NOTE: Addon is stopped either on whole firmware stop, or it is crashed
    //       and will be restarted soon enough, no need to bother

    optAddon->teardown();
}

void ExtensionsGraph::registerPlugin( std::unique_ptr< Plugin > pluginPtr ) {
    const auto& summary = pluginPtr->getBaseConfig().summary;
    auto& plugin = *pluginPtr;

    if( findRegisteredPlugin( summary.id ) ) {
        Log::error( cc::error( "Plugin already registered: " ) + cc::note( summary.toString() ) );
        return;
    }

    m_registeredPlugins.emplace( std::make_pair( summary.id, std::move( pluginPtr ) ) );

    for( auto& dependency : plugin.getBaseConfig().dependencies.requiredAddons ) {
        // If this is first time we see this dependency, try to find addons that satisfies it
        auto it = m_ADRequiringPlugins.find( dependency );
        if( it == m_ADRequiringPlugins.end() ) {
            for( auto& addonPair : m_registeredAddons ) {
                checkIfAddonSatisfiesDependency( *addonPair.second, dependency );
            }
        }

        m_ADRequiringPlugins[ dependency ].insert( &plugin );
    }

    for( auto& dependency : plugin.getBaseConfig().dependencies.requiredPlugins ) {
        // If this is first time we see this dependency, try to find plugin that satisfies it
        auto it = m_PDRequiringPlugins.find( dependency );
        if( it == m_PDRequiringPlugins.end() ) {
            for( auto& pluginPair : m_registeredPlugins ) {
                checkIfPluginSatisfiesDependency( *pluginPair.second, dependency );
            }
        }

        m_PDRequiringPlugins[ dependency ].insert( &plugin );
    }

    // Check if there are dependencies this plugin satisfies
    for( auto& dependencyPair : m_PDRequiringPlugins ) {
        checkIfPluginSatisfiesDependency( plugin, dependencyPair.first );
    }

    std::unordered_set< Plugin* > processedPlugins;
    tryStartPlugin( plugin, processedPlugins );
}

void ExtensionsGraph::unregisterPlugin( Plugin& plugin, bool uninstall ) {
    const auto& summary = plugin.getBaseConfig().summary;
    const auto& id = summary.id;

    if( uninstall ) {
        uninstallPlugin( plugin );
    }
    else {
        teardownPlugin( plugin );
    }

    for( auto& dependency : plugin.getBaseConfig().dependencies.requiredAddons ) {
        m_ADRequiringPlugins[ dependency ].erase( &plugin );
        if( m_ADRequiringPlugins[ dependency ].empty() )
            m_ADRequiringPlugins.erase( dependency );
    }
    for( auto& dependency : plugin.getBaseConfig().dependencies.requiredPlugins ) {
        m_PDRequiringPlugins[ dependency ].erase( &plugin );
        if( m_PDRequiringPlugins[ dependency ].empty() )
            m_PDRequiringPlugins.erase( dependency );
    }

    for( auto& dependency : m_pluginSatisfiedPDs[&plugin ] ) {
        m_PDSatisfyingPlugin.erase( dependency );
    }

    m_pluginSatisfiedPDs.erase( &plugin );

    m_registeredPlugins.erase( summary.id );
}

bool ExtensionsGraph::isAddonRegistered( const std::string& addonId ) const {
    return m_registeredAddons.find( addonId ) != m_registeredAddons.end();
}

const Addon* ExtensionsGraph::findRegisteredAddon( const std::string& addonId ) const {
    auto it = m_registeredAddons.find( addonId );
    if( it == m_registeredAddons.end() )
        return nullptr;

    return it->second.get();
}

Addon* ExtensionsGraph::findRegisteredAddon( const std::string& addonId ) {
    auto it = m_registeredAddons.find( addonId );
    if( it == m_registeredAddons.end() )
        return nullptr;

    return it->second.get();
}

bool ExtensionsGraph::isPluginRegistered( const std::string& pluginId ) const {
    return m_registeredPlugins.find( pluginId ) != m_registeredPlugins.end();
}

const Plugin* ExtensionsGraph::findRegisteredPlugin( const std::string& pluginId ) const {
    auto it = m_registeredPlugins.find( pluginId );
    if( it == m_registeredPlugins.end() )
        return nullptr;

    return it->second.get();
}

Plugin* ExtensionsGraph::findRegisteredPlugin( const std::string& pluginId ) {
    auto it = m_registeredPlugins.find( pluginId );
    if( it == m_registeredPlugins.end() )
        return nullptr;

    return it->second.get();
}

void ExtensionsGraph::checkIfAddonSatisfiesDependency( Addon& addon, AddonDependency dependency ) {
    if( addon.hasSummary() && dependency.isSatisfiedBy( addon.getSummary() ) ) {
        m_ADSatisfyingAddon.insert( std::make_pair( dependency, &addon ) );
        m_addonSatisfiedADs[&addon ].insert( std::move( dependency ) );
    }
}

void ExtensionsGraph::checkIfPluginSatisfiesDependency( Plugin& plugin, PluginDependency dependency ) {
    if( dependency.isSatisfiedBy( plugin.getBaseConfig().summary ) ) {
        m_PDSatisfyingPlugin.insert( std::make_pair( dependency, &plugin ) );
        m_pluginSatisfiedPDs[&plugin ].insert( std::move( dependency ) );
    }
}

Environment& ExtensionsGraph::getEnvironment() {
    return m_environment;
}

}  // namespace ExtensionsManager
}  // namespace HubPlatform
