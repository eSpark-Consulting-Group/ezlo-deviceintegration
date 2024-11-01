#include "binding_interfaces/lua/lua_binding_interface.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

#define BACKUP_LUA_CHECK_INPUT_PARAMETERS( number ) \
    CHECK_INPUT_PARAMETERS( ( std::string{ "backup." } + __func__ ), number )

#define BACKUP_LUA_GET_INPUT_PARAMETERS( type, idx ) \
    GET_INPUT_PARAMETERS( type, idx, ( std::string{ "backup." } + __func__ ) )

namespace HubPlatform {
namespace {

int subscribe( lua_State* luaState ) {
    auto eventsSubscriber = createFunctionalityCreator( luaState ).getPluginEventsSubscribeFunctionality( "backupd" );
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

int setSubscriptionFilters( lua_State* luaState ) {
    BACKUP_LUA_CHECK_INPUT_PARAMETERS( 2 );

    const auto subscriberId = BACKUP_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );
    const auto rules = BACKUP_LUA_GET_INPUT_PARAMETERS( Json, 2 );

    try {
        changeSubscriberEventsFilterRules( luaState, "backupd", subscriberId, rules );
    }
    catch( const std::exception& ex ) {
        return luaL_error(
            luaState, "fail to set event filter rule for a (%s) subscription: %s", subscriberId.c_str(), ex.what() );
    }

    return 0;
}

int unsubscribe( lua_State* luaState ) {
    auto eventsSubscriber = createFunctionalityCreator( luaState ).getPluginEventsSubscribeFunctionality( "backupd" );
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

const char* const moduleName = "backup";
const luaL_Reg module[] = { { "subscribe", errorCatchWrapper< subscribe > },
                            { "unsubscribe", errorCatchWrapper< unsubscribe > },
                            { NULL, NULL } };

}  // namespace
}  // namespace HubPlatform

HUB_MODULE_ENTRY_DEFINITION( HubPlatform::module, HubPlatform::moduleName );
