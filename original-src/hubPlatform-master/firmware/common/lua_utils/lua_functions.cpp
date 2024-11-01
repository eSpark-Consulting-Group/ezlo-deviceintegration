#include "lua_functions.h"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

namespace HubPlatform {
namespace LuaUtils {

void luaTraceback( lua_State* luaState, const char* message, int level ) {
    luaL_Buffer buffer;
    luaL_buffinit( luaState, &buffer );

    if( message ) {
        luaL_addstring( &buffer, message );
        luaL_addchar( &buffer, '\n' );
    }

    luaL_addstring( &buffer, "stack traceback:" );

    lua_Debug ar;
    while( lua_getstack( luaState, level++, &ar ) ) {
        lua_getinfo( luaState, "Slnt", &ar );

        if( ar.currentline <= 0 )
            lua_pushfstring( luaState, "\n\t%s", ar.source );
        else
            lua_pushfstring( luaState, "\n\t%s:%d", ar.source, ar.currentline );
        luaL_addvalue( &buffer );

        if( ar.istailcall )
            luaL_addstring( &buffer, "\n\t(...tail calls...)" );
    }

    luaL_pushresult( &buffer );
}

// NOTE: based on https://github.com/lua/lua/blob/faaf7e481fbadc2ef520a96b638dd910f3c9ff14/lua.c#L111
int messageHandler( lua_State* luaState ) {
    const char* message = lua_tostring( luaState, 1 );

    // is error object not a string?
    if( !message ) {
        if( luaL_callmeta( luaState, 1, "__tostring" ) &&  // does it have a metamethod
            lua_type( luaState, -1 ) == LUA_TSTRING )      // that produces a string?
            return 1;                                      // that is the message
        else
            message = lua_pushfstring( luaState, "(error object is a %s value)", luaL_typename( luaState, 1 ) );
    }
    luaTraceback( luaState, message );  // append a traceback

    return 1;  // return the traceback
}

// moved from luad
int protectedCall( lua_State* luaState, int nargs, int nresults ) {
    const int functionIndex = lua_gettop( luaState ) - nargs;

    // push custom error message handler
    lua_pushcfunction( luaState, messageHandler );

    // move it before function and arguments
    lua_insert( luaState, functionIndex );

    // call lua_pcall function with custom handler
    const int status = lua_pcall( luaState, nargs, nresults, functionIndex );

    // remove custom error message handler from stack
    lua_remove( luaState, functionIndex );

    return status;
}

}  // namespace LuaUtils
}  // namespace HubPlatform
