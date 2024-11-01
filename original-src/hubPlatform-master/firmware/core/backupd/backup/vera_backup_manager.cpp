#include "vera_backup_manager.h"
#include "backupd/resources/backup_resources.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "jsonrpc/jsonrpc.h"

#include <thread>

namespace HubPlatform {
namespace BackupUpdateManager {
namespace {
const char* const coreServiceName = "cored";
const char* const scenesServiceName = "scenes_manager";
const char* const extensionsServiceName = "extensions_manager";
const char* const luaAddonName = "addon.lua";
const char* const zwaveAddonName = "addon.zwave";

const std::map< std::string, std::string > binaryNames = {
    { coreServiceName, "ha-cored" },
    { scenesServiceName, "ha-scenes_manager" },
    { extensionsServiceName, "ha-extensions_manager" },
    { luaAddonName, "ha-luad" },
    { zwaveAddonName, "ha-zwaved" },
};
}  // namespace

VeraBackupManager::VeraBackupManager( Messenger& messenger, ScriptManagerBase& scriptManager )
: m_messenger( messenger )
, m_scriptManager( scriptManager )
, m_actionList( { Resources::VeraBackupActions::MigrateDevicesAction,
                  Resources::VeraBackupActions::MigrateRoomsAction,
                  Resources::VeraBackupActions::MigrateScenesAction } ) {}

void VeraBackupManager::setUp( OnFinishedCallback onFinished ) {
    m_onFinished = std::move( onFinished );
}

void VeraBackupManager::start( std::string backupDirectory ) {
    m_runningDaemons.clear();
    m_backupDirectory = std::move( backupDirectory );

    auto action = m_actionList.front();
    startAction( action );
}

void VeraBackupManager::startAction( const std::string& actionName ) {
    using namespace Resources::VeraBackupActions;
    LOGGER_DEBUG_RAW( cc::debug( "Start action: " + actionName ) );

    if( actionName == MigrateDevicesAction ) {
        migrateDevices();
    }
    else if( actionName == MigrateRoomsAction ) {
        migrateRooms();
    }
    else if( actionName == MigrateScenesAction ) {
        migrateScenes();
    }
    else {
        LOGGER_ERROR_RAW( cc::error( "Failed to perform unknown action: " ) + cc::note( actionName ) );
        m_onFinished( "Unknown action" );
    }
}

bool VeraBackupManager::startDaemons( const std::vector< std::string > daemons ) {
    try {
        bool delayRequired = false;
        for( auto& daemon : daemons ) {
            if( !m_runningDaemons.count( daemon ) ) {
                delayRequired = true;
                m_scriptManager.startDaemon( binaryNames.at( daemon ), { "--vera_backup" } );
                m_runningDaemons.emplace( daemon );
            }
        }

        if( delayRequired )
            std::this_thread::sleep_for( m_daemonsStartupDelay );
        return true;
    }
    catch( const std::exception& e ) {
        std::string error( e.what() );
        std::string errorType( typeid( e ).name() );
        LOGGER_ERROR_RAW(
            cc::error( "Failed to start services: Exception (" + errorType + "): " + cc::note( error ) ) );
        std::this_thread::sleep_for( m_daemonsStartupDelay );
        m_onFinished( "Failed to start services: " + std::string( e.what() ) );
    }
    return false;
}

void VeraBackupManager::sendBackupAction( const std::string daemon, std::shared_ptr< ActionHandler > actionHandler ) {
    m_messenger.sendVeraBackupRequest(
        daemon,
        m_backupDirectory,
        actionHandler->actionName,
        std::bind( &VeraBackupManager::handleActionResponse, this, daemon, actionHandler, std::placeholders::_1 ) );
}

void VeraBackupManager::migrateDevices() {
    if( !startDaemons( { coreServiceName, extensionsServiceName, luaAddonName, zwaveAddonName } ) )
        return;

    auto actionHandler = std::make_shared< ActionHandler >();
    actionHandler->actionName = Resources::VeraBackupActions::MigrateDevicesAction;
    actionHandler->daemonsInProgress.emplace_back( zwaveAddonName );

    std::lock_guard< std::mutex > lock( actionHandler->mutex );
    for( auto& daemon : actionHandler->daemonsInProgress ) {
        sendBackupAction( daemon, actionHandler );
    }
}

void VeraBackupManager::migrateRooms() {
    if( !startDaemons( { coreServiceName } ) )
        return;

    auto actionHandler = std::make_shared< ActionHandler >();
    actionHandler->actionName = Resources::VeraBackupActions::MigrateRoomsAction;
    actionHandler->daemonsInProgress.emplace_back( coreServiceName );

    std::lock_guard< std::mutex > lock( actionHandler->mutex );
    for( auto& daemon : actionHandler->daemonsInProgress ) {
        sendBackupAction( daemon, actionHandler );
    }
}

void VeraBackupManager::migrateScenes() {
    if( !startDaemons( { coreServiceName, luaAddonName, scenesServiceName } ) )
        return;

    auto actionHandler = std::make_shared< ActionHandler >();
    actionHandler->actionName = Resources::VeraBackupActions::MigrateScenesAction;
    actionHandler->daemonsInProgress.emplace_back( scenesServiceName );

    std::lock_guard< std::mutex > lock( actionHandler->mutex );
    for( auto& daemon : actionHandler->daemonsInProgress ) {
        sendBackupAction( daemon, actionHandler );
    }
}

void VeraBackupManager::handleActionResponse( const std::string& service,
                                              std::shared_ptr< ActionHandler > handler,
                                              const JsonObject& response ) {
    std::unique_lock< std::mutex > lock( handler->mutex );
    const auto& actionName = handler->actionName;
    auto& queue = handler->daemonsInProgress;

    auto it = std::find( queue.begin(), queue.end(), service );
    if( it == queue.end() ) {
        LOGGER_WARN_RAW( cc::warn( "Got outdated action (" + actionName + ") response from " ) + cc::note( service ) +
                         cc::warn( " service" ) );
        return;
    }

    std::string status = json_get_str( response, "status" );
    if( status != "in_progress" )
        queue.erase( it );

    std::string error;
    if( hasError( response ) ) {
        std::string message = json_get_str( response[ "error" ], "message" );
        std::string reason = json_get_str( response[ "error" ], "reason" );
        LOGGER_ERROR_RAW( cc::error( "Service " ) + cc::note( service ) +
                          cc::error( " failed to perform action (" + actionName + "): " ) + cc::note( reason ) );
        error = "Service " + service + " failed to perform action: " + actionName + " reason: " + reason;
    }
    else if( status == "in_progress" ) {
        LOGGER_DEBUG_RAW( cc::debug( "Got action (" + actionName + ") response from " ) + cc::note( service ) +
                          cc::debug( " service with status: " ) + cc::note( status ) );
        sendBackupAction( service, handler );
        return;
    }
    else if( status != "finished" ) {
        std::string reason = ( status.empty() ) ? "empty status" : "status: " + status;

        std::string error_description = json_get_str( response, "error_description" );
        if( !error_description.empty() )
            reason += " : " + error_description;

        LOGGER_ERROR_RAW( cc::error( "Service " ) + cc::note( service ) +
                          cc::error( " failed to perform action (" + actionName + "): " ) + cc::note( reason ) );
        error = "Service " + service + " failed to perform action: " + actionName + " reason: " + reason;
    }
    else {
        LOGGER_DEBUG_RAW( cc::debug( "Got action (" + actionName + ") response from " ) + cc::note( service ) +
                          cc::debug( " service with status: " ) + cc::note( status ) );
    }

    if( !error.empty() && handler->error.empty() )
        handler->error = std::move( error );

    if( !queue.empty() )
        return;

    if( !handler->error.empty() ) {
        lock.unlock();
        LOGGER_ERROR_RAW( cc::error( "Vera migration failed on action: " ) + cc::note( handler->actionName ) );
        m_onFinished( handler->error );
        return;
    }

    LOGGER_DEBUG_RAW( cc::debug( "Action " ) + cc::note( actionName ) + cc::debug( " completed" ) );

    auto nextActionIt = std::find( m_actionList.cbegin(), m_actionList.cend(), actionName );
    ++nextActionIt;

    if( nextActionIt == m_actionList.cend() ) {
        LOGGER_DEBUG_RAW( cc::debug( "All actions completed" ) );
        m_onFinished( "" );
        return;
    }

    startAction( *nextActionIt );
}

}  // namespace BackupUpdateManager
}  // namespace HubPlatform
