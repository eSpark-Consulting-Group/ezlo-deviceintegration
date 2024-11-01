#pragma once

extern "C" {
#include <lua.h>
}

namespace HubPlatform {
class LuaModuleLoader;
}  // namespace HubPlatform

namespace HubPlatform {
namespace LuaFunctions {

void registerOverrideFunctions( lua_State* luaState, LuaModuleLoader& luaModuleLoader );

int protectedCall( lua_State* luaState, int nargs, int nresults );

}  // namespace LuaFunctions
}  // namespace HubPlatform
