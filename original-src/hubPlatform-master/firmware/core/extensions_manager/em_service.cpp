#include "extensions_manager/em_service.h"

#include "extensions_manager/extensions/extensions_manager.h"
#include "extensions_manager/resources/em_resources.h"

#include "service_interfaces/bus_wrapper.h"
#include "service_interfaces/groups_names.h"
#include "bus/messagesclasses.h"
#include "json_utils/json_utils.h"
#include "jsonrpc/jsonrpc.h"
#include "common_utils/backup/backup_helper.h"
#include "common_utils/backup/restore_helper.h"
#include "common_utils/lock_helper.h"
#include "common_utils/cpp14.h"
#include "database_client/dbid.h"
#include "database_client/dbqueryparser.h"
#include "cored/remote_interface/cored_interface_factory.h"

namespace HubPlatform {
namespace ExtensionsManager {

namespace {
const std::vector< std::string > BackupTables = { Resources::DBTableNames::AnonymousPlugins,
                                                  Resources::DBTableNames::Plugins,
                                                  Resources::DBTableNames::PluginsExtensionsPriorities,
                                                  Resources::DBTableNames::CustomNodes };
}

Service::Service()
: Services::Service( GroupsNames::EXTENSIONS_MANAGER ) {}

Service::Service( const std::string& name,
                  std::unique_ptr< Services::BusWrapper > impl,
                  std::unique_ptr< IDbClient > dbClient )
: Services::Service( name, std::move( impl ), std::move( dbClient ) ) {}

void Service::setUp() {
    Services::Service::setUp();

    m_addonUtils = std::make_unique< AddonUtils >();
    m_pluginUtils = std::make_unique< PluginUtils >();
    m_anonymousPluginUtils = std::make_unique< AnonymousPluginUtils >( *this );
    m_coreDInterface = CoreDRemoteInterface::makeInstance( *this );
    m_sceneManagerRemoteInterface = std::make_unique< ScenesManager::RemoteInterface >( *this );
    m_updateManager = std::make_unique< UpdateManager::RemoteInterface >( *this );
    m_pluginsManager = std::make_unique< ExtensionsManager >( *m_addonUtils,
                                                              *m_pluginUtils,
                                                              *this,
                                                              getDBClient(),
                                                              *m_coreDInterface,
                                                              *m_updateManager,
                                                              *m_sceneManagerRemoteInterface );

    m_pluginsManager->setUp();
    join( GroupsNames::ADDON_LUA );
}

bool Service::handleAsyncRequestFromBus( const JsonObject& request,
                                         MessageSubClasses requestType,
                                         const std::string& uuid,
                                         const std::string& sender ) {
    using MethodHandler = JsonObject ( Service::* )( const JsonObject& json );
    using MethodHandlersMap = std::unordered_map< MessageSubClasses, MethodHandler, MessageSubClassesHash >;
    static const MethodHandlersMap methodHandlers = {
        { ANONYMOUS_PLUGIN_ADD, &Service::handleAnonymousPluginAdd },
        { ANONYMOUS_PLUGIN_CHANGE, &Service::handleAnonymousPluginChange },
        { ANONYMOUS_PLUGIN_DELETE, &Service::handleAnonymousPluginDelete },
        { ANONYMOUS_PLUGIN_RUN, &Service::handleAnonymousPluginRun },
        { ANONYMOUS_PLUGIN_ACQUIRE, &Service::handleAnonymousPluginAcquire },
        { ANONYMOUS_PLUGIN_RELEASE, &Service::handleAnonymousPluginRelease },
        { GLOBAL_LOCK, &Service::handleLockRequest },
        { GLOBAL_UNLOCK, &Service::handleUnlockRequest },
        { BACKUP_CREATE, &Service::handleBackupCreateRequest },
        { PLUGIN_CONFIG_PRIORITY_SET, &Service::handlePluginConfigPrioritySet },
        { PLUGIN_RUN_REQUEST, &Service::handlePluginRunAsync },
        { CUSTOM_PLUGIN_REGISTER, &Service::handleCustomPluginRegister },
        { CUSTOM_PLUGIN_UNREGISTER, &Service::handleCustomPluginUnregister },
        { CUSTOM_PLUGIN_PAUSE, &Service::handleCustomPluginPause },
    };
    static const MethodHandlersMap postponableMethodHandlers = {
        { PLUGINS_AVAILABLE, &Service::handlePluginsAvailable },
        { PLUGINS_UNAVAILABLE, &Service::handlePluginsUnavailable },
        { PLUGINS_UNINSTALL, &Service::handlePluginsUninstall },
    };
    const auto it = methodHandlers.find( requestType );
    if( it != methodHandlers.end() ) {
        const auto handler = it->second;
        sendAsyncResponse( sender, ( this->*handler )( request ), uuid );
        return true;
    }

    const auto pit = postponableMethodHandlers.find( requestType );
    if( pit != postponableMethodHandlers.end() ) {
        const auto handler = pit->second;
        CommonUtils::LockHelper::global().postponeIfLocked(
            [=]() { sendAsyncResponse( sender, ( this->*handler )( request ), uuid ); } );
        return true;
    }

    return false;
}

void Service::handleMethodCall( const JsonObject& json, MessageSubClasses requestType, const std::string& /*sender*/ ) {
    using MethodHandler = void ( Service::* )( const JsonObject& json );
    using MethodHandlersMap = std::unordered_map< MessageSubClasses, MethodHandler, MessageSubClassesHash >;
    static const MethodHandlersMap methodHandlers = { { PLUGIN_RUN_REQUEST, &Service::handlePluginRun } };
    const auto it = methodHandlers.find( requestType );
    if( it != methodHandlers.end() ) {
        const auto handler = it->second;
        CommonUtils::LockHelper::global().postponeIfLocked( [=]() { ( this->*handler )( json ); } );
    }
}

void Service::handleMessageFromBus( const Services::Message& message, const std::string& sender ) {
    switch( message.msgClass ) {
    case EVENT:
        handleAddonEventFromBus( message );
        break;

    case SYSTEM_INFO:
        if( message.group == GroupsNames::BROADCAST_GROUP && message.msgSubClass == SYSTEM_BROADCAST ) {
            CommonUtils::LockHelper::global().postponeIfLocked( [=]() { handleInfoCmdFromBus( message.data ); } );
        }
        break;

    default:
        Services::Service::handleMessageFromBus( message, sender );
        break;
    }
}

void Service::handleAddonEventFromBus( const Services::Message& message ) {
    switch( message.msgSubClass ) {
    case ADDON_STARTED:
        CommonUtils::LockHelper::global().postponeIfLocked( [=]() { handleAddonStarted( message.data ); } );
        break;

    case ADDON_STOPPED:
        CommonUtils::LockHelper::global().postponeIfLocked( [=]() { handleAddonStopped( message.data ); } );
        break;

    case PLUGIN_RUN_PROGRESS:
        sendProgressStatus( PLUGIN_RUN_PROGRESS, message.data );
        break;

    default:
        break;
    }
}

void Service::handleInfoCmdFromBus( const Json& command ) {
    auto type = json_get_str( command, "type" );
    auto message = json_get_str( command, "message" );

    auto forEachPluginId = [&]( bool requireRegistered, std::function< void( const std::string& ) > callback ) {
        auto installedCustomPlugins = m_pluginsManager->getPluginUtils().getCustomPlugins();
        if( message == "all" ) {
            if( installedCustomPlugins.empty() ) {
                Log::error( cc::error( "No custom plugins found" ) );
            }
        }
        else {
            if( std::find( installedCustomPlugins.begin(), installedCustomPlugins.end(), message ) !=
                installedCustomPlugins.end() ) {
                installedCustomPlugins = { message };
            }
            else {
                installedCustomPlugins.clear();
                Log::error( cc::error( "Custom plugin " ) + cc::note( message ) + cc::error( " not found" ) );
            }
        }
        for( auto& pluginId : installedCustomPlugins ) {
            if( requireRegistered && !m_pluginsManager->isPluginRegistered( pluginId ) ) {
                Log::warn( cc::warn( "Custom plugin " ) + cc::note( pluginId ) + cc::warn( " not registered" ) );
                continue;
            }
            callback( pluginId );
        }
    };

    if( type == "hub.extensions.custom_plugin.register" ) {
        Log::info( cc::info( "Got request to register custom plugins" ) );
        forEachPluginId( false, [this]( const std::string& pluginId ) {
            if( m_pluginsManager->isPluginRegistered( pluginId ) ) {
                m_pluginsManager->unregisterPlugin( pluginId );
            }
            m_pluginsManager->registerPlugin( pluginId );
        } );
    }
    else if( type == "hub.extensions.custom_plugin.unregister" ) {
        Log::info( cc::info( "Got request to unregister custom plugins" ) );
        forEachPluginId( true,
                         [this]( const std::string& pluginId ) { m_pluginsManager->unregisterPlugin( pluginId ); } );
    }
    else if( type == "hub.extensions.custom_plugin.uninstall" ) {
        Log::info( cc::info( "Got request to uninstall custom plugins" ) );
        forEachPluginId( true,
                         [this]( const std::string& pluginId ) { m_pluginsManager->uninstallPlugin( pluginId ); } );
    }
    else if( type.find( "hub.extensions" ) == 0 ) {
        Log::error( cc::error( "Got unknown request " ) + cc::note( type ) );
    }
}

// data = {
//        "id": "zwave",
//        "version": "1.2.64"
//        "setupId": 1547125574962212096
//    }
void Service::handleAddonStarted( const JsonObject& data ) {
    int64_t setupId;
    json_get( data, "setupId", setupId );
    m_pluginsManager->onAddonStarted( CommonUtils::AddonSummary::fromJson( data ), setupId );
}

// message.data = {
//        "id": "zwave",
//        "version": "1.2.64"
//    }
void Service::handleAddonStopped( const JsonObject& data ) {
    m_pluginsManager->onAddonStopped( CommonUtils::AddonSummary::fromJson( data ) );
}

// json = {
//     "plugins": [ "zwave", "bluetooth" ]
// }
JsonObject Service::handlePluginsAvailable( const JsonObject& json ) {
    for( auto& plugin : json[ "plugins" ] ) {
        auto pluginId = json_get_str( plugin );
        m_pluginsManager->registerPlugin( pluginId );
    }
    return {};
}

// json = {
//     "plugins": [ "zwave", "bluetooth" ]
// }
JsonObject Service::handlePluginsUnavailable( const JsonObject& json ) {
    for( auto& plugin : json[ "plugins" ] ) {
        auto pluginId = json_get_str( plugin );
        m_pluginsManager->unregisterPlugin( pluginId );
    }
    return {};
}

// json = {
//     "plugins": [ "zwave", "bluetooth" ]
// }
JsonObject Service::handlePluginsUninstall( const JsonObject& json ) {
    for( auto& plugin : json[ "plugins" ] ) {
        auto pluginId = json_get_str( plugin );
        m_pluginsManager->uninstallPlugin( pluginId );
    }
    return {};
}

// json = {
//        "code": "require \"core\"\n local items = core.get_items()\n...",
//        "name": "the best script ever"
// }
JsonObject Service::handleAnonymousPluginAdd( const JsonObject& json ) {
    using namespace Resources::AnonymousPlugin;

    const auto code = json_get_str( json, Fields::Code );
    const auto name = json_get_str( json, Fields::Name );

    const auto pluginId = m_anonymousPluginUtils->addPlugin( code, name );

    return JSON( Fields::Id << pluginId );
}

// json = {
//        "pluginId": "100500..."
// }
JsonObject Service::handleAnonymousPluginDelete( const JsonObject& json ) {
    const auto pluginId = json_get_str( json, Resources::AnonymousPlugin::Fields::Id );

    return m_anonymousPluginUtils->deletePlugin( pluginId ).toJson();
}

// json = {
//        "pluginId": "100500..."
//        "code": "require \"core\"\n local items = core.get_items()\n...",
//        "name": "the best script ever"
// }
JsonObject Service::handleAnonymousPluginChange( const JsonObject& json ) {
    using namespace Resources::AnonymousPlugin;

    const auto pluginId = json_get_str( json, Fields::Id );
    const auto code = json_get_str( json, Fields::Code );
    const auto name = json_get_str( json, Fields::Name );

    return m_anonymousPluginUtils->changePlugin( pluginId, code, name ).toJson();
}

// json = {
//        "pluginId": "100500..."
// }
JsonObject Service::handleAnonymousPluginRun( const JsonObject& json ) {
    const auto pluginId = json_get_str( json, Resources::AnonymousPlugin::Fields::Id );

    return m_anonymousPluginUtils->runPlugin( pluginId ).toJson();
}

// json = {
//        "pluginId": "100500..."
// }
JsonObject Service::handleAnonymousPluginAcquire( const JsonObject& json ) {
    const auto pluginId = json_get_str( json, Resources::AnonymousPlugin::Fields::Id );

    return m_anonymousPluginUtils->acquirePlugin( pluginId ).toJson();
}

// json = {
//        "pluginId": "100500..."
// }
JsonObject Service::handleAnonymousPluginRelease( const JsonObject& json ) {
    const auto pluginId = json_get_str( json, Resources::AnonymousPlugin::Fields::Id );

    m_anonymousPluginUtils->releasePlugin( pluginId );

    return JSON_EMPTY_OBJ;
}

// json = {
//        "reason": "backup"
// }
JsonObject Service::handleLockRequest( const JsonObject& json ) {
    auto& lockHelper = CommonUtils::LockHelper::global();
    lockHelper.tryLock( json );
    return lockHelper.getResponse();
}

// json = {
//        "reason": "backup"
// }
JsonObject Service::handleUnlockRequest( const JsonObject& json ) {
    auto& lockHelper = CommonUtils::LockHelper::global();
    lockHelper.unlock( json );
    return lockHelper.getResponse();
}

JsonObject Service::handleBackupCreateRequest( const JsonObject& json ) {
    CommonUtils::BackupHelper helper( getDBClient(), json );
    for( const auto& pluginId : helper.getPluginIds() ) {
        if( !helper.backupDatabase( getPluginDBName( pluginId ) ) ) {
            return helper.getResponse();
        }
    }
    for( const auto& tableName : BackupTables ) {
        if( !helper.backupDatabase( tableName ) ) {
            return helper.getResponse();
        }
    }

    // NOTE: Not backing up following databases:
    //  temp.db.firmware.extensions_manager.plugins
    return helper.getResponse();
}

JsonObject Service::handlePluginConfigPrioritySet( const JsonObject& json ) {
    const auto pluginId = json_get_str( json, "pluginId" );
    auto priorities = json_get_vec< std::string >( json, "priorities" );

    m_pluginsManager->updatePluginExtensionsPriorities( pluginId, std::move( priorities ) );
    return JSON( "list" << JSON( "priorities" << m_pluginsManager->getPluginExtensions( pluginId ) ) );
}

int Service::restoreBackup( const std::string& tempBackupFolder ) {
    CommonUtils::RestoreHelper helper( getDBClientSafe(), tempBackupFolder );
    for( auto& pluginId : helper.getPluginIds() ) {
        if( !helper.restoreDatabase( getPluginDBName( pluginId ) ) ) {
            return helper.getReturnCode();
        }
    }

    for( auto& tableName : BackupTables ) {
        if( !helper.restoreDatabase( tableName ) ) {
            return helper.getReturnCode();
        }
    }
    return helper.getReturnCode();
}

// json = {
//        "script": "HUB:zwave/scripts/init",
//        "scriptParams": {},
//        ...
// TODO:  "code": "..."
//    }
void Service::handlePluginRun( const JsonObject& json ) {
    auto scriptName = json_get_str( json, "script" );

    m_pluginUtils->checkScriptFilePath( scriptName );

    Log::info( cc::info( "Running plugin script: " ) + cc::note( scriptName ) );

    std::string operationId = json_get_str( json, "operationId" );
    bool isUserRunScript = json_get_bool( json, "isUserRunScript" );

    if( !m_pluginUtils->getInternalPluginLuaScriptFile( scriptName ) ) {
        JsonObject error = JSON_EMPTY_OBJ;
        setError( error, Errors::LUA_SCRIPT_OPEN_ERROR );
        if( isUserRunScript )
            sendProgressStatus( PLUGIN_RUN_PROGRESS, "failed", operationId, 0, error[ "error" ] );
        else
            LOGGER_ERROR( cc::error( "Script cannot be run: " ) + cc::j( error ) );
        return;
    }

    Json scriptParams = JSON_EMPTY_OBJ;
    json_get_obj( json, "scriptParams", scriptParams );
    sendExecutePluginScript( scriptName, scriptParams, operationId, isUserRunScript );
}

JsonObject Service::handlePluginRunAsync( const JsonObject& json ) {
    auto scriptName = json_get_str( json, "script" );

    m_pluginUtils->checkScriptFilePath( scriptName );

    Log::info( cc::info( "Running plugin script from async method: " ) + cc::note( scriptName ) );

    std::string operationId = json_get_str( json, "operationId" );
    bool isUserRunScript = json_get_bool( json, "isUserRunScript" );

    if( !m_pluginUtils->getInternalPluginLuaScriptFile( scriptName ) ) {
        JsonObject error = JSON_EMPTY_OBJ;
        setError( error, Errors::LUA_SCRIPT_OPEN_ERROR );
        if( isUserRunScript )
            sendProgressStatus( PLUGIN_RUN_PROGRESS, "failed", operationId, 0, error[ "error" ] );
        else
            LOGGER_ERROR( cc::error( "Script cannot be run: " ) + cc::j( error ) );

        return error;
    }

    Json scriptParams = JSON_EMPTY_OBJ;
    json_get_obj( json, "scriptParams", scriptParams );
    JsonObject response = JSON_EMPTY_OBJ;
    ScriptResult scriptResults = sendExecutePluginScriptSync( scriptName, scriptParams, operationId, isUserRunScript );

    if( scriptResults.hasError() ) {
        JsonObject error = JSON_EMPTY_OBJ;
        setError( error, Errors::LUA_SCRIPT_ERROR, scriptResults.getError() );
        if( isUserRunScript )
            sendProgressStatus( PLUGIN_RUN_PROGRESS, "failed", operationId, 0, error[ "error" ] );
        else
            LOGGER_ERROR( cc::error( "Script run error: " ) + cc::j( error ) );

        return error;
    }
    LOGGER_TRACE( cc::trace( "scriptResults: " ) + cc::j( scriptResults.toJson() ) );
    return scriptResults.toJson();
}

JsonObject Service::handleCustomPluginRegister( const JsonObject& json ) {
    LOGGER_INFO_RAW( cc::info( "Got request to register a custom plugin" ) );

    auto pluginId = json_get_str( json, "id" );

    if( m_pluginsManager->isPluginRegistered( pluginId ) ) {
        m_pluginsManager->unregisterPlugin( pluginId );
    }
    m_pluginsManager->registerPlugin( pluginId );
    return {};
}

JsonObject Service::handleCustomPluginUnregister( const JsonObject& json ) {
    LOGGER_INFO_RAW( cc::info( "Got request to uninstall a custom plugin" ) );

    auto pluginId = json_get_str( json, "id" );

    if( m_pluginsManager->isPluginRegistered( pluginId ) ) {
        m_pluginsManager->uninstallPlugin( pluginId );
    }
    return {};
}

JsonObject Service::handleCustomPluginPause( const JsonObject& json ) {
    LOGGER_INFO_RAW( cc::info( "Got request to pause a custom plugin" ) );
    auto pluginId = json_get_str( json, "id" );

    if( m_pluginsManager->isPluginRegistered( pluginId ) ) {
        m_pluginsManager->unregisterPlugin( pluginId );
    }
    return {};
}

void Service::sendProgressStatus( MessageSubClasses processType,
                                  std::string status,
                                  int percentage,
                                  const CommonUtils::PluginSummary& pluginSummary ) {
    JsonObject data;
    json_add_null( data, "error" );
    json_add_int( data, "completed", percentage );
    json_add_str( data, "status", status );

    if( pluginSummary.isValid() )
        data[ "plugin" ] = pluginSummary.toJson();

    Services::Message message = { GroupsNames::UI_GROUP, EVENT, processType, data };
    send( message );
}

void Service::sendProgressError( MessageSubClasses processType,
                                 Errors::Error error,
                                 const CommonUtils::PluginSummary& pluginSummary ) {
    JsonObject data;
    setError( data, error );

    if( pluginSummary.isValid() )
        data[ "plugin" ] = pluginSummary.toJson();

    Services::Message message = { "ui_service", EVENT, processType, data };
    send( message );
}

// scriptParams = {
//        "someKey": "some_value",
//        ...
//    }
void Service::sendExecutePluginScript( const std::string& script,
                                       const Json& scriptParams,
                                       const std::string& operationId,
                                       bool isUserRunScript,
                                       int timeout ) {
    sendExecutePluginScriptSync( script, scriptParams, operationId, isUserRunScript, timeout );
}

ScriptResult Service::sendExecutePluginScriptSync( const std::string& script,
                                                   const Json& scriptParams,
                                                   const std::string& operationId,
                                                   bool isUserRunScript,
                                                   int timeout ) {
    auto data = JSON_EMPTY_OBJ;
    data[ "script" ] = script;
    data[ "operationId" ] = operationId;
    data[ "isUserRunScript" ] = isUserRunScript;
    if( !scriptParams.empty() ) {
        data[ "scriptParams" ] = scriptParams;
    }

    if( m_pluginUtils ) {
        auto pluginId = m_pluginUtils->extractPluginId( script );
        if( pluginId.empty() ) {
            Log::warn( cc::warn( "Can not extract plugin id from script path: " ) + cc::note( script ) );
            return ScriptResult( "Can not extract plugin id from script path" );
        }

        if( m_pluginsManager && !m_pluginsManager->isPluginRegistered( pluginId ) ) {
            Log::warn( cc::warn( "Plugin " ) + cc::note( pluginId ) + cc::error( " is not registered" ) );
            return ScriptResult( "Plugin is not registered" );
        }

        if( m_updateManager && m_updateManager->isPluginUnavailable( pluginId ) ) {
            Log::warn( cc::warn( "Plugin " ) + cc::note( pluginId ) + cc::error( " is unavailable" ) );
            return ScriptResult( "Plugin is unavailable" );
        }
    }

    auto response = sendRequest( "addon.lua", data, EXECUTE_SCRIPT, std::chrono::seconds{ timeout } );
    LOGGER_DEBUG( "response: " + response.dump() );

    if( hasError( response ) ) {
        LOGGER_WARN( "Timeout during script execution." );
        return ScriptResult( "Timeout during script execution." );
    }
    else if( response.count( "error" ) && response[ "error" ].is_string() ) {
        // Lua interpreter error, possibly
        auto message = json_get_str( response, "error" );
        LOGGER_ERROR( cc::error( "return error as string: " ) + cc::attention( message ) );
        return ScriptResult( "During script execution: " + message );
    }

    return ScriptResult::fromJson( response );
}

void Service::sendPingToAddon( const std::string& addonId ) {
    JsonObject data;
    json_add_str( data, "method", "pingAddon" );
    Services::Message message = { "addon." + addonId, JSON_RPC, CALL_METHOD, data };
    send( message );
}

bool Service::sendAddLibrary( const std::string& libraryName, const std::string& entryPoint ) {
    JsonObject data = JSON( "libraryName" << libraryName << "entryPoint" << entryPoint );
    auto response = sendRequest( "addon.lua", data, ADD_PLUGIN_LIBRARY, std::chrono::seconds( 60 ) );

    if( hasError( response ) ) {
        Log::error( cc::error( "response: " ) + cc::j( response ) );
        return false;
    }
    return true;
}

bool Service::sendRemoveLibrary( const std::string& libraryName ) {
    JsonObject data = JSON( "libraryName" << libraryName );
    auto response = sendRequest( "addon.lua", data, REMOVE_PLUGIN_LIBRARY, std::chrono::seconds( 60 ) );

    if( hasError( response ) ) {
        Log::error( cc::error( "response: " ) + cc::j( response ) );
        return false;
    }
    return true;
}

void Service::sendPluginRestarted( const std::string& pluginId ) {
    JsonObject data = JSON( "pluginId" << pluginId );
    Services::Message message = { "addon.lua", EVENT, PLUGIN_RESTARTED, data };
    send( message );
}

void Service::sendPluginTeardown( const std::string& pluginId ) {
    JsonObject data = JSON( "pluginId" << pluginId );
    Services::Message message = { "addon.lua", EVENT, PLUGIN_TEARDOWN, data };
    send( message );
}

void Service::sendPluginUninstall( const std::string& pluginId ) {
    JsonObject data = JSON( "pluginId" << pluginId );
    Services::Message message = { "addon.lua", EVENT, PLUGIN_UNINSTALL, data };
    send( message );
}

void Service::sendProgressStatus( MessageSubClasses processType,
                                  const std::string& status,
                                  const std::string& operationId,
                                  int percentage,
                                  const JsonObject& error ) {
    JsonObject data;
    if( error.empty() ) {
        json_add_null( data, "error" );
    }
    else {
        data[ "error" ] = error;
    }
    json_add_str( data, "status", status );
    json_add_str( data, "operationId", operationId );
    json_add_int( data, "completed", percentage );
    Services::Message message = { GroupsNames::UI_GROUP, EVENT, processType, data };
    send( message );
}

void Service::sendProgressStatus( MessageSubClasses processType, const JsonObject& data ) {
    send( { GroupsNames::UI_GROUP, EVENT, processType, data } );
}

}  // namespace ExtensionsManager
}  // namespace HubPlatform
