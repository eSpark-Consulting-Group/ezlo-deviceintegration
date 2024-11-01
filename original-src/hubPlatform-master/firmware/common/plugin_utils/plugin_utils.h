#pragma once

#include "plugin_utils/plugin_summary.h"
#include "plugin_utils/plugin_version.h"

#include <vector>
#include <fstream>

class IDbClient;

namespace HubPlatform {
namespace ExtensionsManager {

class PluginUtils {
public:
    using StreamUPtr = std::unique_ptr< std::istream >;
    static const char* m_pluginScriptsPrefix;

    virtual ~PluginUtils() = default;

    virtual std::string constructScriptName( const std::string& pluginId, const std::string& internalFileName ) const;

    using PluginsIds = std::vector< std::string >;
    virtual PluginsIds getInstalledPlugins() const;
    virtual PluginsIds getCustomPlugins() const;

    virtual StreamUPtr getInternalPluginTemplate( const std::string& templateName ) const;
    virtual StreamUPtr getInternalPluginLuaScriptFile( const std::string& scriptId ) const;
    virtual StreamUPtr getInternalPluginFile( const std::string& pluginId, const std::string& internalFilePath ) const;
    virtual StreamUPtr getInternalPluginFileById( const std::string& fileId ) const;

    std::string extractPluginId( const std::string& scriptId ) const;

    std::string getLuaScriptFilePath( const std::string& scriptId ) const;

    std::string getFilePathById( const std::string& fileId ) const;

    void checkScriptFilePath( std::string& scriptName ) const;

    using SplittedFileId = std::pair< std::string, std::string >;
    SplittedFileId splitScriptId( const std::string& scriptId ) const;

private:
    PluginsIds getPluginsFrom( const std::string& pluginsFolderPath ) const;

    StreamUPtr getInternalPluginFile( const std::string& internalFilePath ) const;

    std::string getPluginDirectory( const std::string& pluginId ) const;
};

}  // namespace ExtensionsManager
}  // namespace HubPlatform
