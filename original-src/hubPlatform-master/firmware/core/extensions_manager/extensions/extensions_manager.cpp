#include "extensions_manager/extensions/extensions_manager.h"
#include "extensions_manager/resources/em_resources.h"
#include "extensions_manager/common/addon_utils.h"
#include "extensions_manager/em_messenger.h"

#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"
#include "plugin_utils/plugin_utils.h"
#include "database_client/dbqueryparser.h"
#include "database_client/idbclient.h"
#include "service_interfaces/service.h"

#include <tiny/tiny_consolecolors.h>

#include <future>
#include <exception>

namespace HubPlatform {
namespace ExtensionsManager {

ExtensionsManager::ExtensionsManager( AddonUtils& addonUtils,
                                      PluginUtils& pluginUtils,
                                      Messenger& messenger,
                                      IDbClient& dbClient,
                                      CoreDRemoteInterface::CoreDInterface& coreDInterface,
                                      const UpdateManager::RemoteInterface& updateManager,
                                      ScenesManager::BaseRemoteInterface& scenesManager )
: m_addonUtils( addonUtils )
, m_pluginUtils( pluginUtils )
, m_messenger( messenger )
, m_dbClient( dbClient )
, m_coreDInterface( coreDInterface )
, m_updateManager( updateManager )
, m_scenesManager( scenesManager )
, m_pluginsFactory( *this ) {}

void ExtensionsManager::setUp() {
    std::lock_guard< std::mutex > lock( m_mutex );
    getExtensionsGraph();
}

void ExtensionsManager::scanForInstalledAddons() {
    Log::info( cc::info( "Scanning for addons..." ) );

    // TODO: use update manager to get installed addons packages list
    auto installedAddons = getAddonUtils().getInstalledAddons();
    if( installedAddons.empty() )
        Log::info( cc::info( "No addons found" ) );

    for( const auto& installedAddon : installedAddons ) {
        registerAddon( installedAddon );
    }
}

void ExtensionsManager::scanForInstalledPlugins() {
    getDbClient().clear( Resources::DBTableNames::Plugins );
    Log::info( cc::info( "Scanning for plugins..." ) );

    auto installedPlugins = getPluginUtils().getInstalledPlugins();
    if( installedPlugins.empty() )
        Log::info( cc::info( "No plugins found" ) );

    for( const auto& installedPlugin : installedPlugins ) {
        registerPlugin( installedPlugin );
    }

    auto customPlugins = getPluginUtils().getCustomPlugins();
    if( customPlugins.empty() )
        Log::info( cc::info( "No custom plugins found" ) );

    for( const auto& customPlugin : customPlugins ) {
        registerPlugin( customPlugin );
    }
}

void ExtensionsManager::registerAddon( const std::string& addonId ) {
    Log::info( cc::info( "Registering addon: " ) + cc::note( addonId ) );
    auto& addon = getExtensionsGraph().registerAddon( addonId );

    getMessenger().sendPingToAddon( addonId );
}

void ExtensionsManager::unregisterAddon( const std::string& addonId ) {
    Log::info( cc::info( "Unregistering addon: " ) + cc::note( addonId ) );
    if( auto addonPtr = getExtensionsGraph().findRegisteredAddon( addonId ) )
        getExtensionsGraph().unregisterAddon( *addonPtr );
}

bool ExtensionsManager::isPluginRegistered( const std::string& pluginId ) {
    return getExtensionsGraph().findRegisteredPlugin( pluginId ) != nullptr;
}

void ExtensionsManager::registerPlugin( const std::string& pluginId ) {
    Log::info( cc::info( "Try to register plugin: " ) + cc::note( pluginId ) );

    auto configFile = getPluginUtils().getInternalPluginFile( pluginId, "config.json" );
    if( !configFile ) {
        Log::error( cc::error( "Plugin registrar: Could not open plugin config file " ) + cc::note( pluginId ) );
        return;
    }

    auto joConfig = JSON_EMPTY_OBJ;

    try {
        joConfig = Json::parse( *configFile );
    }
    catch( const std::exception& exception ) {
        Log::error( cc::error( "Plugin registrar: Fail to parse a plugin's config file: " ) +
                    cc::note( exception.what() ) );
        return;
    }

    if( isPluginUnavailable( pluginId ) ) {
        Log::warn( cc::warn( "Plugin registrar: Can not register plugin, because of pending update operation" ) );
        return;
    }

    if( auto plugin = m_pluginsFactory.createPlugin( joConfig ) ) {
        Log::info( cc::info( "Registering plugin: " ) + cc::note( plugin->getBaseConfig().summary.toString() ) );
        getPluginExtensionPrioritiesManager().registerPlugin( *plugin );
        getExtensionsGraph().registerPlugin( std::move( plugin ) );
    }
}

void ExtensionsManager::unregisterPlugin( const std::string& pluginId ) {
    Log::info( cc::info( "Unregistering plugin: " ) + cc::note( pluginId ) );
    if( auto pluginPtr = getExtensionsGraph().findRegisteredPlugin( pluginId ) )
        getExtensionsGraph().unregisterPlugin( *pluginPtr );
}

void ExtensionsManager::uninstallPlugin( const std::string& pluginId ) {
    Log::info( cc::info( "Removing plugin: " ) + cc::note( pluginId ) );
    if( auto pluginPtr = getExtensionsGraph().findRegisteredPlugin( pluginId ) ) {
        getPluginExtensionPrioritiesManager().unregisterPlugin( *pluginPtr );
        getExtensionsGraph().unregisterPlugin( *pluginPtr, /*uninstall*/ true );
    }
}

void ExtensionsManager::updatePluginExtensionsPriorities( const std::string& pluginName,
                                                          std::vector< std::string > list ) {
    const auto& graph = getExtensionsGraph();
    std::remove_if( list.begin(), list.end(), [&graph, &pluginName]( const std::string& extensionPluginName ) {
        const auto* extensionPlugin = graph.findRegisteredPlugin( extensionPluginName );
        if( !extensionPlugin ) {
            return true;
        }

        return !extensionPlugin->hasExtensionFor( pluginName );
    } );

    getPluginExtensionPrioritiesManager().changePluginExtensionsPriorities( pluginName, list );
}

std::vector< std::string > ExtensionsManager::getPluginExtensions( const std::string& pluginId ) {
    auto extensions = getPluginExtensionPrioritiesManager().getPluginExtensions( pluginId );
    extensions.erase( std::remove_if( extensions.begin(),
                                      extensions.end(),
                                      [this, &pluginId]( const std::string& extensionId ) {
                                          const auto* extensionPlugin =
                                              getExtensionsGraph().findRegisteredPlugin( extensionId );
                                          return !( extensionPlugin && extensionPlugin->hasExtensionFor( pluginId ) );
                                      } ),
                      extensions.end() );

    return extensions;
}

void ExtensionsManager::onAddonStarted( const CommonUtils::AddonSummary& addonSummary, int64_t setupId ) {
    std::lock_guard< std::mutex > lock( m_mutex );

    getExtensionsGraph().onAddonStarted( addonSummary, setupId );
}

void ExtensionsManager::onAddonStopped( const CommonUtils::AddonSummary& addonSummary ) {
    std::lock_guard< std::mutex > lock( m_mutex );

    getExtensionsGraph().onAddonStopped( addonSummary );
}

ExtensionsGraph& ExtensionsManager::getExtensionsGraph() {
    if( !m_extensionsGraph ) {
        m_extensionsGraph = std::make_unique< ExtensionsGraph >( *this );

        scanForInstalledAddons();
        scanForInstalledPlugins();
    }

    return *m_extensionsGraph;
}

PluginExtensionPrioritiesManager& ExtensionsManager::getPluginExtensionPrioritiesManager() {
    if( !m_pluginExtensionPrioritiesManager ) {
        m_pluginExtensionPrioritiesManager = std::make_unique< PluginExtensionPrioritiesManager >( *this );
    }

    return *m_pluginExtensionPrioritiesManager;
}

AddonUtils& ExtensionsManager::getAddonUtils() {
    return m_addonUtils;
}

PluginUtils& ExtensionsManager::getPluginUtils() {
    return m_pluginUtils;
}

Messenger& ExtensionsManager::getMessenger() {
    return m_messenger;
}

IDbClient& ExtensionsManager::getDbClient() {
    return m_dbClient;
}

const CoreDRemoteInterface::CoreDInterface& ExtensionsManager::getCoreDInterface() const {
    return m_coreDInterface;
}

CoreDRemoteInterface::CoreDInterface& ExtensionsManager::getCoreDInterface() {
    return m_coreDInterface;
}

const ScenesManager::BaseRemoteInterface& ExtensionsManager::getScenesManagerInterface() const {
    return m_scenesManager;
}

ScenesManager::BaseRemoteInterface& ExtensionsManager::getScenesManagerInterface() {
    return m_scenesManager;
}

bool ExtensionsManager::isPluginUnavailable( const std::string& pluginId ) const {
    return m_updateManager.isPluginUnavailable( pluginId );
}

}  // namespace ExtensionsManager
}  // namespace HubPlatform
