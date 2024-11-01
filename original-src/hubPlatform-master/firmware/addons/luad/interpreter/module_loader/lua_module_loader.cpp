#include "luad/interpreter/module_loader/lua_module_loader.h"
#include "luad/interpreter/lua_functions.h"

#include "binding_interfaces/lua/lua_module_interface.h"
#include "addon_interfaces/addon_defs.h"
#include "addon_interfaces/addon.h"
#include "log_utils/ezlo_log.h"
#include "system_utils/environment.h"

extern "C" {
#include <lualib.h>
#include <lauxlib.h>
}

#include <cassert>
#include <dlfcn.h>
#include <tuple>
#include <set>

namespace HubPlatform {

std::map< LuaModuleLoader::DlName, std::shared_ptr< DlHandle > > LuaModuleLoader::m_dynamicModules;
std::mutex LuaModuleLoader::m_dynamicModulesMtx;
LuaModuleLoader::Libraries LuaModuleLoader::m_libraries;
std::mutex LuaModuleLoader::m_librariesMtx;

LuaModuleLoader::LuaModuleLoader( lua_State* luaState,
                                  LuaPluginResourcesProvider& pluginResourcesProvider,
                                  Addons::Addon& addon )
: m_luaState( luaState )
, m_pluginResourcesProvider( pluginResourcesProvider ) {
    LuaFunctions::registerOverrideFunctions( luaState, *this );
    initLuaBindingInterface( luaState, &addon );
}

void LuaModuleLoader::saveModule( const std::string& moduleName, int moduleAbsIndex ) {
    lua_getglobal( m_luaState, "package" );
    lua_getfield( m_luaState, -1, "loaded" );

    lua_pushvalue( m_luaState, moduleAbsIndex );
    lua_setfield( m_luaState, -2, moduleName.c_str() );

    lua_pop( m_luaState, 2 );
}

void LuaModuleLoader::getModuleUnsafe( const std::string& moduleName ) {
    lua_getglobal( m_luaState, "package" );
    lua_getfield( m_luaState, -1, "loaded" );
    lua_getfield( m_luaState, -1, moduleName.c_str() );
}

bool LuaModuleLoader::isModulePresent( const std::string& moduleName ) {
    getModuleUnsafe( moduleName );

    bool res = lua_istable( m_luaState, -1 ) ? true : false;
    lua_pop( m_luaState, 3 );

    return res;
}

int LuaModuleLoader::getModule( const std::string& moduleName ) {
    getModuleUnsafe( moduleName );
    lua_remove( m_luaState, -2 );
    lua_remove( m_luaState, -2 );

    return 1; /* leave a module table on the stack */
}

std::string LuaModuleLoader::getModuleFullName( const std::string& moduleName ) {
    return Environment::getLibrariesFolder() + "/" + Addons::LibPrefix + "lua_" + moduleName + Addons::LibSuffix;
}

bool LuaModuleLoader::isLibrary( const std::string& moduleName ) {
    std::lock_guard< std::mutex > lock( m_librariesMtx );
    auto it = m_libraries.find( moduleName );
    if( it == m_libraries.end() ) {
        return false;
    }
    return true;
}

LuaModuleLoader::Library LuaModuleLoader::findLibrary( const std::string& moduleName ) {
    std::lock_guard< std::mutex > lock( m_librariesMtx );
    auto it = m_libraries.find( moduleName );
    if( it == m_libraries.end() ) {
        return Library();
    }
    return it->second;
}

namespace {

void* locateDynamicModuleSymbol( DlHandle& handle, const std::string& symbol ) {
    auto locatedSymbol = dlsym( handle.getDlHandle(), symbol.c_str() );
    if( !locatedSymbol ) {
        LOGGER_DEBUG_RAW( std::string( "dlerror: " ) + dlerror() );
    }

    return locatedSymbol;
}

}  // namespace

bool LuaModuleLoader::loadBaseLuaModule( const std::string& moduleName ) {
    auto mainHandle = loadDynamicModule( "", RTLD_NOW );
    assert( mainHandle );

    auto moduleEntry = reinterpret_cast< ModuleEntryPointType >(
        locateDynamicModuleSymbol( *mainHandle, moduleName + ModuleEntryPoint ) );
    if( !moduleEntry )
        return false;

    const auto luaAPIRestrictions = getLuaModuleAPIRestrictions();
    LOGGER_TRACE_RAW( cc::trace( "load a " ) + cc::note( moduleName ) + cc::trace( " base module with a " ) +
                      cc::note( getHubAPIRestrictionsString( luaAPIRestrictions ) ) +
                      cc::trace( " Lua API restrictions" ) );

    if( !moduleEntry( m_luaState, luaAPIRestrictions ) )
        return false;

    /*  a lua module left one table on the stack */
    saveModule( moduleName, lua_absindex( m_luaState, -1 ) );
    lua_pop( m_luaState, 1 );

    return true;
}

bool LuaModuleLoader::loadDynamicLuaModule( const std::string& moduleName ) {
    const auto moduleFullName = getModuleFullName( moduleName );

    auto moduleHandle = loadDynamicModule( moduleFullName, RTLD_LAZY );
    if( !moduleHandle )
        return false;

    auto moduleEntry =
        reinterpret_cast< ModuleEntryPointType >( locateDynamicModuleSymbol( *moduleHandle, ModuleEntryPoint ) );
    if( !moduleEntry )
        return false;

    const auto luaAPIRestrictions = getLuaModuleAPIRestrictions();
    LOGGER_TRACE_RAW( cc::trace( "load a " ) + cc::note( moduleName ) + cc::trace( " dynamic module with a " ) +
                      cc::note( getHubAPIRestrictionsString( luaAPIRestrictions ) ) +
                      cc::trace( " Lua API restrictions" ) );

    /* let an API module (lua binding) to make it work - create a module */
    if( !moduleEntry( m_luaState, luaAPIRestrictions ) )
        return false;

    /*  a lua module left one table on the stack */
    saveModule( moduleName, lua_absindex( m_luaState, -1 ) );
    lua_pop( m_luaState, 1 );

    return true;
}

std::shared_ptr< DlHandle > LuaModuleLoader::loadDynamicModule( const std::string& moduleName, int options ) {
    std::lock_guard< std::mutex > lock( m_dynamicModulesMtx );

    auto it = m_dynamicModules.find( moduleName );
    if( it != m_dynamicModules.end() )
        return it->second;

    auto dlHandle = std::make_shared< DlHandle >( moduleName.c_str(), options );
    if( !dlHandle->valid() )
        return nullptr;

    auto res = m_dynamicModules.emplace( std::make_pair( moduleName, std::move( dlHandle ) ) );

    return res.first->second;
}

namespace {
const char* readFromStream( lua_State* luaState, void* streamPtr, size_t* size ) {
    assert( streamPtr && size );

    constexpr auto bufSize = 512U;
    static char buf[ bufSize ];

    auto& stream = *static_cast< std::istream* >( streamPtr );
    *size = stream.readsome( buf, bufSize );
    return buf;
}
}  // namespace

bool LuaModuleLoader::isPluginScript( const std::string& scriptName ) const {
    return m_pluginResourcesProvider.isPluginScript( scriptName );
}

bool LuaModuleLoader::isAnonymousPluginScript( const std::string& scriptName ) const {
    return m_pluginResourcesProvider.isAnonymousPluginScript( scriptName );
}

void LuaModuleLoader::loadPluginScript( const std::string& scriptName ) {
    if( !isPluginScript( scriptName ) && !isAnonymousPluginScript( scriptName ) ) {
        throw LuaModuleLoaderError{ std::string{ "Script '" } + scriptName +
                                    "' requested to load isn't a plugin's script" };
    }

    /* prohibit loading of anonymous plugins/scripts from no-anonymous plugins/scripts */
    if( !isRunningAnonymousPlugin() && isAnonymousPluginScript( scriptName ) ) {
        throw LuaModuleLoaderError(
            ( "Failed to load an anonymous \"" + scriptName + "\" plugin/script, from a regular plugin" ).c_str() );
    }

    const auto scriptStream = m_pluginResourcesProvider.getScript( scriptName );
    if( !scriptStream ) {
        throw LuaModuleLoaderError{ std::string{ "Couldn't find a plugin's script '" } + scriptName + "'" };
    }

#if defined( __HUB_TYPE_LOCAL__ )
    const auto scriptFilePath = m_pluginResourcesProvider.getScriptFilePath( scriptName );
#else
    const auto scriptFilePath = scriptName;
#endif

    if( lua_load( m_luaState, readFromStream, scriptStream.get(), scriptFilePath.c_str(), nullptr ) ) {
        throw LuaModuleLoaderError{ std::string{ "Couldn't load a plugin's script '" } + scriptName + ": " +
                                    lua_tostring( m_luaState, -1 ) };
    }

    LOGGER_DEBUG_RAW( cc::debug( "A plugin's script " ) + cc::p( scriptName ) + cc::debug( " has been loaded" ) );
}

int LuaModuleLoader::loadExecutePluginScript( const std::string& scriptName, const int nargs ) {
    if( isPluginScript( scriptName ) || isAnonymousPluginScript( scriptName ) ) {
        const auto luaStackSize = lua_gettop( m_luaState );

        /* it's not a problem which causes whole lua interpreter to be stopped when we
         * try to load plugin's script handling 'require' method, so we have to not
         * propagate exceptions, but just push nil onto lua stack */
        try {
            loadPluginScript( scriptName );

            assert( luaStackSize > nargs );
            for( auto i = luaStackSize - nargs + 1; i <= luaStackSize; ++i ) {
                lua_pushvalue( m_luaState, i );
            }

            if( LuaFunctions::protectedCall( m_luaState, nargs, LUA_MULTRET ) ) {
                Log::error( "Couldn't execute script '" + scriptName + ": " + lua_tostring( m_luaState, -1 ) );
                lua_pop( m_luaState, lua_gettop( m_luaState ) - luaStackSize );
                lua_pushnil( m_luaState );
            }
        }
        catch( const std::exception& exception ) {
            LOGGER_DEBUG_RAW( exception.what() );
            lua_pushnil( m_luaState );
        }

        return lua_gettop( m_luaState ) - luaStackSize;
    }
    return -1;
}

int LuaModuleLoader::loadLibraryModule( const std::string& moduleName, int nargs ) {
    auto library = findLibrary( moduleName );
    if( library.entryPoint.empty() ) {
        return -1;
    }
    return loadExecutePluginScript( library.entryPoint, nargs );
}

void LuaModuleLoader::setPluginId( const std::string& runningScriptName ) {
    const auto pluginId = m_pluginResourcesProvider.getPluginIdByScriptId( runningScriptName );
    HubPlatform::setPluginId( m_luaState, pluginId );
}

bool LuaModuleLoader::checkPluginPermission( const std::string& moduleToCheck ) const {
    return m_pluginResourcesProvider.checkPluginPermission( getPluginId( m_luaState ), moduleToCheck );
}

bool LuaModuleLoader::addLibrary( const LibraryName& libraryName, const Library& library ) {
    std::lock_guard< std::mutex > lock( m_librariesMtx );
    return m_libraries.insert( { libraryName, library } ).second;
}

void LuaModuleLoader::removeLibrary( const LibraryName& libraryName ) {
    std::lock_guard< std::mutex > lock( m_librariesMtx );
    m_libraries.erase( libraryName );
}

void LuaModuleLoader::setRunningAnonymousPlugin() {
    HubPlatform::setRunningAnonymousPlugin( m_luaState );
}

bool LuaModuleLoader::isRunningAnonymousPlugin() const {
    return HubPlatform::isRunningAnonymousPlugin( m_luaState );
}

void LuaModuleLoader::setLuaModuleAPIRestrictions( HubAPIRestrictions luaAPIRestrictions ) {
    LOGGER_DEBUG_RAW( cc::debug( "Set a " ) + cc::note( getHubAPIRestrictionsString( luaAPIRestrictions ) ) +
                      cc::debug( " Lua API restrictions" ) );
    HubPlatform::setLuaModuleAPIRestrictions( m_luaState, luaAPIRestrictions );
}

HubAPIRestrictions LuaModuleLoader::getLuaModuleAPIRestrictions() const {
    return HubPlatform::getLuaModuleAPIRestrictions( m_luaState );
}

bool LuaModuleLoader::isProhibitedModuleForAnonymousPlugin( const std::string& moduleName ) const {
    if( !isRunningAnonymousPlugin() ) {
        return false;
    }

    static std::set< std::string > prohibitedModules = { "http", "timer" };

    return prohibitedModules.find( moduleName ) != prohibitedModules.end();
}

} /* namespace HubPlatform */
