#pragma once

#include "extensions_manager/extensions/plugins/plugin.h"

#include <unordered_map>

namespace HubPlatform {
namespace ExtensionsManager {

class Environment;

class PluginExtensionPrioritiesManager final {
public:
    PluginExtensionPrioritiesManager( Environment& environment );

    void registerPlugin( const Plugin& plugin );
    void unregisterPlugin( const Plugin& plugin );

    std::vector< std::string > getPluginExtensions( const std::string& pluginName );

    void changePluginExtensionsPriorities( const std::string& pluginName, const std::vector< std::string >& list );

private:
    Environment& m_environment;
};

}  // namespace ExtensionsManager
}  // namespace HubPlatform
