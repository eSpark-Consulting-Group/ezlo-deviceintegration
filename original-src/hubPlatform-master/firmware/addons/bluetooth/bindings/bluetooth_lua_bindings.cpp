#include "bluetooth/bluetooth_client.h"

#include "bluetooth/bluetooth_addon.h"
#include "json_utils/json_utils.h"
#include "binding_interfaces/lua/lua_binding_interface.h"

#define BLUETOOTH_LUA_CHECK_INPUT_PARAMETERS( number ) \
    CHECK_INPUT_PARAMETERS( ( std::string{ "bluetooth." } + __func__ ), number )

#define BLUETOOTH_LUA_GET_INPUT_PARAMETERS( type, idx ) \
    GET_INPUT_PARAMETERS( type, idx, ( std::string{ "bluetooth." } + __func__ ) )

namespace HubPlatform {
namespace {

BluetoothClient createBluetoothClient( lua_State* luaState ) {
    auto& addon = dynamic_cast< Addons::Addon& >( getHostService( luaState ) );
    return BluetoothClient( addon );
}

int startDiscovery( lua_State* luaState ) {
    createBluetoothClient( luaState ).startDiscovery();
    return 0;
}

int stopDiscovery( lua_State* luaState ) {
    createBluetoothClient( luaState ).stopDiscovery();
    return 0;
}

int startNotify( lua_State* luaState ) {
    const auto deviceAddress = lua_tostring( luaState, 1 );
    const auto uuid = lua_tostring( luaState, 2 );
    createBluetoothClient( luaState ).startNotify( deviceAddress, uuid );
    return 0;
}

int stopNotify( lua_State* luaState ) {
    const auto deviceAddress = lua_tostring( luaState, 1 );
    const auto uuid = lua_tostring( luaState, 2 );
    createBluetoothClient( luaState ).stopNotify( deviceAddress, uuid );
    return 0;
}

int removeDevice( lua_State* luaState ) {
    const auto deviceAddress = lua_tostring( luaState, 1 );
    createBluetoothClient( luaState ).removeDevice( deviceAddress );
    return 0;
}

int maintainConnection( lua_State* luaState ) {
    const auto deviceAddress = lua_tostring( luaState, 1 );
    createBluetoothClient( luaState ).maintainConnection( deviceAddress );
    return 0;
}

int pair( lua_State* luaState ) {
    const auto deviceAddress = lua_tostring( luaState, 1 );
    createBluetoothClient( luaState ).pair( deviceAddress );
    return 0;
}

int getDevice( lua_State* luaState ) {
    const auto deviceAddress = lua_tostring( luaState, 1 );
    const auto json = createBluetoothClient( luaState ).getDeviceAsJson( deviceAddress );
    return pushJsonAsLuaObj( luaState, json );
}

int connectToDevice( lua_State* luaState ) {
    const auto deviceAddress = lua_tostring( luaState, 1 );
    createBluetoothClient( luaState ).connectToDevice( deviceAddress );
    return 0;
}

int disconnectDevice( lua_State* luaState ) {
    const auto deviceAddress = lua_tostring( luaState, 1 );
    createBluetoothClient( luaState ).disconnectDevice( deviceAddress );
    return 0;
}

int writeCharacteristic( lua_State* luaState ) {
    createBluetoothClient( luaState )
        .writeCharacteristic( lua_tostring( luaState, 1 ),
                              lua_tostring( luaState, 2 ),
                              json_get_vec< uint8_t >( pullJson( luaState, 3 ) ) );
    return 0;
}

int readCharacteristic( lua_State* luaState ) {
    createBluetoothClient( luaState ).readCharacteristic( lua_tostring( luaState, 1 ), lua_tostring( luaState, 2 ) );
    return 0;
}

int subscribe( lua_State* luaState ) {
    auto eventsSubscriber =
        createFunctionalityCreator( luaState )
            .getPluginEventsSubscribeFunctionality( Addons::Addon::createQualifiedName( BluetoothAddon::Name ) );
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
    BLUETOOTH_LUA_CHECK_INPUT_PARAMETERS( 2 );

    const auto subscriberId = BLUETOOTH_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );
    const auto rules = BLUETOOTH_LUA_GET_INPUT_PARAMETERS( Json, 2 );

    try {
        changeSubscriberEventsFilterRules(
            luaState, Addons::Addon::createQualifiedName( BluetoothAddon::Name ), subscriberId, rules );
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
            .getPluginEventsSubscribeFunctionality( Addons::Addon::createQualifiedName( BluetoothAddon::Name ) );
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

const char* const moduleName = "bluetooth";
const luaL_Reg module[] = { { "start_discovery", errorCatchWrapper< startDiscovery > },
                            { "stop_discovery", errorCatchWrapper< stopDiscovery > },
                            { "start_notify", errorCatchWrapper< startNotify > },
                            { "stop_notify", errorCatchWrapper< stopNotify > },
                            { "remove_device", errorCatchWrapper< removeDevice > },
                            { "subscribe", errorCatchWrapper< subscribe > },
                            { "set_subscription_filters", errorCatchWrapper< setSubscriptionFilters > },
                            { "unsubscribe", errorCatchWrapper< unsubscribe > },
                            { "connect_to_device", errorCatchWrapper< connectToDevice > },
                            { "disconnect_device", errorCatchWrapper< disconnectDevice > },
                            { "write_characteristic", errorCatchWrapper< writeCharacteristic > },
                            { "read_characteristic", errorCatchWrapper< readCharacteristic > },
                            { "pair", errorCatchWrapper< pair > },
                            { "maintain_connection", errorCatchWrapper< maintainConnection > },
                            { "get_device", errorCatchWrapper< getDevice > },
                            { NULL, NULL } };

}  // namespace
}  // namespace HubPlatform

HUB_MODULE_ENTRY_DEFINITION( HubPlatform::module, HubPlatform::moduleName );
