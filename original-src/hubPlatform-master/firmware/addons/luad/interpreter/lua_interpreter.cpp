#include "luad/interpreter/lua_interpreter.h"
#include "luad/common/luup_api.h"
#include "luad/interpreter/lua_functions.h"
#include "luad/interpreter/lua_script_params_holder.h"

#include "binding_interfaces/lua/lua_binding_interface.h" /* TODO: we need it only for pullJson() */
#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"

#include <thread>
#include <future>

extern "C" {
#include <lualib.h>
#include <lauxlib.h>
}

namespace HubPlatform {

const char* const TimeoutErrorMessage = "Script execution stopped due to timeout limitation!";

LuaInterpreter::LuaInterpreter( Addons::Addon& addon,
                                LuaPluginResourcesProvider& plugProvider,
                                std::chrono::steady_clock::duration timeout )
: m_luaState{ luaL_newstate(), lua_close }
, m_isRunning( false )
, m_timeout( timeout ) {
    if( !m_luaState )
        throw std::bad_alloc{};

    luaL_openlibs( m_luaState.get() );

    m_moduleLoader = std::make_unique< LuaModuleLoader >( m_luaState.get(), plugProvider, addon );

    LuupAPI::registerOldAPIModules( m_luaState.get() );
}

LuaInterpreter::~LuaInterpreter() = default;

LuaInterpreter::LuaInterpreter( LuaInterpreter&& rhs ) noexcept
: m_luaState( std::move( rhs.m_luaState ) )
, m_moduleLoader( std::move( rhs.m_moduleLoader ) )
, m_timeout( rhs.m_timeout ) {
    m_isRunning.store( rhs.m_isRunning.load() );
}

LuaInterpreter& LuaInterpreter::operator=( LuaInterpreter&& rhs ) noexcept {
    m_luaState = std::move( rhs.m_luaState );
    m_moduleLoader = std::move( rhs.m_moduleLoader );
    m_isRunning.store( rhs.m_isRunning.load() );
    m_timeout = rhs.m_timeout;
    return *this;
}

void LuaInterpreter::loadLuaFile( const std::string& luaFilePath ) {
    m_moduleLoader->setPluginId( luaFilePath );

    if( luaL_loadfile( m_luaState.get(), luaFilePath.c_str() ) )
        throw LuaInterpreterError{ std::string{ "Couldn't load and/or compile a '" } + luaFilePath +
                                   "' file: " + lua_tostring( m_luaState.get(), -1 ) };
}

void LuaInterpreter::loadLuaString( const std::string& luaString ) {
    m_moduleLoader->setPluginId( "" ); /* we're running a nameless script */

    if( luaL_loadstring( m_luaState.get(), luaString.c_str() ) )
        throw LuaInterpreterError{ std::string{ "Couldn't load and/or compile a '" } + luaString +
                                   "' string: " + lua_tostring( m_luaState.get(), -1 ) };
}

void LuaInterpreter::loadScript( const std::string& script ) {
    m_moduleLoader->setPluginId( script );

    if( m_moduleLoader->isAnonymousPluginScript( script ) ) {
        LOGGER_DEBUG_RAW( cc::debug( "Running an anonymous plugin" ) );
        m_moduleLoader->setRunningAnonymousPlugin();

        const auto luaAPIRestrictions = HubAPIRestrictions::NO_ASYNC_RESTRICTION;
        m_moduleLoader->setLuaModuleAPIRestrictions( luaAPIRestrictions );
    }

    m_moduleLoader->loadPluginScript( script );
}

ScriptResult LuaInterpreter::runLoadedCode( const Json* params, RunCodeType runCodeType ) {
    auto ptr = LuaScriptJsonParams::make( params ? *params : Json() );
    return runLoadedCode( ptr, runCodeType );
}

ScriptResult LuaInterpreter::runLoadedCode( const LuaScriptParamsHolderPtr& paramsHolder, RunCodeType runCodeType ) {
    const int luaStackSizeWithoutLoadedFunction = lua_gettop( m_luaState.get() ) - 1;

    if( paramsHolder && m_luaState.get() ) {
        paramsHolder->pushAsObject( *m_luaState );
    }

    int luaExecCode = LUA_ERRERR;
    if( m_timeout == std::chrono::steady_clock::duration::zero() ) {
        int nargs = paramsHolder != nullptr;
        luaExecCode = LuaFunctions::protectedCall( m_luaState.get(), nargs, LUA_MULTRET );
    }
    else {
        std::promise< void > promiseCodeDone;
        auto watchdogFuture = std::async( std::launch::async, [&promiseCodeDone, this]() {
            auto futureCodeDone = promiseCodeDone.get_future().wait_for( m_timeout );
            if( futureCodeDone == std::future_status::timeout )
                // FIXME: Non-synchronized call to lua state from another thread!
                forceStopScript();
        } );

        luaExecCode = LuaFunctions::protectedCall( m_luaState.get(), int( paramsHolder != nullptr ), LUA_MULTRET );
        promiseCodeDone.set_value();
        watchdogFuture.get();  // Ensure that watchdog thread has finished
    }

    if( luaExecCode != LUA_OK ) {
        const std::string error =
            std::string{ "LuaInterpreter: Couldn't run a Lua code: " } + lua_tostring( m_luaState.get(), -1 );
        Log::error( error );

        return ScriptResult( error );
    }

    std::vector< Json > values;
    const int returnValues = lua_gettop( m_luaState.get() ) - luaStackSizeWithoutLoadedFunction;
    for( int i = returnValues; i >= 1; --i )
        values.push_back( pullJson( m_luaState.get(), -i ) );

    if( returnValues > 0 && runCodeType == RunCodeType::POP_RESULTS ) {
        lua_pop( m_luaState.get(), returnValues );
    }

    return ScriptResult( std::move( values ) );
}

std::string LuaInterpreter::getExecutionState() {
    return lua_tostring( m_luaState.get(), -1 );
}

static void stopScriptHook( lua_State* L, lua_Debug* D ) {
    (void) D;
    lua_sethook( L, nullptr, 0, 0 );
    luaL_error( L, TimeoutErrorMessage );
}

void LuaInterpreter::forceStopScript() {
    lua_sethook( m_luaState.get(), stopScriptHook, LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT, 1 );
}

void LuaInterpreter::setTimeout( int timeout ) {
    m_timeout = std::chrono::seconds( timeout );
}

int LuaInterpreter::getMemoryUsed() {
    int memoryUsedKb = lua_gc( m_luaState.get(), LUA_GCCOUNT, 0 );
    return memoryUsedKb;
}

void LuaInterpreter::collectGarbage() {
    lua_gc( m_luaState.get(), LUA_GCCOLLECT, 0 );
}

}  // namespace HubPlatform
