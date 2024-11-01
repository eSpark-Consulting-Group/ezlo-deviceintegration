#pragma once

#include "binding_interfaces/lua/lua_binding_interface.h"

#define HUB_TO_STRING_IMPL( s ) #s
#define HUB_TO_STRING( s ) HUB_TO_STRING_IMPL( s )

namespace HubPlatform {

const char ModuleEntryPoint[] = HUB_TO_STRING( HUB_MODULE_ENTRY );

void initLuaBindingInterface( lua_State* luaState, Services::Service* service );
void setPluginId( lua_State* luaState, const std::string& pluginId );
void setRunningAnonymousPlugin( lua_State* luaState );
void setLuaModuleAPIRestrictions( lua_State* luaState, HubAPIRestrictions luaAPIRestrictions );

}  // namespace HubPlatform
