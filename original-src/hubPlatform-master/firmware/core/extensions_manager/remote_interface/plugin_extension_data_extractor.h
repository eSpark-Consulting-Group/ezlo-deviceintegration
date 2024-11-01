#pragma once

#include "extensions_manager/extensions/plugins/plugin.h"

#include <unordered_map>

class IDbClient;

namespace HubPlatform {
namespace ExtensionsManager {

class PluginUtils;
class ExtensionsGraph;

class PluginExtensionDataExtractor final {
public:
    PluginExtensionDataExtractor( IDbClient& dbClient, PluginUtils& pluginUtils );

    Json extractData( const std::string& pluginId, const std::string& path, const std::string& filter );

private:
    std::vector< std::string > split( const std::string& path, char delimeter = '/' ) const;

    std::string getExtensionFilePath( const std::string& extensionPluginId,
                                      const std::string& extendedPluginId,
                                      const std::string& extensionName );

    Json filter( const Json& field, const std::string& filter );

    Json getDefaultResult( const std::string& filter ) const;
    bool isNumber( const std::string& s );

private:
    IDbClient& m_dbClient;
    PluginUtils& m_pluginUtils;
};

}  // namespace ExtensionsManager
}  // namespace HubPlatform
