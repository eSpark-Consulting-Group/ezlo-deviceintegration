#include "2gig/2gig_lua_client.h"
#include "2gig/2gig_addon.h"

#include "binding_interfaces/lua/lua_binding_interface.h"
#include "addon_interfaces/addon.h"
#include "log_utils/ezlo_log.h"
#include <tiny/tiny_consolecolors.h>

#define TWOGIG_LUA_CHECK_INPUT_PARAMETERS( number ) \
    CHECK_INPUT_PARAMETERS( ( std::string{ "twogig." } + __func__ ), number )

#define TWOGIG_LUA_GET_INPUT_PARAMETERS( type, idx ) \
    GET_INPUT_PARAMETERS( type, idx, ( std::string{ "twogig." } + __func__ ) )

namespace HubPlatform {
namespace {

TwoGig::LuaClient createClient( lua_State* luaState ) {
    auto& addon = dynamic_cast< Addons::Addon& >( getHostService( luaState ) );
    return TwoGig::LuaClient( addon );
}

int subscribe( lua_State* luaState ) {
    auto eventsSubscriber =
        createFunctionalityCreator( luaState )
            .getPluginEventsSubscribeFunctionality( Addons::Addon::createQualifiedName( TwoGig::Addon::Name ) );
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
    TWOGIG_LUA_CHECK_INPUT_PARAMETERS( 2 );

    const auto subscriberId = TWOGIG_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );
    const auto rules = TWOGIG_LUA_GET_INPUT_PARAMETERS( Json, 2 );

    try {
        changeSubscriberEventsFilterRules(
            luaState, Addons::Addon::createQualifiedName( TwoGig::Addon::Name ), subscriberId, rules );
    }
    catch( const std::exception& ex ) {
        return luaL_error(
            luaState, "fail to set event filter rule for a (%s) subscription: %s", subscriberId.c_str(), ex.what() );
    }

    return 0;
}

int unsubscribe( lua_State* luaState ) {
    auto eventsSubscriber =
        createFunctionalityCreator( luaState )
            .getPluginEventsSubscribeFunctionality( Addons::Addon::createQualifiedName( TwoGig::Addon::Name ) );
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

int reset( lua_State* luaState ) {
    createClient( luaState ).reset();

    return 0;
}

int startDeviceDiscovering( lua_State* luaState ) {
    createClient( luaState ).startDeviceDiscovering();

    return 0;
}

int stopDeviceDiscovering( lua_State* luaState ) {
    createClient( luaState ).stopDeviceDiscovering();

    return 0;
}

int addDevice( lua_State* luaState ) {
    TWOGIG_LUA_CHECK_INPUT_PARAMETERS( 1 )

    auto deviceInfo = pullJson( luaState, 1 );

    createClient( luaState ).addDevice( std::move( deviceInfo ) );

    return 0;
}

int removeDevice( lua_State* luaState ) {
    TWOGIG_LUA_CHECK_INPUT_PARAMETERS( 1 )

    auto txIds = pullJson( luaState, 1 );

    createClient( luaState ).removeDevice( txIds );

    return 0;
}

int configureDevice( lua_State* luaState ) {
    TWOGIG_LUA_CHECK_INPUT_PARAMETERS( 2 )

    auto txIds = pullJson( luaState, 1 );
    auto config = pullJson( luaState, 2 );

    createClient( luaState ).configureDevice( txIds, std::move( config ) );

    return 0;
}

const char* const moduleName = "2gig";
const luaL_Reg module[] = { { "subscribe", errorCatchWrapper< subscribe > },
                            { "set_subscription_filters", errorCatchWrapper< setSubscriptionFilters > },
                            { "unsubscribe", errorCatchWrapper< unsubscribe > },
                            { "reset", errorCatchWrapper< reset > },
                            { "start_device_discovering", errorCatchWrapper< startDeviceDiscovering > },
                            { "stop_device_discovering", errorCatchWrapper< stopDeviceDiscovering > },
                            { "add_device", errorCatchWrapper< addDevice > },
                            { "remove_device", errorCatchWrapper< removeDevice > },
                            { "configure_device", errorCatchWrapper< configureDevice > },
                            { nullptr, nullptr } };

}  // namespace
}  // namespace HubPlatform

HUB_MODULE_ENTRY_DEFINITION( HubPlatform::module, HubPlatform::moduleName );
