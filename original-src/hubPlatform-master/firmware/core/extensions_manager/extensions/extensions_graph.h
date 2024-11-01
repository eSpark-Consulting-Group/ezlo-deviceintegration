#pragma once

#include "extensions_manager/extensions/addon.h"
#include "extensions_manager/extensions/plugins/base_config.h"
#include "extensions_manager/extensions/plugins/gateway/gateway_config.h"
#include "extensions_manager/extensions/plugins/plugin.h"
#include "plugin_utils/plugin_summary.h"

#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace HubPlatform {
namespace ExtensionsManager {

class Environment;

class ExtensionsGraph final {
public:
    ExtensionsGraph( Environment& environment );

    Addon& registerAddon( const std::string& addonId );
    void unregisterAddon( Addon& addon );

    void onAddonStarted( const CommonUtils::AddonSummary& addonSummary, int64_t setupId );
    void onAddonStopped( const CommonUtils::AddonSummary& addonSummary );

    void registerPlugin( std::unique_ptr< Plugin > pluginPtr );
    void unregisterPlugin( Plugin& plugin, bool uninstall = false );

public:
    bool isAddonRegistered( const std::string& addonId ) const;
    const Addon* findRegisteredAddon( const std::string& addonId ) const;
    Addon* findRegisteredAddon( const std::string& addonId );

    bool isPluginRegistered( const std::string& pluginId ) const;
    const Plugin* findRegisteredPlugin( const std::string& pluginId ) const;
    Plugin* findRegisteredPlugin( const std::string& pluginId );

private:
    bool canStartPlugin( Plugin& plugin ) const;
    void tryStartPlugin( Plugin& plugin, std::unordered_set< Plugin* >& processedPlugins );
    void teardownPlugin( Plugin& plugin );
    void uninstallPlugin( Plugin& plugin );
    std::vector< Plugin* > getPluginsTeardownOrder( Plugin& plugin );

    using PluginPtrs = std::vector< Plugin* >;
    void addDependentPlugins( PluginPtrs& plugins, const Plugin& plugin ) const;

    void checkIfAddonSatisfiesDependency( Addon& addon, AddonDependency dependency );
    void checkIfPluginSatisfiesDependency( Plugin& plugin, PluginDependency dependency );

    Environment& getEnvironment();

private:
    Environment& m_environment;

private:
    std::unordered_map< std::string, std::unique_ptr< Addon > > m_registeredAddons;
    std::unordered_map< std::string, std::unique_ptr< Plugin > > m_registeredPlugins;

    std::unordered_map< AddonDependency, std::unordered_set< Plugin* > > m_ADRequiringPlugins;
    std::unordered_map< PluginDependency, std::unordered_set< Plugin* > > m_PDRequiringPlugins;

    std::unordered_map< AddonDependency, const Addon* > m_ADSatisfyingAddon;
    std::unordered_map< const Addon*, std::unordered_set< AddonDependency > > m_addonSatisfiedADs;

    std::unordered_map< PluginDependency, const Plugin* > m_PDSatisfyingPlugin;
    std::unordered_map< const Plugin*, std::unordered_set< PluginDependency > > m_pluginSatisfiedPDs;
};

}  // namespace ExtensionsManager
}  // namespace HubPlatform
