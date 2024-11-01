#include <tiny/tiny_consolecolors.h>
#include "binding_interfaces/lua/lua_binding_interface.h"

#include "cloud_storage_module.h"
#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"

namespace HubPlatform {
namespace {
static const std::string moduleName{ "cloud_storage." };

#define GET_FUNCTION ( moduleName + __func__ )

#define CHECK_LUA_INPUT_PARAMETERS( number ) CHECK_INPUT_PARAMETERS( GET_FUNCTION, number )

#define GET_LUA_INPUT_PARAMETERS( type, idx ) GET_INPUT_PARAMETERS( type, idx, GET_FUNCTION )

#define CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( callFunction, number ) \
    CHECK_INPUT_OPTIONAL_PARAMETERS( ( std::string{ "cloud_storage." } + ( callFunction ) ), number )

int fetchToken( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 0 );

    try {
        LuaD::CloudStorageModule::CloudStorage cloudStorageModule( "Luad" );
        auto tokenResult = cloudStorageModule.fetchTokenRecord();

        return pushJsonAsLuaObj( luaState, tokenResult );
    }
    catch( const std::exception& exception ) {
        using namespace LuaD::CloudStorageModule::Resources;
        return luaL_error( luaState, Fields::Errors::FetchTokenError, exception.what() );
    }
}

int fetchEndpoints( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( __func__, 1 );

    try {
        auto params = pullJson( luaState, 1 );
        LuaD::CloudStorageModule::CloudStorage cloudStorageModule( "Luad" );
        const auto endPointsData = cloudStorageModule.fetchEndpoints( params );

        return pushJsonAsLuaObj( luaState, endPointsData );
    }
    catch( const std::exception& exception ) {
        using namespace LuaD::CloudStorageModule::Resources;
        return luaL_error( luaState, Fields::Errors::FetchEndpointsError, exception.what() );
    }
}

int fetchEndpoint( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( __func__, 1 );

    try {
        const auto endpointName = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
        auto options = pullJson( luaState, 2 );
        LuaD::CloudStorageModule::CloudStorage cloudStorageModule( "Luad" );
        const auto url = cloudStorageModule.fetchEndpoint( endpointName, options );

        lua_pushlstring( luaState, url.c_str(), url.size() );
        return 1;
    }
    catch( const std::exception& exception ) {
        using namespace LuaD::CloudStorageModule::Resources;
        return luaL_error( luaState, Fields::Errors::FetchEndpointError, exception.what() );
    }
}

int subscribe( lua_State* luaState ) {
    auto eventsSubscriber =
        createFunctionalityCreator( luaState ).getPluginEventsSubscribeFunctionality( GroupsNames::BROADCAST_GROUP );
    if( !eventsSubscriber ) {
        Log::warn( cc::warn( "Attempt to use eventsSubscriber functionality without proper permissions" ) );
        return luaL_error( luaState, "fail to subscribe" );
    }

    try {
        const auto subscriberId = eventsSubscriber->subscribe( getPluginEventsSubscriber( luaState ) );
        lua_pushstring( luaState, subscriberId.c_str() );
    }
    catch( const std::exception& ex ) {
        return luaL_error( luaState, "fail to subscribe: %s", ex.what() );
    }

    return 1;
}

int unsubscribe( lua_State* luaState ) {
    auto eventsSubscriber =
        createFunctionalityCreator( luaState ).getPluginEventsSubscribeFunctionality( GroupsNames::BROADCAST_GROUP );
    if( !eventsSubscriber ) {
        Log::warn( cc::warn( "Attempt to use eventsSubscriber functionality without proper permissions" ) );
        return luaL_error( luaState, "fail to unsubscribe" );
    }

    try {
        eventsSubscriber->unsubscribe( getPluginEventsSubscriber( luaState ) );
    }
    catch( const std::exception& ex ) {
        return luaL_error( luaState, "fail to unsubscribe: %s", ex.what() );
    }

    return 0;
}

}  // namespace

inline namespace Bindings {
constexpr char moduleName[] = "cloud_storage";
const luaL_Reg module[] = { { "fetch_token", fetchToken },       { "fetch_endpoints", fetchEndpoints },
                            { "fetch_endpoint", fetchEndpoint }, { "subscribe", subscribe },
                            { "unsubscribe", unsubscribe },      { nullptr, nullptr } };
}  // namespace Bindings

}  // namespace HubPlatform

HUB_BASE_MODULE_ENTRY_DEFINITION( cloud_storage, HubPlatform::Bindings::module, HubPlatform::Bindings::moduleName );