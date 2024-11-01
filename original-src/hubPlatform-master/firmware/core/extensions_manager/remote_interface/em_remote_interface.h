#pragma once

#include "extensions_manager/common/anonymous_plugin_utils.h"
#include "extensions_manager/common/execution_policy.h"

#include "common_utils/addons/addon_summary.h"
#include "plugin_utils/plugin_utils.h"

#include "json_utils/json_fwd.h"
#include "jsonrpc/message.h"

#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <map>

namespace HubPlatform {
namespace Services {
class Service;
}
namespace ExtensionsManager {

namespace UT {
class RemoteInterfaceFactory;
}

class RemoteInterface final {
    friend class RemoteAnonymousPlugin;

public:
    using PluginUtilsUPtr = std::unique_ptr< PluginUtils >;
    using AnonymousPluginUtilsUPtr = std::unique_ptr< IAnonymousPluginUtils >;
    using AnonymousPluginId = std::string;
    using AnonymousPlugins = std::vector< AnonymousPlugin >;

    class RemoteAnonymousPlugin final {
        friend class RemoteInterface;
        RemoteAnonymousPlugin( RemoteInterface& remoteInterface, std::string pluginId );

    public:
        ~RemoteAnonymousPlugin();

    private:
        RemoteInterface& m_remoteInterface;
        const std::string m_pluginId;
    };

    using SharedRemoteAnonymousPlugin = std::shared_ptr< RemoteAnonymousPlugin >;

    RemoteInterface( Services::Service& hostService );

    JsonObject requestPluginRun( const std::string& scriptName,
                                 const Json& scriptParams,
                                 const std::string& operationId );

    Json requestConfigList();

    Json requestConfigPrioritySet( const std::string& pluginId, const std::vector< std::string >& priorities );

    // uid/scened api
    AnonymousPluginId addAnonymousPlugin( const std::string& scriptCode, const std::string& scriptName );
    AnonymousPluginError changeAnonymousPlugin( const std::string& pluginId,
                                                const std::string& scriptCode,
                                                const std::string& scriptName );
    AnonymousPlugin getAnonymousPluginContent( const std::string& pluginId );
    AnonymousPluginError deleteAnonymousPlugin( const std::string& pluginId );
    AnonymousPlugins listAnonymousPlugins();
    AnonymousPluginError runAnonymousPlugin( const std::string& pluginId );

    std::string getAnonymousScriptIdByPluginId( const std::string& pluginId ) const;

    ExecutionPolicy getExecutionPolicyByPluginId( const std::string& pluginId ) const;

    SharedRemoteAnonymousPlugin acquireAnonymousPlugin( const std::string& pluginId );

    JsonObject registerCustomPlugin( const std::string& componentId );
    JsonObject unregisterCustomPlugin( const std::string& componentId );
    JsonObject pauseCustomPlugin( const std::string& componentId );

    // LuaPluginSourceProvider
    bool isPluginScript( const std::string& scriptId ) const;
    bool isAnonymousPluginScript( const std::string& scriptId ) const;
    PluginUtils::StreamUPtr getLuaScript( const std::string& scriptId );

    /* @brief get lua script file path in filesystem, adds .lua extension */
    std::string getLuaScriptFilePath( const std::string& scriptId ) const;

    /* @brief get data file path in filesystem, doesn't add anything */
    std::string getDataFilePath( const std::string& scriptId ) const;

    /* @brief get an id of a plugin owning a script with @c scriptId id */
    std::string getPluginIdByScriptId( const std::string& scriptId ) const;

    /* @brief split a fileId in a (HUB:component_name/relative_path) notation to a component name and
     * a relative file path */
    PluginUtils::SplittedFileId splitFileId( const std::string& scriptId ) const;

    /* @brief check if a @c pluginId plugin has a @c permission permission (obtained from a plugin's config)*/
    bool checkPluginPermission( const std::string& pluginId, const std::string& permission ) const;

    /* @brief get template for device by @c deviceId */
    std::string getTemplateForDevice( const std::string& templateName );

    PluginUtils::StreamUPtr getPluginFile( const std::string& fileId ) const;

    Json getPluginExtensionsIds( const std::string& pluginId );

    Json getPluginExtensionData( const std::string& pluginId, const std::string& path, const std::string& filter );

    std::vector< JsonObject > fetchPluginsMetadata( const std::string& pluginFullName = "" );
    std::vector< JsonObject > fetchPluginsMetadataWithQuery( const JsonObject& query );
    void saveToDb( const JsonObject& meta );
    void removeFromDB( const JsonObject& obj );
    bool isPluginData( const std::string& fullName );
    void validatePluginData( std::istream& config,
                             std::istream& interface,
                             std::function< bool( std::string ) >& func );
    std::string setPluginData( std::istream& config,
                               std::istream& interface,
                               std::function< bool( std::string ) >& func );
    bool removePluginData( const std::string& fullName );
    bool checkId( const JsonObject& meta );

private:
    using RemoteAnonymousPluginsMap = std::map< std::string, std::weak_ptr< RemoteAnonymousPlugin > >;

    friend class UT::RemoteInterfaceFactory;
    RemoteInterface( Services::Service& hostService,
                     PluginUtilsUPtr pluginUtils,
                     AnonymousPluginUtilsUPtr anonymousPluginUtils );

    bool isAnonymousPluginAcquired( const std::string& pluginId );
    void releaseAnonymousPlugin( const std::string& pluginId );

private:
    Services::Service& m_hostService;
    PluginUtilsUPtr m_pluginUtils;
    AnonymousPluginUtilsUPtr m_anonymousPluginUtils;
    mutable std::mutex m_mtx;
    RemoteAnonymousPluginsMap m_remoteAnonymousPluginsMap;
};

}  // namespace ExtensionsManager
}  // namespace HubPlatform
