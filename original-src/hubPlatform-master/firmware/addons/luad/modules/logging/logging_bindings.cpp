#include "binding_interfaces/lua/lua_binding_interface.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "tiny/tiny_utils.h"

namespace HubPlatform {
namespace {
static const std::string moduleName{ "logging." };

#define GET_FUNCTION ( moduleName + __func__ )

#define CHECK_LUA_INPUT_PARAMETERS( number ) CHECK_INPUT_PARAMETERS( GET_FUNCTION, number )

#define GET_LUA_INPUT_PARAMETERS( type, idx ) GET_INPUT_PARAMETERS( type, idx, GET_FUNCTION )

namespace Support {
Log::PriorityLevel toGeneralLoggerPriority( const std::string& severity ) {
    if( severity == "trace" )
        return Log::PriorityLevel::TRACE;

    if( severity == "debug" )
        return Log::PriorityLevel::DEBUG;

    if( severity == "info" )
        return Log::PriorityLevel::INFO;

    if( severity == "warning" )
        return Log::PriorityLevel::WARN;

    if( severity == "error" )
        return Log::PriorityLevel::ERROR;

    return Log::PriorityLevel::INFO;
}

static const std::string errorCaseMessage( "Error during logging from Lua script." );

std::string withPluginName( const std::string& message, lua_State* luaState ) {
    return cc::note( getPluginId( luaState ) + ": " ) + message;
}

std::string luaBool2String( lua_State* luaState, int index ) {
    std::stringstream ss;
    ss << std::boolalpha << static_cast< NativeBool >( lua_toboolean( luaState, index ) );
    return ss.str();
}

std::string luaNumber2String( lua_State* luaState, int index ) {
    if( lua_isinteger( luaState, index ) )
        return std::to_string( static_cast< NativeInteger >( lua_tointeger( luaState, index ) ) );

    return std::to_string( static_cast< NativeNumber >( lua_tonumber( luaState, index ) ) );
}

std::string luaTable2String( lua_State* luaState, int index ) {
    constexpr const size_t maxContentLenght = 1024 * 4;  // TODO: check and correct this value

    auto content = pullJson( luaState, index ).dump();

    return ( content.size() > maxContentLenght ) ? std::string( content.data(), maxContentLenght ) : content;
}

std::string luaObject2String( lua_State* luaState, int index ) {
    switch( lua_type( luaState, index ) ) {
    case LUA_TBOOLEAN:
        return luaBool2String( luaState, index );
    case LUA_TNUMBER:
        return luaNumber2String( luaState, index );
    case LUA_TSTRING:
        return std::string( lua_tostring( luaState, index ) );
    case LUA_TTABLE:
        return luaTable2String( luaState, index );
    case LUA_TNIL:
        return "LUA_TNIL";
    case LUA_TLIGHTUSERDATA:
        return "LUA_TLIGHTUSERDATA";
    case LUA_TFUNCTION:
        return "LUA_TFUNCTION";
    case LUA_TUSERDATA:
        return "LUA_TUSERDATA";
    case LUA_TTHREAD:
        return "LUA_TTHREAD";
    case LUA_TNONE:
        return "LUA_TNONE";
    default:
        return "";
    }
}

std::string logLevel2LuaSeverity() {
    switch( Log::getLogLevel() ) {
    case Log::PriorityLevel::ERROR:
        return "error";
    case Log::PriorityLevel::WARN:
        return "warning";
    case Log::PriorityLevel::DEBUG:
        return "debug";
    case Log::PriorityLevel::TRACE:
        return "trace";
    case Log::PriorityLevel::INFO:
    default:
        return "info";
    }
}

using concreteLogFunction = std::function< void( const std::string& ) >;

int concreteLog( lua_State* luaState, Log::PriorityLevel level, const concreteLogFunction& logMethod ) {
    if( Log::getLogLevel() >= level ) {
        CHECK_LUA_INPUT_PARAMETERS( 1 );

        logMethod( withPluginName( luaObject2String( luaState, 1 ), luaState ) );
    }

    return 0;
}

}  // namespace Support

inline namespace Adaptors {
int getLevel( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 0 );

    const auto severity = Support::logLevel2LuaSeverity();
    lua_pushlstring( luaState, severity.data(), severity.size() );

    return 1;
}

int log( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 2 );

    const auto severity =
        Support::toGeneralLoggerPriority( tiny::tools::to_lower( GET_LUA_INPUT_PARAMETERS( std::string, 1 ) ) );

    if( Log::getLogLevel() >= severity )
        Log::print( severity, Support::withPluginName( Support::luaObject2String( luaState, 2 ), luaState ) );

    return 0;
}

int trace( lua_State* luaState ) {
    return Support::concreteLog( luaState, Log::TRACE, &Log::trace );
}

int debug( lua_State* luaState ) {
    return Support::concreteLog( luaState, Log::DEBUG, std::bind( &Log::debug, std::placeholders::_1, false ) );
}

int info( lua_State* luaState ) {
    return Support::concreteLog( luaState, Log::INFO, &Log::info );
}

int warning( lua_State* luaState ) {
    return Support::concreteLog( luaState, Log::WARN, &Log::warn );
}

int error( lua_State* luaState ) {
    return Support::concreteLog( luaState, Log::ERROR, &Log::error );
}
}  // namespace Adaptors

inline namespace Bindings {
constexpr char moduleName[] = "logging";
constexpr luaL_Reg module[] = {
    { "get_level", errorCatchWrapper< Adaptors::getLevel > }, { "log", errorCatchWrapper< Adaptors::log > },
    { "trace", errorCatchWrapper< Adaptors::trace > },        { "debug", errorCatchWrapper< Adaptors::debug > },
    { "info", errorCatchWrapper< Adaptors::info > },          { "warning", errorCatchWrapper< Adaptors::warning > },
    { "error", errorCatchWrapper< Adaptors::error > },        { nullptr, nullptr }
};
}  // namespace Bindings

}  // namespace
}  // namespace HubPlatform

HUB_BASE_MODULE_ENTRY_DEFINITION( logging, HubPlatform::Bindings::module, HubPlatform::Bindings::moduleName );