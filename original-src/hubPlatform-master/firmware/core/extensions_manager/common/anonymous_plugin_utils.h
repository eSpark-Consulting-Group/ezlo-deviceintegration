#pragma once

#include "json_utils/json_fwd.h"
#include "jsonrpc/error_codes.h"
#include "database_client/idbclient.h"
#include "service_interfaces/service.h"

#include <string>
#include <vector>
#include <memory>
#include <istream>

namespace HubPlatform {
namespace ExtensionsManager {

struct AnonymousPlugin final {
    AnonymousPlugin();
    explicit AnonymousPlugin( const Json& plugin );

    bool operator!() const;
    Json toJson() const;

private:
    Json m_plugin;
};

struct AnonymousPluginError final {
    AnonymousPluginError();
    explicit AnonymousPluginError( Errors::Error errorCode );
    explicit AnonymousPluginError( const Json& errorJson );

    operator bool() const;
    bool operator!() const;
    Json toJson() const;
    Errors::Error getError() const;

private:
    Errors::Error m_errorCode;
};

class IAnonymousPluginUtils {
public:
    using PluginId = std::string;
    using ScriptId = std::string;
    using ScriptStreamUPtr = std::unique_ptr< std::istream >;
    using AnonymousPlugins = std::vector< AnonymousPlugin >;

    virtual ~IAnonymousPluginUtils();

    /* uid/scened related api */
    virtual PluginId addPlugin( const std::string& scriptCode, const std::string& scriptName ) = 0;
    virtual AnonymousPluginError changePlugin( const std::string& pluginId,
                                               const std::string& scriptCode,
                                               const std::string& scriptName ) = 0;
    virtual AnonymousPlugin getPlugin( const std::string& pluginId ) const = 0;
    virtual AnonymousPlugins listPlugins() const = 0;
    virtual AnonymousPluginError deletePlugin( const std::string& pluginId ) = 0;
    virtual AnonymousPluginError runPlugin( const std::string& pluginId ) = 0;

    virtual std::string getPluginScriptContent( const std::string& pluginId ) const = 0;
    virtual bool checkPluginCode( const std::string& scriptCode ) const = 0;
    virtual bool checkPluginName( const std::string& scriptName ) const = 0;
    virtual bool pluginExists( const std::string& pluginId ) const = 0;

    virtual bool isPluginAcquired( const std::string& pluginId ) = 0;
    virtual AnonymousPluginError acquirePlugin( const std::string& pluginId, bool shouldIncrementCount = false ) = 0;
    virtual void releasePlugin( const std::string& pluginId ) = 0;

    /* luad related api */
    virtual ScriptId getScriptIdByPluginId( const std::string& pluginId ) const = 0;
    virtual PluginId getPluginIdByScriptId( const std::string& scriptId ) const = 0;
    virtual bool isPluginScript( const std::string& scriptId ) const = 0;
    virtual ScriptStreamUPtr getLuaScript( const std::string& scriptId ) const = 0;
    virtual std::string getLuaScriptFilePath( const std::string& scriptId ) const = 0;
    virtual std::string getDataFilePath( const std::string& scriptId ) const = 0;
};

class AnonymousPluginUtils final : public IAnonymousPluginUtils {
public:
    AnonymousPluginUtils( Services::Service& service );

    /* uid/scened related api */
    PluginId addPlugin( const std::string& scriptCode, const std::string& scriptName ) override;
    AnonymousPluginError changePlugin( const std::string& pluginId,
                                       const std::string& scriptCode,
                                       const std::string& scriptName ) override;
    AnonymousPlugin getPlugin( const std::string& pluginId ) const override;
    AnonymousPlugins listPlugins() const override;
    AnonymousPluginError deletePlugin( const std::string& pluginId ) override;
    AnonymousPluginError runPlugin( const std::string& pluginId ) override;

    std::string getPluginScriptContent( const std::string& pluginId ) const override;
    bool checkPluginCode( const std::string& scriptCode ) const override;
    bool checkPluginName( const std::string& scriptName ) const override;
    bool pluginExists( const std::string& pluginId ) const override;

    bool isPluginAcquired( const std::string& pluginId ) override;
    AnonymousPluginError acquirePlugin( const std::string& pluginId, bool shouldIncrementCount = false ) override;
    void releasePlugin( const std::string& pluginId ) override;

    /* luad related api */
    ScriptId getScriptIdByPluginId( const std::string& pluginId ) const override;
    PluginId getPluginIdByScriptId( const std::string& scriptId ) const override;
    bool isPluginScript( const std::string& scriptId ) const override;
    ScriptStreamUPtr getLuaScript( const std::string& scriptId ) const override;
    std::string getLuaScriptFilePath( const std::string& scriptId ) const override;
    std::string getDataFilePath( const std::string& scriptId ) const override;

private:
    void removePlugin( const std::string& pluginId );

private:
    Services::Service& m_service;
    IDbClient& m_dbClient;
};

}  // namespace ExtensionsManager
}  // namespace HubPlatform
