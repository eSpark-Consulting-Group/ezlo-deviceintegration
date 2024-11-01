#include "hub_backup_action.h"
#include "firmware/common/service_interfaces/service.h"
#include "scenes_manager/scenes_processor/resources.h"
#include "scenes_manager/scenes_engine/scene_manager.h"

namespace HubPlatform {
namespace ScenesManager {

HubBackupRun::HubBackupRun( ControllerFactory& controllerFactory,
                            Services::Service& hostService,
                            ScenesRequestHandlerInterface& scenesHandler )
: ControllerCoreActions( controllerFactory )
, m_scenesHandler( scenesHandler )
, m_hostService( hostService ) {}

void HubBackupRun::run( const ActionInterface::Action& action, ActionInterface::ActionExecutorPtr actionExecutorPtr ) {
    if( !action.isInitialized() ) {
        LOGGER_ERROR( cc::error( "Hub Backup Run Action is not initialized" ) );
        return;
    }
    m_scenesHandler.saveHubBackupRunCallbackPtr( action, actionExecutorPtr );

    try {
        auto requestData = getRequestData( action.getArgs() );

        m_hostService.sendAsyncRequest( requestData.m_serviceName,
                                        requestData.m_requestBody,
                                        requestData.m_requestType,
                                        [this]( const JsonObject& message ) mutable {
                                            if( hasError( message ) ) {
                                                m_scenesHandler.setHubBackupRunState( false );
                                            }
                                        },
                                        std::chrono::minutes( 1 ) );
    }
    catch( const InvalidActionArgs& e ) {
        LOGGER_ERROR( cc::error( "Hub backup run error: " ) + cc::attention( e.what() ) );
        m_scenesHandler.setHubBackupRunState( false );
    }
}

HubBackupRun::RequestData HubBackupRun::getRequestData( const Action::Arguments& arguments ) {
    JsonObject message = JSON_EMPTY_OBJ;
    if( arguments[ ActionFields::HubBackupFields::Action ] == ActionFields::HubBackupFields::Create ) {
        return { GroupsNames::BACKUP, BACKUP_CREATE, std::move( message ) };
    }
    else if( arguments[ ActionFields::HubBackupFields::Action ] == ActionFields::HubBackupFields::Restore ) {
        JsonObject value;
        if( json_get_obj( arguments, ActionFields::HubBackupFields::BackupId, value ) ) {
            message[ ActionFields::HubBackupFields::Key ] = json_get_str( value, ActionFields::HubBackupFields::Key );
            message[ ActionFields::HubBackupFields::Uuid ] = json_get_str( value, ActionFields::HubBackupFields::Uuid );
            return { GroupsNames::BACKUP, BACKUP_RESTORE, std::move( message ) };
        }
        else {
            throw InvalidActionArgs( " \"backupId\" field value is not an object " );
        }
    }
    else {
        auto actionValue = json_get_str( arguments, ActionFields::HubBackupFields::Action );
        throw InvalidActionArgs( "invalid \"action\" field value: " + actionValue );
    }
    return {};
}

}  // namespace ScenesManager
}  // namespace HubPlatform
