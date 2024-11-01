#include "interpreter.h"
#include "binding_interfaces/lua/lua_binding_interface.h"
#include "log_utils/ezlo_log.h"
#include "lua_utils/lua_functions.h"
#include "tiny/tiny_consolecolors.h"

extern "C" {
#include <lualib.h>
#include <lauxlib.h>
}

namespace HubPlatform {
namespace ScenesExpressions {

namespace {
static const luaL_Reg loadedlibs[] = { { "_G", luaopen_base },
                                       //{LUA_LOADLIBNAME, luaopen_package},
                                       //{LUA_COLIBNAME, luaopen_coroutine},
                                       { LUA_TABLIBNAME, luaopen_table },
                                       //{LUA_IOLIBNAME, luaopen_io},
                                       //{LUA_OSLIBNAME, luaopen_os},
                                       { LUA_STRLIBNAME, luaopen_string },
                                       { LUA_MATHLIBNAME, luaopen_math },
                                       { LUA_UTF8LIBNAME, luaopen_utf8 },
                                       //{LUA_DBLIBNAME, luaopen_debug},
                                       { NULL, NULL } };

void loadSafeLibs( lua_State* L ) {
    const luaL_Reg* lib;
    /* "require" functions from 'loadedlibs' and set results to global table */
    for( lib = loadedlibs; lib->func; lib++ ) {
        luaL_requiref( L, lib->name, lib->func, 1 );
        lua_pop( L, 1 ); /* remove lib */
    }
}

void removeGlobal( lua_State* L, const char* name ) {
    lua_pushnil( L );
    lua_setglobal( L, name );
}
}  // namespace

ScenesLuaInterpreter::ScenesLuaInterpreter()
: m_luaState{ luaL_newstate(), lua_close } {
    if( !m_luaState )
        throw ExpressionsError( "Could not create lua state" );

    // Do not use environment variables
    lua_pushboolean( m_luaState.get(), 1 ); /* signal for libraries to ignore env. vars. */
    lua_setfield( m_luaState.get(), LUA_REGISTRYINDEX, "LUA_NOENV" );

    // load default global libraries
    loadSafeLibs( m_luaState.get() );

    // overwrite global functions require, loadfile, dofile
    removeGlobal( m_luaState.get(), "require" );
    removeGlobal( m_luaState.get(), "loadfile" );
    removeGlobal( m_luaState.get(), "dofile" );
}

bool ScenesLuaInterpreter::protectedCall( std::string expression, Json expressionParams, Json& result ) {
    const int luaStackSizeWithoutLoadedFunction = lua_gettop( m_luaState.get() ) - 1;

    try {
        if( luaL_loadstring( m_luaState.get(), expression.c_str() ) ) {
            throw ExpressionsError{ std::string{ "Couldn't load and/or compile a '" } + expression +
                                    "' string: " + lua_tostring( m_luaState.get(), -1 ) };
        }
    }
    catch( const ExpressionsError& ex ) {
        Json error = { { "error", ex.what() } };
        result = std::move( error );
        return false;
    }

    pushJsonAsLuaObj( m_luaState.get(), expressionParams );
    if( LUA_OK == LuaUtils::protectedCall( m_luaState.get(), 1, LUA_MULTRET ) ) {
        std::vector< Json > values;
        const int returnValues = lua_gettop( m_luaState.get() ) - luaStackSizeWithoutLoadedFunction;
        for( int i = returnValues; i >= 1; --i ) {
            const Json& json = pullJson( m_luaState.get(), -i );
            values.push_back( json );
        }
        if( !values.empty() )
            result = values.back();

        return true;
    }

    std::string message = std::string{ "Could not run Lua code: " }.append( lua_tostring( m_luaState.get(), -1 ) );
    Log::error( cc::error( "ScenesLuaInterpreter: " + message ) );
    Json error = { { "error", message } };
    result = std::move( error );
    return false;
}

}  // namespace ScenesExpressions
}  // namespace HubPlatform