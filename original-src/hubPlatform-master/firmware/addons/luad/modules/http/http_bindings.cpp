#include "luad/addon/lua_addon.h"
#include "luad/modules/http/http_dispatch.h"

#include "binding_interfaces/lua/lua_binding_interface.h"

#define LUA_CHECK_INPUT_PARAMETERS( number ) CHECK_INPUT_PARAMETERS( ( std::string{ "http." } + __func__ ), number )

namespace HubPlatform {
namespace Luad {

namespace {

HttpDispatch& getHttp( lua_State* luaState ) {
    auto& service = dynamic_cast< LuaAddon::LuaAddon& >( getHostService( luaState ) );

    static HttpDispatch dispatcher( service, service, service );
    return dispatcher;
}

int httpRequest( lua_State* luaState ) {
    LUA_CHECK_INPUT_PARAMETERS( 1 );

    std::string pluginId = getPluginId( luaState );
    const auto json = pullJson( luaState, 1 );

    auto& dispatch = getHttp( luaState );

    try {
        const auto result = dispatch.request( pluginId, json );
        lua_pushstring( luaState, result.c_str() );

        return 1;
    }
    catch( const HttpFileDoesntExist& e ) {
        std::string message = e.what();
        if( !e.getFileName().empty() )
            message += std::string( ": " ) + e.getFileName();

        return luaL_error( luaState, "%s", message.c_str() );
    }
    catch( const HttpPluginPermissionsError& e ) {
        std::string message = e.what();
        if( !e.getFileName().empty() )
            message += std::string( ": " ) + e.getFileName();

        return luaL_error( luaState, "%s", message.c_str() );
    }
    catch( const std::exception& e ) {
        const char* message = e.what();
        if( !message || !*message )
            message = "Fail to perform http request";
        return luaL_error( luaState, "%s", message );
    }
}

/** Returns: void */
int httpRepeatRequest( lua_State* luaState ) {
    LUA_CHECK_INPUT_PARAMETERS( 1 );

    std::string pluginId = getPluginId( luaState );
    const auto json = pullJson( luaState, 1 );

    auto& dispatch = getHttp( luaState );

    try {
        dispatch.repeatRequest( pluginId, json );
        return 0;
    }
    catch( const HttpFileDoesntExist& e ) {
        std::string message = e.what();
        if( !e.getFileName().empty() )
            message += std::string( ": " ) + e.getFileName();

        return luaL_error( luaState, "%s", message.c_str() );
    }
    catch( const HttpPluginPermissionsError& e ) {
        std::string message = e.what();
        if( !e.getFileName().empty() )
            message += std::string( ": " ) + e.getFileName();

        return luaL_error( luaState, "%s", message.c_str() );
    }
    catch( const std::exception& e ) {
        const char* message = e.what();
        if( !message || !*message )
            message = "Fail to perform http request";
        return luaL_error( luaState, "%s", message );
    }
}

int httpCloseConnection( lua_State* luaState ) {
    LUA_CHECK_INPUT_PARAMETERS( 1 );

    std::string pluginId = getPluginId( luaState );
    const auto json = pullJson( luaState, 1 );

    auto& dispatch = getHttp( luaState );

    try {
        bool result = dispatch.closeConnection( pluginId, json );
        lua_pushboolean( luaState, result );

        return 1;
    }
    catch( const std::exception& e ) {
        const char* message = e.what();
        if( !message || !*message )
            message = "Fail to close connection";
        return luaL_error( luaState, "%s", message );
    }
}

int httpSendData( lua_State* luaState ) {
    LUA_CHECK_INPUT_PARAMETERS( 1 );

    std::string pluginId = getPluginId( luaState );
    auto json = pullJson( luaState, 1 );

    auto& dispatch = getHttp( luaState );

    try {
        int bytesSent = dispatch.sendData( pluginId, std::move( json ) );
        lua_pushinteger( luaState, bytesSent );

        return 1;
    }
    catch( const std::exception& e ) {
        const char* message = e.what();
        if( !message || !*message )
            message = "Fail to send data";
        return luaL_error( luaState, "%s", message );
    }
}

int httpGetConnectionInfo( lua_State* luaState ) {
    LUA_CHECK_INPUT_PARAMETERS( 1 );

    std::string pluginId = getPluginId( luaState );
    const auto json = pullJson( luaState, 1 );
    auto& dispatch = getHttp( luaState );

    try {
        const auto result = dispatch.getConnectionInfo( pluginId, json );
        return pushJsonAsLuaObj( luaState, result );
    }
    catch( const HttpConnectionDoesntExistError& e ) {
        return luaL_error( luaState, "%s", e.what() );
    }
    catch( const std::exception& e ) {
        const char* message = e.what();
        if( !message || !*message )
            message = "Fail get connection info";
        return luaL_error( luaState, "%s", message );
    }
}

constexpr char moduleName[] = "http";
constexpr luaL_Reg module[] = { { "request", errorCatchWrapper< httpRequest > },
                                { "repeat_request", errorCatchWrapper< httpRepeatRequest > },
                                { "close_connection", errorCatchWrapper< httpCloseConnection > },
                                { "send_data", errorCatchWrapper< httpSendData > },
                                { "get_connection_info", errorCatchWrapper< httpGetConnectionInfo > },
                                { nullptr, nullptr } };

}  // namespace
}  // namespace Luad
}  // namespace HubPlatform

HUB_BASE_MODULE_ENTRY_DEFINITION( http, HubPlatform::Luad::module, HubPlatform::Luad::moduleName );
