#include "binding_interfaces/lua/lua_binding_interface.h"
#include "service_interfaces/service.h"
#include "database_client/key_value_db_client.h"
#include "database_client/dbtools.h"
#include "config_reader/config_reader.h"
#include "config_reader/config_keys.h"
#include "log_utils/ezlo_log.h"

#include <stdexcept>
#include <string>
#include <utility>

/* see the documentation on http://log.ezlo.lan/new/scripting/lua/#Storage module */
/* we use a plugin name as a table name, as the documentation requires only one 'volume' for a plugin */

namespace HubPlatform {
namespace {

#define CHECK_LUA_INPUT_PARAMETERS( callFunction, number ) \
    CHECK_INPUT_PARAMETERS( ( std::string{ "storage." } + callFunction ), number )

constexpr std::size_t defaultPluginStorageLimit = 1024 * 1024;

class DbLuaBindingError : public std::runtime_error {
public:
    DbLuaBindingError( const std::string& errDesription )
    : runtime_error( "DbLuaBinding: storage." + errDesription ) {}
};

std::size_t getPluginStorageMaxSize( const std::string& /* pluginId */ ) {
    /* TODO: each plugin has to have individual settings */
    try {
        ConfigReader configReader;

        if( configReader.hasValue( ConfigKeys::PluginStorageLimit ) )
            return configReader.getInt( ConfigKeys::PluginStorageLimit );
    }
    catch( const ConfigReader::ReadingError& exception ) {
        throw DbLuaBindingError{ exception.what() };
    }

    Log::warn( "DbLuaBinding: use a default plugin storage limit" );
    return defaultPluginStorageLimit;
}

Db::KeyValueDbClient getPluginKeyValueDbClient( lua_State* luaState ) {
    auto pluginId = getPluginId( luaState );
    auto maxSize = getPluginStorageMaxSize( pluginId );
    auto pluginStorageDbTableName = getPluginDBName( pluginId );

    return Db::KeyValueDbClient{ getHostService( luaState ).getDBClient(), pluginStorageDbTableName, maxSize };
}

template < typename ValueType >
using KeyValue = std::pair< std::string, ValueType >;

template < typename ValueType >
KeyValue< ValueType > getKeyValue( lua_State* luaState, const char* const callFunction ) {
    auto key = LuaToNative< NativeString >{}( luaState, 1 );
    auto value = LuaToNative< ValueType >{}( luaState, 2, {}, true );

    if( !key )
        throw DbLuaBindingError{ std::string{ callFunction } + ": an incorrect key" };

    if( !value )
        throw DbLuaBindingError{ std::string{ callFunction } + ": an incorrect value" };

    return { std::move( key ).value(), std::move( value ).value() };
}

std::string getKey( lua_State* luaState, const char* const callFunction ) {
    auto key = LuaToNative< NativeString >{}( luaState, 1 );
    if( !key )
        throw DbLuaBindingError{ std::string{ callFunction } + ": an incorrect key" };

    return std::move( key ).value();
}

/* arguments: key (string), value (bool)
 * return: nothing, in case of an error throws an exception */
int setBool( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( __func__, 2 );

    const auto keyValue = getKeyValue< NativeBool >( luaState, __func__ );
    auto db = getPluginKeyValueDbClient( luaState );

    auto res = db.setBool( keyValue.first, keyValue.second );
    if( !res )
        throw DbLuaBindingError{ "setBool: fail to set a bool value" };

    return 0;
}

/* arguments: key (string), value (float)
 * return: nothing, in case of an error throws an exception */
int setNumber( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( __func__, 2 );

    auto db = getPluginKeyValueDbClient( luaState );

    bool res = false;
    if( lua_isinteger( luaState, 2 ) ) {
        auto keyValue = getKeyValue< lua_Integer >( luaState, __func__ );
        res = db.setInteger( keyValue.first, keyValue.second );
    }
    else {
        auto keyValue = getKeyValue< lua_Number >( luaState, __func__ );
        res = db.setNumber( keyValue.first, keyValue.second );
    }

    if( !res )
        throw DbLuaBindingError{ "setNumber: fail to set a number value" };

    return 0;
}

/* arguments: key (string), value (string)
 * return: nothing, in case of an error throws an exception */
int setString( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( __func__, 2 );

    const auto keyValue = getKeyValue< NativeString >( luaState, __func__ );
    auto db = getPluginKeyValueDbClient( luaState );

    auto res = db.setString( keyValue.first, keyValue.second );
    if( !res )
        throw DbLuaBindingError{ "setString: fail to set a string value" };

    return 0;
}

/* arguments: key (string), value (table)
 * return: nothing, in case of an error throws an exception */
int setTable( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( __func__, 2 );

    const auto keyValue = getKeyValue< Json >( luaState, __func__ );
    auto db = getPluginKeyValueDbClient( luaState );

    auto res = db.setJson( keyValue.first, keyValue.second );
    if( !res )
        throw DbLuaBindingError{ "setTable: fail to set a table value" };

    return 0;
}

/* arguments: key (string)
 * return: a result of the operation (bool) or nil */
int getBool( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( __func__, 1 );

    const auto key = getKey( luaState, __func__ );
    auto db = getPluginKeyValueDbClient( luaState );

    auto res = db.getBool( key );
    if( res.second )
        lua_pushboolean( luaState, res.first );
    else
        lua_pushnil( luaState );

    return 1;
}

/* arguments: key (string)
 * return: a result of the operation (float) or nil */
int getNumber( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( __func__, 1 );

    const auto key = getKey( luaState, __func__ );
    auto db = getPluginKeyValueDbClient( luaState );

    auto res = db.getJson( key );
    if( !res.second || !res.first.is_number() )
        lua_pushnil( luaState );
    else
        pushJsonAsLuaObj( luaState, res.first, nullptr, 0, true );

    return 1;
}

/* arguments: key (string)
 * return: a result of the operation (string) or nil */
int getString( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( __func__, 1 );

    const auto key = getKey( luaState, __func__ );
    auto db = getPluginKeyValueDbClient( luaState );

    auto res = db.getString( key );
    if( res.second )
        lua_pushlstring( luaState, res.first.c_str(), res.first.size() );
    else
        lua_pushnil( luaState );

    return 1;
}

/* arguments: key (string)
 * return: a result of the operation (table) or nil */
int getTable( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( __func__, 1 );

    const auto key = getKey( luaState, __func__ );
    auto db = getPluginKeyValueDbClient( luaState );

    auto res = db.getJson( key );
    if( res.second )
        pushJsonAsLuaObj( luaState, res.first, nullptr, 0, true );
    else
        lua_pushnil( luaState );

    return 1;
}

/* arguments: key (string)
 * return: a result of the operation (does the pair with such a key exist) (bool) */
int exists( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( __func__, 1 );

    const auto key = getKey( luaState, __func__ );
    auto db = getPluginKeyValueDbClient( luaState );

    auto res = db.exist( key );
    lua_pushboolean( luaState, res ? 1 : 0 );

    return 1;
}

/* arguments: key (string)
 * return: nothing */
int remove( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( __func__, 1 );

    const auto key = getKey( luaState, __func__ );
    auto db = getPluginKeyValueDbClient( luaState );

    db.remove( key );

    return 0;
}

/* arguments: nothing
 * return: nothing */
int clear( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( __func__, 0 );

    getPluginKeyValueDbClient( luaState ).clear();

    return 0;
}

const char moduleName[] = "storage";
const luaL_Reg module[] = {
    { "set_bool", errorCatchWrapper< setBool > },     { "set_number", errorCatchWrapper< setNumber > },
    { "set_string", errorCatchWrapper< setString > }, { "set_table", errorCatchWrapper< setTable > },
    { "get_bool", errorCatchWrapper< getBool > },     { "get_number", errorCatchWrapper< getNumber > },
    { "get_string", errorCatchWrapper< getString > }, { "get_table", errorCatchWrapper< getTable > },
    { "exists", errorCatchWrapper< exists > },        { "delete", errorCatchWrapper< remove > },
    { "delete_all", errorCatchWrapper< clear > },     { nullptr, nullptr }
};

}  // namespace
}  // namespace HubPlatform

HUB_MODULE_ENTRY_DEFINITION( HubPlatform::module, HubPlatform::moduleName );
