#pragma once

#include "extensions_manager/em_environment.h"
#include "common_utils/addons/addon_summary.h"
#include "extensions_manager/extensions/plugins/base_config.h"
#include "extensions_manager/extensions/extensions_graph.h"
#include "extensions_manager/extensions/plugins/plugins_factory.h"
#include "extensions_manager/extensions/plugins/plugin_extension_priorities_manager.h"

#include <unordered_set>
#include <mutex>

class IDbClient;

namespace HubPlatform {

namespace Services {
class Service;
}

namespace ExtensionsManager {

class AddonUtils;
class PluginUtils;
class Messenger;

class ExtensionsManager final : public Environment {
public:
    ExtensionsManager( AddonUtils& addonUtils,
                       PluginUtils& pluginUtils,
                       Messenger& messenger,
                       IDbClient& dbClient,
                       CoreDRemoteInterface::CoreDInterface& coreDInterface,
                       const UpdateManager::RemoteInterface& updateManager,
                       ScenesManager::BaseRemoteInterface& scenesManager );

    void setUp();

    void onAddonStarted( const CommonUtils::AddonSummary& addonSummary, int64_t setupId );
    void onAddonStopped( const CommonUtils::AddonSummary& addonSummary );

    void registerAddon( const std::string& addonId );
    void unregisterAddon( const std::string& addonId );

    bool isPluginRegistered( const std::string& pluginId );

    void registerPlugin( const std::string& pluginId );
    void unregisterPlugin( const std::string& pluginId );

    void uninstallPlugin( const std::string& pluginId );

    void updatePluginExtensionsPriorities( const std::string& pluginName, std::vector< std::string > list );

    std::vector< std::string > getPluginExtensions( const std::string& pluginId );

public:
    AddonUtils& getAddonUtils() override;
    PluginUtils& getPluginUtils() override;
    Messenger& getMessenger() override;
    IDbClient& getDbClient() override;

    const CoreDRemoteInterface::CoreDInterface& getCoreDInterface() const override;
    CoreDRemoteInterface::CoreDInterface& getCoreDInterface() override;

    const ScenesManager::BaseRemoteInterface& getScenesManagerInterface() const override;
    ScenesManager::BaseRemoteInterface& getScenesManagerInterface() override;

    bool isPluginUnavailable( const std::string& pluginId ) const override;

private:
    void scanForInstalledAddons();

private:
    void scanForInstalledPlugins();

    ExtensionsGraph& getExtensionsGraph();
    PluginExtensionPrioritiesManager& getPluginExtensionPrioritiesManager();

private:
    AddonUtils& m_addonUtils;
    PluginUtils& m_pluginUtils;
    Messenger& m_messenger;
    IDbClient& m_dbClient;
    CoreDRemoteInterface::CoreDInterface& m_coreDInterface;
    const UpdateManager::RemoteInterface& m_updateManager;
    ScenesManager::BaseRemoteInterface& m_scenesManager;

private:
    PluginsFactory m_pluginsFactory;
    std::unique_ptr< ExtensionsGraph > m_extensionsGraph;
    std::unique_ptr< PluginExtensionPrioritiesManager > m_pluginExtensionPrioritiesManager;

    std::mutex m_mutex;
};

}  // namespace ExtensionsManager
}  // namespace HubPlatform
