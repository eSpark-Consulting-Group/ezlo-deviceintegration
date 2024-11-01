#include "luad/interpreter/lua_functions.h"
#include "luad/interpreter/module_loader/lua_module_loader.h"
#include "luad/common/exceptions.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "lua_utils/lua_functions.h"

extern "C" {
#include <lauxlib.h>
}

namespace HubPlatform {
namespace LuaFunctions {

namespace {
const char* const LuaModuleLoaderKey{ "__lua_module_loader" };

const char* const OriginalPrint{ "basic_original_print" };

const char* const OriginalLoadfile{ "basic_original_loadfile" };

const char* const OriginalRequire{ "package_original_require" };

void registerLuaModuleLoader( lua_State* luaState, LuaModuleLoader& luaModuleLoader ) {
    lua_pushstring( luaState, LuaModuleLoaderKey );
    lua_pushlightuserdata( luaState, &luaModuleLoader );
    lua_rawset( luaState, LUA_REGISTRYINDEX );
}

LuaModuleLoader& getLuaModuleLoader( lua_State* luaState ) {
    lua_pushstring( luaState, LuaModuleLoaderKey );
    lua_gettable( luaState, LUA_REGISTRYINDEX );
    auto info = static_cast< LuaModuleLoader* >( lua_touserdata( luaState, -1 ) );
    lua_pop( luaState, 1 );

    return *info;
}

int callOriginPrint( lua_State* luaState ) {
    const auto nargs = lua_gettop( luaState );

    lua_getglobal( luaState, OriginalPrint );
    const auto luaStackSize = lua_gettop( luaState );

    lua_insert( luaState, -luaStackSize );
    if( !lua_isfunction( luaState, -luaStackSize ) )
        throw OverriddenLuaFunctionError{ "fail to load an original print()" };

    if( lua_pcall( luaState, nargs, LUA_MULTRET, 0 ) != LUA_OK )
        throw OverriddenLuaFunctionError{ "fail to call an original print()." };

    return 0; /* We assume that for the print method, return value is zero */
}

int customPrint( lua_State* luaState ) {
    const auto luaStackSize = lua_gettop( luaState );
    if( luaStackSize < 1 ) {
        Log::error( "incorrect amount of arguments to print()" );
        return 0;
    }

    constexpr bool shouldPrintToLogger{ false };
    if( shouldPrintToLogger ) {
        std::string resultString;

        for( auto i = 1; i <= luaStackSize; ++i ) {
            size_t length;
            auto cPrintValue = lua_tolstring( luaState, i, &length );

            if( !cPrintValue ) {
                Log::error( "an unsupported value to print()" );
                return lua_gettop( luaState ) - luaStackSize;
            }

            resultString += std::string{ cPrintValue, length };
        }

        std::cout << resultString << std::endl;

        return 0; /* We assume that for the print method, return value is zero */
    }

    return callOriginPrint( luaState );
}

int customRequire( lua_State* luaState ) {
    const auto luaStackSize = lua_gettop( luaState );
    if( luaStackSize < 1 ) {
        Log::error( "incorrect amount of arguments to require()" );

        lua_pushnil( luaState );
        return 1;
    }

    size_t length;
    auto cModuleName = lua_tolstring( luaState, 1, &length );
    if( !cModuleName ) {
        Log::error( "an incorrect module name to require()" );

        lua_pushnil( luaState );
        return 1;
    }

    std::string moduleName{ cModuleName, length };

    auto& luaModuleLoader = getLuaModuleLoader( luaState );

    // load and execute script from plugin
    int result{ luaModuleLoader.loadExecutePluginScript( moduleName, luaStackSize - 1 ) };
    if( result != -1 ) {
        return result;
    }

    // ignore permissions for anonymous plugins
    if( !luaModuleLoader.isRunningAnonymousPlugin() && !luaModuleLoader.checkPluginPermission( moduleName ) ) {
        Log::error( cc::error( "plugin has no permissions to use the required module " ) + cc::note( moduleName ) );

        lua_pushnil( luaState );
        return 1;
    }

    if( luaModuleLoader.isProhibitedModuleForAnonymousPlugin( moduleName ) ) {
        Log::error( cc::error( "access to a " ) + cc::note( moduleName ) +
                    cc::error( " module is prohibited from anonymous plugins" ) );

        lua_pushnil( luaState );
        return 1;
    }

    result = luaModuleLoader.loadLibraryModule( moduleName, luaStackSize - 1 );
    if( result != -1 ) {
        return result;
    }

    /* a second and further call(s) to require() with the same module name
     * within one interpreter (one script) should return the same result (the same table) */
    if( luaModuleLoader.isModulePresent( moduleName ) ) {
        LOGGER_TRACE_RAW( "a module '" + moduleName + "' is already present" );
        return luaModuleLoader.getModule( moduleName );
    }
    LOGGER_TRACE_RAW( cc::trace( "try to load module '" ) + cc::note( moduleName ) +
                      cc::trace( "' as a dynamic module..." ) );

    /* at first try to load a dynamic module to give a shot to overload a base implementation */
    if( luaModuleLoader.loadDynamicLuaModule( moduleName ) ) {
        LOGGER_TRACE_RAW( cc::trace( "a dynamic module '" ) + cc::note( moduleName ) + cc::trace( "' has been " ) +
                          cc::success( "loaded" ) );
        return luaModuleLoader.getModule( moduleName );
    }
    LOGGER_TRACE_RAW( cc::trace( "try to load module '" ) + cc::note( moduleName ) +
                      cc::trace( "' as a base module..." ) );
    if( luaModuleLoader.loadBaseLuaModule( moduleName ) ) {
        LOGGER_TRACE_RAW( cc::trace( "a base module '" ) + cc::note( moduleName ) + cc::trace( "' has been " ) +
                          cc::success( "loaded" ) );
        return luaModuleLoader.getModule( moduleName );
    }

    Log::warn( cc::warn( "a module '" ) + cc::note( moduleName ) +
               cc::warn( "' can't be loaded by a custom require, use an original one..." ) );

    lua_getglobal( luaState, OriginalRequire );
    if( !lua_isfunction( luaState, -1 ) ) {
        Log::error( "fail to load an original require()." );
        lua_pop( luaState, lua_gettop( luaState ) - luaStackSize );
        lua_pushnil( luaState );
    }

    lua_pushstring( luaState, moduleName.c_str() );
    if( lua_pcall( luaState, 1, LUA_MULTRET, 0 ) != LUA_OK ) {
        Log::error( "fail to call an original require()." );
        // will return nil value in case of shared module could not be loaded
        lua_pop( luaState, lua_gettop( luaState ) - luaStackSize );
        lua_pushnil( luaState );
    }

    return lua_gettop( luaState ) - luaStackSize; /* return all original require()'s returns */
}

int customLoadfile( lua_State* luaState ) {
    auto& luaModuleLoader = getLuaModuleLoader( luaState );

    const std::string scriptName = luaL_checkstring( luaState, 1 );

    try {
        if( luaModuleLoader.isPluginScript( scriptName ) || luaModuleLoader.isAnonymousPluginScript( scriptName ) ) {
            luaModuleLoader.loadPluginScript( scriptName );
        }
        else {
            if( luaL_loadfile( luaState, scriptName.c_str() ) != LUA_OK ) {
                throw OverriddenLuaFunctionError{ std::string{ "fail to call luaL_loadfile: " } +
                                                  lua_tostring( luaState, -1 ) };
            }
        }
    }
    catch( const std::exception& e ) {
        lua_pushnil( luaState );
        lua_pushstring( luaState, e.what() );
        return 2;
    }

    return 1;
}

void overrideOriginalPrint( lua_State* luaState ) {
    lua_getglobal( luaState, "print" );
    lua_setglobal( luaState, OriginalPrint );

    lua_pushcfunction( luaState, customPrint );
    lua_setglobal( luaState, "print" );
}

void overrideOriginalRequire( lua_State* luaState ) {
    lua_getglobal( luaState, "require" );
    lua_setglobal( luaState, OriginalRequire );

    lua_pushcfunction( luaState, customRequire );
    lua_setglobal( luaState, "require" );
}

void overrideOriginalLoadfile( lua_State* luaState ) {
    lua_getglobal( luaState, "loadfile" );
    lua_setglobal( luaState, OriginalLoadfile );

    lua_pushcfunction( luaState, customLoadfile );
    lua_setglobal( luaState, "loadfile" );
}

void luaTraceback( lua_State* luaState, const char* message, int level = 1 ) {
    LuaUtils::luaTraceback( luaState, message, level );
}

// NOTE: based on https://github.com/lua/lua/blob/faaf7e481fbadc2ef520a96b638dd910f3c9ff14/lua.c#L111
int messageHandler( lua_State* luaState ) {
    return LuaUtils::messageHandler( luaState );
}

}  // namespace

void registerOverrideFunctions( lua_State* luaState, LuaModuleLoader& luaModuleLoader ) {
    registerLuaModuleLoader( luaState, luaModuleLoader );

    overrideOriginalPrint( luaState );
    overrideOriginalRequire( luaState );
    overrideOriginalLoadfile( luaState );
}

int protectedCall( lua_State* luaState, int nargs, int nresults ) {
    return LuaUtils::protectedCall( luaState, nargs, nresults );
}

}  // namespace LuaFunctions
}  // namespace HubPlatform
