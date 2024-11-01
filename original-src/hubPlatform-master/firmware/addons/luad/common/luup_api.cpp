#include "luad/common/luup_api.h"

#include <cstring>
#include <exception>

namespace HubPlatform {

LuupAPI::LuupAPIModules LuupAPI::m_APIModules;

void LuupAPI::registerModule( const std::string& moduleName, const luaL_Reg* functionsSet ) {
    m_APIModules.push_back( { moduleName, functionsSet } );
}

void LuupAPI::registerOldAPIModules( lua_State* luaState ) {
    /* TODO: create the same hierarchy as the MiOS Luup has */
    for( const auto& module : m_APIModules ) {
        luaL_newlib( luaState, module.m_functionsSet );
        lua_setglobal( luaState, module.m_moduleName.c_str() );
    }
}

LuupAPI::NativeStringTable LuupAPI::getNativeStringTable( lua_State* luaState, int luaTableIdx ) {
    NativeStringTable nativeTable;

    if( !luaState )
        throw std::invalid_argument{ "LuupAPI::getNativeStringTable: luaState is nullptr." };

    lua_pushnil( luaState );
    while( lua_next( luaState, luaTableIdx ) != 0 ) {
        /* uses 'key' (at index -2) and 'value' (at index -1) */
        if( lua_type( luaState, -2 ) != LUA_TSTRING || lua_type( luaState, -1 ) != LUA_TSTRING )
            throw std::invalid_argument{ "LuupAPI::getNativeStringTable: it's not a Lua string->string table." };

        nativeTable[ lua_tostring( luaState, -2 ) ] = lua_tostring( luaState, -1 );

        /* removes 'value'; keeps 'key' for next iteration */
        lua_pop( luaState, 1 );
    }

    return nativeTable;
}

std::string LuupAPI::getLastErrnoDescription() {
    char buf[ 512 ];

    /* Note: if something is going to be added before this call, be aware that it may change the errno */
#if _LIBCPP_HAS_MUSL_LIBC
    return std::string{ !strerror_r( errno, buf, sizeof( buf ) ) ? buf : "strerror_r failed" };
#else
    return std::string{ strerror_r( errno, buf, sizeof( buf ) ) ?: "strerror_r failed" };
#endif
}

}  // namespace HubPlatform
