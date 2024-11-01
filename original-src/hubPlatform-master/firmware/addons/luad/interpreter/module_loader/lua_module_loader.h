#pragma once

#include "luad/interpreter/module_loader/dl_handle.h"
#include "binding_interfaces/common/hub_api_restrictions.h"
#include "jsonrpc/jsonrpc.h"

extern "C" {
#include <lua.h>
}

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <stdexcept>

namespace HubPlatform {

class LuaModuleLoaderError : public std::runtime_error {
public:
    LuaModuleLoaderError( const std::string& errDescription )
    : runtime_error( "LuaModuleLoader: " + errDescription ) {}
};

class LuaPluginResourcesProvider {
public:
    virtual ~LuaPluginResourcesProvider() = default;

    virtual bool isPluginScript( const std::string& scriptId ) const = 0;
    virtual bool isAnonymousPluginScript( const std::string& scriptId ) const = 0;
    virtual std::unique_ptr< std::istream > getScript( const std::string& scriptId ) = 0;
    virtual std::string getScriptFilePath( const std::string& scriptId ) const = 0;
    virtual std::string getDataFilePath( const std::string& scriptId ) const = 0;
    virtual std::string getPluginIdByScriptId( const std::string& scriptId ) const = 0;
    virtual bool checkPluginPermission( const std::string& pluginId, const std::string& permission ) const = 0;
};

namespace Addons {
class Addon;
}

class LuaModuleLoader final {
public:
    using LibraryName = std::string;
    struct Library {
        std::string entryPoint;
    };
    using Libraries = std::map< LibraryName, Library >;

    LuaModuleLoader( lua_State* luaState, LuaPluginResourcesProvider& pluginResourcesProvider, Addons::Addon& addon );

    bool isModulePresent( const std::string& moduleName );
    int getModule( const std::string& moduleName );
    bool loadBaseLuaModule( const std::string& moduleName );
    bool loadDynamicLuaModule( const std::string& moduleName );

    bool isPluginScript( const std::string& scriptName ) const;
    bool isAnonymousPluginScript( const std::string& scriptName ) const;
    void loadPluginScript( const std::string& scriptName );
    int loadExecutePluginScript( const std::string& scriptName, const int nargs = 0 );
    int loadLibraryModule( const std::string& moduleName, int nargs );

    /* @brief set a plugin id of a plugin owning a script with @c runningScriptName name to the lua_state */
    void setPluginId( const std::string& runningScriptName );

    /* @brief mark running plugin/lua_state as an anonymous one */
    void setRunningAnonymousPlugin();
    bool isRunningAnonymousPlugin() const;

    void setLuaModuleAPIRestrictions( HubAPIRestrictions luaAPIRestrictions );
    HubAPIRestrictions getLuaModuleAPIRestrictions() const;

    /* @brief check if the current script (plugin) has a right to load a @c moduleName module */
    bool checkPluginPermission( const std::string& moduleName ) const;

    bool isProhibitedModuleForAnonymousPlugin( const std::string& moduleName ) const;

    static bool addLibrary( const LibraryName& libraryName, const Library& library );
    static void removeLibrary( const LibraryName& libraryName );

private:
    void saveModule( const std::string& moduleName, int moduleAbsIndex );
    void getModuleUnsafe( const std::string& moduleName ); /* it doesn't clean a lua stack */
    std::string getModuleFullName( const std::string& moduleName );
    bool isLibrary( const std::string& moduleName );
    Library findLibrary( const std::string& moduleName );

    std::shared_ptr< DlHandle > loadDynamicModule( const std::string& moduleName, int options = 0 );

private:
    lua_State* m_luaState; /* this class doesn't own lua_state, just refer */
    LuaPluginResourcesProvider& m_pluginResourcesProvider;

    using DlName = std::string;
    static std::map< DlName, std::shared_ptr< DlHandle > > m_dynamicModules;
    static std::mutex m_dynamicModulesMtx;
    static std::map< LibraryName, Library > m_libraries;
    static std::mutex m_librariesMtx;
};

} /* namespace HubPlatform */
