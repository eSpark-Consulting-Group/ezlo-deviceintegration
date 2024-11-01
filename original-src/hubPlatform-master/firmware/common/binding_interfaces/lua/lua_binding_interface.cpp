#include "binding_interfaces/lua/lua_binding_interface.h"
#include "binding_interfaces/lua/lua_module_interface.h"

#include "addon_interfaces/addon.h"
#include "json_utils/json_wrapper.h"
#include "jsonrpc/jsonrpc.h"

#include <cassert>
#include <numeric>
#include <stdexcept>

namespace HubPlatform {

const KeyMap DefaultDbToLuaKeyMap = { { "_id", "id" } };

template <>
LuaLookupStringRes LuaToNative< NativeString >::operator()( lua_State* luaState,
                                                            int idx,
                                                            std::string name,
                                                            bool handleKeyTypes );

template <>
LuaLookupBoolRes LuaToNative< NativeBool >::operator()( lua_State* luaState,
                                                        int idx,
                                                        std::string name,
                                                        bool handleKeyTypes );

template <>
LuaLookupIntRes LuaToNative< NativeInteger >::operator()( lua_State* luaState,
                                                          int idx,
                                                          std::string name,
                                                          bool handleKeyTypes );

template <>
LuaLookupInt64Res LuaToNative< NativeInt64 >::operator()( lua_State* luaState,
                                                          int idx,
                                                          std::string name,
                                                          bool handleKeyTypes );

template <>
LuaLookupUint8Res LuaToNative< NativeUint8 >::operator()( lua_State* luaState,
                                                          int idx,
                                                          std::string name,
                                                          bool handleKeyTypes );

template <>
LuaLookupUint32Res LuaToNative< NativeUint32 >::operator()( lua_State* luaState,
                                                            int idx,
                                                            std::string name,
                                                            bool handleKeyTypes );

namespace {

constexpr char luaDataKey[] = "__lua_binding_interface_data";
constexpr char luaHostServiceKey[] = "host_service";
constexpr char luaRunningPluginIdKey[] = "running_plugin_id";
constexpr char luaRunningAnonymousPluginKey[] = "running_anonymous_plugin";
constexpr char luaModuleAPIRestrictionsKey[] = "lua_module_api_restrictions";

constexpr char KeyTypeEscapingSymbol = '\\';
constexpr char KeyTypeNumberId = 'f';
constexpr char KeyTypeIntegerId = 'i';
constexpr char KeyTypeBoolId = 'b';

std::string formatKey( const KeyFormatter& keyFormatter, std::string str ) {
    return keyFormatter ? keyFormatter( std::move( str ) ) : std::move( str );
}

void setHostService( lua_State* luaState, Services::Service* service ) {
    lua_getglobal( luaState, luaDataKey );

    lua_pushstring( luaState, luaHostServiceKey );
    lua_pushlightuserdata( luaState, service );
    lua_rawset( luaState, -3 );
    lua_pop( luaState, 1 );
}

void registerDataTable( lua_State* luaState ) {
    lua_newtable( luaState );
    lua_setglobal( luaState, luaDataKey );
}

/* there's no any standard way to get whether a lua table is empty or not... */
bool isLuaTableEmpty( lua_State* luaState, int idx ) {
    if( !lua_istable( luaState, idx ) ) {
        throw LuaError{ "trying to check is_table_empty on a non-table object" };
    }

    if( idx < 0 ) {
        idx = lua_absindex( luaState, idx );
    }

    lua_pushnil( luaState );
    if( lua_next( luaState, idx ) == 0 ) {
        return true;
    }

    lua_pop( luaState, 2 );
    return false;
}

bool isLuaArray( lua_State* luaState, int idx, int& arraySize ) {
    if( !lua_istable( luaState, idx ) ) {
        throw LuaError{ "trying to check is_table_empty on a non-table object" };
    }

    if( idx < 0 ) {
        idx = lua_absindex( luaState, idx );
    }

    arraySize = 0;
    lua_pushnil( luaState );

    while( lua_next( luaState, idx ) ) {
        if( !lua_isinteger( luaState, -2 ) ) {
            lua_pop( luaState, 2 );
            return false;
        }

        if( lua_geti( luaState, idx, ++arraySize ) == LUA_TNIL ) {
            lua_pop( luaState, 3 );
            return false;
        }

        lua_pop( luaState, 2 );
    }

    return true;
}

}  // namespace

void initLuaBindingInterface( lua_State* luaState, Services::Service* service ) {
    assert( service );
    registerDataTable( luaState );
    setHostService( luaState, service );
}

Services::Service& getHostService( lua_State* luaState ) {
    lua_getglobal( luaState, luaDataKey );

    lua_pushstring( luaState, luaHostServiceKey );
    lua_gettable( luaState, -2 );
    auto info = static_cast< Services::Service* >( lua_touserdata( luaState, -1 ) );
    lua_pop( luaState, 2 );

    return *info;
}

/* lua stack before a call:
 *   #  Name          Type                     Required
 *   1. scriptName    string                   +
 *   2. filterRules   table (array of tables)  -
 */
PluginEventsSubscriber getPluginEventsSubscriber( lua_State* luaState, const std::string& moduleName ) {
    const auto scriptName = lua_isstring( luaState, 1 ) ? lua_tostring( luaState, 1 ) : nullptr;
    if( !scriptName ) {
        throw LuaError{ "invalid script name for event listener" };
    }

    auto eventFilterRules = pullJson( luaState, 2 );
    if( eventFilterRules.is_null() ) {
        eventFilterRules = JSON_EMPTY_OBJ;
    }

    auto& addon = dynamic_cast< Addons::Addon& >( getHostService( luaState ) );

    return PluginEventsSubscriber(
        addon.getHostChannel(), getPluginId( luaState ), moduleName, scriptName, eventFilterRules );
}

void changeSubscriberEventsFilterRules( lua_State* luaState,
                                        const std::string& eventsEmitterName,
                                        const std::string& subscriberId,
                                        const JsonArray& rules,
                                        const std::string& moduleName ) {
    if( !PluginEventsSubscriber::checkFilterRules( rules ) ) {
        throw std::runtime_error( "invalid events filter rules" );
    }

    const auto result =
        getHostService( luaState )
            .sendRequest( eventsEmitterName,
                          CHANGE_SUBSCRIBER_EVENT_FILTERS,
                          JSON( "moduleName" << moduleName << "subscriberId" << subscriberId << "rules" << rules ) );
    if( hasError( result ) ) {
        throw std::runtime_error( "invalid subscriberId" );
    }
}

void setPluginId( lua_State* luaState, const std::string& pluginId ) {
    lua_getglobal( luaState, luaDataKey );

    lua_pushstring( luaState, luaRunningPluginIdKey );
    lua_pushlstring( luaState, pluginId.c_str(), pluginId.size() );
    lua_rawset( luaState, -3 );
    lua_pop( luaState, 1 );
}

std::string getPluginId( lua_State* luaState ) {
    lua_getglobal( luaState, luaDataKey );

    lua_pushstring( luaState, luaRunningPluginIdKey );
    lua_gettable( luaState, -2 );
    auto scriptName = LuaToNative< NativeString >{}( luaState, -1 );
    lua_pop( luaState, 2 );

    return scriptName.valueOr( std::string() );
}

void setRunningAnonymousPlugin( lua_State* luaState ) {
    lua_getglobal( luaState, luaDataKey );

    lua_pushstring( luaState, luaRunningAnonymousPluginKey );
    lua_pushboolean( luaState, true );
    lua_rawset( luaState, -3 );
    lua_pop( luaState, 1 );
}

bool isRunningAnonymousPlugin( lua_State* luaState ) {
    lua_getglobal( luaState, luaDataKey );

    lua_pushstring( luaState, luaRunningAnonymousPluginKey );
    lua_gettable( luaState, -2 );
    auto pluginType = LuaToNative< NativeBool >{}( luaState, -1 );
    lua_pop( luaState, 2 );

    return pluginType.valueOr( bool() );
}

void setLuaModuleAPIRestrictions( lua_State* luaState, HubAPIRestrictions luaAPIRestrictions ) {
    lua_getglobal( luaState, luaDataKey );

    lua_pushstring( luaState, luaModuleAPIRestrictionsKey );
    lua_pushinteger( luaState, static_cast< int >( luaAPIRestrictions ) );
    lua_rawset( luaState, -3 );
    lua_pop( luaState, 1 );
}

HubAPIRestrictions getLuaModuleAPIRestrictions( lua_State* luaState ) {
    lua_getglobal( luaState, luaDataKey );

    lua_pushstring( luaState, luaModuleAPIRestrictionsKey );
    lua_gettable( luaState, -2 );
    auto restrictions = LuaToNative< NativeInteger >{}( luaState, -1 );
    lua_pop( luaState, 2 );

    return static_cast< HubAPIRestrictions >( restrictions.valueOr( int() ) );
}

FunctionalityCreator createFunctionalityCreator( lua_State* luaState ) {
    return FunctionalityCreator( getHostService( luaState ), getLuaModuleAPIRestrictions( luaState ) );
}

int pushJsonAsLuaObj( lua_State* luaState,
                      const Json& json,
                      const KeyFormatter& keyFormatter,
                      size_t keyFormattingDepth,
                      bool handleKeyTypes ) {
    auto pushValueOnStack = [&]( const Json& value ) {
        pushJsonAsLuaObj( luaState,
                          value,
                          keyFormattingDepth ? keyFormatter : nullptr,
                          keyFormattingDepth ? keyFormattingDepth - 1 : 0,
                          handleKeyTypes );
    };

    switch( json.type() ) {
    case Json::value_t::object: {
        lua_createtable( luaState, 0, static_cast< int >( json.size() ) );
        for( auto it = json.begin(); it != json.end(); ++it ) {
            auto key = formatKey( keyFormatter, it.key() );

            const auto top = lua_gettop( luaState );

            if( handleKeyTypes && key.size() > 1 && key[ 0 ] == KeyTypeEscapingSymbol ) {
                if( key[ 1 ] == KeyTypeNumberId ) {
                    char* end = nullptr;
                    const auto number = strtod( key.c_str() + 2, &end );
                    if( end == key.c_str() + key.size() ) {
                        lua_pushnumber( luaState, static_cast< lua_Number >( number ) );
                    }
                }
                else if( key[ 1 ] == KeyTypeIntegerId ) {
                    char* end = nullptr;
                    const auto value = strtoll( key.c_str() + 2, &end, 10 );
                    if( end == key.c_str() + key.size() ) {
                        lua_pushinteger( luaState, static_cast< lua_Integer >( value ) );
                    }
                }
                else if( key[ 1 ] == KeyTypeBoolId ) {
                    if( key.compare( 2, key.size() - 2, "true" ) == 0 ) {
                        lua_pushboolean( luaState, true );
                    }
                    else if( key.compare( 2, key.size() - 2, "false" ) == 0 ) {
                        lua_pushboolean( luaState, false );
                    }
                }
                else if( key[ 1 ] == KeyTypeEscapingSymbol ) {
                    lua_pushlstring( luaState, key.c_str() + 1, key.size() - 1 );
                }
            }

            if( top == lua_gettop( luaState ) ) {
                lua_pushlstring( luaState, key.c_str(), key.size() );
            }

            pushValueOnStack( it.value() );
            lua_settable( luaState, -3 );
        }
        break;
    }

    case Json::value_t::array: {
        const auto size = json.size();
        lua_createtable( luaState, static_cast< int >( size ), 0 );
        for( size_t i = 0; i < size; ++i ) {
            pushValueOnStack( json[ i ] );
            lua_seti( luaState, -2, static_cast< lua_Integer >( i + 1 ) );
        }
        break;
    }

    case Json::value_t::number_float: {
        const auto value = json.get< Json::number_float_t >();

        if( std::abs( value - static_cast< lua_Integer >( value ) ) <= std::numeric_limits< lua_Number >::epsilon() ) {
            lua_pushinteger( luaState, static_cast< lua_Integer >( value ) );
        }
        else {
            lua_pushnumber( luaState, static_cast< lua_Number >( value ) );
        }

        break;
    }

    case Json::value_t::number_integer:
        lua_pushinteger( luaState, json );
        break;

    case Json::value_t::string: {
        const std::string* str = json.get< const std::string* >();
        lua_pushlstring( luaState, str->data(), str->size() );
        break;
    }

    case Json::value_t::boolean:
        lua_pushboolean( luaState, json.get< Json::boolean_t >() );
        break;

    case Json::value_t::null:
    case Json::value_t::discarded:
        lua_pushnil( luaState );
        break;
    }
    return 1;
}

Json pullJsonOpt( lua_State* luaState, int index ) {
    if( lua_gettop( luaState ) >= index ) {
        return pullJson( luaState, index );
    }
    else {
        return JSON_EMPTY_OBJ;
    }
}

namespace {
using LuaPointers = std::set< const void* >;

Json pullJsonImpl( lua_State* luaState,
                   LuaPointers& parentTablePointers,
                   int index,
                   const KeyFormatter& keyFormatter,
                   size_t keyFormattingDepth,
                   bool handleKeyTypes ) {
    const auto type = lua_type( luaState, index );

    switch( type ) {
    case LUA_TBOOLEAN:
        return Json( static_cast< bool >( lua_toboolean( luaState, index ) ) );

    case LUA_TNUMBER: {
        if( lua_isinteger( luaState, index ) ) {
            return lua_tointeger( luaState, index );
        }
        else {
            const lua_Number floatNum = lua_tonumber( luaState, index );
            const lua_Integer intNum = static_cast< lua_Integer >( floatNum );

            return std::abs( floatNum - intNum ) <= std::numeric_limits< lua_Number >::epsilon() ? Json( intNum ) :
                                                                                                   Json( floatNum );
        }
    }

    case LUA_TSTRING: {
        // Use one copy of a string: ctop and swap
        Json temp( std::string{} );
        std::string instance( lua_tostring( luaState, index ), lua_rawlen( luaState, index ) );
        // swap instance and inner string of json, no copies made.
        temp.swap( instance );
        return std::move( temp );
    }

    case LUA_TTABLE: {
        if( index < 0 ) {
            index = lua_absindex( luaState, index );
        }

        const auto tablePointer = lua_topointer( luaState, index );
        if( !parentTablePointers.insert( tablePointer ).second ) {
            LOGGER_WARN( "Recursive table pulling is not supported!" );
            break;
        }
        auto removePointer = [tablePointer]( LuaPointers* pointers ) { pointers->erase( tablePointer ); };

        std::unique_ptr< LuaPointers, decltype( removePointer ) > tablePointerRemovingGuard( &parentTablePointers,
                                                                                             removePointer );

        Json result;

        auto pullJsonValue = [&]() {
            return pullJsonImpl( luaState,
                                 parentTablePointers,
                                 -1,
                                 keyFormattingDepth ? keyFormatter : nullptr,
                                 keyFormattingDepth ? keyFormattingDepth - 1 : 0,
                                 handleKeyTypes );
        };

        if( lua_getmetatable( luaState, index ) ) {
            // TODO:
            // auto metatable = pullJsonValue();
            // ...

            LOGGER_WARN( "Metatables actually are not supported" );
            lua_pop( luaState, 1 );
        }

        int arraySize = 0;
        const bool isArray = isLuaArray( luaState, index, arraySize );

        if( isArray && arraySize > 0 ) {
            result = Json::array();

            for( int i = 1; i <= arraySize; ++i ) {
                lua_geti( luaState, index, i );
                result.push_back( pullJsonValue() );
                lua_pop( luaState, 1 );
            }
        }
        else {
            result = Json::object();
            lua_pushnil( luaState );

            auto getTypePrefix = [&]( char typeId ) {
                return handleKeyTypes ? std::string{ { KeyTypeEscapingSymbol, typeId } } : std::string();
            };

            auto onKeyFound = [&]( std::string&& key ) {
                result[ formatKey( keyFormatter, std::move( key ) ) ] = pullJsonValue();
            };

            while( lua_next( luaState, index ) ) {
                const auto keyType = lua_type( luaState, -2 );

                switch( keyType ) {
                case LUA_TBOOLEAN: {
                    lua_toboolean( luaState, -2 );
                    onKeyFound( getTypePrefix( KeyTypeBoolId ) + ( lua_toboolean( luaState, -2 ) ? "true" : "false" ) );
                    break;
                }
                case LUA_TNUMBER: {
                    std::ostringstream stream;
                    if( lua_isinteger( luaState, -2 ) )
                        stream << getTypePrefix( KeyTypeIntegerId ) << lua_tointeger( luaState, -2 );
                    else
                        stream << getTypePrefix( KeyTypeNumberId ) << lua_tonumber( luaState, -2 );

                    onKeyFound( stream.str() );
                    break;
                }
                case LUA_TSTRING: {
                    const auto str = lua_tostring( luaState, -2 );
                    onKeyFound( *str == '\\' && handleKeyTypes ? getTypePrefix( KeyTypeEscapingSymbol ) + ( str + 1 ) :
                                                                 str );
                    break;
                }

                default:
                    LOGGER_WARN( std::string( "Unexpected lua key type: " ) + lua_typename( luaState, keyType ) );
                }

                lua_pop( luaState, 1 );
            }
        }

        return result;
    }

    case LUA_TNIL:
    case LUA_TLIGHTUSERDATA:
    case LUA_TFUNCTION:
    case LUA_TUSERDATA:
    case LUA_TTHREAD:
    case LUA_TNONE:
    default:
        LOGGER_WARN( std::string( "Unexpected lua type: " ) + lua_typename( luaState, type ) );
        break;
    }
    return Json( Json::value_t::null );
}
}  // namespace

Json pullJson( lua_State* luaState,
               int index,
               const KeyFormatter& keyFormatter,
               size_t keyFormattingDepth,
               bool handleKeyTypes ) {
    LuaPointers parentTablePointers;
    return pullJsonImpl( luaState, parentTablePointers, index, keyFormatter, keyFormattingDepth, handleKeyTypes );
}

NativeUint8Vector pullUint8Vector( lua_State* luaState, int index ) {
    NativeUint8Vector result;
    if( isLuaTableEmpty( luaState, index ) ) {
        return result;
    }

    lua_pushvalue( luaState, index );
    lua_pushnil( luaState );
    while( lua_next( luaState, -2 ) ) {
        const bool isNum = lua_type( luaState, -2 ) == LUA_TNUMBER;
        if( isNum ) {
            result.push_back( lua_tonumber( luaState, -1 ) );
        }
        lua_pop( luaState, 1 );
    }
    lua_pop( luaState, 1 );

    return result;
}

namespace {
int dummy( lua_State* luaState ) {
    return luaL_error( luaState, "This functionality is prohibited" );
}
}  // namespace

std::vector< luaL_Reg > applyLuaAPIRestrictions( const luaL_Reg* moduleAPI, HubAPIRestrictions luaAPIRestrictions ) {
    std::vector< luaL_Reg > restrictedModuleAPI;
    const luaL_Reg* moduleAPIIt = moduleAPI;

    if( !moduleAPIIt ) {
        return { { nullptr, nullptr } };
    }

    while( moduleAPIIt->func != nullptr && moduleAPIIt->name != nullptr ) {
        restrictedModuleAPI.emplace_back( luaL_Reg{ moduleAPIIt->name, moduleAPIIt->func } );
        moduleAPIIt++;
    }

    restrictedModuleAPI.emplace_back( luaL_Reg{ nullptr, nullptr } );

    if( restrictedModuleAPI.size() == 1 ) {
        return restrictedModuleAPI;
    }

    /* TODO: it seems such functionality can be useful... */
    /*if( luaAPIRestrictions == LuaAPIRestrictions::NO_ASYNC_RESTRICTION ) {
        Log::trace( cc::trace( "Apply a " ) + cc::note( getLuaAPIRestrictionsString( luaAPIRestrictions ) ) +
                    cc::trace( " Lua API restrictions" ) );

        auto it = restrictedModuleAPI.begin();

        while( it != restrictedModuleAPI.end() ) {
            auto& elm = *it;

            if( !elm.name ) {
                break;  // ignore a terminate structure
            }

            if( std::string( elm.name ) == "subscribe" ) {
                it = restrictedModuleAPI.erase( it );
            }
            else if( std::string( elm.name ) == "unsubscribe" ) {
                it = restrictedModuleAPI.erase( it );
            }
            else {
                it++;
            }
        }
    }*/

    return restrictedModuleAPI;
}

KeyFormatter createMappingKeyFormatter( std::reference_wrapper< const KeyMap > mapping,
                                        KeyFormatter baseKeyFormatter ) {
    auto result = [mapping]( std::string str, KeyFormatter baseKeyFormatter ) -> std::string {
        auto found = mapping.get().find( str );
        return found == mapping.get().end() ? formatKey( baseKeyFormatter, std::move( str ) ) : found->second;
    };

    return std::bind( result, std::placeholders::_1, std::move( baseKeyFormatter ) );
}

constexpr char LowLine = '_';

std::string camelToSnakeCase( std::string str ) {
    auto upperCount = std::accumulate( str.begin(), str.end(), size_t( 0 ), []( size_t prev, unsigned char c ) {
        return prev + !!std::isupper( c );
    } );

    str.resize( upperCount + str.size() );

    for( auto it = str.rbegin(); upperCount; ++it ) {
        const auto c = *std::next( it, upperCount );
        if( std::isupper( c ) ) {
            *( it++ ) = static_cast< char >( std::tolower( static_cast< unsigned char >( c ) ) );
            *it = LowLine;
            --upperCount;
        }
        else {
            *it = c;
        }
    }

    return str;
}

std::string snakeToCamelCase( std::string str ) {
    auto endIt = str.end();
    auto firstIt = str.begin();

    for( auto i = firstIt; i != endIt; ++i ) {
        if( *i != LowLine ) {
            *firstIt++ = *i;
        }
        else if( ++i != endIt ) {
            *firstIt++ = static_cast< char >( std::toupper( *i ) );
        }
        else {
            break;
        }
    }

    str.erase( firstIt, endIt );

    return str;
}

bool isParameterNil( lua_State* luaState, int index ) {
    return lua_isnil( luaState, index );
}

template <>
LuaLookupStringRes LuaToNative< NativeString >::operator()( lua_State* luaState,
                                                            int idx,
                                                            std::string name,
                                                            bool handleKeyTypes ) {
    if( name.empty() ) {
        name = std::to_string( idx );
    }

    size_t size = 0;

    auto value = lua_tolstring( luaState, idx, &size );
    if( value ) {
        return { std::move( name ), std::string{ value, size } };
    }

    return { std::move( name ) };
}

template <>
LuaLookupIntRes LuaToNative< NativeInteger >::operator()( lua_State* luaState,
                                                          int idx,
                                                          std::string name,
                                                          bool handleKeyTypes ) {
    if( name.empty() ) {
        name = std::to_string( idx );
    }

    int isNum = 0;

    auto value = lua_tointegerx( luaState, idx, &isNum );
    if( isNum ) {
        return { std::move( name ), value };
    }

    return { std::move( name ) };
}

template <>
LuaLookupUint8Res LuaToNative< NativeUint8 >::operator()( lua_State* luaState,
                                                          int idx,
                                                          std::string name,
                                                          bool handleKeyTypes ) {
    if( name.empty() ) {
        name = std::to_string( idx );
    }

    int isNum = 0;

    auto value = lua_tointegerx( luaState, idx, &isNum );
    if( isNum ) {
        return { std::move( name ), static_cast< uint8_t >( value ) };
    }

    return { std::move( name ) };
}

template <>
LuaLookupUint32Res LuaToNative< NativeUint32 >::operator()( lua_State* luaState,
                                                            int idx,
                                                            std::string name,
                                                            bool handleKeyTypes ) {
    if( name.empty() ) {
        name = std::to_string( idx );
    }

    int isNum = 0;

    auto value = lua_tointegerx( luaState, idx, &isNum );
    if( isNum ) {
        return { std::move( name ), static_cast< uint32_t >( value ) };
    }

    return { std::move( name ) };
}

template <>
LuaLookupInt64Res LuaToNative< NativeInt64 >::operator()( lua_State* luaState,
                                                          int idx,
                                                          std::string name,
                                                          bool handleKeyTypes ) {
    if( name.empty() ) {
        name = std::to_string( idx );
    }

    int isNum = 0;

    auto value = lua_tointegerx( luaState, idx, &isNum );
    if( isNum ) {
        return { std::move( name ), value };
    }

    return { std::move( name ) };
}

template <>
LuaLookupBoolRes LuaToNative< NativeBool >::operator()( lua_State* luaState,
                                                        int idx,
                                                        std::string name,
                                                        bool handleKeyTypes ) {
    if( name.empty() ) {
        name = std::to_string( idx );
    }

    if( lua_isboolean( luaState, idx ) ) {
        return { std::move( name ), static_cast< bool >( lua_toboolean( luaState, idx ) ) };
    }

    return { std::move( name ) };
}

template <>
LuaLookupStringStringTableRes LuaToNative< NativeStringStringTable >::operator()( lua_State* luaState,
                                                                                  int idx,
                                                                                  std::string name,
                                                                                  bool handleKeyTypes ) {
    if( name.empty() ) {
        name = std::to_string( idx );
    }

    if( idx < 0 ) {
        idx = lua_absindex( luaState, idx );
    }

    if( lua_istable( luaState, idx ) ) {
        NativeStringStringTable table;

        /* traverse a table */
        lua_pushnil( luaState );
        while( lua_next( luaState, idx ) != 0 ) {
            if( lua_type( luaState, -2 ) != LUA_TSTRING ||   // key
                lua_type( luaState, -1 ) != LUA_TSTRING ) {  // value
                return { std::move( name ) };
            }

            table[ lua_tostring( luaState, -2 ) ] = lua_tostring( luaState, -1 );
            lua_pop( luaState, 1 );
        }

        return { std::move( name ), std::move( table ) };
    }

    return { std::move( name ) };
}

template <>
LuaLookupStringVectorRes LuaToNative< NativeStringVector >::operator()( lua_State* luaState,
                                                                        int idx,
                                                                        std::string name,
                                                                        bool handleKeyTypes ) {
    if( name.empty() ) {
        name = std::to_string( idx );
    }

    if( idx < 0 ) {
        idx = lua_absindex( luaState, idx );
    }

    if( lua_istable( luaState, idx ) ) {
        NativeStringVector array;

        /* traverse a table */

        lua_pushnil( luaState );
        while( lua_next( luaState, idx ) != 0 ) {
            if( lua_type( luaState, -1 ) != LUA_TSTRING ) {
                return { std::move( name ) };
            }

            array.push_back( lua_tostring( luaState, -1 ) );
            lua_pop( luaState, 1 );
        }

        return { std::move( name ), std::move( array ) };
    }

    return { std::move( name ) };
}

template <>
LuaLookupInt32VectorRes LuaToNative< NativeInt32Vector >::operator()( lua_State* luaState,
                                                                      int idx,
                                                                      std::string name,
                                                                      bool handleKeyTypes ) {
    if( name.empty() ) {
        name = std::to_string( idx );
    }

    if( idx < 0 ) {
        idx = lua_absindex( luaState, idx );
    }

    if( lua_istable( luaState, idx ) ) {
        NativeInt32Vector array;

        /* traverse a table */
        lua_pushnil( luaState );
        while( lua_next( luaState, idx ) != 0 ) {
            if( lua_type( luaState, -1 ) != LUA_TNUMBER ) {
                return { std::move( name ) };
            }

            array.push_back( lua_tointeger( luaState, -1 ) );
            lua_pop( luaState, 1 );
        }

        return { std::move( name ), std::move( array ) };
    }

    return { std::move( name ) };
}

template <>
LuaLookupJsonRes LuaToNative< Json >::operator()( lua_State* luaState,
                                                  int idx,
                                                  std::string name,
                                                  bool handleKeyTypes ) {
    if( name.empty() ) {
        name = std::to_string( idx );
    }

    return { std::move( name ), pullJson( luaState, idx, nullptr, 0, handleKeyTypes ) };
}

template <>
LuaLookupNumberRes LuaToNative< NativeNumber >::operator()( lua_State* luaState,
                                                            int idx,
                                                            std::string name,
                                                            bool handleKeyTypes ) {
    if( name.empty() ) {
        name = std::to_string( idx );
    }

    const bool isNum = lua_type( luaState, idx ) == LUA_TNUMBER;
    if( isNum ) {
        return { std::move( name ), lua_tonumber( luaState, idx ) };
    }

    return { std::move( name ) };
}

template < typename T >
LuaLookupRes< T > lookupLuaTable( lua_State* luaState, const std::string& key, int tableIdx, bool isMandatory ) {
    if( tableIdx < 0 ) {
        tableIdx = lua_absindex( luaState, tableIdx );
    }

    lua_pushlstring( luaState, key.c_str(), key.size() );
    if( lua_gettable( luaState, tableIdx ) == LUA_TNIL ) {
        lua_pop( luaState, 1 );

        if( isMandatory ) {
            throw LuaError{ "mandatory field '" + key + "' is absent in lua table" };
        }

        return { key };
    }

    auto ret = LuaToNative< T >{}( luaState, -1, key );
    lua_pop( luaState, 1 );

    return ret;
}

template LuaLookupStringRes lookupLuaTable( lua_State* luaState,
                                            const std::string& key,
                                            int tableIdx,
                                            bool isMandatory );
template LuaLookupIntRes lookupLuaTable( lua_State* luaState, const std::string& key, int tableIdx, bool isMandatory );
template LuaLookupNumberRes lookupLuaTable( lua_State* luaState,
                                            const std::string& key,
                                            int tableIdx,
                                            bool isMandatory );
template LuaLookupBoolRes lookupLuaTable( lua_State* luaState, const std::string& key, int tableIdx, bool isMandatory );
template LuaLookupStringStringTableRes lookupLuaTable( lua_State* luaState,
                                                       const std::string& key,
                                                       int tableIdx,
                                                       bool isMandatory );
template LuaLookupRes< Json > lookupLuaTable( lua_State* luaState,
                                              const std::string& key,
                                              int tableIdx,
                                              bool isMandatory );

}  // namespace HubPlatform
