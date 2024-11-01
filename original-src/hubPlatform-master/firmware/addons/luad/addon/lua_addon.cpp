#include "luad/addon/lua_addon.h"
#include "luad/addon/lua_task.h"
#include "luad/version/luad_addon_version.h"

#include "luad/modules/timer/timer_manager.h"
#include "luad/interpreter/lua_interpreter.h"

#include "json_utils/json_utils.h"
#include "log_utils/ezlo_log.h"

#include "service_interfaces/bus_wrapper.h"
#include "common_utils/lock_helper.h"
#include "common_utils/cpp14.h"
#include "system_utils/environment.h"
#include "system_status/ram_usage.h"

#include "config_reader/config_reader.h"
#include "config_reader/config_keys.h"

#include "cored/remote_interface/cored_interface_factory.h"
#include "service_interfaces/resources.h"
#include "luad/cloud_storage_events/cloud_storage_event_emitter.h"

#include <memory>
#include <cassert>
#include <fstream>
#include <stdexcept>

namespace HubPlatform {
namespace LuaAddon {

const char* const LuaAddon::Name = "lua";
const char* const LuaAddon::ExecuteScriptMethod = "executeScript";
const char* const LuaAddon::ScriptFieldName = "script";
const char* const LuaAddon::ScriptParamsFieldName = "scriptParams";
const char* const LuaAddon::LibraryNameFieldName = "libraryName";
const char* const LuaAddon::LibraryEntryPointFieldName = "entryPoint";
const char* const LuaAddon::PluginId = "pluginId";

LuaAddon::LuaAddon()
: Addons::Addon( Name )
, m_scriptTimeout( DefaultScriptTimeout ) {}

LuaAddon::LuaAddon( std::unique_ptr< Services::BusWrapper > busWrapper, std::unique_ptr< IDbClient > dbClient )
: Addons::Addon( Name, std::move( busWrapper ), std::move( dbClient ) )
, m_scriptTimeout( DefaultScriptTimeout ) {}

LuaAddon::~LuaAddon() = default;

CommonUtils::AddonVersion LuaAddon::getVersion() const {
    return Addons::Luad::getVersion();
}

void LuaAddon::setUp() {
    Addons::Addon::setUp();

    m_timerManager = std::make_unique< Luad::TimerManager >( *this, *this, *this );
    m_extensionsManager = std::make_unique< ExtensionsManager::RemoteInterface >( *this );

    setUpTimeouts();

    const auto pathValue = Environment::getLuaPackagesFolder() + "/?.lua";
    setenv( "LUA_PATH", pathValue.c_str(), true );

    const auto cpathValue = Environment::getLuaPackagesFolder() + "/?.so";
    setenv( "LUA_CPATH", cpathValue.c_str(), true );
}

void LuaAddon::tearDown() {
    m_pluginRestartedCallbacks.process( []( PluginStatusCallbacksCollection& collection ) { collection.clear(); } );
    m_pluginTeardownCallbacks.process( []( PluginStatusCallbacksCollection& collection ) { collection.clear(); } );
    m_pluginUninstallCallbacks.process( []( PluginStatusCallbacksCollection& collection ) { collection.clear(); } );

    m_luaInterpreters.process( []( PluginsLuaInterpretersCollection& collection ) { collection.clear(); } );

    m_timerManager.reset();

    Addons::Addon::tearDown();
}

void LuaAddon::checkAndLogDefaultTimeout( int defaultTimeout ) const {
    if( defaultTimeout < 0 ) {
        LOGGER_WARN_RAW( "Luad timeout is set to zero: no timeout. Use with care, only for debugging!" );
    }
    else if( defaultTimeout < m_scriptMinTimeout ) {
        Log::warn( cc::warn( "Luad timeout is set less then min timeout value, will use minimum timeout: " +
                             cc::num( m_scriptMinTimeout ) ) );
    }
}

void LuaAddon::setUpTimeouts() {
    using namespace Services::Resources::HubSettings;

    int defaultConfigTimeout = getConfigTimeoutValue( ConfigKeys::LuadScriptTimeout );
    int minConfigTimeout = getConfigTimeoutValue( ConfigKeys::LuadScriptMinTimeout );

    auto core = CoreDRemoteInterface::makeInstance( *this );

    int defaultTimeout = getTimeoutSettingValue( *core, LuaDefultTimeout, defaultConfigTimeout );
    m_scriptMinTimeout = getTimeoutSettingValue( *core, LuaMinTimeout, minConfigTimeout );

    setDefaultTimeout( defaultTimeout );
}

void LuaAddon::setDefaultTimeout( int timeout ) {
    checkAndLogDefaultTimeout( timeout );
    if( timeout < 0 ) {
        m_scriptTimeout = 0;
    }
    else {
        m_scriptTimeout = std::max( m_scriptMinTimeout, timeout );
    }
}

int LuaAddon::getTimeoutSettingValue( CoreDRemoteInterface::CoreDInterface& core,
                                      const std::string& settingName,
                                      int defaultValue ) {
    auto settingPtr = core.findSetting( settingName );

    if( settingPtr ) {
        return settingPtr->getIntValue();
    }
    else {
        LOGGER_ERROR( "Setting: " + settingName + " is not found using default value: " + cc::num10( defaultValue ) );
    }

    return defaultValue;
}

Luad::TimerManager* LuaAddon::getTimerManager() {
    return m_timerManager.get();
}

JsonObject LuaAddon::handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) {
    switch( requestType ) {
    case EXECUTE_SCRIPT:
        return executeScriptSync( request, nullptr ).toJson();
    case ADD_PLUGIN_LIBRARY:
        handleAddLibrary( request );
        break;
    case REMOVE_PLUGIN_LIBRARY:
        handleRemoveLibrary( request );
        break;
    case GLOBAL_LOCK: {
        auto& lockHelper = CommonUtils::LockHelper::global();
        lockHelper.tryLock( request );
        return lockHelper.getResponse();
    }
    case GLOBAL_UNLOCK: {
        auto& lockHelper = CommonUtils::LockHelper::global();
        lockHelper.unlock( request );
        return lockHelper.getResponse();
    }
    default:
        return Service::handleRequestFromBus( request, requestType );
    }
    return JSON_EMPTY_OBJ;
}

void LuaAddon::handleMessageFromBus( const Services::Message& message, const std::string& sender ) {
    switch( message.msgClass ) {
    case EVENT:
        handleEventFromBus( message );
        break;

    default:
        Service::handleMessageFromBus( message, sender );
        break;
    }
}

void LuaAddon::handleEventFromBus( const Services::Message& message ) {
    switch( message.msgSubClass ) {
    case PLUGIN_RESTARTED:
        handlePluginRestarted( message.data );
        break;

    case PLUGIN_TEARDOWN:
        handlePluginTeardown( message.data );
        break;

    case PLUGIN_UNINSTALL:
        handlePluginUninstall( message.data );
        break;

    case HUB_SETTING_UPDATED: {
        handleSettingsUpdated( message );
        break;
    }
    case HUB_STORAGE_CONTROLLER_TOKEN_UPDATED:
        handleCloudStorageEvents( "updated" );
        break;
    case HUB_STORAGE_CONTROLLER_TOKEN_EXPIRED:
        handleCloudStorageEvents( "expired" );
        break;
    case HUB_CLOUD_ENVIRONMENT_CHANGED:
        localEmit( "Luad", "cloud_environment_changed", JSON_EMPTY_OBJ );
        break;

    default:
        break;
    }
}

void LuaAddon::handleSettingsUpdated( const Services::Message& message ) {
    auto name = json_get_str( message.data, "name" );
    if( name == Services::Resources::HubSettings::LuaDefultTimeout ) {
        int timeout;
        if( json_get( message.data, "value", timeout ) ) {
            setDefaultTimeout( timeout );
            updateTimeoutForStoredInterpreters();
        }
    }
}

void LuaAddon::handleLegacyMethodCall( const JsonObject& json, const std::string& sender ) {
    const auto method = json_get_str( json[ "method" ] );
    if( method == ExecuteScriptMethod ) {
        executeScript( json[ "params" ], nullptr );
    }
    else {
        Addon::handleLegacyMethodCall( json, sender );
    }
}

void LuaAddon::handleAddLibrary( const JsonObject& params ) {
    LuaModuleLoader::Library library;
    auto name = json_get_str( params, LibraryNameFieldName );
    library.entryPoint = json_get_str( params, LibraryEntryPointFieldName );
    LuaModuleLoader::addLibrary( name, library );
}

void LuaAddon::handleRemoveLibrary( const JsonObject& params ) {
    auto name = json_get_str( params, LibraryNameFieldName );
    LuaModuleLoader::removeLibrary( name );
}

void LuaAddon::processAllCallbacks( threads::UniqueSafeWrapper< PluginStatusCallbacksCollection >& safeCallbacks,
                                    const std::string& pluginId ) {
    PluginStatusCallbacksCollection callbacks;
    safeCallbacks.process( [&callbacks]( PluginStatusCallbacksCollection& collection ) { callbacks = collection; } );

    for( auto& callback : callbacks ) {
        callback( pluginId );
    }
}

void LuaAddon::handlePluginRestarted( const JsonObject& params ) {
    auto pluginId = json_get_str( params, PluginId );
    LOGGER_DEBUG( cc::note( pluginId ) + cc::debug( " plugin has been restarted." ) );
    cleanPluginInfo( pluginId );
    processAllCallbacks( m_pluginRestartedCallbacks, pluginId );
}

void LuaAddon::handlePluginTeardown( const JsonObject& params ) {
    auto pluginId = json_get_str( params, PluginId );
    LOGGER_DEBUG( cc::note( pluginId ) + cc::debug( " plugin has been teardown." ) );
    cleanPluginInfo( pluginId );
    processAllCallbacks( m_pluginTeardownCallbacks, pluginId );
}

void LuaAddon::handlePluginUninstall( const JsonObject& params ) {
    auto pluginId = json_get_str( params, PluginId );
    LOGGER_DEBUG( cc::note( pluginId ) + cc::debug( " plugin has been uninstalled." ) );
    cleanPluginInfo( pluginId );
    processAllCallbacks( m_pluginUninstallCallbacks, pluginId );
}

void LuaAddon::localEmit( const std::string& eventHandler, const std::string& eventType, const Json& eventData ) {
    auto ptr = HubPlatform::LuaScriptJsonParams::make( eventData );
    ptr->objectTemplate( JSON( "event" << eventType ), "data" );
    localEmit( eventHandler, eventType, std::move( ptr ) );
}

void LuaAddon::localEmit( const std::string& eventHandler, const Json& eventData ) {
    localEmit( eventHandler, HubPlatform::LuaScriptJsonParams::make( eventData ) );
}

void LuaAddon::localEmit( const std::string& eventHandler,
                          const std::string& eventType,
                          LuaScriptParamsHolderPtr eventData ) {
    eventData->objectTemplate( JSON( "event" << eventType ), "data" );

    auto json = JSON( "script" << eventHandler );
    LOGGER_DEBUG( cc::debug( "execute script: " ) + eventHandler );
    executeScript( json, eventData );
}

void LuaAddon::localEmit( const std::string& eventHandler, LuaScriptParamsHolderPtr scriptParams ) {
    const auto json = JSON( "script" << eventHandler );
    executeScript( json, scriptParams );
}

void LuaAddon::cleanPluginInfo( const std::string& pluginId ) {
    m_luaInterpreters.process(
        [&pluginId]( PluginsLuaInterpretersCollection& collection ) { collection.erase( pluginId ); } );
}

LuaAddon::LuaInterpreterPtr LuaAddon::createLuaInterpreter() {
    std::chrono::steady_clock::duration timeout = std::chrono::steady_clock::duration::zero();
    if( m_scriptTimeout > 0 ) {
        timeout = std::chrono::seconds( m_scriptTimeout );
    }

    return std::make_shared< LuaInterpreter >( *this, *this, timeout );
}

void LuaAddon::updateTimeoutForStoredInterpreters() {
    m_luaInterpreters.process( [this]( PluginsLuaInterpretersCollection& collection ) {
        for( const auto& interpreterItem : collection ) {
            if( interpreterItem.second.policy == ExtensionsManager::ExecutionPolicy::RestoreLastScriptState ) {
                interpreterItem.second.interpreter->setTimeout( m_scriptTimeout );
            }
        }
    } );
}

LuaAddon::LuaInterpreterPtr LuaAddon::findInterpreterForPlugin( const std::string& pluginId ) {
    LuaInterpreterPtr luaInterpreter;

    m_luaInterpreters.process( [&luaInterpreter, &pluginId]( PluginsLuaInterpretersCollection& collection ) {
        auto itFind = collection.find( pluginId );

        if( itFind == collection.end() ) {
            return;
        }
        else if( itFind->second.policy == ExtensionsManager::ExecutionPolicy::RestoreLastScriptState ) {
            assert( itFind->second.interpreter );
            luaInterpreter = itFind->second.interpreter;
        }
    } );

    return luaInterpreter;
}

LuaAddon::LuaInterpreterPtr LuaAddon::createInterpreterForPlugin( const std::string& pluginId ) {
    LuaInterpreterPtr luaInterpreter;

    m_luaInterpreters.process( [&luaInterpreter, &pluginId, this]( PluginsLuaInterpretersCollection& collection ) {
        auto itFind = collection.find( pluginId );

        if( itFind == collection.end() ) {
            luaInterpreter = createLuaInterpreter();

            PluginExecutionState state;
            state.policy = m_extensionsManager->getExecutionPolicyByPluginId( pluginId );
            if( state.policy == ExtensionsManager::ExecutionPolicy::RestoreLastScriptState ) {
                state.interpreter = luaInterpreter;
            }

            collection.emplace( std::make_pair( pluginId, std::move( state ) ) );
        }
        else if( itFind->second.policy != ExtensionsManager::ExecutionPolicy::RestoreLastScriptState ) {
            luaInterpreter = createLuaInterpreter();
        }
    } );

    return luaInterpreter;
}

// json = {
//        "script": "HUB:zwave/scripts/init",
//        "scriptParams": {},
//        ...
// TODO:  "code": "..."
//    }
void LuaAddon::executeScript( const Json& params, LuaScriptParamsHolderPtr scriptParams ) {
    addTask( [this, params, scriptParams]() { executeScriptSync( params, scriptParams ); } );
}

ScriptResult LuaAddon::executeScriptSync( const Json& params, LuaScriptParamsHolderPtr scriptParams ) {
    auto script = json_get_str( params[ ScriptFieldName ] );
    std::string pluginId;
    const std::string operationId = json_get_str( params, "operationId" );
    bool isUserRunScript = json_get_bool( params, "isUserRunScript" );

    if( !scriptParams.get() ) {
        LOGGER_TRACE(
            "LuaAddon:::executeScriptSync: scriptParams are empty, create empty LuaScriptJsonParams object..." );
        // Try to get parameters from a json object
        auto scriptParamsJson = JSON_EMPTY_OBJ;
        json_get_obj( params, ScriptParamsFieldName, scriptParamsJson );
        scriptParams = HubPlatform::LuaScriptJsonParams::make( scriptParamsJson );
    }

    /* if we caught an exception during running lua code, only the current lua
     * interpreter has to be stopped, not whole luaD */
    try {
        // Print script to log with params: Extract to a single method
        if( Log::getLogLevel() >= Log::PriorityLevel::DEBUG ) {
            auto netSize = scriptParams->payloadSize();  // netto
            const unsigned logMessageMaxLength = 256;

            auto traceParams = [&]() -> std::string {
                if( Log::getLogLevel() < Log::PriorityLevel::DEBUG )
                    return {};

                auto dump = scriptParams->toString();  // brutto
                auto result = cc::debug( ", parameters: " ) +
                              ( netSize > logMessageMaxLength ?
                                    std::string{ dump.substr( 0, logMessageMaxLength ).c_str() } +
                                        "... (truncated, payload size: " + cc::num10( netSize ) + ")" :
                                    cc::j( dump ) );
                return result;
            };
            LOGGER_DEBUG_RAW( cc::debug( "LuaAddon: start to execute a script: " ) + cc::p( script ) + traceParams() );
        }

        pluginId = m_extensionsManager->getPluginIdByScriptId( script );
        auto luaInterpreter = findInterpreterForPlugin( pluginId );
        if( luaInterpreter == nullptr ) {
            luaInterpreter = createInterpreterForPlugin( pluginId );
            emitInterpreterCreated( pluginId );
        }

        emitScriptStarted( pluginId, script, operationId );

        ExtensionsManager::ExecutionPolicy policy = fetchPluginPolicy( pluginId );

        if( policy == ExtensionsManager::ExecutionPolicy::RestoreLastScriptState && Log::getLogLevel() >= Log::DEBUG ) {
            int kiloBytes = luaInterpreter->getMemoryUsed();
            printPluginMemoryUsage( kiloBytes, pluginId, "start" );
        }

        sendProgressStatus( { "started", operationId, 10, isUserRunScript } );
        LuaTaskExecuteScript luaTaskExecuteScript( luaInterpreter, script, std::move( scriptParams ) );

        const auto start = std::chrono::steady_clock::now();
        const auto result = luaTaskExecuteScript.runTask();
        const auto finish = std::chrono::steady_clock::now();

        if( result.hasError() ) {
            sendProgressStatus( { "failed", operationId, 0, isUserRunScript },
                                luaTaskExecuteScript.getExecutionState() );
            Log::warn( cc::warn( "LuaAddon: a script: " ) + cc::p( script ) + cc::warn( " has been terminated " ) +
                       cc::fail( "abnormally" ) );
            emitScriptFailed( pluginId, script, operationId, ( finish - start ) );
        }
        else {
            sendProgressStatus( { "finished", operationId, 100, isUserRunScript } );
            Log::info( cc::info( "LuaAddon: a script: " ) + cc::p( script ) + cc::info( " has been executed " ) +
                       cc::success( "successfully" ) );
            emitScriptFinished( pluginId, script, operationId, ( finish - start ) );
        }

        if( policy == ExtensionsManager::ExecutionPolicy::RestoreLastScriptState ) {
            int kiloBytes = luaInterpreter->getMemoryUsed();
            emitPluginResourceUsage( pluginId, "restore", kiloBytes );

            if( Log::getLogLevel() >= Log::DEBUG ) {
                printPluginMemoryUsage( kiloBytes, pluginId, "end" );
                printProcessMemoryUsage();
            }
        }

        return result;
    }
    catch( const std::exception& exception ) {
        sendProgressStatus( { "failed", operationId, 0, isUserRunScript }, exception.what() );
        Log::error( cc::error( "LuaAddon: error during loading/execution lua code: " ) + cc::note( exception.what() ) );
        Log::error( cc::error( "LuaAddon: error during loading/execution lua code: " ) + cc::note( params.dump() ) );

        emitScriptAborted( pluginId, script, operationId );

        return ScriptResult( "error during loading/execution lua code:" );
    }
}

bool LuaAddon::isPluginScript( const std::string& scriptId ) const {
    return m_extensionsManager->isPluginScript( scriptId );
}

bool LuaAddon::isAnonymousPluginScript( const std::string& scriptId ) const {
    return m_extensionsManager->isAnonymousPluginScript( scriptId );
}

std::unique_ptr< std::istream > LuaAddon::getScript( const std::string& scriptId ) {
    return m_extensionsManager->getLuaScript( scriptId );
}

std::string LuaAddon::getScriptFilePath( const std::string& scriptId ) const {
    return m_extensionsManager->getLuaScriptFilePath( scriptId );
}

std::string LuaAddon::getDataFilePath( const std::string& scriptId ) const {
    return m_extensionsManager->getDataFilePath( scriptId );
}

std::string LuaAddon::getPluginIdByScriptId( const std::string& scriptId ) const {
    return m_extensionsManager->getPluginIdByScriptId( scriptId );
}

bool LuaAddon::checkPluginPermission( const std::string& pluginId, const std::string& permission ) const {
    return m_extensionsManager->checkPluginPermission( pluginId, permission );
}

void LuaAddon::sendProgressStatus( const StatusData& statusData, const std::string& errorMessage ) {
    if( statusData.canBeSent ) {
        JsonObject data = statusData.toJson();
        if( errorMessage.empty() ) {
            json_add_null( data, "error" );
        }
        else {
            if( errorMessage.find( TimeoutErrorMessage ) != std::string::npos ) {
                setError( data, Errors::LUA_SCRIPT_TIMEOUT, errorMessage );
            }
            else {
                setError( data, Errors::LUA_SCRIPT_ERROR, errorMessage );
            }
        }
        sendBroadcast( PLUGIN_RUN_PROGRESS, data, GroupsNames::ADDON_LUA );
    }
}

int LuaAddon::getConfigTimeoutValue( const std::string& key ) {
    ConfigReader configReader;
    const auto configKey = key.c_str();
    if( configReader.hasValue( configKey ) )
        return configReader.getInt( configKey );

    LOGGER_WARN( "Cannot read value in \"" + cc::note( configKey ) + "\"." );
    return 0;
}

void LuaAddon::subscribeOnPluginRestarted( PluginStatusChangedCallback callback ) {
    m_pluginRestartedCallbacks.process( [&callback]( PluginStatusCallbacksCollection& collection ) {
        collection.emplace_back( std::move( callback ) );
    } );
}

void LuaAddon::subscribeOnPluginTeardown( PluginStatusChangedCallback callback ) {
    m_pluginTeardownCallbacks.process( [&callback]( PluginStatusCallbacksCollection& collection ) {
        collection.emplace_back( std::move( callback ) );
    } );
}

void LuaAddon::subscribeOnPluginUninstall( PluginStatusChangedCallback callback ) {
    m_pluginUninstallCallbacks.process( [&callback]( PluginStatusCallbacksCollection& collection ) {
        collection.emplace_back( std::move( callback ) );
    } );
}

void LuaAddon::handleCloudStorageEvents( const std::string& eventType ) {
    LuaD::CloudStorageEventsEmitter emitter( *this );
    if( eventType == "updated" ) {
        emitter.emitTokenUpdatedEvent();
    }
    else if( eventType == "expired" ) {
        emitter.emitTokenExpiredEvent();
    }
}

ExtensionsManager::ExecutionPolicy LuaAddon::fetchPluginPolicy( const std::string& pluginId ) {
    ExtensionsManager::ExecutionPolicy policy = ExtensionsManager::ExecutionPolicy::Unknown;
    m_luaInterpreters.process( [&pluginId, &policy]( PluginsLuaInterpretersCollection& collection ) {
        auto foundIt = collection.find( pluginId );

        if( foundIt != collection.end() ) {
            policy = foundIt->second.policy;
        }
    } );
    return policy;
}

void LuaAddon::printPluginMemoryUsage( int kiloBytes, const std::string& pluginId, const std::string& label ) {
    LOGGER_DEBUG_RAW( cc::debug( "Memory used by plugin with state: " ) + pluginId + cc::debug( ", " ) + label +
                      cc::debug( ": " ) + cc::num10( kiloBytes ) );
}

void LuaAddon::printProcessMemoryUsage() {
    SystemUtils::ProcFS::MemoryInfo memInfo;
    bool status = false;
    SystemUtils::ProcFS::MemoryInfo::ProcessStatus processInfo;
    std::tie( status, processInfo ) = memInfo.fetchProcessMemoryUsage( "self" );
    LOGGER_DEBUG_RAW( cc::debug( "Process memory usage: name: " ) + processInfo.name +
                      cc::debug( ", heap allocated: " ) + cc::num10( processInfo.rssAnon ) +
                      cc::debug( ", virtual: " ) + cc::num10( processInfo.vmRss ) );
}

void LuaAddon::emitInterpreterCreated( std::string pluginId ) {
    m_luaMetrics.onInterpreterCreated( pluginId );
}

void LuaAddon::emitScriptStarted( std::string plugin, std::string script, std::string operationId ) {
    m_luaMetrics.onScriptStarted( plugin, script );
}

void LuaAddon::emitScriptFinished( std::string plugin,
                                   std::string script,
                                   std::string operationId,
                                   MetricsEventEmitter::Duration duration ) {
    m_luaMetrics.onScriptFinished( plugin, script, duration );
}

void LuaAddon::emitScriptFailed( std::string plugin,
                                 std::string script,
                                 std::string operationId,
                                 MetricsEventEmitter::Duration duration ) {
    m_luaMetrics.onScriptFailed( plugin, script, duration );
}

void LuaAddon::emitScriptAborted( std::string plugin, std::string script, std::string operationId ) {
    m_luaMetrics.onScriptAborted( plugin, script, Duration::zero() );
}

void LuaAddon::emitScriptTimeout( std::string plugin, std::string script, std::string operationId, Duration duration ) {
    m_luaMetrics.onScriptTimeout( plugin, script, duration );
}

void LuaAddon::emitPluginResourceUsage( std::string pluginId, std::string policy, int kiloBytes ) {
    m_luaMetrics.onPluginResourceUsage( pluginId, policy, kiloBytes );
}

JsonObject LuaAddon::getServiceInfo() {
    JsonObject info;
    auto metrics = m_luaMetrics.toJson();
    if( !metrics.empty() ) {
        info[ "metrics" ] = metrics;
        return info;
    }
    return JSON_EMPTY_OBJ;
}

} /* namespace LuaAddon */
} /* namespace HubPlatform */
