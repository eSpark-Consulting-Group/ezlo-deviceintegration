#include "luad/common/luup_api.h"

#include "common_utils/curl_wrapper.h"
#include "log_utils/ezlo_log.h"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include <string>
#include <exception>
#include <memory>

/* The implementation of the luup.inet sub-API --
 * http://wiki.micasaverde.com/index.php/Luup_Lua_extensions#Module:_luup.inet */

namespace {
extern const luaL_Reg luupInetModule[];
}

using namespace HubPlatform;

class LuupInetAPI final : public LuupAPI {
public:
    LuupInetAPI() {
        registerModule( "inet", luupInetModule );
    }

    static int luupInetWget( lua_State* luaState );
    static int luupInetRequest( lua_State* luaState );
};

namespace {

const luaL_Reg luupInetModule[] = { { "wget", LuupInetAPI::luupInetWget },
                                    { "request", LuupInetAPI::luupInetRequest },
                                    { NULL, NULL } };
}

class InetError : public std::runtime_error {
public:
    InetError( const std::string& errDesription )
    : runtime_error( "inet." + errDesription ) {}
};

static LuupInetAPI luupInetAPI;

/* arguments: url^ (string), timeout (number), username (string), password (string);
 * return: operationStatusCode (number)(0 is success), content (string), httpStatusCode (number); */
int LuupInetAPI::luupInetWget( lua_State* luaState ) {
    if( lua_gettop( luaState ) < 1 || lua_gettop( luaState ) > 4 )
        throw InetError{ "wget: an incorrect number of arguments." };

    if( lua_type( luaState, 1 ) != LUA_TSTRING )
        throw InetError{ "wget: an incorrect url argument." };

    auto url = lua_tostring( luaState, 1 );
    auto timeout = ( lua_type( luaState, 2 ) == LUA_TNUMBER ? lua_tonumber( luaState, 2 ) : 5 ) >= 0 ?: 5;
    auto username = lua_type( luaState, 3 ) == LUA_TSTRING ? lua_tostring( luaState, 3 ) : "";
    auto password = lua_type( luaState, 4 ) == LUA_TSTRING ? lua_tostring( luaState, 4 ) : "";

    std::string requestContent;
    long httpStatusCode = 500;
    auto status = CurlWrapper::curlGet( url, requestContent, nullptr, timeout, username, password, &httpStatusCode );

    if( !status ) {
        Log::error( "luup.wget: fail to CurlWrapper::curlGet()." );
        lua_pushnumber( luaState, 1 );

        return 1;
    }

    lua_pushnumber( luaState, 0 );
    lua_pushlstring( luaState, requestContent.c_str(), requestContent.size() ); /* std::string doesn't
                                                                                   use '/0' as a termination symbol */
    lua_pushnumber( luaState, httpStatusCode );

    return 3;
}

/* arguments: [ url^ (string), timeout (number), username (string), password (string), data (table) | table^ (table) ];
 * return: curl_exit_code (number), response (string), http_status_code (number);
 * Note: we're allowed to return nothing (in case of some error) making a caller to check values before it uses them */
int LuupInetAPI::luupInetRequest( lua_State* luaState ) {
    const char *url, *username = "", *password = "";
    bool followLocations = true; /* the API requires as a default value */
    bool performGETRequest = true;
    std::string postFields;
    std::shared_ptr< curl_slist > headers;
    int timeout = 15;

    std::string requestContent;
    long httpStatusCode = LONG_MAX;
    CURLcode curlCode = CURL_LAST;

    auto getPostFields = []( lua_State* luaState, int luaTableIdx ) {
        auto postArgsTable = getNativeStringTable( luaState, luaTableIdx );
        std::string postFields;

        for( const auto& arg : postArgsTable )
            postFields += arg.first + "=" + arg.second + "&";

        postFields.erase( postFields.size() - 1 ); /* remove a last extra '&' symbol */

        return postFields;
    };

    auto getHeaders = []( lua_State* luaState, int luaTableIdx ) {
        auto httpHeadersTable = getNativeStringTable( luaState, luaTableIdx );
        curl_slist* headers = nullptr;

        for( const auto& arg : httpHeadersTable )
            headers = curl_slist_append( headers, ( arg.first + ": " + arg.second ).c_str() );

        /* an std::unique_ptr requires a deleter to be set as a template parameter what isn't convenient enough */
        return std::shared_ptr< curl_slist >{ headers, curl_slist_free_all };
    };

    if( lua_gettop( luaState ) < 1 || lua_gettop( luaState ) > 5 )
        throw InetError{ "request: an incorrect number of arguments." };

    if( lua_type( luaState, 1 ) != LUA_TSTRING && lua_type( luaState, 1 ) != LUA_TTABLE )
        throw InetError{ "request: an incorrect first argument." };

    /* if we got one argument - a table */
    if( lua_type( luaState, 1 ) == LUA_TTABLE ) {
        /* get a 'url' value */
        if( lua_getfield( luaState, 1, "url" ) != LUA_TSTRING )
            throw InetError{ "request: an incorrect url field in the table." };
        url = lua_tostring( luaState, 2 );
        lua_pop( luaState, 1 );

        /* get a 'data' value */
        if( lua_getfield( luaState, 1, "data" ) == LUA_TTABLE ) {
            postFields = getPostFields( luaState, 2 );
            performGETRequest = false;
        }
        lua_pop( luaState, 1 );

        /* get a 'headers' value */
        if( lua_getfield( luaState, 1, "headers" ) == LUA_TTABLE )
            headers = getHeaders( luaState, 2 );
        lua_pop( luaState, 1 );

        /* get a 'timeout' value */
        timeout = lua_getfield( luaState, 1, "timeout" ) == LUA_TNUMBER ? lua_tonumber( luaState, 2 ) : timeout;
        lua_pop( luaState, 1 );

        username = lua_getfield( luaState, 1, "username" ) == LUA_TSTRING ? lua_tostring( luaState, 2 ) : username;
        lua_pop( luaState, 1 );

        password = lua_getfield( luaState, 1, "password" ) == LUA_TSTRING ? lua_tostring( luaState, 2 ) : password;
        lua_pop( luaState, 1 );

        /* get a 'follow' value */
        auto objType = lua_getfield( luaState, 1, "follow" );
        if( objType == LUA_TBOOLEAN || objType == LUA_TNUMBER )
            followLocations = lua_toboolean( luaState, 2 );
        lua_pop( luaState, 1 );
    }
    else {
        url = lua_tostring( luaState, 1 );
        timeout = ( lua_type( luaState, 2 ) == LUA_TNUMBER ? lua_tonumber( luaState, 2 ) : timeout ) >= 0 ?: timeout;
        username = lua_type( luaState, 3 ) == LUA_TSTRING ? lua_tostring( luaState, 3 ) : username;
        password = lua_type( luaState, 4 ) == LUA_TSTRING ? lua_tostring( luaState, 4 ) : password;

        /* if we have 'data' we have to make an HTTP POST request */
        if( lua_type( luaState, 5 ) == LUA_TTABLE ) {
            postFields = getPostFields( luaState, 5 );
            performGETRequest = false;
        }
    }

    if( performGETRequest )
        CurlWrapper::curlGet( url,
                              requestContent,
                              headers.get(),
                              timeout,
                              username,
                              password,
                              &httpStatusCode,
                              &curlCode,
                              followLocations );
    else
        CurlWrapper::curlPost( url,
                               headers.get(),
                               postFields,
                               requestContent,
                               timeout,
                               username,
                               password,
                               &httpStatusCode,
                               &curlCode,
                               followLocations );

    lua_pushnumber( luaState, curlCode );
    lua_pushlstring( luaState, requestContent.c_str(), requestContent.size() ); /* std::string doesn't
                                                                                   use '/0' as a termination symbol */
    lua_pushnumber( luaState, httpStatusCode );

    return 3;
}
