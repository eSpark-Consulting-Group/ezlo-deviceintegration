#include "luad/common/luup_api.h"
#include "luad/old_luup/luup_engine.h"

#include "log_utils/ezlo_log.h"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include <string>
#include <exception>

/* The implementation of the luup.job sub-API --
 * http://wiki.micasaverde.com/index.php/Luup_Lua_extensions#Module:_luup.job */

namespace {
extern const luaL_Reg luupJobModule[];
}

using namespace HubPlatform;

class LuupJobAPI final : public LuupAPI {
public:
    LuupJobAPI() {
        registerModule( "job", luupJobModule );
    }

    static int luupJobStatus( lua_State* luaState );
    static int luupJobSet( lua_State* luaState );
    static int luupJobSetting( lua_State* luaState );
};

class JobError : public std::runtime_error {
public:
    JobError( const std::string& errDesription )
    : runtime_error( "job." + errDesription ) {}
};

namespace {

const luaL_Reg luupJobModule[] = { { "status", LuupJobAPI::luupJobStatus },
                                   { "set", LuupJobAPI::luupJobSet },
                                   { "setting", LuupJobAPI::luupJobSetting },
                                   { NULL, NULL } };
}

static LuupJobAPI luupIOAPI;

/* arguments: job_number (number), device (string or number);
 * return: job_status (number), notes (string) */
int LuupJobAPI::luupJobStatus( lua_State* luaState ) {
    if( lua_gettop( luaState ) != 2 )
        throw JobError{ "status: an incorrect number of arguments" };

    int isNum = 0;
    int asyncTaskID = lua_tonumberx( luaState, 1, &isNum ); /* async task (new luup) == job (old luup) */
    if( !isNum )
        throw JobError{ "status: an incorrect 'job_number' argument" };

    /* TODO: now support only a device id */
    auto deviceID = lua_type( luaState, 2 ) == LUA_TNUMBER ? lua_tonumber( luaState, 1 ) :
                                                             throw JobError{ "status: an incorrect 'device' argument" };

    const auto& asyncTask = Device::makeDevice( deviceID ).getAsyncLuaTask( asyncTaskID );

    if( !asyncTask.isValid() ) {
        Log::error( "job.status: fail to find" );
        lua_pushnumber( luaState, static_cast< int >( AsyncLuaTask::Status::None ) );
        return 1;
    }

    lua_pushnumber( luaState, static_cast< int >( asyncTask.getStatus() ) );
    lua_pushstring( luaState, asyncTask.getNotes().c_str() );
    return 2;
}

/* TODO: job.set and job.setting get a raw pointer (a full lua userdata) as a first argument,
 *       but there's no example or description where and how an object the pointer points to
 *       gets created, the old luup casts such usedata to a 'Job' class, we have to follow
 *       the same way... */
/* TODO: an incorrect userdata passed from a lua script may cause an undefined behaviour... */

/* arguments: job (userdata), setting (string), value (string);
 * return: nothing */
int LuupJobAPI::luupJobSet( lua_State* luaState ) {
    if( lua_gettop( luaState ) != 3 )
        throw JobError{ "set: an incorrect number of arguments" };

    auto* asyncTask = static_cast< AsyncLuaTask* >( lua_touserdata( luaState, 1 ) );
    if( !asyncTask )
        throw JobError{ "set: an incorrect 'job' argument" };

    size_t settingLen = 0;
    auto cSetting = lua_tolstring( luaState, 2, &settingLen );
    if( !cSetting )
        throw JobError{ "set: an incorrect 'setting' argument" };

    std::string setting{ cSetting, settingLen };

    size_t valueLen = 0;
    auto cValue = lua_tolstring( luaState, 3, &valueLen );
    if( !cValue )
        throw JobError{ "set: an incorrect 'value' argument" };

    std::string value{ cValue, valueLen };

    if( !asyncTask->isValid() ) {
        Log::error( "job.set: an async task is invalid" );
        return 0;
    }

    asyncTask->setSetting( setting, value );

    return 0;
}

/* arguments: job (userdata), setting (string);
 * return: value (string) */
int LuupJobAPI::luupJobSetting( lua_State* luaState ) {
    if( lua_gettop( luaState ) != 2 )
        throw JobError{ "setting: an incorrect number of arguments" };

    auto* asyncTask = static_cast< AsyncLuaTask* >( lua_touserdata( luaState, 1 ) );
    if( !asyncTask )
        throw JobError{ "setting: an incorrect 'job' argument" };

    size_t settingLen = 0;
    auto cSetting = lua_tolstring( luaState, 2, &settingLen );
    if( !cSetting )
        throw JobError{ "setting: an incorrect 'setting' argument" };

    std::string setting{ cSetting, settingLen };

    if( !asyncTask->isValid() ) {
        Log::error( "job.setting: an async task is invalid" );
        return 0;
    }

    std::string value = asyncTask->getSettingValue( setting );
    if( value.empty() )
        Log::warn( "job.setting: a requested setting '" + setting + "' doesn't exist" );

    lua_pushlstring( luaState, value.c_str(), value.size() );
    return 1;
}
