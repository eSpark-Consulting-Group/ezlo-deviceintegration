#include "binding_interfaces/lua/lua_binding_interface.h"
#include "uid/remote_interface/ui_remote_interface.h"
#include "uid/extensions/http_links/resources.h"
#include "binding_interfaces/lua/lua_params_checker.h"
#include "uid/bindings/errors.h"
#include "extensions_manager/remote_interface/em_remote_interface.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include "tiny/tiny_consolecolors.h"
#include <string>

#define HTTP_SERVER_LUA_CHECK_INPUT_PARAMETERS( number )                               \
    do {                                                                               \
        CHECK_INPUT_PARAMETERS( ( std::string{ "http_server." } + __func__ ), number ) \
    } while( false )

#define HTTP_SERVER_LUA_GET_INPUT_PARAMETERS( type, idx ) \
    GET_INPUT_PARAMETERS( type, idx, ( std::string{ "http_server." } + __func__ ) )

namespace HubPlatform {
namespace HttpServerBindings {

struct HttpLinksErrorsMixin {
    using RequiredParameter = HttpLinks::RequiredParameterError;
    using NoFile = HttpLinks::NoFileError;
    using FilePermissions = HttpLinks::FilePermissionsError;
    using RequestPermissions = HttpLinks::RequestPermissionsError;
    using WrongParameterFormat = HttpLinks::WrongParameterFormatError;
    using ParameterLimit = HttpLinks::ParameterLimitError;
    using TotalLimit = HttpLinks::HttpLinksError;
    using NoObject = HttpLinks::HttpLinksError;
};

using HttpLinksParamsChecker =
    LuaParamsChecker< HttpLinksErrorsMixin, Services::Service, ExtensionsManager::RemoteInterface >;

Uid::RemoteInterface makeUiRemoteInterface( lua_State* luaState ) {
    return { getHostService( luaState ) };
}

/** Gets a table and creates HTTP link for it
 * @param [index=1] Lua table {link_id: string, handler: string (req), user_data: any, timeout: int, answer: bool }
 * @returns table {link_id:string, url: string, secure_url: string}, all required */
int createLinkUrl( lua_State* luaState ) {
    HTTP_SERVER_LUA_CHECK_INPUT_PARAMETERS( 1 );
    auto paramsJson = pullJson( luaState, 1 );
    std::string linkId;
    const std::string& pluginId = getPluginId( luaState );
    LOGGER_TRACE( "pluginId: " + pluginId );

    auto& service = getHostService( luaState );
    ExtensionsManager::RemoteInterface extManager( service );
    HttpLinksParamsChecker paramsChecker( service, extManager, pluginId );

    try {
        // handler: string; hab script path, required
        auto pathStr = HttpLinksParamsChecker::checkRequiredString( paramsJson, "handler" );
        auto fullPath = paramsChecker.makeFullPath( pathStr, true );
        paramsChecker.checkFullPathExists( fullPath, pathStr );
        paramsChecker.checkPluginPermissions( pluginId, pathStr );

        HttpLinksParamsChecker::checkOptionalString( paramsJson, "link_id" );
        HttpLinksParamsChecker::checkOptionalInt( paramsJson, "timeout" );
        HttpLinksParamsChecker::checkOptionalBool( paramsJson, "answer" );
    }
    catch( const Uid::HttpServerError& ex ) {
        LOGGER_ERROR( cc::error( "Error creating a link: " ) + cc::note( ex.what() ) );
        return luaL_error( luaState, ex.what() );
    }

    using namespace HttpLinks::Resources;
    paramsJson[ HttpLinkFields::linkId ] = paramsJson[ "link_id" ];
    paramsJson[ HttpLinkFields::userData ] = paramsJson[ "user_data" ];
    json_remove( paramsJson, "link_id" );
    json_remove( paramsJson, "user_data" );

    try {
        Json answer = makeUiRemoteInterface( luaState ).httpServer().createLinkUrl( pluginId, paramsJson );
        auto errorIt = answer.find( "error" );
        if( errorIt != answer.end() ) {
            return luaL_error( luaState, json_get_str( *errorIt, "message" ).c_str() );
        }
        linkId = json_get_str( answer[ "result" ], "linkId" );
    }
    catch( const Uid::HttpServerError& ex ) {
        LOGGER_ERROR( cc::error( "Error creating a link: " ) + cc::note( ex.what() ) );
        return luaL_error( luaState, ex.what() );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Error creating a link: " ) + cc::note( ex.what() ) );
        return luaL_error( luaState, std::string{ "Error creating a link: " }.append( ex.what() ).c_str() );
    }

    // get linkInfo by the linkId
    if( linkId.empty() ) {
        LOGGER_ERROR( cc::error( "Error creating this link" ) );
        return luaL_error( luaState, "Error creating a link" );
    }

    try {
        Json answer = makeUiRemoteInterface( luaState ).httpServer().getLinkInfo( pluginId, linkId );
        auto errorIt = answer.find( "error" );
        if( errorIt != answer.end() ) {
            return luaL_error( luaState, json_get_str( *errorIt, "message" ).c_str() );
        }
        Json linkInfo;
        if( !json_get( answer[ "result" ], "linkInfo", linkInfo ) ) {
            return luaL_error( luaState, "Could not create a link" );
        }

        Json result = JSON( "link_id" << linkId << "url" << json_get_str( linkInfo, "url" ) << "secure_url"
                                      << json_get_str( linkInfo, "secureUrl" ) );
        pushJsonAsLuaObj( luaState, result );
        return 1;
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Error creating the link: " ) + cc::note( ex.what() ) );
        return luaL_error( luaState, std::string{ "Error creating a link: " }.append( ex.what() ).c_str() );
    }
}

/** Gets a table and creates HTTP link for it
 * @param [index=1] Lua table {link_id: string, handler: string (req), user_data: any, timeout: int, answer: bool }
 * @returns nothing */
int deleteLinkUrl( lua_State* luaState ) {
    HTTP_SERVER_LUA_CHECK_INPUT_PARAMETERS( 1 );
    const auto linkId = HTTP_SERVER_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );
    auto pluginId = getPluginId( luaState );

    try {
        if( linkId.empty() )
            throw HttpLinksErrorsMixin::WrongParameterFormat( "id" );

        bool isDeleted = makeUiRemoteInterface( luaState ).httpServer().deleteLinkUrl( pluginId, linkId );
        lua_pushboolean( luaState, isDeleted );
        return 1;
    }
    catch( const Uid::HttpServerError& ex ) {
        LOGGER_ERROR( cc::error( "Error deleting a link: " ) + cc::note( ex.what() ) );
        return luaL_error( luaState, ex.what() );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Error deleting the link: " ) + cc::note( ex.what() ) );
        luaL_error( luaState, "Error deleting the link" );
        return 1;
    }
}

int getLinkInfo( lua_State* luaState ) {
    HTTP_SERVER_LUA_CHECK_INPUT_PARAMETERS( 1 );
    const auto linkId = HTTP_SERVER_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );
    auto pluginId = getPluginId( luaState );

    try {
        if( linkId.empty() )
            throw HttpLinksErrorsMixin::WrongParameterFormat( "link_id" );

        Json answer = makeUiRemoteInterface( luaState ).httpServer().getLinkInfo( pluginId, linkId );
        auto errorIt = answer.find( "error" );
        if( errorIt != answer.end() ) {
            return luaL_error( luaState, json_get_str( *errorIt, "message" ).c_str() );
        }
        Json linkInfo;
        if( !json_get( answer[ "result" ], "linkInfo", linkInfo ) ) {
            return luaL_error( luaState, "Could not get a link" );
        }

        namespace Fields = HttpLinks::Resources::HttpLinkFields;
        Json result;
        jsonCopy( linkInfo, Fields::url, result, "url" );
        jsonCopy( linkInfo, Fields::connections, result, "connections" );
        jsonCopy( linkInfo, Fields::linkId, result, "link_id" );
        jsonCopy( linkInfo, Fields::secureUrl, result, "secure_url" );
        jsonCopy( linkInfo, Fields::userData, result, "user_data" );

        pushJsonAsLuaObj( luaState, result );
        return 1;
    }
    catch( const Uid::HttpServerError& ex ) {
        LOGGER_ERROR( cc::error( "Error fetching a link: " ) + cc::note( ex.what() ) );
        return luaL_error( luaState, ex.what() );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Error fetching a link: " ) + cc::note( ex.what() ) );
        return luaL_error( luaState, std::string{ "Error fetching a link: " }.append( ex.what() ).c_str() );
    }
}

int closeConnection( lua_State* luaState ) {
    HTTP_SERVER_LUA_CHECK_INPUT_PARAMETERS( 1 );
    const auto connectionId = HTTP_SERVER_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );
    auto pluginId = getPluginId( luaState );

    try {
        if( connectionId.empty() )
            throw HttpLinksErrorsMixin::WrongParameterFormat( "id" );

        bool connectionClosed =
            makeUiRemoteInterface( luaState ).httpServer().closeConnection( pluginId, connectionId );
        lua_pushboolean( luaState, connectionClosed );
        return 1;
    }
    catch( const Uid::HttpServerError& ex ) {
        LOGGER_ERROR( cc::error( "Error fetching connection info: " ) + cc::note( ex.what() ) );
        return luaL_error( luaState, ex.what() );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Error fetching connection info: " ) + cc::note( ex.what() ) );
        return luaL_error( luaState, std::string{ "Error fetching connection info: " }.append( ex.what() ).c_str() );
    }
}

int getConnectionInfo( lua_State* luaState ) {
    HTTP_SERVER_LUA_CHECK_INPUT_PARAMETERS( 1 );
    const auto connectionId = HTTP_SERVER_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );
    auto pluginId = getPluginId( luaState );

    try {
        if( connectionId.empty() )
            throw HttpLinksErrorsMixin::WrongParameterFormat( "id" );

        Json answer = makeUiRemoteInterface( luaState ).httpServer().getConnectionInfo( pluginId, connectionId );
        auto errorIt = answer.find( "error" );
        if( errorIt != answer.end() ) {
            return luaL_error( luaState, json_get_str( *errorIt, "message" ).c_str() );
        }
        Json connInfo;
        if( !json_get( answer[ "result" ], "connectionInfo", connInfo ) ) {
            return luaL_error( luaState, "Could not get connection info" );
        }

        namespace Fields = HttpLinks::Resources::ConnectionDbFields;
        Json result;
        jsonCopy( connInfo, Fields::created, result );
        if( connInfo.count( Fields::headers ) )
            jsonCopy( connInfo, Fields::headers, result );
        if( connInfo.count( Fields::params ) )
            jsonCopy( connInfo, Fields::params, result );
        jsonCopy( connInfo, Fields::id, result, "id" );
        jsonCopy( connInfo, Fields::linkId, result, "link_id" );
        jsonCopy( connInfo, Fields::clientIp, result, "client_ip" );
        int closed = json_get_int( connInfo, "closed", 0 );
        result[ Fields::status ] = closed > 0 ? "closed" : "pending";

        pushJsonAsLuaObj( luaState, result );
        return 1;
    }
    catch( const Uid::HttpServerError& ex ) {
        LOGGER_ERROR( cc::error( "Error fetching connection info: " ) + cc::note( ex.what() ) );
        return luaL_error( luaState, ex.what() );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Error fetching connection info: " ) + cc::note( ex.what() ) );
        return luaL_error( luaState, std::string{ "Error fetching connection info: " }.append( ex.what() ).c_str() );
    }
}

/** Gets a table and sends more data to a connection mentioned in it.
 *  If some problem with parameters is found, call sendData() anyway with { code = 400, code_status = "Bad request" }
 *
 * @param [index=1] Lua table {id: string, data: string, last: bool, code: int, code_status: string, headers:
 * [{key_name: value: string:}] }
 * @returns nothing */
int sendData( lua_State* luaState ) {
    HTTP_SERVER_LUA_CHECK_INPUT_PARAMETERS( 1 );
    auto paramsJson = pullJson( luaState, 1 );
    auto pluginId = getPluginId( luaState );

    auto sendError = []( lua_State* luaState,
                         const std::string& pluginId,
                         const Json& paramsJson,
                         int code,
                         const std::string& codeStatus ) -> bool {
        Json error;
        jsonCopy( paramsJson, "id", error );
        error[ "data" ] = "";
        error[ "last" ] = true;
        error[ "code" ] = code;
        if( !codeStatus.empty() )
            error[ "code_status" ] = codeStatus;
        error[ "headers" ] = JSON_ARRAY_( JSON( "connection"
                                                << "close" ) );

        try {
            makeUiRemoteInterface( luaState ).httpServer().sendData( pluginId, error );
            return true;
        }
        catch( const HttpLinks::HttpLinksError& ex ) {
            LOGGER_ERROR_RAW( cc::error( "sendData: failed to send error information back: " ) +
                              cc::note( ex.what() ) );
        }
        return false;
    };

    try {
        HttpLinksParamsChecker::checkRequiredString( paramsJson, "id" );
        HttpLinksParamsChecker::checkOptionalBool( paramsJson, "last" );
        HttpLinksParamsChecker::checkOptionalInt( paramsJson, "code" );
        HttpLinksParamsChecker::checkOptionalString( paramsJson, "code_status" );
        if( paramsJson.count( "headers" ) ) {
            try {
                auto arrayOfObjectsOneKeyStringValue = []( const JsonArray& arr ) -> void {
                    for( auto it = arr.begin(); it != arr.end(); it++ ) {
                        if( !it->is_object() )
                            throw HttpLinksErrorsMixin::WrongParameterFormat( "headers" );
                        if( it->size() != 1 )
                            throw HttpLinksErrorsMixin::WrongParameterFormat( "headers" );

                        const Json& obj = *it;
                        for( auto keyValue = obj.begin(); keyValue != obj.end(); keyValue++ ) {
                            if( !keyValue.value().is_string() ) {
                                LOGGER_ERROR_RAW( cc::error( "send_data: string value required for headers[]." ) +
                                                  cc::note( it.key() ) + ": " + it.value().dump() );
                                throw HttpLinksErrorsMixin::WrongParameterFormat( "headers" );
                            }
                        }
                    }
                };

                jsonCheckArray( paramsJson, "headers", arrayOfObjectsOneKeyStringValue );
            }
            catch( const std::runtime_error& ex ) {
                LOGGER_ERROR( cc::error( "Error executing send_data() call: " ) + cc::note( ex.what() ) );
                throw HttpLinksErrorsMixin::WrongParameterFormat( "headers" );
            }
        }
        // params.data : string with length <= Resources::Limits::MAX_DATA_SEND_PORTION
        auto dataIt = paramsJson.find( "data" );
        if( dataIt != paramsJson.end() ) {
            if( auto dataPtr = dataIt.value().get_ptr< const std::string* >() ) {
                LOGGER_TRACE( "Params checking: data.size = " + cc::num10( dataPtr->size() ) );
                if( dataPtr->size() > HttpLinks::Resources::Limits::MAX_DATA_SEND_PORTION ) {
                    throw HttpLinksErrorsMixin::ParameterLimit( "data" );
                }
            }
        }
    }
    catch( const Uid::HttpServerError& ex ) {
        LOGGER_ERROR( cc::error( "Error executing send_data() call: " ) + cc::note( ex.what() ) );
        sendError( luaState, pluginId, paramsJson, 400, "Bad request" );
        return luaL_error( luaState, ex.what() );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Error executing send_data() call: " ) + cc::note( ex.what() ) );
        sendError( luaState, pluginId, paramsJson, 400, "Bad request" );
        return luaL_error( luaState, std::string{ "Error fetching connection info: " }.append( ex.what() ).c_str() );
    }

    try {
        makeUiRemoteInterface( luaState ).httpServer().sendData( pluginId, paramsJson );
        lua_pushnil( luaState );
        return 1;
    }
    catch( const Uid::HttpServerError& ex ) {
        LOGGER_ERROR( cc::error( "Error executing send_data() call: " ) + cc::note( ex.what() ) );
        return luaL_error( luaState, ex.what() );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Error executing send_data() call: " ) + cc::note( ex.what() ) );
        return luaL_error( luaState, std::string{ "Error fetching connection info: " }.append( ex.what() ).c_str() );
    }
}

const char moduleName[] = "http_server";
const luaL_Reg module[] = { { "create_link_url", errorCatchWrapper< createLinkUrl > },
                            { "delete_link_url", errorCatchWrapper< deleteLinkUrl > },
                            { "get_link_info", errorCatchWrapper< getLinkInfo > },
                            { "close_connection", errorCatchWrapper< closeConnection > },
                            { "get_connection_info", errorCatchWrapper< getConnectionInfo > },
                            { "send_data", errorCatchWrapper< sendData > },
                            { nullptr, nullptr } };

}  // namespace HttpServerBindings
}  // namespace HubPlatform

HUB_MODULE_ENTRY_DEFINITION( HubPlatform::HttpServerBindings::module, HubPlatform::HttpServerBindings::moduleName )
