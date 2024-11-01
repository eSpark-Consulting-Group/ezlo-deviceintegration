#include "binding_interfaces/lua/lua_binding_interface.h"

#include "luad/modules/timer/timer_manager.h"
#include "luad/modules/timer/timer_type.h"
#include "luad/addon/lua_addon.h"

#define TIMER_LUA_CHECK_INPUT_PARAMETERS( number ) \
    CHECK_INPUT_PARAMETERS( ( std::string{ "timer." } + __func__ ), number )

#define TIMER_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( number ) \
    CHECK_INPUT_OPTIONAL_PARAMETERS( ( std::string{ "timer." } + __func__ ), number )

#define TIMER_LUA_GET_INPUT_PARAMETERS( type, idx ) \
    GET_INPUT_PARAMETERS( type, idx, ( std::string{ "timer." } + __func__ ) )

namespace HubPlatform {
namespace Luad {
namespace {

class TimerNotReadyException : public std::logic_error {
public:
    TimerNotReadyException()
    : std::logic_error( "timer is not ready" ) {}
};

TimerManager& getTimerManager( lua_State* luaStates ) {
    auto& addon = dynamic_cast< LuaAddon::LuaAddon& >( getHostService( luaStates ) );
    auto result = addon.getTimerManager();
    if( !result ) {
        throw TimerNotReadyException();
    }
    return *result;
}

int setTimeout( lua_State* luaState ) {
    TIMER_LUA_CHECK_INPUT_PARAMETERS( 3 );

    const auto delay = TIMER_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 );
    auto script = TIMER_LUA_GET_INPUT_PARAMETERS( NativeString, 2 );
    auto params = TIMER_LUA_GET_INPUT_PARAMETERS( Json, 3 );

    try {
        const auto timerId = getTimerManager( luaState )
                                 .setTimer( getPluginId( luaState ),
                                            TimerDuration{ delay },
                                            std::move( script ),
                                            std::move( params ),
                                            TimerType::Single );
        lua_pushstring( luaState, timerId.c_str() );

        return 1;
    }
    catch( const std::exception& e ) {
        return luaL_error(
            luaState, "fail to set timeout (%s), params: %s : %s", script.data(), params.dump().data(), e.what() );
    }
}

int setTimeoutWithId( lua_State* luaState ) {
    TIMER_LUA_CHECK_INPUT_PARAMETERS( 4 );

    const auto delay = TIMER_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 );
    auto timerId = TIMER_LUA_GET_INPUT_PARAMETERS( NativeString, 2 );
    auto script = TIMER_LUA_GET_INPUT_PARAMETERS( NativeString, 3 );
    auto params = TIMER_LUA_GET_INPUT_PARAMETERS( Json, 4 );

    try {
        getTimerManager( luaState )
            .setTimerWithId( getPluginId( luaState ),
                             std::move( timerId ),
                             TimerDuration{ delay },
                             std::move( script ),
                             std::move( params ),
                             TimerType::Single );
        return 0;
    }
    catch( const std::exception& e ) {
        return luaL_error(
            luaState, "fail to set interval (%s), params: %s : %s", script.data(), params.dump().data(), e.what() );
    }
}

int setInterval( lua_State* luaState ) {
    TIMER_LUA_CHECK_INPUT_PARAMETERS( 3 );

    const auto delay = TIMER_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 );
    auto script = TIMER_LUA_GET_INPUT_PARAMETERS( NativeString, 2 );
    auto params = TIMER_LUA_GET_INPUT_PARAMETERS( Json, 3 );

    try {
        const auto timerId = getTimerManager( luaState )
                                 .setTimer( getPluginId( luaState ),
                                            TimerDuration{ delay },
                                            std::move( script ),
                                            std::move( params ),
                                            TimerType::Periodic );
        lua_pushstring( luaState, timerId.c_str() );

        return 1;
    }
    catch( const std::exception& e ) {
        return luaL_error(
            luaState, "fail to set interval (%s), params: %s : %s", script.data(), params.dump().data(), e.what() );
    }
}

int setIntervalWithId( lua_State* luaState ) {
    TIMER_LUA_CHECK_INPUT_PARAMETERS( 4 );

    const auto delay = TIMER_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 );
    auto timerId = TIMER_LUA_GET_INPUT_PARAMETERS( NativeString, 2 );
    auto script = TIMER_LUA_GET_INPUT_PARAMETERS( NativeString, 3 );
    auto params = TIMER_LUA_GET_INPUT_PARAMETERS( Json, 4 );

    try {
        getTimerManager( luaState )
            .setTimerWithId( getPluginId( luaState ),
                             std::move( timerId ),
                             TimerDuration{ delay },
                             std::move( script ),
                             std::move( params ),
                             TimerType::Periodic );
        return 0;
    }
    catch( const std::exception& e ) {
        return luaL_error(
            luaState, "fail to set interval (%s), params: %s : %s", script.data(), params.dump().data(), e.what() );
    }
}

int getArguments( lua_State* luaState ) {
    TIMER_LUA_CHECK_INPUT_PARAMETERS( 1 );

    const auto timerId = TIMER_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );

    if( const auto parameters = getTimerManager( luaState ).getTimerParameters( getPluginId( luaState ), timerId ) ) {
        return pushJsonAsLuaObj( luaState, *parameters );
    }

    lua_pushnil( luaState );
    return 1;
}

int resetArguments( lua_State* luaState ) {
    TIMER_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 1 );

    const auto timerId = TIMER_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );
    auto params = pullJson( luaState, 2 );

    getTimerManager( luaState ).setTimerParameters( getPluginId( luaState ), timerId, std::move( params ) );

    return 0;
}

int cancel( lua_State* luaState ) {
    TIMER_LUA_CHECK_INPUT_PARAMETERS( 1 );

    const auto timerId = TIMER_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );

    try {
        getTimerManager( luaState ).stopTimer( getPluginId( luaState ), timerId );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to cancel timer (%s) : %s", timerId.c_str(), e.what() );
    }

    return 0;
}

int exists( lua_State* luaState ) {
    TIMER_LUA_CHECK_INPUT_PARAMETERS( 1 );

    const auto timerId = TIMER_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );

    try {
        lua_pushboolean( luaState, getTimerManager( luaState ).hasTimer( getPluginId( luaState ), timerId ) );
        return 1;
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to <error_message> timer (%s) : %s", timerId.c_str(), e.what() );
    }
}

int reset( lua_State* luaState ) {
    TIMER_LUA_CHECK_INPUT_PARAMETERS( 2 );

    const auto delay = TIMER_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 );
    const auto timerId = TIMER_LUA_GET_INPUT_PARAMETERS( NativeString, 2 );

    try {
        getTimerManager( luaState ).restartTimer( getPluginId( luaState ), timerId, TimerDuration{ delay } );
        return 0;
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to restart timer (%s) : %s", timerId.c_str(), e.what() );
    }
}

constexpr char moduleName[] = "timer";
constexpr luaL_Reg module[] = { { "set_timeout", errorCatchWrapper< setTimeout > },
                                { "set_timeout_with_id", errorCatchWrapper< setTimeoutWithId > },
                                { "set_interval", errorCatchWrapper< setInterval > },
                                { "set_interval_with_id", errorCatchWrapper< setIntervalWithId > },
                                { "get_arguments", errorCatchWrapper< getArguments > },
                                { "reset_arguments", errorCatchWrapper< resetArguments > },
                                { "cancel", errorCatchWrapper< cancel > },
                                { "exists", errorCatchWrapper< exists > },
                                { "reset", errorCatchWrapper< reset > },
                                { nullptr, nullptr } };

}  // namespace
}  // namespace Luad
}  // namespace HubPlatform

HUB_BASE_MODULE_ENTRY_DEFINITION( timer, HubPlatform::Luad::module, HubPlatform::Luad::moduleName );
