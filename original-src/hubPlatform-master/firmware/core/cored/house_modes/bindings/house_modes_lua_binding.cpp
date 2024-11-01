#include "binding_interfaces/lua/lua_binding_interface.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/resources/core_resources.h"

#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"

#include <string>
#include <tiny/tiny_consolecolors.h>

#define HOUSE_MODES_LUA_CHECK_INPUT_PARAMETERS( number ) \
    CHECK_INPUT_PARAMETERS( ( std::string{ "house_modes." } + __func__ ), number )

#define HOUSE_MODES_LUA_GET_INPUT_PARAMETERS( type, idx ) \
    GET_INPUT_PARAMETERS( type, idx, ( std::string{ "house_modes." } + __func__ ) )

namespace HubPlatform {
namespace {

CoreDRemoteInterface::CoreDRemoteInterface makeRemoteInterface( lua_State* luaState ) {
    return { getHostService( luaState ) };
}

Json serializeHouseMode( const CoreDHouseModes::HouseModeEntry& houseMode ) {
    return JSON( "id" << houseMode.m_id << "name" << houseMode.m_name << "default" << houseMode.m_disarmedDefault
                      << "disarmed_devices" << houseMode.m_disarmedDeviceIds << "alarms_off_devices"
                      << houseMode.m_alarmsOffDeviceIds << "cameras_off_devices" << houseMode.m_camerasOffDeviceIds
                      << "bypass_devices" << houseMode.m_bypassDeviceIds << "armed" << houseMode.m_armed );
}

Json serializeHouseModes( const CoreDRemoteInterface::CoreDInterface::HouseModesGetResult& houseModes ) {
    Json result;

    for( const auto& houseMode : houseModes )
        result.push_back( houseMode.m_id );

    return result;
}

int subscribe( lua_State* luaState ) {
    using namespace CoreD::Resources;

    auto eventsSubscriber =
        createFunctionalityCreator( luaState ).getPluginEventsSubscribeFunctionality( GroupsNames::CORED );
    if( !eventsSubscriber ) {
        Log::warn( cc::warn( "Attempt to use eventsSubscriber functionality without proper permissions" ) );
        return luaL_error( luaState, "fail to subscribe" );
    }

    try {
        const auto id = eventsSubscriber->subscribe( getPluginEventsSubscriber( luaState, ModuleNames::HouseModes ) );
        lua_pushstring( luaState, id.c_str() );
    }
    catch( const std::exception& ex ) {
        return luaL_error( luaState, "fail to subscribe: %s", ex.what() );
    }

    return 1;
}

int unsubscribe( lua_State* luaState ) {
    using namespace CoreD::Resources;

    auto eventsSubscriber =
        createFunctionalityCreator( luaState ).getPluginEventsSubscribeFunctionality( GroupsNames::CORED );
    if( !eventsSubscriber ) {
        Log::warn( cc::warn( "Attempt to use eventsSubscriber functionality without proper permissions" ) );
        return luaL_error( luaState, "fail to unsubscribe" );
    }

    try {
        eventsSubscriber->unsubscribe( getPluginEventsSubscriber( luaState, ModuleNames::HouseModes ) );
    }
    catch( const std::exception& ex ) {
        return luaL_error( luaState, "fail to unsubscribe: %s", ex.what() );
    }

    return 0;
}

int startSwitch( lua_State* luaState ) {
    HOUSE_MODES_LUA_CHECK_INPUT_PARAMETERS( 1 )

    const auto id = HOUSE_MODES_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );

    const auto result = makeRemoteInterface( luaState ).setHouseMode( id );

    lua_pushnumber( luaState, json_get_int( result[ "result" ], "switchToDelay" ) );
    return 1;
}

int cancelSwitch( lua_State* luaState ) {
    HOUSE_MODES_LUA_CHECK_INPUT_PARAMETERS( 0 )

    const auto id = makeRemoteInterface( luaState ).cancelHouseModeSwitching();

    lua_pushstring( luaState, id.c_str() );
    return 1;
}

int getCurrent( lua_State* luaState ) {
    HOUSE_MODES_LUA_CHECK_INPUT_PARAMETERS( 0 )

    const auto id = makeRemoteInterface( luaState ).getCurrentHouseModeId();

    lua_pushstring( luaState, id.c_str() );
    return 1;
}

int getModes( lua_State* luaState ) {
    HOUSE_MODES_LUA_CHECK_INPUT_PARAMETERS( 0 )

    const auto houseModes = makeRemoteInterface( luaState ).getHouseModes();

    pushJsonAsLuaObj( luaState, serializeHouseModes( houseModes ) );
    return 1;
}

int getMode( lua_State* luaState ) {
    HOUSE_MODES_LUA_CHECK_INPUT_PARAMETERS( 1 )

    const auto id = HOUSE_MODES_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );

    if( auto houseMode = makeRemoteInterface( luaState ).getHouseModeById( id ) ) {
        return pushJsonAsLuaObj( luaState, serializeHouseMode( *houseMode ) );
    }
    else {
        lua_pushnil( luaState );
        return 1;
    }
}

int findModeByName( lua_State* luaState ) {
    HOUSE_MODES_LUA_CHECK_INPUT_PARAMETERS( 1 )

    const auto name = HOUSE_MODES_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );

    if( auto houseMode = makeRemoteInterface( luaState ).getHouseModeByName( name ) ) {
        return pushJsonAsLuaObj( luaState, serializeHouseMode( *houseMode ) );
    }
    else {
        lua_pushnil( luaState );
        return 1;
    }
}

int addProtectButton( lua_State* luaState ) {
    HOUSE_MODES_LUA_CHECK_INPUT_PARAMETERS( 2 )

    const auto deviceId = HOUSE_MODES_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );
    const auto serviceName = HOUSE_MODES_LUA_GET_INPUT_PARAMETERS( NativeString, 2 );

    try {
        auto jsonObj = makeRemoteInterface( luaState )
                           .houseModesProtectButtonsSet( deviceId, CoreDHouseModes::toProtectService( serviceName ) );
        if( !hasError( jsonObj ) ) {
            lua_pushnil( luaState );
            return 1;
        }
        return luaL_error( luaState, "Failed to set sos buttons" );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Fail to add sos button: " ) + ex.what() );
        return luaL_error( luaState, "fail to add sos button: %s", ex.what() );
    }
}

int removeProtectButton( lua_State* luaState ) {
    HOUSE_MODES_LUA_CHECK_INPUT_PARAMETERS( 1 )

    const auto deviceId = HOUSE_MODES_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );

    try {
        auto jsonObj = makeRemoteInterface( luaState )
                           .houseModesProtectButtonsSet( deviceId, CoreDHouseModes::toProtectService( "" ) );
        if( !hasError( jsonObj ) ) {
            lua_pushnil( luaState );
            return 1;
        }
        return luaL_error( luaState, "Failed to set sos buttons" );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "fail to remove sos button: " ) + ex.what() );
        return luaL_error( luaState, "fail to remove sos button: %s", ex.what() );
    }
}

int getProtectButtonList( lua_State* luaState ) {
    auto protectButtons = makeRemoteInterface( luaState ).getHouseModesProtectButtons();

    JsonArray buttonsResult = JSON_EMPTY_ARR;
    for( const auto& button : protectButtons ) {
        buttonsResult.push_back(
            JSON( "device_id" << button.deviceId() << "service" << CoreDHouseModes::toString( button.service() ) ) );
    }

    if( !buttonsResult.empty() ) {
        return pushJsonAsLuaObj( luaState, buttonsResult );
    }
    else {
        lua_pushnil( luaState );
        return 1;
    }
}

const char moduleName[] = "house_modes";
const luaL_Reg module[] = { { "subscribe", errorCatchWrapper< subscribe > },
                            { "unsubscribe", errorCatchWrapper< unsubscribe > },
                            { "switch", errorCatchWrapper< startSwitch > },
                            { "cancel_switch", errorCatchWrapper< cancelSwitch > },
                            { "get_current", errorCatchWrapper< getCurrent > },
                            { "get_modes", errorCatchWrapper< getModes > },
                            { "get_mode", errorCatchWrapper< getMode > },
                            { "find_mode_by_name", errorCatchWrapper< findModeByName > },
                            { "add_protect_sos_button", errorCatchWrapper< addProtectButton > },
                            { "remove_protect_sos_button", errorCatchWrapper< removeProtectButton > },
                            { "get_protect_sos_buttons", errorCatchWrapper< getProtectButtonList > },
                            { nullptr, nullptr } };

}  // namespace
}  // namespace HubPlatform

HUB_MODULE_ENTRY_DEFINITION( HubPlatform::module, HubPlatform::moduleName );
