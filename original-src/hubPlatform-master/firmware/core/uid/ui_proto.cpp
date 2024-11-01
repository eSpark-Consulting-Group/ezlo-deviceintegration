
#include "uid/ui_proto.h"

#include "uid/ezlo.h"
#include "uid/broadcast.h"
#include "uid/fo_converter.h"
#include "uid/answers.h"
#include "uid/firmware.h"
#include "uid/response_builders/response_builders_factory.h"
#include "uid/response_builders/hub_modes_get/iresponse_builder.h"
#include "uid/response_builders/gateway_settings_get/iresponse_builder.h"
#include "uid/field_names.h"
#include "uid/nma_client.h"
#include "uid/gateway_ids_filter.h"
#include "uid/ui_service.h"

#include "uid/ui_preferences/ui_preferences_manager.h"
#include "uid/handlers/hub_firmware_commands_handler.h"
#include "uid/handlers/hub_offline_commands_handler.h"
#include "uid/handlers/hub_extensions_commands_handler.h"
#include "uid/handlers/hub_ssh_tunnel_commands_handler.h"
#include "uid/handlers/hub_swinnger_shutdown_commands_handler.h"
#include "uid/handlers/hub_walk_test_commands_handler.h"

#include "common_utils/firmware_info_helper.h"
#include "base_utils/utils.h"
#include "base_utils/ezlodb.h"
#include "base_utils/acl.h"
#include "bus/spread_bus.h"
#include "json_utils/json_path.h"

#include "database_client/dbqueryparser.h"
#include "extensions_manager/exceptions.h"
#include "extensions_manager/remote_interface/em_remote_interface.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/remote_interface/system_attribute_types.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/cored_gateway_setting.h"
#include "hub_settings/hub_settings_manager.h"
#include "extensions_manager/remote_interface/em_remote_interface.h"

#include "version/ha_version.h"
#include "hardware_manager/remote_interface/hm_remote_interface.h"
#include <hardware_manager/battery/battery_info.h>
#include "custom_nodes/custom_nodes_list.h"
#include "cored/remote_interface/cored_interface_factory.h"

namespace HubPlatform {
namespace Uid {

void UiProto::createRoom( rpc::Message& rpcMessage ) {
    JsonObject& joAnswer = rpcMessage.getAnswer();
    const JsonObject& joParams = rpcMessage.getParams();
    std::string name;
    if( !getField( joParams, "name", joAnswer, name ) )
        return;

    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
    auto response = coreDInterface.createRoom( name );

    if( hasError( response ) ) {
        joAnswer[ "error" ] = std::move( response[ "error" ] );
    }
    else {
        joAnswer[ "result" ] = std::move( response );
    }
}

void UiProto::setRoomName( rpc::Message& rpcMessage ) {
    JsonObject& joAnswer = rpcMessage.getAnswer();
    const JsonObject& joParams = rpcMessage.getParams();
    std::string id, name;
    if( !getField( joParams, "_id", joAnswer, id ) || !getNonEmptyField( joParams, "name", joAnswer, name ) )
        return;

    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
    auto response = coreDInterface.setRoomName( id, name );

    if( hasError( response ) ) {
        joAnswer[ "error" ] = std::move( response[ "error" ] );
    }
    else if( !response.empty() ) {
        joAnswer[ "result" ] = std::move( response );
    }
}

void UiProto::deleteRoom( rpc::Message& rpcMessage ) {
    JsonObject& joAnswer = rpcMessage.getAnswer();

    const JsonObject& joParams = rpcMessage.getParams();
    std::string id;
    if( !getField( joParams, "_id", joAnswer, id ) )
        return;

    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
    auto response = coreDInterface.deleteRoom( id );

    if( hasError( response ) ) {
        joAnswer[ "error" ] = std::move( response[ "error" ] );
    }
    else {
        joAnswer[ "result" ] = std::move( response );
    }
}

void UiProto::deleteAllRooms( rpc::Message& rpcMessage ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
    auto response = coreDInterface.deleteAllRooms();

    auto& joAnswer = rpcMessage.getAnswer();
    if( hasError( response ) ) {
        joAnswer[ "error" ] = std::move( response[ "error" ] );
    }
    else {
        joAnswer[ "result" ] = std::move( response );
    }
}

void UiProto::getRoom( rpc::Message& rpcMessage ) {
    auto& joAnswer = rpcMessage.getAnswer();
    std::string id;
    if( !getField( rpcMessage.getParams(), "_id", joAnswer, id ) )
        return;

    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
    auto response = coreDInterface.getRoom( id );

    if( hasError( response ) ) {
        joAnswer[ "error" ] = std::move( response[ "error" ] );
    }
    else {
        joAnswer[ "result" ] = std::move( response );
    }
}

void UiProto::setRoomsOrder( rpc::Message& rpcMessage ) {
    JsonObject& joAnswer = rpcMessage.getAnswer();
    const JsonObject& joParams = rpcMessage.getParams();
    JsonArray roomsOrder = JSON_EMPTY_ARR;
    if( !getFieldAsArray( joParams, "roomsId", joAnswer, roomsOrder ) )
        return;

    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
    auto response = coreDInterface.setRoomsOrder( roomsOrder );

    if( hasError( response ) ) {
        joAnswer[ "error" ] = std::move( response[ "error" ] );
    }
    else {
        joAnswer[ "result" ] = std::move( response );
    }
}

void UiProto::getGateways( rpc::Message& rpcMessage ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );

    auto response = coreDInterface.getGateways( rpcMessage );
    auto& answer = rpcMessage.getAnswer();

    if( !response.second ) {
        setErrorIfHas( answer, response.first );
        return;
    }

    answer[ "result" ] = std::move( response.first );
}

void UiProto::buildGetGatewaysSettingsResponse(
    rpc::Message& rpcMessage,
    const CoreDRemoteInterface::CoreDRemoteInterface::RawGatewaysSettings& gatewaysSettings ) {
    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
        GatewayIdsFilter gatewayIdsFilter( rpcMessage.getParams() );

        auto responseBuilder = ResponseBuildersFactory::createGetGatewaySettingResponseBuilder(
            rpcMessage, coreDInterface.getConfiguration() );
        if( !responseBuilder ) {
            LOGGER_WARN( cc::warn( "there is no GetGatewaySetting response builder" ) );
            return;
        }

        using namespace CoreD::Model;
        for( const auto& gatewaySetting : gatewaysSettings ) {
            if( !gatewayIdsFilter.empty() && !gatewayIdsFilter.checkGatewayId( json_get_str(
                                                 gatewaySetting, Resources::GatewaySettingFields::GatewayId ) ) ) {
                continue;
            }

            const auto settingId = json_get_str( gatewaySetting, db::ID_FIELD_NAME );
            auto setting = GatewaySetting::fromJson( gatewaySetting );

            bool* hasSetterPtr = nullptr;
            bool hasSetter = true;
            if( setting.isHasSetter() ) {
                hasSetter = setting.getHasSetter();
                hasSetterPtr = &hasSetter;
            }

            responseBuilder->addGatewaySetting( settingId,
                                                setting.getGatewayId(),
                                                setting.getLabel(),
                                                setting.getDescription(),
                                                setting.getValueType(),
                                                setting.getValue(),
                                                setting.getDefaultValue(),
                                                setting.getLowerLimitValue(),
                                                setting.getUpperLimitValue(),
                                                setting.getEnum(),
                                                setting.getInfo(),
                                                setting.getSynchronizationStatus(),
                                                hasSetterPtr );
        }

        JsonObject& joAnswer = rpcMessage.getAnswer();
        auto response = responseBuilder->buildResponse();
        joAnswer[ "result" ] = std::move( response[ "result" ] );
        joAnswer[ "api" ] = std::move( response[ "api" ] );
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( cc::error( "failed to build gateways settings response: " ) + cc::note( e.what() ) );
        rpcMessage.setError( Errors::INTERNAL_OPERATION_ERROR );
    }
}

void UiProto::listGatewaySettings( rpc::Message& rpcMessage ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
    buildGetGatewaysSettingsResponse( rpcMessage, coreDInterface.getGatewaysSettings() );
}

void UiProto::setGatewaySettingValue( rpc::Message& rpcMessage ) {
    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
        coreDInterface.setGatewaySettingValue( rpcMessage.getParams() );
    }
    catch( const std::exception& e ) {
        setError( rpcMessage.getAnswer(), Errors::WRONG_FIELD, e.what() );
    }
}

void UiProto::getItemsByDeviceId( rpc::Message& rpcMessage ) {
    const JsonObject& joParams = rpcMessage.getParams();
    JsonObject& joAnswer = rpcMessage.getAnswer();

    std::string deviceId;
    if( !getField( joParams, "deviceId", joAnswer, deviceId ) ) {
        return;
    }

    buildGetItemsResponse( rpcMessage, Fo::getItemsByDeviceId( deviceId ) );
}

void UiProto::renameDevice( rpc::Message& rpcMessage ) {
    auto& joAnswer = rpcMessage.getAnswer();

    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
    auto response = coreDInterface.renameDevice( rpcMessage.getParams() );
    Json error;
    if( json_get_obj( response, Errors::ERROR_FIELD, error ) ) {
        setErrorObject( joAnswer, error );
    }
}

void UiProto::setDeviceArmed( rpc::Message& rpcMessage ) {
    const JsonObject& joParams = rpcMessage.getParams();
    JsonObject& joAnswer = rpcMessage.getAnswer();

    std::string deviceId = json_get_str( joParams, "_id" );
    bool armed = json_get_bool( joParams, "armed", true );

    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    JsonObject joResponse = remoteInterface.setDeviceArmed( deviceId, armed );

    if( hasError( joResponse ) ) {
        joAnswer[ "error" ] = joResponse[ "error" ];
        joAnswer[ "result" ] = JSON_EMPTY_OBJ;
    }
    else {
        joAnswer[ "result" ] = joResponse[ "result" ];
    }
}

void UiProto::setDeviceProtectConfig( rpc::Message& rpcMessage ) {
    const JsonObject& joParams = rpcMessage.getParams();
    JsonObject& joAnswer = rpcMessage.getAnswer();

    std::string deviceId = json_get_str( joParams, "_id" );
    std::string protectConfig = json_get_str( joParams, "protectConfig" );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
        JsonObject joResponse = remoteInterface.setDeviceProtectConfig( deviceId, protectConfig );

        if( hasError( joResponse ) ) {
            joAnswer[ "error" ] = joResponse[ "error" ];
            joAnswer[ "result" ] = JSON_EMPTY_OBJ;
        }
        else {
            joAnswer[ "result" ] = joResponse[ "result" ];
        }
    }
    catch( ... ) {
        setError( joAnswer, Errors::WRONG_PARAMS, "Unknown ID passed" );
        return;
    }
}

void UiProto::placeDevice( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& joAnswer = rpcMessage.getAnswer();

    if( !checkField( rpcMessage.getParams(), "roomId", rpcMessage.getAnswer() ) ) {
        setError( joAnswer, Errors::WRONG_PARAMS );
        return;
    }

    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );

    auto deviceInfo = rpcMessage.getParams();

    const auto deviceId = json_get_str( deviceInfo, "_id" );
    const auto roomId = json_get_str( deviceInfo, DeviceFields::RoomIdFieldName );

    JsonArray separateChildDevicesArray;

    if( json_get( deviceInfo, DeviceFields::SeparateChildDevices, separateChildDevicesArray ) ) {
        if( !separateChildDevicesArray.is_array() ) {
            setErrorWithField( joAnswer, Errors::WRONG_PARAMS, "Incorrect type", DeviceFields::SeparateChildDevices );
            return;
        }
    }

    std::set< std::string > separateChildDevices;

    try {
        fromJsonStringArray( separateChildDevicesArray, [&separateChildDevices]( const std::string& value ) {
            separateChildDevices.emplace( value );
        } );
    }
    catch( ... ) {
        setErrorWithField( joAnswer, Errors::WRONG_PARAMS, "Incorrect type", DeviceFields::SeparateChildDevices );
        return;
    }

    JsonObject joResponse = coreDInterface.placeDevice( deviceId, roomId, separateChildDevices );

    joAnswer[ "result" ] = JSON_EMPTY_OBJ;
    if( hasError( joResponse ) ) {
        joAnswer[ "error" ] = joResponse[ "error" ];
    }
}

void UiProto::foSetFavorite( rpc::Message& rpcMessage ) {
    const JsonObject& joParams = rpcMessage.getParams();
    JsonObject& joAnswer = rpcMessage.getAnswer();
    foSetFavoriteImpl( joParams, joAnswer );
}

void UiProto::foGetFavorites( rpc::Message& rpcMessage ) {
    const JsonObject& joParams = rpcMessage.getParams();
    JsonObject& joAnswer = rpcMessage.getAnswer();
    foGetFavoriteImpl( joParams, joAnswer );
}

void UiProto::foGetFavoriteImpl( const JsonObject& joParams, JsonObject& joAnswer ) {
    JsonObject favoritesResult;

    JsonObject senderFavorites = Db::findOne( HaFavoriteDBName, JSON( "record" << 1 ) );

    JsonArray jaFavorites = JSON_EMPTY_ARR;

    JsonArray jaRequestedFavorites;
    json_get_arr( joParams, "request", jaRequestedFavorites );
    if( jaRequestedFavorites.empty() ) {
        jaRequestedFavorites.push_back( "devices" );
        jaRequestedFavorites.push_back( "items" );
        jaRequestedFavorites.push_back( "rules" );
    }

    for( const auto& element : jaRequestedFavorites ) {
        const std::string arrayKey = json_get_str( element );
        if( arrayKey.empty() ) {
            Log::error( cc::error( "foGetFavoriteImpl: failed to get arrayKey from element - " + cc::j( element ) ) );
            continue;
        }
        if( json_get_arr( senderFavorites, arrayKey, jaFavorites ) ) {
            favoritesResult[ arrayKey ] = jaFavorites;
            jaFavorites.clear();
        }
        else {
            favoritesResult[ arrayKey ] = JSON_EMPTY_ARR;
        }
    }

    joAnswer[ "result" ][ "favorites" ] = favoritesResult;
}

void UiProto::handleGetScene( rpc::Message& rpcMessage ) {
    const auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();
    std::string id;
    if( !getField( params, db::ID_FIELD_NAME, answer, id ) ) {
        return;
    }

    if( !Acl::check( params, Resource::SCENES, Permission::VIEW, answer ) ) {
        return;
    }

    answer[ "result" ] = getScenesManager().getScene( id );
}

void UiProto::handleTriggerDevicesList( rpc::Message& rpcMessage ) {
    const auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();
    if( !Acl::check( params, Resource::SCENES, Permission::VIEW, answer ) ) {
        return;
    }
    answer[ "result" ][ "devices" ] = getScenesManager().getTriggerDevicesList();
}

void UiProto::handleGetTimeList( rpc::Message& rpcMessage ) {
    const auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();
    if( !Acl::check( params, Resource::SCENES, Permission::VIEW, answer ) ) {
        return;
    }
    answer[ "result" ][ "timeScenes" ] = getScenesManager().getTimeList();
}

void UiProto::handleGetBlocks( rpc::Message& rpcMessage ) {
    const auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();
    if( !Acl::check( params, Resource::SCENES, Permission::VIEW, answer ) ) {
        return;
    }
    auto result = getScenesManager().getBlocksList( params );
    if( hasError( result ) ) {
        answer[ "error" ] = result[ "error" ];
        return;
    }
    std::string blockType;
    if( !getField( params, "blockType", answer, blockType ) ) {
        return;
    }
    answer[ "result" ][ blockType ] = result;
}

void UiProto::handleCreateScene( rpc::Message& rpcMessage ) {
    handleCreateScene( rpcMessage, /*isGlobal*/ false );
}

void UiProto::handleCreateGlobalScene( rpc::Message& rpcMessage ) {
    handleCreateScene( rpcMessage, /*isGlobal*/ true );
}

void UiProto::handleDeleteGlobalScene( rpc::Message& rpcMessage ) {
    handleDeleteScene( rpcMessage, /*isGlobal*/ true );
}

void UiProto::handleDeleteAllGlobalScenes( rpc::Message& rpcMessage ) {
    handleDeleteAllScenes( rpcMessage, /*isGlobal*/ true );
}

void UiProto::handleEditGlobalScene( rpc::Message& rpcMessage ) {
    handleEditScene( rpcMessage, /*isGlobal*/ true );
}

void UiProto::handleEnableGlobalScene( rpc::Message& rpcMessage ) {
    handleEnableScene( rpcMessage, /*isGlobal*/ true );
}

void UiProto::handleCreateScene( rpc::Message& rpcMessage, bool isGlobal ) {
    auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();
    if( !Acl::check( params, Resource::SCENES, Permission::CREATE, answer ) ) {
        return;
    }
    if( isGlobal ) {
        params[ "global" ] = true;
    }
    auto result = getScenesManager().createScene( params );
    if( hasError( result ) ) {
        answer[ "error" ] = result[ "error" ];
        return;
    }
    answer[ "result" ] = result;
}

void UiProto::handleDeleteScene( rpc::Message& rpcMessage, bool isGlobal ) {
    const auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();
    std::string id;
    if( !getNonEmptyField( params, db::ID_FIELD_NAME, answer, id ) ) {
        return;
    }
    if( !Acl::check( id, params, Resource::SCENES, Permission::DELETE, answer ) ) {
        return;
    }
    auto result = getScenesManager().deleteScene( id, isGlobal );
    if( hasError( result ) ) {
        answer[ "error" ] = result[ "error" ];
        return;
    }
    answer[ "result" ] = result;
}

void UiProto::handleDeleteAllScenes( rpc::Message& rpcMessage, bool isGlobal ) {
    auto& answer = rpcMessage.getAnswer();
    auto result = getScenesManager().deleteAllScenes( isGlobal );
    if( hasError( result ) ) {
        answer[ "error" ] = result[ "error" ];
        return;
    }
    answer[ "result" ] = result;
}

void UiProto::handleEditScene( rpc::Message& rpcMessage, bool isGlobal ) {
    auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();
    std::string id;
    if( !getField( params, db::ID_FIELD_NAME, answer, id ) ) {
        return;
    }
    if( !Acl::check( id, params, Resource::SCENES, Permission::EDIT, answer ) ) {
        return;
    }
    if( isGlobal ) {
        params[ "global" ] = true;
    }
    auto result = getScenesManager().editScene( params );
    if( hasError( result ) ) {
        answer[ "error" ] = result[ "error" ];
        return;
    }
    answer[ "result" ] = result;
}

void UiProto::handleCloneScene( rpc::Message& rpcMessage, bool isGlobal ) {
    auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();
    std::string id;
    if( !getField( params, db::ID_FIELD_NAME, answer, id ) ) {
        return;
    }

    std::string name;
    if( !getField( params, "name", answer, name ) ) {
        return;
    }
    if( !Acl::check( id, params, Resource::SCENES, Permission::CLONE, answer ) ) {
        return;
    }
    if( isGlobal ) {
        params[ "global" ] = true;
    }
    auto result = getScenesManager().cloneScene( id, name, isGlobal );
    if( hasError( result ) ) {
        answer[ "error" ] = result[ "error" ];
        return;
    }
    answer[ "result" ] = result;
}

void UiProto::handleEnableScene( rpc::Message& rpcMessage, bool isGlobal ) {
    const auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();
    std::string id;
    if( !getField( params, db::ID_FIELD_NAME, answer, id ) ) {
        return;
    }
    if( !Acl::check( id, params, Resource::SCENES, Permission::EDIT, answer ) ) {
        return;
    }
    bool enabled = false;
    if( !getField( params, "enabled", answer, enabled ) ) {
        return;
    }
    auto result = getScenesManager().enableScene( id, enabled, isGlobal );
    if( hasError( result ) ) {
        answer[ "error" ] = result[ "error" ];
        return;
    }
    answer[ "result" ] = result;
}

void UiProto::handleEditScene( rpc::Message& rpcMessage ) {
    handleEditScene( rpcMessage, /*isGlobal*/ false );
}

void UiProto::handleCloneScene( rpc::Message& rpcMessage ) {
    handleCloneScene( rpcMessage, /*isGlobal*/ false );
}

void UiProto::handleDeleteScene( rpc::Message& rpcMessage ) {
    handleDeleteScene( rpcMessage, /*isGlobal*/ false );
}

void UiProto::handleDeleteAllScenes( rpc::Message& rpcMessage ) {
    handleDeleteAllScenes( rpcMessage, /*isGlobal*/ false );
}

void UiProto::handleEnableScene( rpc::Message& rpcMessage ) {
    handleEnableScene( rpcMessage, /*isGlobal*/ false );
}

void UiProto::handleSetRoom( rpc::Message& rpcMessage ) {
    const auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();
    std::string sceneId, roomId;
    if( !getField( params, "sceneId", answer, sceneId ) || !getField( params, "roomId", answer, roomId ) ) {
        return;
    }
    if( !Acl::check( sceneId, params, Resource::SCENES, Permission::EDIT, answer ) ) {
        return;
    }
    auto result = getScenesManager().assignRoomToScene( sceneId, roomId );
    if( hasError( result ) ) {
        answer[ "error" ] = result[ "error" ];
        return;
    }
    answer[ "result" ] = result;
}

void UiProto::handleSetIcon( rpc::Message& rpcMessage ) {
    const auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();
    std::string sceneId, icon;
    if( !getField( params, "_id", answer, sceneId ) || !getField( params, "icon", answer, icon ) ) {
        return;
    }
    if( !Acl::check( sceneId, params, Resource::SCENES, Permission::EDIT, answer ) ) {
        return;
    }
    auto result = getScenesManager().assignIconToScene( sceneId, icon );
    if( hasError( result ) ) {
        answer[ "error" ] = result[ "error" ];
        return;
    }
    answer[ "result" ] = result;
}

void UiProto::handleAddNotification( rpc::Message& rpcMessage ) {
    const auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();
    std::string sceneId, userId;
    if( !getField( params, "sceneId", answer, sceneId ) || !getField( params, "userId", answer, userId ) ) {
        return;
    }
    if( !Acl::check( params, Resource::SCENES, Permission::EDIT, answer ) ) {
        return;
    }
    auto result = getScenesManager().addUserForNotification( sceneId, userId );
    if( hasError( result ) ) {
        answer[ "error" ] = result[ "error" ];
        return;
    }
    answer[ "result" ] = result;
}

void UiProto::handleRemoveNotification( rpc::Message& rpcMessage ) {
    const auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();
    std::string sceneId, userId;
    if( !getField( params, "sceneId", answer, sceneId ) || !getField( params, "userId", answer, userId ) ) {
        return;
    }
    if( !Acl::check( params, Resource::SCENES, Permission::EDIT, answer ) ) {
        return;
    }
    auto result = getScenesManager().removeUserFromNotifications( sceneId, userId );
    if( hasError( result ) ) {
        answer[ "error" ] = result[ "error" ];
        return;
    }
    answer[ "result" ] = result;
}

void UiProto::handleActionBlockTest( rpc::Message& rpcMessage ) {
    const auto& params = rpcMessage.getParams();
    auto& joAnswer = rpcMessage.getAnswer();
    if( !Acl::check( params, Resource::SCENES, Permission::EXECUTE, joAnswer ) ) {
        return;
    }
    JsonArray block;
    if( !getField( params, "block", joAnswer, block ) ) {
        return;
    }
    try {
        auto peer = rpcMessage.getNetworkObject();
        getScenesManager().actionBlockTest( block, [joAnswer, peer]( const JsonObject& response ) mutable {
            setErrorIfHas( joAnswer, response );
            joAnswer[ "result" ] = std::move( response );
            json_remove( joAnswer[ "result" ], Errors::ERROR_FIELD );
            if( peer ) {
                peer->sendMessage( joAnswer );
            }
        } );
        rpcMessage.getAnswer() = nullptr;
    }
    catch( const std::exception& e ) {
        setError( rpcMessage.getAnswer(), Errors::WRONG_FIELD, e.what() );
    }
}

void UiProto::handleNameSet( rpc::Message& rpcMessage ) {
    const auto& params = rpcMessage.getParams();
    auto& joAnswer = rpcMessage.getAnswer();
    if( !Acl::check( params, Resource::SCENES, Permission::EDIT, joAnswer ) ) {
        return;
    }
    std::string sceneId, name;
    if( !getField( params, "_id", joAnswer, sceneId ) || !getField( params, "name", joAnswer, name ) ) {
        return;
    }
    auto result = getScenesManager().setSceneName( sceneId, name );
    if( hasError( result ) ) {
        joAnswer[ "error" ] = result[ "error" ];
    }
}

void UiProto::handleHouseModeSet( rpc::Message& rpcMessage ) {
    const auto& params = rpcMessage.getParams();
    auto& joAnswer = rpcMessage.getAnswer();
    if( !Acl::check( params, Resource::SCENES, Permission::EDIT, joAnswer ) ) {
        return;
    }
    std::string sceneId;
    JsonArray houseModes;
    if( !getField( params, "_id", joAnswer, sceneId ) ||
        !getFieldAsArray( params, "houseModes", joAnswer, houseModes ) ) {
        return;
    }
    auto result = getScenesManager().setHouseModes( sceneId, houseModes );
    if( hasError( result ) ) {
        joAnswer[ "error" ] = result[ "error" ];
    }
}

void UiProto::handleBlockEnabledSet( rpc::Message& rpcMessage ) {
    const auto& params = rpcMessage.getParams();
    auto& joAnswer = rpcMessage.getAnswer();
    if( !Acl::check( params, Resource::SCENES, Permission::EDIT, joAnswer ) ) {
        return;
    }
    std::string sceneId;
    std::string blockId;
    bool enabled;
    if( !getField( params, "sceneId", joAnswer, sceneId ) || !getField( params, "blockId", joAnswer, blockId ) ||
        !getField( params, "enabled", joAnswer, enabled ) ) {
        return;
    }
    auto result = getScenesManager().setBlockEnabled( sceneId, blockId, enabled );
    if( hasError( result ) ) {
        joAnswer[ "error" ] = result[ "error" ];
    }
}

void UiProto::handleBlockStatusReset( rpc::Message& rpcMessage ) {
    const auto& params = rpcMessage.getParams();
    auto& joAnswer = rpcMessage.getAnswer();
    if( !Acl::check( params, Resource::SCENES, Permission::EDIT, joAnswer ) ) {
        return;
    }

    std::string sceneId;
    std::string blockId;
    bool enabled;
    if( !getField( params, "sceneId", joAnswer, sceneId ) ) {
        return;
    }
    getOptionalField( params, "blockId", blockId );
    LOGGER_TRACE( cc::trace( "sceneId: " + sceneId + ", blockId: " + blockId ) );

    Json result;
    if( blockId.empty() )
        result = getScenesManager().sceneAllBlocksStatusReset( sceneId );
    else
        result = getScenesManager().sceneBlockStatusReset( sceneId, blockId );

    if( hasError( result ) ) {
        joAnswer[ "error" ] = result[ "error" ];
    }
}

void UiProto::handleSceneMetaSet( rpc::Message& rpcMessage ) {
    const auto& params = rpcMessage.getParams();
    auto& joAnswer = rpcMessage.getAnswer();
    if( !Acl::check( params, Resource::SCENES, Permission::EDIT, joAnswer ) ) {
        return;
    }
    std::string sceneId;
    JsonObject meta;
    if( !getField( params, "sceneId", joAnswer, sceneId ) || !getField( params, "meta", joAnswer, meta ) ) {
        return;
    }
    auto result = getScenesManager().setMeta( sceneId, meta );
    if( hasError( result ) ) {
        joAnswer[ "error" ] = result[ "error" ];
    }
}

void UiProto::handleSceneBlockMetaSet( rpc::Message& rpcMessage ) {
    const auto& params = rpcMessage.getParams();
    auto& joAnswer = rpcMessage.getAnswer();
    if( !Acl::check( params, Resource::SCENES, Permission::EDIT, joAnswer ) ) {
        return;
    }
    std::string sceneId;
    std::string blockId;
    JsonObject meta;
    if( !getField( params, "sceneId", joAnswer, sceneId ) || !getField( params, "blockId", joAnswer, blockId ) ||
        !getField( params, "meta", joAnswer, meta ) ) {
        return;
    }
    auto result = getScenesManager().setBlockMeta( sceneId, blockId, meta );
    if( hasError( result ) ) {
        joAnswer[ "error" ] = result[ "error" ];
    }
}

void UiProto::handleSceneStatusGet( rpc::Message& rpcMessage ) {
    const auto& params = rpcMessage.getParams();
    auto& joAnswer = rpcMessage.getAnswer();
    if( !Acl::check( params, Resource::SCENES, Permission::VIEW, joAnswer ) ) {
        return;
    }

    std::string sceneId;
    if( !getField( params, "sceneId", joAnswer, sceneId ) ) {
        return;
    }

    LOGGER_TRACE( cc::trace( "sceneId: " + sceneId ) );
    auto result = getScenesManager().getSceneStatus( sceneId );
    if( hasError( result ) ) {
        joAnswer[ "error" ] = std::move( result[ "error" ] );
    }
    else {
        joAnswer[ "result" ] = std::move( result );
    }
}

void UiProto::handleSceneScriptAdd( rpc::Message& rpcMessage ) {
    ExtensionsManager::RemoteInterface extensionsInterface( getService() );
    std::string scriptCode, scriptName;

    const auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();

    if( !Acl::check( params, Resource::SCENES, Permission::CREATE, answer ) ) {
        return;
    }

    if( !getField( params, "code", answer, scriptCode ) ) {
        return;
    }

    scriptName = json_get_str( params, "name" );

    const auto pluginId = extensionsInterface.addAnonymousPlugin( scriptCode, scriptName );
    if( pluginId.empty() ) {
        setError( answer, Errors::BAD_PARAMS );
        return;
    }

    answer[ "result" ] = JSON( db::ID_FIELD_NAME << pluginId );
}

void UiProto::handleSceneScriptSet( rpc::Message& rpcMessage ) {
    ExtensionsManager::RemoteInterface extensionsInterface( getService() );
    std::string scriptId, scriptCode, scriptName;

    auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();

    if( !Acl::check( params, Resource::SCENES, Permission::EDIT, answer ) ) {
        return;
    }

    if( !getField( params, db::ID_FIELD_NAME, answer, scriptId ) ) {
        return;
    }

    scriptCode = json_get_str( params, "code" );
    scriptName = json_get_str( params, "name" );

    const auto result = extensionsInterface.changeAnonymousPlugin( scriptId, scriptCode, scriptName );
    if( !result ) {
        setError( answer, result.getError() );
        return;
    }

    answer[ "result" ] = JSON_EMPTY_OBJ;
}

void UiProto::handleSceneScriptGet( rpc::Message& rpcMessage ) {
    ExtensionsManager::RemoteInterface extensionsInterface( getService() );
    std::string scriptId;

    auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();

    if( !Acl::check( params, Resource::SCENES, Permission::VIEW, answer ) ) {
        return;
    }

    if( !getField( params, db::ID_FIELD_NAME, answer, scriptId ) ) {
        return;
    }

    const auto plugin = extensionsInterface.getAnonymousPluginContent( scriptId );
    if( !plugin ) {
        setError( answer, Errors::BAD_PARAMS );
        return;
    }

    answer[ "result" ] = plugin.toJson();
}

void UiProto::handleSceneScriptDelete( rpc::Message& rpcMessage ) {
    ExtensionsManager::RemoteInterface extensionsInterface( getService() );
    std::string scriptId;

    auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();

    if( !Acl::check( params, Resource::SCENES, Permission::DELETE, answer ) ) {
        return;
    }

    if( !getField( params, db::ID_FIELD_NAME, answer, scriptId ) ) {
        return;
    }

    const auto result = extensionsInterface.deleteAnonymousPlugin( scriptId );
    if( !result ) {
        setError( answer, result.getError() );
        return;
    }

    answer[ "result" ] = JSON_EMPTY_OBJ;
}

void UiProto::handleSceneScriptsList( rpc::Message& rpcMessage ) {
    ExtensionsManager::RemoteInterface extensionsInterface( getService() );

    auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();

    if( !Acl::check( params, Resource::SCENES, Permission::VIEW, answer ) ) {
        return;
    }

    const auto plugins = extensionsInterface.listAnonymousPlugins();

    auto result = JSON_EMPTY_ARR;
    for( const auto& elm : plugins ) {
        result.push_back( elm.toJson() );
    }

    answer[ "result" ] = result;
}

void UiProto::handleSceneScriptRun( rpc::Message& rpcMessage ) {
    ExtensionsManager::RemoteInterface extensionsInterface( getService() );
    std::string scriptId;

    auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();

    if( !Acl::check( params, Resource::SCENES, Permission::EXECUTE, answer ) ) {
        return;
    }

    if( !getField( params, db::ID_FIELD_NAME, answer, scriptId ) ) {
        return;
    }

    const auto result = extensionsInterface.runAnonymousPlugin( scriptId );
    if( !result ) {
        setError( answer, result.getError() );
        return;
    }

    answer[ "result" ] = JSON_EMPTY_OBJ;
}

void UiProto::getWhatHappened( rpc::Message& rpcMessage ) {
    const JsonObject& joParams = rpcMessage.getParams();
    JsonObject& joAnswer = rpcMessage.getAnswer();
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
    rpcMessage.setResult( coreDInterface.makeWhatHappenedRequest( joParams, GET_WHAT_HAPPENED ) );
}

void UiProto::setPreferences( rpc::Message& rpcMessage ) {
    const auto& joParams = rpcMessage.getParams();
    auto& joAnswer = rpcMessage.getAnswer();

    JsonObject joItemPreferences = JSON_EMPTY_ARR;
    if( json_get_arr( joParams, "items", joItemPreferences ) ) {
        auto& preferencesManager = UIPreferencesManager::getInstance();
        for( const auto& preferencesEntry : joItemPreferences ) {
            std::string id;
            JsonObject preferences = JSON_EMPTY_OBJ;
            if( json_get( preferencesEntry, "_id", id ) &&
                json_get_obj( preferencesEntry, "preferences", preferences ) ) {
                preferencesManager.setPreferences(
                    EntityType::ITEM, preferencesEntry[ "_id" ], preferencesEntry[ "preferences" ] );
            }
        }
    }
}

void UiProto::getPreferences( rpc::Message& rpcMessage ) {
    const auto& joParams = rpcMessage.getParams();
    auto& joAnswer = rpcMessage.getAnswer();

    auto& preferencesManager = UIPreferencesManager::getInstance();

    JsonObject joItems = JSON_EMPTY_ARR;
    if( json_get( joParams, "items", joItems ) ) {
        std::vector< JsonObject > preferenceEntries;
        if( !joItems.empty() ) {
            for( const auto& itemId : joParams[ "items" ] ) {
                preferenceEntries.push_back( preferencesManager.getPreferencesEntry( EntityType::ITEM, itemId ) );
            }
        }
        else {
            preferenceEntries = preferencesManager.getAllPreferenceEntries( EntityType::ITEM );
        }

        JsonObject joItemPreferences = JSON_EMPTY_ARR;
        for( auto& entry : preferenceEntries ) {
            if( !entry.empty() ) {
                JsonObject itemPreferences = JSON_EMPTY_OBJ;
                itemPreferences[ "_id" ] = std::move( entry[ "_id" ] );
                itemPreferences[ "preferences" ] = std::move( entry[ "preferences" ] );

                joItemPreferences.push_back( std::move( itemPreferences ) );
            }
        }

        joAnswer[ "result" ][ "items" ] = std::move( joItemPreferences );
    }
}

void UiProto::rebootEzlo( rpc::Message& /*rpcMessage*/ ) {
    Broadcast::sendToUis( EZLO_REBOOTED, JSON( "reboot" << true << "serial" << Ezlo::serial() ) );
    Ezlo::reboot();
}

void UiProto::getPowerStatus( rpc::Message& rpcMessage ) {
    JsonObject& joAnswer = rpcMessage.getAnswer();
    std::ifstream powerOnBattery( "/sys/class/power_supply/battery/online" );
    std::ifstream powerCapacity( "/sys/class/power_supply/battery/capacity" );
    std::string powerOnBatteryStr;
    powerOnBattery >> powerOnBatteryStr;
    std::string powerCapacityStr;
    powerCapacity >> powerCapacityStr;
    JsonObject joParams;
    int powerCapacityInt = 0;
    try {
        if( !powerCapacityStr.empty() )
            powerCapacityInt = std::stoi( powerCapacityStr );
    }
    catch( ... ) {
    }
    const bool isPowerOnBattery = powerOnBatteryStr != "0";
    json_add_bool( joParams, "isOnBattery", isPowerOnBattery );
    json_add_int( joParams, "capacity", powerCapacityInt );
    json_add_str( joParams, "serial", Ezlo::serial() );
    JsonObject result;
    result[ "params" ] = joParams;
    joAnswer[ "result" ] = result;
}

void UiProto::foSetFavoriteImpl( const JsonObject& joParams, JsonObject& joAnswer ) {
    Service& service = getService();
    service.addTask( [joParams, this]() {
        JsonArray devices, items, rules;
        JsonObject addedResult, removedResult;
        CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );

        auto processFavorite = [&addedResult, &removedResult]( const std::string& arrayName,
                                                               const std::set< std::string >& existingElements,
                                                               const JsonArray& jaElements ) {
            std::string elementId;
            JsonArray jaAdded, jaRemoved;
            for( const auto& element : jaElements ) {
                if( !json_get( element, "_id", elementId ) )  // if no element _id found - go further
                    continue;

                if( json_get_bool( element, "favorite", true ) ) {
                    if( !Db::has( HaFavoriteDBName, JSON( "record" << 1 ) ) ) {
                        Db::insert( HaFavoriteDBName, JSON( "record" << 1 ) );
                    }
                    if( existingElements.find( elementId ) == existingElements.end() ) {
                        continue;
                    }
                    Db::update( HaFavoriteDBName,
                                JSON( "record" << 1 ),
                                JSON( "$addToSet" << JSON( arrayName << elementId ) ) );
                    jaAdded.push_back( elementId );
                }
                else {
                    Db::update(
                        HaFavoriteDBName, JSON( "record" << 1 ), JSON( "$pull" << JSON( arrayName << elementId ) ) );
                    if( existingElements.find( elementId ) == existingElements.end() ) {
                        continue;
                    }
                    jaRemoved.push_back( elementId );
                }
            }
            if( !jaAdded.empty() ) {
                addedResult[ arrayName ] = std::move( jaAdded );
            }
            if( !jaRemoved.empty() ) {
                removedResult[ arrayName ] = std::move( jaRemoved );
            }
        };

        auto getIds = []( const std::vector< JsonObject >& elements ) -> std::set< std::string > {
            std::set< std::string > elementIds;
            for( const auto& element : elements ) {
                elementIds.insert( json_get_str( element, "_id" ) );
            }
            return elementIds;
        };

        if( json_get_arr( joParams, "devices", devices ) ) {
            processFavorite( "devices", getIds( coreDInterface.getDevices() ), devices );
        }

        if( json_get_arr( joParams, "items", items ) )
            processFavorite( "items", getIds( coreDInterface.getItems() ), items );

        if( json_get_arr( joParams, "rules", rules ) ) {
            std::set< std::string > elementIds;
            auto scenesIds = getScenesManager().getSceneIds();
            for( const auto& id : scenesIds ) {
                elementIds.insert( id );
            }
            processFavorite( "rules", elementIds, rules );
        }

        if( !addedResult.empty() ) {
            Broadcast::sendToUis( HUB_FAVORITE_ADDED, addedResult );
        }
        if( !removedResult.empty() ) {
            Broadcast::sendToUis( HUB_FAVORITE_REMOVED, removedResult );
        }
    } );
}

void UiProto::justIgnoreMethod( rpc::Message& /*rpcMessage*/ ) {}

void UiProto::getHubStatus( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
    auto hubStatus = coreDInterface.getHubStatus( rpcMessage );
    auto& answer = rpcMessage.getAnswer();

    if( hasError( hubStatus ) ) {
        answer[ "error" ] = hubStatus[ "error" ];
        answer[ "result" ] = JSON_EMPTY_OBJ;
    }
    else {
        answer[ "result" ] = hubStatus;
    }
}

void UiProto::getTemplateForDevice( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;

    const auto deviceId = json_get_str( rpcMessage.getParams(), "deviceId" );

    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
    auto device = coreDInterface.getSingleDevice( deviceId );

    // No such device
    if( device.empty() ) {
        setError( rpcMessage.getAnswer(), Errors::NO_DEVICE_FOUND );
        return;
    }

    const auto templateName = json_get_str( device, "templateName" );
    if( templateName.empty() ) {
        setError( rpcMessage.getAnswer(), Errors::WRONG_PARAMS );
        return;
    }

    try {
        ExtensionsManager::RemoteInterface extensionsInterface( getService() );
        const auto templateStr = extensionsInterface.getTemplateForDevice( templateName );
        rpcMessage.getAnswer()[ "result" ][ "template" ] = templateStr;
    }
    catch( const ExtensionsManager::TemplateNotFoundError& exception ) {
        setError( rpcMessage.getAnswer(), Errors::WRONG_PARAMS, "Template file not found" );
        return;
    }
    catch( ... ) {
        setError( rpcMessage.getAnswer(), Errors::UNEXPECTED_ERROR );
        return;
    }
}

void UiProto::setHubCoords( rpc::Message& rpcMessage ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
    JsonObject& params = rpcMessage.getParams();

    JsonObject::const_iterator latIt = params.find( CoreDRemoteInterface::SettingsFields::LAT_NAME );
    JsonObject::const_iterator lngIt = params.find( CoreDRemoteInterface::SettingsFields::LNG_NAME );
    if( latIt != params.end() && lngIt != params.end() ) {
        try {
            float lat = latIt->get< float >();
            float lng = lngIt->get< float >();
            const auto result = coreDInterface.setGeoPosition( { lat, lng } );
            rpcMessage.setResult( result );
        }
        catch( const std::exception& ex ) {
            rpcMessage.setError( Errors::INTERNAL_OPERATION_ERROR );
        }
    }
    else {
        rpcMessage.setError( Errors::WRONG_PARAMS );
    }
}

void UiProto::getHubCoords( rpc::Message& rpcMessage ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
    try {
        CoreDRemoteInterface::GeoPosition pos = coreDInterface.getGeoPosition().first;
        rpcMessage.addToResult( CoreDRemoteInterface::SettingsFields::LAT_NAME, pos.latitude );
        rpcMessage.addToResult( CoreDRemoteInterface::SettingsFields::LNG_NAME, pos.longitude );
    }
    catch( const std::exception& ex ) {
        Log::error( "UiProto::GetHubCoords, error :" + std::string( ex.what() ) );
        rpcMessage.setError( Errors::BAD_PARAMS );
    }
}

void UiProto::setHubTimeZone( rpc::Message& rpcMessage ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( Service::getInstance() );
    const JsonObject& joParams = rpcMessage.getParams();
    auto result = coreDInterface.setHubTimeZone( joParams );
    auto& answer = rpcMessage.getAnswer();
    setErrorIfHas( answer, result );
}

void UiProto::getHubTimeZone( rpc::Message& rpcMessage ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( Service::getInstance() );
    // TODO: uncomment argument and create task to add errors to documentation of "hub.time.location.get" method
    std::string timeZone = coreDInterface.getHubTimeZone( /*&rpcMessage*/ );
    JsonObject result = { { HubPlatform::Uid::FieldNames::TimeLocationFieldName, timeZone } };
    rpcMessage.setResult( result );
}

void UiProto::listHubTimeZones( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( Service::getInstance() );
    auto result = coreDInterface.getAllSupportedTimeZones();
    if( hasError( result ) ) {
        auto& answer = rpcMessage.getAnswer();
        setErrorIfHas( answer, result );
        return;
    }
    rpcMessage.setResult( result );
}

void UiProto::addItemDictionaryValue( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
        auto& joAnswer = rpcMessage.getAnswer();
        auto peer = rpcMessage.getNetworkObject();
        coreDInterface.addItemDictionaryValue( rpcMessage.getParams(),
                                               [joAnswer, peer]( const JsonObject& response ) mutable {
                                                   setErrorIfHas( joAnswer, response );
                                                   joAnswer[ "result" ] = std::move( response );
                                                   json_remove( joAnswer[ "result" ], Errors::ERROR_FIELD );
                                                   if( peer ) {
                                                       peer->sendMessage( joAnswer );
                                                   }
                                               } );
        joAnswer = nullptr;
    }
    catch( const CoreDRemoteInterface::DictionaryValueOverflow& e ) {
        setError( rpcMessage.getAnswer(), Errors::INTERNAL_OPERATION_ERROR, e.what() );
    }
    catch( const std::exception& e ) {
        setError( rpcMessage.getAnswer(), Errors::WRONG_PARAMS, e.what() );
    }
}

void UiProto::setItemDictionaryValue( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
        auto& joAnswer = rpcMessage.getAnswer();
        auto peer = rpcMessage.getNetworkObject();
        coreDInterface.setItemDictionaryValue( rpcMessage.getParams(),
                                               [joAnswer, peer]( const JsonObject& response ) mutable {
                                                   setErrorIfHas( joAnswer, response );
                                                   joAnswer[ "result" ] = std::move( response );
                                                   json_remove( joAnswer[ "result" ], Errors::ERROR_FIELD );
                                                   if( peer ) {
                                                       peer->sendMessage( joAnswer );
                                                   }
                                               } );
        joAnswer = nullptr;
    }
    catch( const MaxMessageLengthError& e ) {
        setError( rpcMessage.getAnswer(), Errors::BUS_MESSAGE_TOO_LONG, e.what() );
    }
    catch( const std::exception& e ) {
        setError( rpcMessage.getAnswer(), Errors::WRONG_PARAMS, e.what() );
    }
}

void UiProto::removeItemDictionaryValue( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
        auto& joAnswer = rpcMessage.getAnswer();
        auto peer = rpcMessage.getNetworkObject();
        coreDInterface.removeItemDictionaryValue( rpcMessage.getParams(),
                                                  [joAnswer, peer]( const JsonObject& response ) mutable {
                                                      setErrorIfHas( joAnswer, response );
                                                      joAnswer[ "result" ] = std::move( response );
                                                      json_remove( joAnswer[ "result" ], Errors::ERROR_FIELD );
                                                      if( peer ) {
                                                          peer->sendMessage( joAnswer );
                                                      }
                                                  } );
        joAnswer = nullptr;
    }
    catch( const MaxMessageLengthError& e ) {
        setError( rpcMessage.getAnswer(), Errors::BUS_MESSAGE_TOO_LONG, e.what() );
    }
    catch( const std::exception& e ) {
        setError( rpcMessage.getAnswer(), Errors::WRONG_PARAMS, e.what() );
    }
}

void UiProto::clearItemDictionary( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
        auto& joAnswer = rpcMessage.getAnswer();
        auto peer = rpcMessage.getNetworkObject();
        coreDInterface.clearItemDictionary( rpcMessage.getParams(),
                                            [joAnswer, peer]( const JsonObject& response ) mutable {
                                                setErrorIfHas( joAnswer, response );
                                                joAnswer[ "result" ] = std::move( response );
                                                json_remove( joAnswer[ "result" ], Errors::ERROR_FIELD );
                                                if( peer ) {
                                                    peer->sendMessage( joAnswer );
                                                }
                                            } );
        joAnswer = nullptr;
    }
    catch( const std::exception& e ) {
        setError( rpcMessage.getAnswer(), Errors::WRONG_PARAMS, e.what() );
    }
}

void UiProto::removeDeviceForce( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;

    const auto deviceId = json_get_str( rpcMessage.getParams(), "_id" );
    LOGGER_TRACE( "force-remove a device with id: " + deviceId );
    try {
        if( deviceId.empty() ) {
            setError( rpcMessage.getAnswer(), Errors::ID_IS_EMPTY, "Device Id is empty" );
            return;
        }

        CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );

        using namespace CoreD::Model::Resources::DeviceFields;
        auto query = JSON( IdFieldName << deviceId );
        auto rawDevices = coreDInterface.getDevicesBy( query );
        if( rawDevices.empty() ) {
            setError( rpcMessage.getAnswer(), Errors::NO_DEVICE_FOUND, "ezlo.device.notfound" );
            return;
        }
        else {
            const auto& device = rawDevices.at( 0 );
            if( device.count( "persistent" ) > 0 && true == json_get_bool( device, "persistent", false ) ) {
                setError( rpcMessage.getAnswer(), Errors::DEVICE_IS_PERSISTENT, "ezlo.device.persistent" );
                return;
            }
        }

        coreDInterface.removeDeviceForce( deviceId );
    }
    catch( const MaxMessageLengthError& ex ) {
        setError( rpcMessage.getAnswer(), Errors::BUS_MESSAGE_TOO_LONG, ex.what() );
    }
    catch( const std::exception& e ) {
        setError( rpcMessage.getAnswer(), Errors::WRONG_PARAMS, e.what() );
    }
}

void UiProto::processBroadcast( const Services::Message& message ) {
    auto removeFavoriteItem = []( const std::string& elementName, const JsonObject& message ) {
        auto elementId = json_get_str( message, "_id" );
        if( elementId.empty() ) {
            Log::error( "processBroadcast: Empty id for element: " + elementName );
            return;
        }
        Db::update( HaFavoriteDBName, JSON( "record" << 1 ), JSON( "$pull" << JSON( elementName << elementId ) ) );
    };
    switch( message.msgSubClass ) {
    case HUB_DEVICE_REMOVED:
        removeFavoriteItem( "devices", message.data );
        break;
    case HUB_ITEM_REMOVED:
        removeFavoriteItem( "items", message.data );
        break;
    case SCENE_DELETED:
        removeFavoriteItem( "rules", message.data );
        break;
    }
}

bool UiProto::invokeHandler( const std::string& method, rpc::Message& message ) {
    using RequestHandler = void ( UiProto::* )( rpc::Message& );
    using RequestHandlersMap = std::unordered_map< std::string, RequestHandler >;
    static const RequestHandlersMap handlers = { { "getWhatHappened", &UiProto::getWhatHappened },
                                                 { "setPreferences", &UiProto::setPreferences },
                                                 { "getPreferences", &UiProto::getPreferences },
                                                 { "rebootEzlo", &UiProto::rebootEzlo },
                                                 { "getPowerStatus", &UiProto::getPowerStatus },

                                                 // TODO: remove it when it will be stabilized
                                                 { "hub.device.firmware.update", &UiProto::updateDeviceFirmware },

                                                 { "hub.status.get", &UiProto::getHubStatus },
                                                 { "hub.device.template.get", &UiProto::getTemplateForDevice },
                                                 { "hub.device.force_remove", &UiProto::removeDeviceForce },
                                                 { "hub.coordinates.set", &UiProto::setHubCoords },
                                                 { "hub.coordinates.get", &UiProto::getHubCoords },
                                                 { "hub.time.location.set", &UiProto::setHubTimeZone },
                                                 { "hub.time.location.get", &UiProto::getHubTimeZone },
                                                 { "hub.time.location.list", &UiProto::listHubTimeZones },
                                                 { "hub.reset", &UiProto::hubReset },
                                                 { "hub.bootloader.update", &UiProto::hubBootloaderUpdate },
                                                 { "hub.recovery_partititon.update",
                                                   &UiProto::hubRecoveryPartitionUpdate },
                                                 { "hub.custom.nodes.list", &UiProto::handleHubCustomNodesList } };

    const auto it = handlers.find( method );
    if( it != handlers.end() ) {
        const auto handler = it->second;
        ( this->*handler )( message );
        return true;
    }
    else {
        return Proto::invokeHandler( method, message );
    }
}

void UiProto::invokeExtensionsHandler( const std::string& method, rpc::Message& rpcMessage ) {
    HubExtensionsCommandsHandler( getService() ).invoke( method, rpcMessage );
}

// NOTE: legacy, do not use
void UiProto::invokeHubFirmwareHandler( const std::string& method, rpc::Message& rpcMessage ) {
    HubFirmwareCommandsHandler( getService() ).invoke( method, rpcMessage );
}

void UiProto::invokeHubSshTunnelHandler( const std::string& method, rpc::Message& rpcMessage ) {
    HubSshTunnelCommandsHandler( getService() ).invoke( method, rpcMessage );
}

void UiProto::hubReset( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;
    using namespace HubPlatform::UpdateManager;

    const JsonObject& params = rpcMessage.getParams();
    HubResetType resetType = hubResetTypeFromJson( params );

    UpdateManager::RemoteInterface updateManager( getService() );
    updateManager.executeHubReset( resetType );
}

void UiProto::hubBootloaderUpdate( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;
    using namespace HubPlatform::UpdateManager;

#if defined( __HUB_TYPE_H2__ ) || defined( __HUB_TYPE_H2_SECURE__ ) || defined( __HUB_TYPE_BEEWAVE__ )
    HardwareManager::RemoteInterface hardwareManager( getService() );
    auto battery = hardwareManager.getBatteryInfo();
    if( battery && ( ( battery->isDischarging() && battery->isLow() ) || battery->depleted ) ) {
        JsonObject& joAnswer = rpcMessage.getAnswer();
        setError( joAnswer, Errors::BATTERY_LOW );
        return;
    }

    UpdateManager::RemoteInterface updateManager( getService() );
    updateManager.executeBootloaderUpdate();
#else
    JsonObject& joAnswer = rpcMessage.getAnswer();
    setError( joAnswer, Errors::UNKNOWN_METHOD );
#endif
}

/* it's a dangerous operation, only an end user can make a decision about performing such an operation */
void UiProto::hubRecoveryPartitionUpdate( rpc::Message& rpcMessage ) {
#if defined( __HUB_TYPE_H2__ ) || defined( __HUB_TYPE_H2_SECURE__ )
    HardwareManager::RemoteInterface hardwareManager( getService() );
    auto battery = hardwareManager.getBatteryInfo();
    if( battery && ( ( battery->isDischarging() && battery->isLow() ) || battery->depleted ) ) {
        JsonObject& joAnswer = rpcMessage.getAnswer();
        setError( joAnswer, Errors::BATTERY_LOW );
        return;
    }

    UpdateManager::RemoteInterface updateManager( getService() );
    updateManager.executeRecoveryPartitionUpdate();

#else
    JsonObject& joAnswer = rpcMessage.getAnswer();
    setError( joAnswer, Errors::UNKNOWN_METHOD );
#endif
}

void UiProto::invokeDeviceAddingWizardHandler( const std::string& deviceWizardMethod, rpc::Message& rpcMessage ) {
    using namespace HubPlatform;
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
    coreDInterface.processCmd( deviceWizardMethod, rpcMessage );
}

bool UiProto::invokeHubHandler( const std::string& method, rpc::Message& rpcMessage ) {
    using RequestHandler = void ( UiProto::* )( const std::string&, rpc::Message& );
    using RequestHandlersMap = std::unordered_map< std::string, RequestHandler >;

    auto submethodPos = method.find( '.' );
    if( submethodPos != std::string::npos ) {
        const auto methodId = method.substr( 0, submethodPos );
        const auto submethodId = method.substr( submethodPos + 1 );

        static const RequestHandlersMap handlers = {
            { "scenes", &UiProto::invokeScenesHandler },         { "modes", &UiProto::invokeHouseModeHandler },
            { "extensions", &UiProto::invokeExtensionsHandler }, { "ssh_tunnel", &UiProto::invokeHubSshTunnelHandler },
            { "firmware", &UiProto::invokeHubFirmwareHandler },
        };

        const auto it = handlers.find( methodId );
        if( it != handlers.end() ) {
            const auto handler = it->second;
            ( this->*handler )( submethodId, rpcMessage );
            return true;
        }
        else {
            // represent as the leaf-nodes !!!
            using ConcreteHandler = void ( UiProto::* )( rpc::Message& );
            using SimpleHandlersMap = std::unordered_map< std::string, ConcreteHandler >;

            static const SimpleHandlersMap leafHandlers = {
                { "gateways.list", &UiProto::getGateways },
                { "gateway.settings.list", &UiProto::listGatewaySettings },
                { "gateway.setting.value.set", &UiProto::setGatewaySettingValue },
                { "device.name.set", &UiProto::renameDevice },
                { "device.room.set", &UiProto::placeDevice },
                { "device.armed.set", &UiProto::setDeviceArmed },
                { "device.protect_config.set", &UiProto::setDeviceProtectConfig },
                { "item.dictionary.value.add", &UiProto::addItemDictionaryValue },
                { "item.dictionary.value.set", &UiProto::setItemDictionaryValue },
                { "item.dictionary.value.remove", &UiProto::removeItemDictionaryValue },
                { "item.dictionary.clear", &UiProto::clearItemDictionary },
                { "room.create", &UiProto::createRoom },
                { "room.name.set", &UiProto::setRoomName },
                { "room.order.set", &UiProto::setRoomsOrder },
                { "room.get", &UiProto::getRoom },
                { "room.delete", &UiProto::deleteRoom },
                { "room.all.delete", &UiProto::deleteAllRooms },
                { "favorite.set", &UiProto::foSetFavorite },
                { "favorite.list", &UiProto::foGetFavorites }
            };

            // represent the tag.name as the leaf node
            const auto sIt = leafHandlers.find( method );
            if( sIt != leafHandlers.end() ) {
                const auto handler = sIt->second;
                ( this->*handler )( rpcMessage );
                return true;
            }
        }
    }
    return false;
}

void UiProto::invokeScenesHandler( const std::string& method, rpc::Message& rpcMessage ) {
    using SimpleHandler = void ( UiProto::* )( rpc::Message& );
    using SimpleHandlersMap = std::unordered_map< std::string, SimpleHandler >;

    static const SimpleHandlersMap handlers = {
        { "get", &UiProto::handleGetScene },
        { "trigger.devices.list", &UiProto::handleTriggerDevicesList },
        { "blocks.list", &UiProto::handleGetBlocks },
        { "time.list", &UiProto::handleGetTimeList },
        { "create", &UiProto::handleCreateScene },
        { "edit", &UiProto::handleEditScene },
        { "clone", &UiProto::handleCloneScene },
        { "delete", &UiProto::handleDeleteScene },
        { "delete.all", &UiProto::handleDeleteAllScenes },
        { "enabled.set", &UiProto::handleEnableScene },
        { "room.set", &UiProto::handleSetRoom },
        { "icon.set", &UiProto::handleSetIcon },
        { "notification.add", &UiProto::handleAddNotification },
        { "notification.remove", &UiProto::handleRemoveNotification },
        { "action.block.test", &UiProto::handleActionBlockTest },
        { "name.set", &UiProto::handleNameSet },
        { "house_modes.set", &UiProto::handleHouseModeSet },
        { "block.enabled.set", &UiProto::handleBlockEnabledSet },
        { "block.status.reset", &UiProto::handleBlockStatusReset },
        { "global.create", &UiProto::handleCreateGlobalScene },
        { "global.delete", &UiProto::handleDeleteGlobalScene },
        { "global.delete.all", &UiProto::handleDeleteAllGlobalScenes },
        { "global.edit", &UiProto::handleEditGlobalScene },
        { "global.enabled.set", &UiProto::handleEnableGlobalScene },
        { "global.name.set", &UiProto::handleNameSet },
        { "global.house_modes.set", &UiProto::handleHouseModeSet },
        { "scripts.add", &UiProto::handleSceneScriptAdd },
        { "scripts.set", &UiProto::handleSceneScriptSet },
        { "scripts.get", &UiProto::handleSceneScriptGet },
        { "scripts.delete", &UiProto::handleSceneScriptDelete },
        { "scripts.list", &UiProto::handleSceneScriptsList },
        { "scripts.run", &UiProto::handleSceneScriptRun },
        { "block.data.list", &UiProto::handleHubScenesBlockDataList },
        { "status.get", &UiProto::handleSceneStatusGet },
        { "meta.set", &UiProto::handleSceneMetaSet },
        { "block.meta.set", &UiProto::handleSceneBlockMetaSet },
    };

    const auto it = handlers.find( method );
    if( it != handlers.end() ) {
        const auto handler = it->second;
        ( this->*handler )( rpcMessage );
        return;
    }

    JsonObject& joAnswer = rpcMessage.getAnswer();
    setError( joAnswer, Errors::UNKNOWN_METHOD );
}

void UiProto::invokeHouseModeHandler( const std::string& houseModeMethod, rpc::Message& rpcMessage ) {
    using namespace std::placeholders;

    auto submethodPos = houseModeMethod.find( '.' );
    const auto methodId = houseModeMethod.substr( 0, submethodPos );
    const auto submethodId = houseModeMethod.substr( submethodPos + 1 );
    if( houseModeMethod.find( "swinger.shutdown." ) != std::string::npos ) {
        HubSwingerShutdownCommandsHandler swingerShutdownCommandsHandler( getService() );
        swingerShutdownCommandsHandler.invoke( houseModeMethod, rpcMessage );
        return;
    }
    else if( houseModeMethod.find( "protect.system.test." ) != std::string::npos ) {
        HubWalkTestCommandsHandler walkTestCommandsHandler( getService() );
        walkTestCommandsHandler.invoke( houseModeMethod, rpcMessage );
        return;
    }

    using LeafHandler = std::function< void( UiProto&, rpc::Message& ) >;
    using CoredMethodsMap = std::unordered_map< std::string, LeafHandler >;

    static const CoredMethodsMap handlers = {
        { "get", []( UiProto& owner, rpc::Message& message ) { owner.invokeHouseModeGetHandler( message ); } },
        { "current.get",
          []( UiProto& owner, rpc::Message& message ) { owner.invokeHouseModeGetCurrentHandler( message ); } },
        { "set", []( UiProto& owner, rpc::Message& message ) { owner.invokeHouseModeSet( message ); } },
        { "switch", []( UiProto& owner, rpc::Message& message ) { owner.invokeHouseModeSet( message ); } },
        { "cancel_switch",
          []( UiProto& owner, rpc::Message& message ) { owner.invokeHouseModeCancelSwitching( message ); } },
        { "entry_delay.cancel",
          []( UiProto& owner, rpc::Message& message ) { owner.invokeHouseModeCancelEntryDelay( message ); } },
        { "entry_delay.skip",
          []( UiProto& owner, rpc::Message& message ) { owner.invokeHouseModeSkipEntryDelay( message ); } },
        { "entry_delay.set",
          []( UiProto& owner, rpc::Message& message ) { owner.invokeHouseModeSetEntryDelay( message ); } },
        { "entry_delay.reset",
          []( UiProto& owner, rpc::Message& message ) { owner.invokeHouseModeEntryDelayReset( message ); } },
        { "switch_to_delay.set",
          []( UiProto& owner, rpc::Message& message ) { owner.invokeHouseModeSetSwitchDelay( message ); } },
        { "alarm_delay.set",
          []( UiProto& owner, rpc::Message& message ) { owner.invokeHouseModeSetAlarmDelay( message ); } },
        { "disarmed_default.set",
          []( UiProto& owner, rpc::Message& message ) { owner.invokeSetDisarmedDefaultForHouseMode( message ); } },
        { "protect.set", []( UiProto& owner, rpc::Message& message ) { owner.invokeHouseModeProtectSet( message ); } },
        { "protect.buttons.set",
          []( UiProto& owner, rpc::Message& message ) { owner.invokeHouseModeProtectButtonsSet( message ); } },
        { "protect.devices.add",
          []( UiProto& owner, rpc::Message& message ) { owner.invokeHouseModeProtectDevicesAdd( message ); } },
        { "protect.devices.remove",
          []( UiProto& owner, rpc::Message& message ) { owner.invokeHouseModeProtectDevicesRemove( message ); } },
        { "disarmed_devices.add",
          []( UiProto& owner, rpc::Message& message ) { owner.invokeAddDisarmedDeviceForHouseMode( message ); } },
        { "disarmed_devices.remove",
          []( UiProto& owner, rpc::Message& message ) { owner.invokeRemoveDisarmedDeviceForHouseMode( message ); } },
        { "notifications.set",
          []( UiProto& owner, rpc::Message& message ) { owner.invokeHouseModeSetNotifications( message ); } },
        { "alarms_off.add",
          []( UiProto& owner, rpc::Message& message ) { owner.invokeAddAlarmsOffDeviceForHouseMode( message ); } },
        { "alarms_off.remove",
          []( UiProto& owner, rpc::Message& message ) { owner.invokeRemoveAlarmsOffDeviceForHouseMode( message ); } },
        { "cameras_off.add",
          []( UiProto& owner, rpc::Message& message ) { owner.invokeAddCamerasOffDeviceForHouseMode( message ); } },
        { "cameras_off.remove",
          []( UiProto& owner, rpc::Message& message ) { owner.invokeRemoveCamerasOffDeviceForHouseMode( message ); } },
        { "bypass_devices.add",
          []( UiProto& owner, rpc::Message& message ) { owner.invokeAddBypassDevicesForHouseMode( message ); } },
        { "bypass_devices.remove",
          []( UiProto& owner, rpc::Message& message ) { owner.invokeRemoveBypassDevicesForHouseMode( message ); } },
        { "local.alarm.off", []( UiProto& owner, rpc::Message& message ) { owner.invokeLocalAlarmOff( message ); } },
        { "force_disarm", []( UiProto& owner, rpc::Message& message ) { owner.invokeForceDisarm( message ); } },
    };

    const auto it = handlers.find( houseModeMethod );
    if( it != handlers.end() ) {
        const auto handler = it->second;
        handler( *this, rpcMessage );
        return;
    }

    setError( rpcMessage.getAnswer(), Errors::UNKNOWN_METHOD );
}

std::string UiProto::getCurrentHouseModeImpl() {
    using namespace HubPlatform;

    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    return remoteInterface.getCurrentHouseModeId();
}

std::string UiProto::getHouseModeSwitchToImpl() {
    using namespace HubPlatform;

    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    return remoteInterface.getHouseModeIdSwitchTo();
}

namespace {
void purgeProtectFields( Json& response ) {
    if( json_contains( response, "modes" ) ) {
        auto& modes = response[ "modes" ];

        for( auto& mode : modes ) {
            mode.erase( "protect" );
        }
    }
    static std::initializer_list< const char* const > protectFields = { "protectButtons", "protectDevices" };
    for( const auto& field : protectFields ) {
        if( json_contains( response, field ) )
            response.erase( field );
    }
}
}  // namespace

void UiProto::invokeHouseModeGetHandler( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;

    auto responseBuilder = ResponseBuildersFactory::createHubModesGetBuilder( rpcMessage );

    CoreDRemoteInterface::CoreDRemoteInterface cored( getService() );
    responseBuilder->setSettings( cored.getCurrentHouseModeId(),
                                  cored.getHouseModeIdSwitchTo(),
                                  cored.getSwitchDelayLeft(),
                                  cored.getSwitchDelay(),
                                  cored.getAlarmDelay(),
                                  cored.getEntryDelay(),
                                  cored.getAbortWindow(),
                                  cored.getArmedDeviceIDsAffectedByHouseModes(),
                                  cored.getAlarmDeviceIDsAffectedByHouseModes(),
                                  cored.getCameraDeviceIDs(),
                                  cored.getHouseModesProtectButtons(),
                                  cored.getHouseModesProtectDevicesIDs() );

    auto houseModes = cored.getHouseModes();
    for( auto& houseMode : houseModes ) {
        responseBuilder->addHouseMode( houseMode.m_id,
                                       houseMode.m_name,
                                       houseMode.m_description,
                                       houseMode.m_ignoreDelay,
                                       houseMode.m_disarmedDefault,
                                       houseMode.m_protect,
                                       houseMode.m_armed,
                                       houseMode.m_notificationUserIds,
                                       houseMode.m_disarmedDeviceIds,
                                       houseMode.m_alarmsOffDeviceIds,
                                       houseMode.m_camerasOffDeviceIds,
                                       houseMode.m_bypassDeviceIds );
    }

    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );

    auto response = responseBuilder->buildResponse( getService().getDBClient() );

    if( !remoteInterface.checkFeatureStatus( "protect" ) ) {
        purgeProtectFields( response[ "result" ] );
    }

    rpcMessage.getAnswer()[ "result" ] = std::move( response[ "result" ] );
    rpcMessage.getAnswer()[ "api" ] = std::move( response[ "api" ] );
}

void UiProto::invokeHouseModeGetCurrentHandler( rpc::Message& rpcMessage ) {
    auto& result = rpcMessage.getAnswer()[ "result" ];
    result[ "modeId" ] = getCurrentHouseModeImpl();
}

void UiProto::invokeHouseModeSet( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;
    using HouseModesGetResult = HubPlatform::CoreDRemoteInterface::CoreDInterface::HouseModesGetResult;
    using HouseModeEntry = CoreDHouseModes::HouseModeEntry;

    auto houseModeId = json_get_str( rpcMessage.getParams(), "modeId" );

    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    auto currentHouseModeId = remoteInterface.getCurrentHouseModeId();
    auto& answer = rpcMessage.getAnswer();
    if( houseModeId == currentHouseModeId ) {
        setError( answer, Errors::WRONG_PARAMS );
        return;
    }

    auto findMatchingModes = [&answer]( HouseModesGetResult& houseModes,
                                        std::string const& houseModeName,
                                        std::string& houseModeId ) -> bool {
        auto matchByName = [&houseModeName]( HouseModeEntry const& entry ) -> bool {
            return entry.m_name == houseModeName;
        };

        // divide by name
        auto secondPartIt = std::partition( begin( houseModes ), end( houseModes ), matchByName );

        // find intervals with equal names
        ptrdiff_t foundByName = std::distance( begin( houseModes ), secondPartIt );
        if( foundByName == 1 ) {
            // found exactly one item. Get its Id and set mode to it
            houseModeId = begin( houseModes )->m_id;
            return true;
        }
        else if( foundByName > 1 ) {
            setError( answer, Errors::HOUSE_MODES_AMBIGUOUS_NAME );
        }
        else if( foundByName == 0 ) {
            setError( answer, Errors::HOUSE_MODE_NAME_DOES_NOT_EXIST );
        }
        return false;
    };

    std::string houseModeName = json_get_str( rpcMessage.getParams(), "name" );
    HouseModesGetResult houseModes = remoteInterface.getHouseModes();
    if( houseModeId.empty() && !houseModeName.empty() ) {
        if( !houseModes.empty() ) {
            std::for_each( begin( houseModes ), end( houseModes ), [&]( HouseModeEntry& houseModeEntry ) {
                tiny::tools::to_lower( houseModeEntry.m_name );
            } );
            tiny::tools::to_lower( houseModeName );

            if( !findMatchingModes( houseModes, houseModeName, houseModeId ) ) {
                // Errors already set up.
                return;
            }
        }
        // continue to search by house mode id.
    }
    else if( !houseModeId.empty() ) {
        // check if this id is valid
        auto foundItem = std::find_if( begin( houseModes ), end( houseModes ), [&]( HouseModeEntry const& item ) {
            return item.m_id == houseModeId;
        } );
        if( foundItem == end( houseModes ) ) {
            setError( answer, Errors::HOUSE_MODE_DOES_NOT_EXIST );
            return;
        }
    }

    auto setHouseModeResult = remoteInterface.setHouseMode( houseModeId );
    if( hasError( setHouseModeResult ) ) {
        answer[ "error" ] = setHouseModeResult[ "error" ];
        answer[ "result" ] = JSON_EMPTY_OBJ;
    }
    else
        answer[ "result" ] = setHouseModeResult[ "result" ];
}

void UiProto::invokeHouseModeCancelSwitching( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;

    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    JsonObject& result = rpcMessage.getAnswer()[ "result" ];
    result[ "modeId" ] = remoteInterface.cancelHouseModeSwitching();
}

void UiProto::invokeHouseModeCancelEntryDelay( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;

    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );

    auto result = remoteInterface.cancelEntryDelay();
    if( hasError( result ) ) {
        auto& answer = rpcMessage.getAnswer();
        setErrorIfHas( answer, result );
        return;
    }

    rpcMessage.setResult( result );
}

void UiProto::invokeHouseModeSkipEntryDelay( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;

    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );

    auto result = remoteInterface.skipEntryDelay();
    if( hasError( result ) ) {
        auto& answer = rpcMessage.getAnswer();
        setErrorIfHas( answer, result );
        return;
    }

    rpcMessage.setResult( result );
}

void UiProto::invokeHouseModeSetEntryDelay( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;

    CoreDRemoteInterface::CoreDInterfaceUPtr remoteInterface = CoreDRemoteInterface::makeInstance( getService() );

    auto result = remoteInterface->setEntryDelay( rpcMessage.getParams() );
    if( hasError( result ) ) {
        auto& answer = rpcMessage.getAnswer();
        setErrorIfHas( answer, result );
        return;
    }

    rpcMessage.setResult( result );
}

void UiProto::invokeHouseModeEntryDelayReset( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;

    CoreDRemoteInterface::CoreDInterfaceUPtr remoteInterface = CoreDRemoteInterface::makeInstance( getService() );

    auto result = remoteInterface->resetEntryDelay();
    if( hasError( result ) ) {
        auto& answer = rpcMessage.getAnswer();
        setErrorIfHas( answer, result );
        return;
    }

    rpcMessage.setResult( result );
}

void UiProto::invokeHouseModeSetSwitchDelay( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;

    int delay = 0;
    if( json_get( rpcMessage.getParams(), "switchToDelay", delay ) ) {
        if( delay >= 0 ) {
            CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
            remoteInterface.setSwitchDelay( delay );
            JsonObject& answer = rpcMessage.getAnswer();
            answer[ "result" ] = JSON_EMPTY_OBJ;
        }
        else {
            setError( rpcMessage.getAnswer(), Errors::WRONG_PARAMS, "switchToDelay field has wrong value" );
        }
    }
    else {
        setError( rpcMessage.getAnswer(), Errors::BAD_PARAMS, "switchToDelay field is missing" );
    }
}

void UiProto::invokeHouseModeSetAlarmDelay( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;

    int delay = 0;
    if( json_get( rpcMessage.getParams(), "alarmDelay", delay ) ) {
        if( delay >= 0 ) {
            CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
            remoteInterface.setAlarmDelay( delay );
            JsonObject& answer = rpcMessage.getAnswer();
            answer[ "result" ] = JSON_EMPTY_OBJ;
        }
        else {
            setError( rpcMessage.getAnswer(), Errors::WRONG_PARAMS, "alarmDelay field has wrong value" );
        }
    }
    else {
        setError( rpcMessage.getAnswer(), Errors::BAD_PARAMS, "alarmDelay field is missing" );
    }
}

void UiProto::invokeSetDisarmedDefaultForHouseMode( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;

    auto houseModeId = json_get_str( rpcMessage.getParams(), "modeId" );
    auto disarmedDefault = json_get_bool( rpcMessage.getParams(), "disarmedDefault" );

    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    auto result = remoteInterface.setDisarmedDefault( houseModeId, disarmedDefault );
    JsonObject& answer = rpcMessage.getAnswer();
    answer[ "result" ] = JSON_EMPTY_OBJ;
    setErrorIfHas( answer, result );
}

void UiProto::invokeHouseModeProtectSet( rpc::Message& rpcMessage ) {
    auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );

    if( !remoteInterface.checkFeatureStatus( "protect" ) ) {
        setError( answer, Errors::FIRMWARE_FEATURE_DISABLED );
        return;
    }

    std::string modeId;
    if( !getField( params, "modeId", answer, modeId ) ) {
        return;
    }

    bool protect;
    if( !getField( params, "protect", answer, protect ) ) {
        return;
    }

    if( !remoteInterface.getHouseModeById( modeId ) ) {
        setError( answer, Errors::HOUSE_MODE_DOES_NOT_EXIST );
        return;
    }

    auto response = remoteInterface.houseModeProtectSet( modeId, protect );
    setErrorIfHas( answer, response );
    answer[ "result" ] = std::move( response[ "result" ] );
}

void UiProto::invokeHouseModeProtectButtonsSet( rpc::Message& rpcMessage ) {
    auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );

    if( !remoteInterface.checkFeatureStatus( "protect" ) ) {
        setError( answer, Errors::FIRMWARE_FEATURE_DISABLED );
        return;
    }

    std::string deviceId;
    if( !getField( params, "deviceId", answer, deviceId ) ) {
        return;
    }
    auto serviceIt = params.find( "service" );
    if( serviceIt != params.end() && !serviceIt->is_string() ) {
        JsonObject error = JSON_EMPTY_OBJ;
        Errors::getWrongParams( "service", error );
        setErrorObject( answer, error, "Format of a required field is wrong" );
        return;
    }

    const std::string& protectService = json_get_str( *serviceIt );
    auto protectServiceEnum = !protectService.empty() ? CoreDHouseModes::toProtectService( protectService ) :
                                                        CoreDHouseModes::ProtectServiceEnum::NONE;

    if( !protectService.empty() && CoreDHouseModes::ProtectServiceEnum::NONE == protectServiceEnum ) {
        LOGGER_DEBUG( cc::debug( "protectService is bad" ) );
        JsonObject error = JSON_EMPTY_OBJ;
        Errors::getWrongParams( "service", error );
        setErrorObject( answer, error, "Invalid name of a protect service" );
        return;
    }

    LOGGER_DEBUG( "Call to remoteInterface.houseModesProtectButtonsSet" );
    auto response = remoteInterface.houseModesProtectButtonsSet( deviceId, protectServiceEnum );
    setErrorIfHas( answer, response );
    answer[ "result" ] = std::move( response[ "result" ] );
}

namespace {
std::tuple< bool, std::vector< std::string >, JsonObject > readDeviceIdsParam( const JsonObject& params ) {
    JsonObject result;
    std::vector< std::string > deviceIds;
    auto deviceIdsIt = params.find( "deviceIds" );
    if( deviceIdsIt == params.end() || deviceIdsIt->empty() ) {
        JsonObject error = JSON_EMPTY_OBJ;
        Errors::getBadRequest( "deviceIds", error );
        setErrorObject( result, error );
        return std::make_tuple( false, deviceIds, result );
    }

    try {
        jsonCheckArray( params, "deviceIds", []( const JsonArray& arr ) {
            for( const auto& item : arr )
                jsonCheckString( item.get< std::string >(), JsonStringChecks::NON_EMPTY );
        } );

        fromJsonStringArray( *deviceIdsIt, [&deviceIds]( const std::string& item ) { deviceIds.push_back( item ); } );
        return std::make_tuple( true, deviceIds, result );
    }
    catch( const std::exception& ex ) {
        JsonObject error = JSON_EMPTY_OBJ;
        Errors::getWrongParams( "deviceIds", error );
        setErrorObject( result, error, "Format of a required field is wrong" );
        return std::make_tuple( false, deviceIds, result );
    }
}
}  // namespace

void UiProto::invokeHouseModeProtectDevicesAdd( rpc::Message& rpcMessage ) {
    auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );

    if( !remoteInterface.checkFeatureStatus( "protect" ) ) {
        setError( answer, Errors::FIRMWARE_FEATURE_DISABLED );
        return;
    }

    bool isGood;
    std::vector< std::string > deviceIds;
    JsonObject result;
    std::tie( isGood, deviceIds, result ) = readDeviceIdsParam( params );

    if( isGood ) {
        try {
            LOGGER_DEBUG_RAW( "Call to remoteInterface.houseModesProtectDevicesAdd" );
            auto response = remoteInterface.houseModesProtectDevicesAdd( deviceIds );
            setErrorIfHas( answer, response );
            answer[ "result" ] = std::move( response[ "result" ] );
        }
        catch( const std::exception& ex ) {
            LOGGER_ERROR_RAW( cc::error( "Failed to add protectDevices: " ) + cc::note( std::string( ex.what() ) ) );
            setError( answer, Errors::INTERNAL_OPERATION_ERROR );
        }
    }
    else {
        setErrorIfHas( answer, result );
        answer[ "result" ] = JSON_EMPTY_OBJ;
    }
}

void UiProto::invokeHouseModeProtectDevicesRemove( rpc::Message& rpcMessage ) {
    auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );

    if( !remoteInterface.checkFeatureStatus( "protect" ) ) {
        setError( answer, Errors::FIRMWARE_FEATURE_DISABLED );
        return;
    }

    bool isGood;
    std::vector< std::string > deviceIds;
    JsonObject result;
    std::tie( isGood, deviceIds, result ) = readDeviceIdsParam( params );

    if( isGood ) {
        try {
            LOGGER_DEBUG_RAW( "Call to remoteInterface.houseModesProtectDevicesRemove" );
            auto response = remoteInterface.houseModesProtectDevicesRemove( deviceIds );
            setErrorIfHas( answer, response );
            answer[ "result" ] = std::move( response[ "result" ] );
        }
        catch( const std::exception& ex ) {
            LOGGER_ERROR_RAW( cc::error( "Failed to remove protectDevices: " ) + cc::note( std::string( ex.what() ) ) );
            setError( answer, Errors::INTERNAL_OPERATION_ERROR );
        }
    }
    else {
        setErrorIfHas( answer, result );
        answer[ "result" ] = JSON_EMPTY_OBJ;
    }
}

void UiProto::invokeAddDisarmedDeviceForHouseMode( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;

    auto houseModeId = json_get_str( rpcMessage.getParams(), "modeId" );
    auto deviceId = json_get_str( rpcMessage.getParams(), "deviceId" );

    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    auto result = remoteInterface.addDisarmedDevice( houseModeId, deviceId );
    JsonObject& answer = rpcMessage.getAnswer();
    answer[ "result" ] = JSON_EMPTY_OBJ;
    setErrorIfHas( answer, result );
}

void UiProto::invokeRemoveDisarmedDeviceForHouseMode( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;

    auto houseModeId = json_get_str( rpcMessage.getParams(), "modeId" );
    auto deviceId = json_get_str( rpcMessage.getParams(), "deviceId" );

    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    auto result = remoteInterface.removeDisarmedDevice( houseModeId, deviceId );
    JsonObject& answer = rpcMessage.getAnswer();
    answer[ "result" ] = JSON_EMPTY_OBJ;
    setErrorIfHas( answer, result );
}

void UiProto::invokeAddAlarmsOffDeviceForHouseMode( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;

    auto houseModeId = json_get_str( rpcMessage.getParams(), "modeId" );
    auto deviceId = json_get_str( rpcMessage.getParams(), "deviceId" );

    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    Json result = remoteInterface.addAlarmsOffDevice( houseModeId, deviceId );
    JsonObject& answer = rpcMessage.getAnswer();
    answer[ "result" ] = JSON_EMPTY_OBJ;
    setErrorIfHas( answer, result );
}

void UiProto::invokeRemoveAlarmsOffDeviceForHouseMode( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;

    auto houseModeId = json_get_str( rpcMessage.getParams(), "modeId" );
    auto deviceId = json_get_str( rpcMessage.getParams(), "deviceId" );

    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    Json result = remoteInterface.removeAlarmsOffDevice( houseModeId, deviceId );
    JsonObject& answer = rpcMessage.getAnswer();
    answer[ "result" ] = JSON_EMPTY_OBJ;
    setErrorIfHas( answer, result );
}

void UiProto::invokeAddCamerasOffDeviceForHouseMode( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;

    auto houseModeId = json_get_str( rpcMessage.getParams(), "modeId" );
    auto deviceId = json_get_str( rpcMessage.getParams(), "deviceId" );

    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    Json result = remoteInterface.addCamerasOffDevice( houseModeId, deviceId );
    JsonObject& answer = rpcMessage.getAnswer();
    answer[ "result" ] = JSON_EMPTY_OBJ;
    setErrorIfHas( answer, result );
}

void UiProto::invokeRemoveCamerasOffDeviceForHouseMode( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;

    auto houseModeId = json_get_str( rpcMessage.getParams(), "modeId" );
    auto deviceId = json_get_str( rpcMessage.getParams(), "deviceId" );

    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    Json result = remoteInterface.removeCamerasOffDevice( houseModeId, deviceId );
    JsonObject& answer = rpcMessage.getAnswer();
    answer[ "result" ] = JSON_EMPTY_OBJ;
    setErrorIfHas( answer, result );
}

void UiProto::invokeAddBypassDevicesForHouseMode( rpc::Message& rpcMessage ) {
    bool isGood;
    std::vector< std::string > deviceIds;
    JsonObject result;
    std::tie( isGood, deviceIds, result ) = readDeviceIdsParam( rpcMessage.getParams() );

    if( isGood ) {
        HubPlatform::CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
        const std::string houseModeId = json_get_str( rpcMessage.getParams(), "modeId" );
        result = remoteInterface.addBypassDevices( houseModeId, deviceIds );
    }

    Json& answer = rpcMessage.getAnswer();
    answer[ "result" ] = JSON_EMPTY_OBJ;
    setErrorIfHas( answer, result );
}

void UiProto::invokeRemoveBypassDevicesForHouseMode( rpc::Message& rpcMessage ) {
    bool isGood;
    std::vector< std::string > deviceIds;
    JsonObject result;
    std::tie( isGood, deviceIds, result ) = readDeviceIdsParam( rpcMessage.getParams() );

    if( isGood ) {
        HubPlatform::CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
        const std::string houseModeId = json_get_str( rpcMessage.getParams(), "modeId" );
        result = remoteInterface.removeBypassDevices( houseModeId, deviceIds );
    }

    Json& answer = rpcMessage.getAnswer();
    answer[ "result" ] = JSON_EMPTY_OBJ;
    setErrorIfHas( answer, result );
}

void UiProto::invokeHouseModeSetNotifications( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;

    auto houseModeId = json_get_str( rpcMessage.getParams(), "modeId" );
    auto notifyAll = json_get_bool( rpcMessage.getParams(), "all" );
    auto userIds = json_get_vec< std::string >( rpcMessage.getParams(), "userIds" );

    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    auto result = remoteInterface.setNotificationsForHouseMode( houseModeId, notifyAll, userIds );
    JsonObject& answer = rpcMessage.getAnswer();
    answer[ "result" ] = JSON_EMPTY_OBJ;
    setErrorIfHas( answer, result );
}

bool UiProto::invokeExtendedHandler( const std::string& method, const std::string& submethod, rpc::Message& message ) {
    using NonLeafHandler = std::function< bool( UiProto&, const std::string&, rpc::Message& ) >;
    using RequestHandlersMap = std::unordered_map< std::string, NonLeafHandler >;

    // TODO: might be implemented as general functional object( leaf & non-leaf node )
    // which could hold the inner map. So it gives tree-base execution by name

    static const RequestHandlersMap handlers = {
        { "extensions",
          []( UiProto& owner, const std::string& method, rpc::Message& message ) -> bool {
              owner.invokeExtensionsHandler( method, message );
              return true;
          } },
        { "devices_wizard",
          []( UiProto& owner, const std::string& method, rpc::Message& message ) -> bool {
              owner.invokeDeviceAddingWizardHandler( method, message );
              return true;
          } },
        { "hub",
          []( UiProto& owner, const std::string& method, rpc::Message& message ) -> bool {
              return owner.invokeHubHandler( method, message );
          } }
    };

    const auto it = handlers.find( method );
    if( it != handlers.end() ) {
        const auto handler = it->second;
        LOGGER_DEBUG_RAW( cc::debug( "Process UI command: " ) + cc::info( method ) + "." + cc::note( submethod ) );
        if( handler( *this, submethod, message ) ) {
            return true;
        }
    }

    return Proto::invokeExtendedHandler( method, submethod, message );
}

void UiProto::handleHubCustomNodesList( rpc::Message& rpcMessage ) {
    CustomNodesList customNodesList( getService() );
    customNodesList.handler( rpcMessage );
}

void UiProto::setWrongParamError( rpc::Message& rpcMessage, const std::string& key ) {
    JsonObject error;
    Errors::getWrongParams( key, error );
    setErrorObject( rpcMessage.getAnswer(), error );
}

void UiProto::handleHubScenesBlockDataList( rpc::Message& rpcMessage ) {
    const auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();

    auto checkBoolParam = [this, &rpcMessage, params]( const std::string& key ) -> bool {
        if( params.find( key ) != params.cend() ) {
            if( !params[ key ].is_boolean() ) {
                setWrongParamError( rpcMessage, key );
                return false;
            }
        }
        return true;
    };

    if( !checkBoolParam( "valueTypes" ) || !checkBoolParam( "scalableValueTypes" ) ||
        !checkBoolParam( "valueScales" ) || !checkBoolParam( "scenesValueTypes" ) ||
        !checkBoolParam( "valueTypeFamilies" ) || !checkBoolParam( "comparisonOperators" ) ||
        !checkBoolParam( "comparisonMethods" ) || !checkBoolParam( "actionMethods" ) ||
        !checkBoolParam( "advancedScenesVersion" ) ) {
        return;
    }

    auto result = getScenesManager().getBlocksDataList( params );

    if( json_get_bool( params, "advancedScenesVersion" ) ) {
        CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
        auto advancedScenesFeature = remoteInterface.findFeature( "advanced_scenes" );
        if( !advancedScenesFeature ) {
            setError( answer, Errors::FIRMWARE_FEATURE_DISABLED );
            return;
        }

        result[ "advancedScenesVersion" ] = JSON( "version" << advancedScenesFeature->getVersion().toString() );
    }

    answer[ "result" ] = result;
}

void UiProto::invokeLocalAlarmOff( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;

    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );

    auto result = remoteInterface.turnOffLocalAlarm( rpcMessage.getParams() );
    if( hasError( result ) ) {
        auto& answer = rpcMessage.getAnswer();
        setErrorIfHas( answer, result );
        return;
    }

    rpcMessage.setResult( result );
}

void UiProto::invokeForceDisarm( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;

    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );

    auto result = remoteInterface.forceDisarm( rpcMessage.getParams() );
    if( hasError( result ) ) {
        auto& answer = rpcMessage.getAnswer();
        setErrorIfHas( answer, result );
        return;
    }

    rpcMessage.setResult( result );
}

}  // namespace Uid
}  // namespace HubPlatform
