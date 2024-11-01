#include "forward_notifications.h"

#include "hub_settings/hub_settings_reader.h"
#include "scenes_manager/modify_scene/modify_scene.h"
#include "scenes_manager/modify_scene/modify_scene_on_fetch.h"
#include "scenes_manager/scenes_engine/actions/scene_context_manager.h"
#include "uid/response_builders/getdevices/devices_list_builder.h"
#include "cloud_devices/remote_interface/cloud_devices_remote_interface.h"
#include "retry_command_manager/cloud_command.h"
#include "retry_command_manager/retry_command_manager.h"

#include "json_utils/json_utils.h"
#include <base_utils/timeutils.h>
#include <utility>

namespace HubPlatform {
namespace ScenesManager {

ForwardNotifications::ForwardNotifications( Services::Service& hostService )
: m_hostService( hostService ) {
    m_core = CoreDRemoteInterface::makeInstance( m_hostService );

    m_commandManager = std::make_unique< RetryCommandManager >( "ForwardNotifications",
                                                                std::chrono::seconds( 15 ),
                                                                std::chrono::seconds( 15 ),
                                                                std::chrono::minutes( 5 ),
                                                                m_hostService );
}

std::string ForwardNotifications::temperatureScale() {
    std::string defaultScale = CoreD::Model::ValueScales::Fahrenheit;
    Hub::SettingsReader reader( m_hostService.getDBClient() );
    auto scaleSetting = reader.findSetting( "scale.temperature" );
    if( scaleSetting && !scaleSetting->getTokenValue().empty() )
        defaultScale = scaleSetting->getTokenValue();
    return defaultScale;
}

std::pair< bool, Json > modifySceneScales( const Json& scene, const std::string& defaultScale ) {
    try {
        ModifyScene doIt( factoryModifySceneOnFetch() );
        return doIt.processScene( scene, defaultScale );
    }
    catch( const std::domain_error& e ) {
        LOGGER_ERROR( cc::error( "Exception: " ).append( e.what() ).append( ", domain_error" ) );
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( cc::error( "Exception: " ).append( e.what() ).append( typeid( e ).name() ) );
    }
    return std::make_pair< bool, Json >( false, nullptr );
}

JsonObject ForwardNotifications::getScene( const std::string& sceneId ) {
    auto& dbClient = m_hostService.getDBClient();
    auto result = dbClient.findOne( HaScenesDBName, JSON( db::ID_FIELD_NAME << sceneId ), nullptr );

    std::string defaultScale = temperatureScale();
    auto modifiedResult = modifySceneScales( result, defaultScale );
    if( modifiedResult.first ) {
        return modifiedResult.second;
    }
    return result;
}

JsonArray ForwardNotifications::getItems( const std::string& deviceId ) {
    JsonArrayBuilder res;
    auto items = m_core->getItemsByDeviceId( deviceId );
    for( const auto& item : items ) {
        res << item;
    }

    return res;
}

JsonObject ForwardNotifications::getDeviceInfo( const std::string& deviceId ) {
    ScenesManager::ResponseMessage message;
    auto device = m_core->getSingleDevice( deviceId );
    std::vector< JsonObject > devices;
    devices.push_back( device );
    HubPlatform::Uid::ResponseBuilder::DevicesListBuilder::buildDataListDevicesResponse( message, devices, nullptr );
    auto& answer = message.getAnswer();

    return message.getAnswer();
}

std::string ForwardNotifications::getRoomName( const JsonObject& device ) {
    std::string roomId = device[ "roomId" ];
    if( !roomId.empty() ) {
        auto room = m_core->getRoom( roomId );
        return room[ "name" ];
    }
    return "";
}

JsonObject ForwardNotifications::createControllerNotificationSendParams( const ActionInterface::Action& action,
                                                                         SendFinishCallbackPtr actionExecutorPtr ) {
    const auto args = action.getArgs();
    const auto& deviceId = json_get_str( args, "device" );
    const auto& itemId = json_get_str( args, "item" );

    JsonArray targets;
    json_get_arr( args, "targets", targets );
    if( !targets.size() ) {
        LOGGER_ERROR( cc::error( "invalid item targets" ) );
        actionExecutorPtr->setResult( false );
        actionExecutorPtr.reset();
        return JSON_EMPTY_OBJ;
    }

    const auto& notificationTemplate = json_get_str( args, "notificationTemplate" );
    if( notificationTemplate.empty() ) {
        LOGGER_ERROR( cc::error( "invalid item notificationTemplate" ) );
        actionExecutorPtr->setResult( false );
        actionExecutorPtr.reset();
        return JSON_EMPTY_OBJ;
    }

    JsonObject notificationBlock;
    json_get_obj( args, "notificationBlock", notificationBlock );
    if( notificationBlock.empty() ) {
        LOGGER_ERROR( cc::error( "invalid item notificationBlock" ) );
        actionExecutorPtr->setResult( false );
        actionExecutorPtr.reset();
        return JSON_EMPTY_OBJ;
    }

    std::string sceneId = action.getSceneId();
    std::string contextId = action.getContextId();
    auto broadcastMessage = action.getMessage();
    JsonObject params = JSON_EMPTY_OBJ;

    params[ "trace_id" ] = contextId;
    params[ "template_uuid" ] = notificationTemplate;
    if( !deviceId.empty() ) {
        auto device = getDeviceInfo( deviceId );
        params[ "device" ] = device;
        auto deviceInfo = m_core->getSingleDevice( deviceId );
        std::string parentDeviceId = deviceInfo[ "parentDeviceId" ];
        if( !parentDeviceId.empty() ) {
            params[ "parent_device" ] = getDeviceInfo( parentDeviceId );
        }
        params[ "items" ] = getItems( deviceId );
        params[ "room_name" ] = getRoomName( deviceInfo );
    }
    if( !sceneId.empty() ) {
        auto scene = getScene( sceneId );
        params[ "scene" ] = scene;
    }

    if( !broadcastMessage.empty() ) {
        params[ "meta" ][ "broadcast" ] = broadcastMessage;
    }

    if( !itemId.empty() ) {
        params[ "meta" ][ "itemId" ] = itemId;
    }

    params[ "targets" ] = targets;
    params[ "template_uuid" ] = notificationTemplate;
    params[ "firmware_timestamp" ] = TimeUtils::currentSystemTime().count();
    CloudDevices::RemoteInterface remoteInterface( m_hostService );
    params[ "files" ] = remoteInterface.getLocalNotificationDataFiles();

    return params;
}

void ForwardNotifications::run( const ActionInterface::Action& action,
                                ActionInterface::ActionExecutorPtr actionExecutorPtr ) {
    auto sendFinishCallbackPtr = std::make_shared< SendFinishCallback >( action, actionExecutorPtr );
    try {
        if( !action.isInitialized() ) {
            LOGGER_TRACE( "action is not initialized" );
            return;
        }

        const JsonObject params = createControllerNotificationSendParams( action, sendFinishCallbackPtr );

        JsonObject message;
        message[ "method" ] = "cloud.controller_notification_send";
        message[ "api" ] = 1;
        message[ "params" ] = std::move( params );

        const auto MaxRetriesNumber = 5;

        auto onAsyncCallSuccess = [sendFinishCallbackPtr]( const Json& sendData ) mutable {
            LOGGER_DEBUG( cc::debug( "ForwardNotifications: success callback" ) );
            sendFinishCallbackPtr->setResult( true );
            sendFinishCallbackPtr.reset();
        };

        auto onAsyncCallFail = [sendFinishCallbackPtr]( const Json& sendData ) mutable {
            LOGGER_DEBUG( cc::debug( "ForwardNotifications: fail callback" ) );
            sendFinishCallbackPtr->setResult( false );
            sendFinishCallbackPtr.reset();
        };

        m_command = std::make_shared< CloudCommand >(
            MaxRetriesNumber,
            message,
            std::move( [this, sendFinishCallbackPtr]( const Json& sendData ) mutable {
                m_hostService.sendAsyncRequest(
                    GroupsNames::UI_GROUP,
                    sendData,
                    CLOUD_MESSAGE,
                    [this, sendFinishCallbackPtr]( const JsonObject& message ) mutable {
                        LOGGER_DEBUG_RAW( cc::debug( "ForwardNotifications: receive callback" ) );
                        m_command->receive( isStatusComplete( message ) );
                        m_commandManager->checkResult();
                        m_command.reset();
                    },
                    [this, sendFinishCallbackPtr]() mutable {
                        LOGGER_DEBUG_RAW( cc::debug( "ForwardNotifications: timeout callback" ) );
                        sendFinishCallbackPtr->setResult( true );
                        sendFinishCallbackPtr.reset();
                        m_command->fail();
                        m_commandManager->checkResult();
                    },
                    std::chrono::minutes( 1 ) );
            } ),
            std::move( onAsyncCallSuccess ),
            std::move( onAsyncCallFail ) );

        m_commandManager->sendCommand( m_command );
    }
    catch( InvalidActionArgs& e ) {
        LOGGER_ERROR( cc::error( "Send cloud request error: + " ) + cc::attention( e.what() ) );
        sendFinishCallbackPtr->setResult( false );
        sendFinishCallbackPtr.reset();
    }
}

bool ForwardNotifications::isStatusComplete( const JsonObject& message ) {
    return json_get_int( message, "status" ) == 1 && json_get_int( message, "complete" ) == 1;
}

}  // namespace ScenesManager
}  // namespace HubPlatform
