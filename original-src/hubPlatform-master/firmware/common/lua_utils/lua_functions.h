#pragma once

extern "C" {
#include <lua.h>
}

namespace HubPlatform {
namespace LuaUtils {

void luaTraceback( lua_State* luaState, const char* message, int level = 1 );
int messageHandler( lua_State* luaState );
int protectedCall( lua_State* luaState, int nargs, int nresults );

}  // namespace LuaUtils
}  // namespace HubPlatform