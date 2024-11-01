#include "cloud_devices/cloud_devices_client.h"
#include "cloud_devices/cloud_devices_service.h"
#include "cloud_devices/resources/cloud_devices_resources.h"

#include "binding_interfaces/lua/lua_binding_interface.h"
#include "addon_interfaces/addon.h"
#include "log_utils/ezlo_log.h"
#include <tiny/tiny_consolecolors.h>

#define CLOUD_DEVICES_LUA_CHECK_INPUT_PARAMETERS( number ) \
    CHECK_INPUT_PARAMETERS( ( std::string{ "cloud_devices." } + __func__ ), number )

#define CLOUD_DEVICES_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( number ) \
    CHECK_INPUT_OPTIONAL_PARAMETERS( ( std::string{ "cloud_devices." } + __func__ ), number )

#define CLOUD_DEVICES_LUA_GET_INPUT_PARAMETERS( type, idx ) \
    GET_INPUT_PARAMETERS( type, idx, ( std::string{ "cloud_devices." } + __func__ ) )

namespace HubPlatform {
namespace {

CloudDevices::CloudDevicesClient createCloudDevicesClient( lua_State* luaState ) {
    auto& addon = dynamic_cast< Addons::Addon& >( getHostService( luaState ) );
    return CloudDevices::CloudDevicesClient( addon );
}

int getAllDevicesIds( lua_State* luaState ) {
    const auto json = createCloudDevicesClient( luaState ).getAllDevicesIds();
    return pushJsonAsLuaObj( luaState, json );
}

int getDevice( lua_State* luaState ) {
    CLOUD_DEVICES_LUA_CHECK_INPUT_PARAMETERS( 1 );

    const auto deviceId = CLOUD_DEVICES_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );

    const auto json = createCloudDevicesClient( luaState ).getDevice( deviceId );
    if( json.empty() ) {
        return luaL_error( luaState, "fail to get device with id: %s", deviceId.c_str() );
    }
    return pushJsonAsLuaObj( luaState, json, createMappingKeyFormatter( DefaultDbToLuaKeyMap ) );
}

int subscribe( lua_State* luaState ) {
    auto eventsSubscriber = createFunctionalityCreator( luaState )
                                .getPluginEventsSubscribeFunctionality( CloudDevices::CloudDevicesService::Name );
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
    CLOUD_DEVICES_LUA_CHECK_INPUT_PARAMETERS( 2 );

    const auto subscriberId = CLOUD_DEVICES_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );
    const auto rules = CLOUD_DEVICES_LUA_GET_INPUT_PARAMETERS( Json, 2 );

    try {
        changeSubscriberEventsFilterRules( luaState, CloudDevices::CloudDevicesService::Name, subscriberId, rules );
    }
    catch( const std::exception& ex ) {
        return luaL_error(
            luaState, "fail to set event filter rule for a (%s) subscription: %s", subscriberId.c_str(), ex.what() );
    }

    return 0;
}

int unsubscribe( lua_State* luaState ) {
    auto eventsSubscriber = createFunctionalityCreator( luaState )
                                .getPluginEventsSubscribeFunctionality( CloudDevices::CloudDevicesService::Name );
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

int sendCommand( lua_State* luaState ) {
    CLOUD_DEVICES_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 4 );
    const auto commandId = CLOUD_DEVICES_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );
    const auto deviceId = CLOUD_DEVICES_LUA_GET_INPUT_PARAMETERS( NativeString, 2 );
    const auto capability = CLOUD_DEVICES_LUA_GET_INPUT_PARAMETERS( NativeString, 3 );
    const auto command = CLOUD_DEVICES_LUA_GET_INPUT_PARAMETERS( NativeString, 4 );
    const auto parameters = pullJsonOpt( luaState, 5 );

    const auto error =
        createCloudDevicesClient( luaState ).sendCommand( commandId, deviceId, capability, command, parameters );

    if( !error.empty() ) {
        return luaL_error( luaState,
                           "fail to send command. command id: %s, device id: %s, capability: %s, command: %s. %s",
                           commandId.c_str(),
                           deviceId.c_str(),
                           capability.c_str(),
                           command.c_str(),
                           error.c_str() );
    }

    return 0;
}

int storeSubscriptionRecord( lua_State* luaState ) {
    const auto params = pullJsonOpt( luaState, 1 );
    const std::string command = HubPlatform::CloudDevices::Resources::GatewayScripts::Commands::StoreSubscriptionRecord;
    const auto error = createCloudDevicesClient( luaState ).sendCommand( command, params );

    if( !error.empty() ) {
        return luaL_error( luaState,
                           "fail to send command. command: %s, params: %s. %s",
                           command.c_str(),
                           params.dump().c_str(),
                           error.c_str() );
    }

    return 0;
}

int removeSubscriptionRecord( lua_State* luaState ) {
    const auto params = pullJsonOpt( luaState, 1 );
    const std::string command =
        HubPlatform::CloudDevices::Resources::GatewayScripts::Commands::RemoveSubscriptionRecord;
    const auto error = createCloudDevicesClient( luaState ).sendCommand( command, params );

    if( !error.empty() ) {
        return luaL_error( luaState,
                           "fail to send command. command: %s, params: %s. %s",
                           command.c_str(),
                           params.dump().c_str(),
                           error.c_str() );
    }

    return 0;
}

const char* const moduleName = "cloud_devices";
const luaL_Reg module[] = { { "get_all_devices_ids", errorCatchWrapper< getAllDevicesIds > },
                            { "get_device", errorCatchWrapper< getDevice > },
                            { "send_command", errorCatchWrapper< sendCommand > },
                            { "subscribe", errorCatchWrapper< subscribe > },
                            { "set_subscription_filters", errorCatchWrapper< setSubscriptionFilters > },
                            { "unsubscribe", errorCatchWrapper< unsubscribe > },
                            { "store_subscription_record", errorCatchWrapper< storeSubscriptionRecord > },
                            { "remove_subscription_record", errorCatchWrapper< removeSubscriptionRecord > },
                            { NULL, NULL } };

}  // namespace
}  // namespace HubPlatform

HUB_MODULE_ENTRY_DEFINITION( HubPlatform::module, HubPlatform::moduleName );
