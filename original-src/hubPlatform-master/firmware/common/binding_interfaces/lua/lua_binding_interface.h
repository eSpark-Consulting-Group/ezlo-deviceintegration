#pragma once

#include "json_utils/json_fwd.h"
#include "plugin_events_subscriber/plugin_events_subscriber.h"
#include "binding_interfaces/common/hub_api_restrictions.h"
#include "binding_interfaces/common/functionality_creator.h"

#include "bus/base_bus.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

#include <string>
#include <utility>
#include <map>
#include <vector>
#include <unordered_map>
#include <stdexcept>

/* TODO: a HUB_BASE_MODULE_ENTRY_DEFINITION macro uses value of this macro directly */
#define HUB_MODULE_ENTRY moduleEntryPoint

/* in Lua, 'require "module_name"' returns a table describing this module and sets a global variable
 * $moduleName to this table, so to mimic the same behavior we also name our module table */
#define MODULE_ENTRY_DEFINITION( prefix, module, moduleName )                                                \
    extern "C" int prefix( lua_State* luaState, HubPlatform::HubAPIRestrictions luaAPIRestrictions ) {       \
        const auto restrictedModuleAPI = HubPlatform::applyLuaAPIRestrictions( module, luaAPIRestrictions ); \
        luaL_newlib( luaState, restrictedModuleAPI.data() );                                                 \
        lua_setglobal( luaState, moduleName );                                                               \
        lua_getglobal( luaState, moduleName ); /* we have to leave a table on the stack before we return */  \
        return 1;                                                                                            \
    }

#define HUB_MODULE_ENTRY_DEFINITION( module, moduleName ) \
    MODULE_ENTRY_DEFINITION( HUB_MODULE_ENTRY, module, moduleName )

#define HUB_BASE_MODULE_ENTRY_DEFINITION( prefix, module, moduleName ) \
    MODULE_ENTRY_DEFINITION( prefix##moduleEntryPoint, module, moduleName )

class LuaBindingInvalidArgument;

/* the reason we use macros instead of template functions is the fact that we make return,
 * making this from function would confuse those who reads/uses it;
 * return luaL_error causes a lua exception to be thrown and a lua call stack to be unwound,
 * thus it doesn't return a control to a caller function */

/* it's a lua recommendation to use 'return' with luaL_error, however it seems it's used only
 * to make code more readable, thus using it in the macro isn't very useful... */
/* TODO: luaL_error makes a longjump(), does it compose an error string before a jump happens? */
#define CHECK_INPUT_PARAMETERS( token, number ) \
    if( lua_gettop( luaState ) != number )      \
        return luaL_error( luaState, "%s: an incorrect number of parameters.", token.c_str() );

#define CHECK_INPUT_OPTIONAL_PARAMETERS( token, number ) \
    if( lua_gettop( luaState ) < number )                \
        return luaL_error( luaState, "%s: an incorrect number of parameters.", token.c_str() );

#define GET_INPUT_PARAMETERS( type, idx, token )                                                                       \
    ( {                                                                                                                \
        auto res = LuaToNative< type >{}( luaState, idx );                                                             \
        if( !res )                                                                                                     \
            return luaL_error( luaState, "%s: fail to get a parameter (%s) at %d index.", token.c_str(), #type, idx ); \
        res.value();                                                                                                   \
    } )

namespace HubPlatform {

class LuaBindingInvalidArgument : public std::runtime_error {
public:
    LuaBindingInvalidArgument( const std::string& errDescription )
    : runtime_error( "LuaBinding: " + errDescription ) {}
};

class LuaError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

using ModuleEntryPointType = int ( * )( lua_State*, HubPlatform::HubAPIRestrictions );

namespace Services {
class Service;
}

Services::Service& getHostService( lua_State* luaState );
std::string getPluginId( lua_State* luaState );
bool isRunningAnonymousPlugin( lua_State* luaState );
HubAPIRestrictions getLuaModuleAPIRestrictions( lua_State* luaState );
PluginEventsSubscriber getPluginEventsSubscriber( lua_State* luaState, const std::string& moduleName = "default" );
void changeSubscriberEventsFilterRules( lua_State* luaState,
                                        const std::string& eventsEmitterName,
                                        const std::string& subscriberId,
                                        const JsonArray& rules,
                                        const std::string& moduleName = "default" );

FunctionalityCreator createFunctionalityCreator( lua_State* luaState );

using KeyFormatter = std::function< std::string( std::string ) >;
using KeyMap = std::unordered_map< std::string, std::string >;

extern const KeyMap DefaultDbToLuaKeyMap;

KeyFormatter createMappingKeyFormatter( std::reference_wrapper< const KeyMap >,
                                        KeyFormatter baseKeyFormatter = nullptr );

int pushJsonAsLuaObj( lua_State* luaState,
                      const Json& json,
                      const KeyFormatter& keyFormatter = nullptr,
                      size_t keyFormattingDepth = 0,
                      bool handleKeyTypes = false );
std::vector< uint8_t > pullUint8Vector( lua_State* luaState, int index );

Json pullJsonOpt( lua_State* luaState, int index );
Json pullJson( lua_State* luaState,
               int index,
               const KeyFormatter& keyFormatter = nullptr,
               size_t keyFormattingDepth = 0,
               bool handleKeyTypes = false );

std::vector< luaL_Reg > applyLuaAPIRestrictions( const luaL_Reg* moduleAPI, HubAPIRestrictions luaAPIRestrictions );

std::string camelToSnakeCase( std::string str );

std::string snakeToCamelCase( std::string str );

bool isParameterNil( lua_State* luaState, int index );

/* @brief type for a result of a lookup operation on the lua stack */
template < typename T >
class LuaLookupRes {
public:
    LuaLookupRes( std::string name )
    : m_name( std::move( name ) )
    , m_value()
    , m_isValid( false ) {}

    LuaLookupRes( std::string name, T value )
    : m_name( std::move( name ) )
    , m_value( std::move( value ) )
    , m_isValid( true ) {}

    explicit operator bool() const {
        return isValid();
    }

    bool isValid() const {
        return m_isValid;
    }

    T value() const& {
        if( !isValid() ) {
            throw LuaError{ m_name + " is invalid" };
        }

        return m_value;
    }

    T value() && {
        if( !isValid() ) {
            throw LuaError{ m_name + " is invalid" };
        }

        return std::move( m_value );
    }

    template < typename U >
    T valueOr( U altValue ) const {
        static_assert(
            std::is_same< T, U >::value,
            "Value and altValue have different types and may be non-convertible (in case of signed/unsigned)" );
        return isValid() ? value() : std::move( altValue );
    }

private:
    const std::string m_name;
    T m_value;
    const bool m_isValid;
};

using NativeString = std::string;
using NativeInteger = int;
using NativeInt64 = int64_t;
using NativeUint8 = uint8_t;
using NativeUint32 = uint32_t;
using NativeNumber = float;
using NativeBool = bool;
using NativeStringStringTable = std::map< std::string, std::string >;
using NativeUint8Vector = std::vector< uint8_t >;
using NativeStringVector = std::vector< std::string >;
using NativeInt32Vector = std::vector< int32_t >;

using LuaLookupStringRes = LuaLookupRes< NativeString >;
using LuaLookupIntRes = LuaLookupRes< NativeInteger >;
using LuaLookupInt64Res = LuaLookupRes< NativeInt64 >;
using LuaLookupUint8Res = LuaLookupRes< NativeUint8 >;
using LuaLookupUint32Res = LuaLookupRes< NativeUint32 >;
using LuaLookupNumberRes = LuaLookupRes< NativeNumber >;
using LuaLookupBoolRes = LuaLookupRes< NativeBool >;
using LuaLookupStringStringTableRes = LuaLookupRes< NativeStringStringTable >;
using LuaLookupJsonRes = LuaLookupRes< Json >;
using LuaLookupUint8VectorRes = LuaLookupRes< NativeUint8Vector >;
using LuaLookupStringVectorRes = LuaLookupRes< NativeStringVector >;
using LuaLookupInt32VectorRes = LuaLookupRes< NativeInt32Vector >;

/* @brief tries to convert a lua object to a native object at @c idx index */
template < typename T >
struct LuaToNative final {
    LuaLookupRes< T > operator()( lua_State* luaState,
                                  int idx = -1,
                                  std::string name = "",
                                  bool handleKeyTypes = false );
};

/* @brief looks up a table at @c tableIdx index for an object of a @c T type at a @c key position
 *
 * @param[in] luaState -
 * @param[in] key - a key of an element to look for
 * @param[in] tableIdx - an index (absolute or relative) of a table to look in
 * @param[in] isMandatory - is field mandatory
 */
template < typename T >
LuaLookupRes< T > lookupLuaTable( lua_State* luaState,
                                  const std::string& key,
                                  int tableIdx = -1,
                                  bool isMandatory = false );

template < int Binding( lua_State* ) >
int errorCatchWrapper( lua_State* luaState ) {
    try {
        return Binding( luaState );
    }
    catch( const MaxMessageLengthError& e ) {
        return luaL_error( luaState, "Bus message is too long: %s", e.what() );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "Fatal error: %s", e.what() );
    }
}

}  // namespace HubPlatform
