#include "binding_interfaces/lua/lua_binding_interface.h"
#include "extensions_manager/remote_interface/em_remote_interface.h"

#include <stdexcept>
#include <string>
#include <tuple>

#define CHECK_LUA_INPUT_PARAMETERS( number ) \
    CHECK_INPUT_PARAMETERS( ( std::string{ "extensions_manager." } + __func__ ), number )
#define CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( number ) \
    CHECK_INPUT_OPTIONAL_PARAMETERS( ( std::string{ "extensions_manager." } + __func__ ), number )

#define GET_LUA_INPUT_PARAMETERS( type, idx ) \
    GET_INPUT_PARAMETERS( type, idx, ( std::string{ "extensions_manager." } + __func__ ) )

namespace HubPlatform {
namespace {

int getPluginExtensions( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 1 );

    const auto pluginId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );

    ExtensionsManager::RemoteInterface emRemoteInterface( getHostService( luaState ) );

    auto extensions = emRemoteInterface.getPluginExtensionsIds( pluginId );

    return pushJsonAsLuaObj( luaState, extensions );
}

int getPluginExtensionData( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 3 );

    const auto pluginId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
    const auto path = GET_LUA_INPUT_PARAMETERS( NativeString, 2 );
    const auto filter = GET_LUA_INPUT_PARAMETERS( NativeString, 3 );

    ExtensionsManager::RemoteInterface emRemoteInterface( getHostService( luaState ) );

    auto data = emRemoteInterface.getPluginExtensionData( pluginId, path, filter );

    return pushJsonAsLuaObj( luaState, data );
}

const char moduleName[] = "extensions_manager";
const luaL_Reg module[] = { { "get_plugin_extensions", errorCatchWrapper< getPluginExtensions > },
                            { "get_plugin_extension_data", errorCatchWrapper< getPluginExtensionData > },
                            { NULL, NULL } };

}  // namespace
}  // namespace HubPlatform

HUB_MODULE_ENTRY_DEFINITION( HubPlatform::module, HubPlatform::moduleName );
