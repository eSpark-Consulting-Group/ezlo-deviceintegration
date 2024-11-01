#include "scenes_manager/remote_interface/scenes_remote_interface.h"
#include "service_interfaces/groups_names.h"
#include "service_interfaces/service.h"
#include "database_client/dbqueryparser.h"
#include "database_client/db_tables_names.h"
#include "config_reader/config_reader.h"
#include "scenes_manager/modify_scene/modify_scene.h"
#include "scenes_manager/modify_scene/modify_scene_on_fetch.h"
#include "scenes_manager/scenes_engine/states_registry_resources.h"
#include "hub_settings/hub_settings_reader.h"
#include "cored/model/values/value_type.h"
#include <unordered_set>

namespace HubPlatform {
namespace ScenesManager {

RemoteInterface::RemoteInterface( Services::Service& hostService )
: m_hostService( hostService )
, m_dbClient( hostService.getDBClient() ) {}

BaseRemoteInterface::Scene RemoteInterface::getScene( const SceneId& id ) {
    auto result = m_dbClient.findOne( HaScenesDBName, JSON( db::ID_FIELD_NAME << id ), nullptr );

    std::string defaultScale = temperatureScale();
    auto modifiedResult = modifySceneScales( result, defaultScale );
    if( modifiedResult.first ) {
        return modifiedResult.second;
    }
    return result;
}

BaseRemoteInterface::Scenes RemoteInterface::getScenesList( const JsonObject& params ) {
    auto results = m_dbClient.findAll(
        HaScenesDBName,
        JSON( "$or" << JSON_ARRAY_( JSON( "global" << false ), JSON( "global" << JSON( "$exists" << false ) ) ) ),
        nullptr );

    if( !params.empty() ) {
        JsonObject generator;
        json_get( params, { "filters", "include", "generator" }, generator );
        auto role = json_get_str( generator, "role" );
        if( role != "all" ) {
            results.erase(
                std::remove_if( results.begin(),
                                results.end(),
                                [&]( const JsonObject& scene ) { return scene[ "generator" ][ "role" ] != role; } ),
                results.end() );
        }
    }
    else {
        results.erase( std::remove_if( results.begin(),
                                       results.end(),
                                       [&]( const JsonObject& scene ) {
                                           return scene.find( "generator" ) != scene.end() &&
                                                  scene[ "generator" ][ "role" ] != "user";
                                       } ),
                       results.end() );
    }

    std::string defaultScale = temperatureScale();
    for( auto& scene : results ) {
        auto modifiedResult = modifySceneScales( scene, defaultScale );
        if( modifiedResult.first )
            scene = modifiedResult.second;
    }
    return results;
}

BaseRemoteInterface::Scenes RemoteInterface::getGlobalScenesList( bool onlyId ) {
    if( onlyId ) {
        auto fieldToReturn = JSON( "_id" << onlyId );
        return m_dbClient.findAll( HaScenesDBName, JSON( "global" << true ), &fieldToReturn );
    }
    else {
        auto results = m_dbClient.findAll( HaScenesDBName, JSON( "global" << true ), nullptr );

        std::string defaultScale = temperatureScale();
        for( auto& scene : results ) {
            // TODO: EFWR-2396: implement modification for global scenes using remote controllers interface
            auto modifiedResult = modifySceneScales( scene, defaultScale );
            if( modifiedResult.first )
                scene = modifiedResult.second;
        }
        return results;
    }
}

BaseRemoteInterface::TriggerDeviceIds RemoteInterface::getTriggerDevicesList() {
    const JsonObject fieldsToReturn = JSON( "deviceId" << 1 );
    const auto items = m_dbClient.findAll( HaTempScenesItemsDBName, JSON_EMPTY_OBJ, &fieldsToReturn );
    std::set< std::string > deviceIds;
    for( const auto& item : items ) {
        deviceIds.emplace( json_get_str( item, "deviceId" ) );
    }
    return deviceIds;
}

BaseRemoteInterface::SceneIds RemoteInterface::getSceneIds() {
    const JsonObject fieldsToReturn = JSON( "_id" << 1 );
    const auto scenes = m_dbClient.findAll( HaScenesDBName, JSON_EMPTY_OBJ, &fieldsToReturn );
    SceneIds sceneIds;
    for( const auto& scene : scenes ) {
        sceneIds.emplace_back( json_get_str( scene, "_id" ) );
    }
    return sceneIds;
}

JsonArray RemoteInterface::getTimeList() {
    return m_hostService.sendRequest(
        GroupsNames::SCENE_MANAGER, JSON_EMPTY_OBJ, SCENE_TIME_LIST, std::chrono::seconds( 60 ) );
}

JsonArray RemoteInterface::getBlocksList( const JsonObject& message ) {
    return m_hostService.sendRequest(
        GroupsNames::SCENE_MANAGER, message, SCENE_BLOCKS_LIST, std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::getBlocksDataList( const JsonObject& message ) {
    return m_hostService.sendRequest(
        GroupsNames::SCENE_MANAGER, message, SCENE_BLOCKS_DATA_LIST, std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::createScene( const JsonObject& scene ) {
    return m_hostService.sendRequest( GroupsNames::SCENE_MANAGER, scene, SCENE_CREATE, std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::editScene( const JsonObject& message ) {
    return m_hostService.sendRequest( GroupsNames::SCENE_MANAGER, message, SCENE_EDIT, std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::deleteScene( const BaseRemoteInterface::SceneId& id, bool isGlobal ) {
    return m_hostService.sendRequest( GroupsNames::SCENE_MANAGER,
                                      JSON( "_id" << id << "global" << isGlobal ),
                                      SCENE_DELETE,
                                      std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::deleteAllScenes( bool isGlobal ) {
    return m_hostService.sendRequest(
        GroupsNames::SCENE_MANAGER, JSON( "global" << isGlobal ), SCENE_DELETE_ALL, std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::enableScene( const BaseRemoteInterface::SceneId& id, bool enable, bool isGlobal ) {
    return m_hostService.sendRequest( GroupsNames::SCENE_MANAGER,
                                      JSON( "_id" << id << "enabled" << enable << "global" << isGlobal ),
                                      SCENE_ENABLE,
                                      std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::cloneScene( const BaseRemoteInterface::SceneId& id,
                                        const std::string& name,
                                        bool isGlobal ) {
    return m_hostService.sendRequest( GroupsNames::SCENE_MANAGER,
                                      JSON( "_id" << id << "name" << name << "global" << isGlobal ),
                                      SCENE_CLONE,
                                      std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::runScene( const BaseRemoteInterface::SceneId& id, bool isGlobal ) {
    return m_hostService.sendRequest( GroupsNames::SCENE_MANAGER,
                                      JSON( "sceneId" << id << "global" << isGlobal ),
                                      SCENE_RUN,
                                      std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::runScene( const BaseRemoteInterface::SceneId& id,
                                      bool isGlobal,
                                      const std::string& block ) {
    return m_hostService.sendRequest( GroupsNames::SCENE_MANAGER,
                                      JSON( "sceneId" << id << "global" << isGlobal << "block" << block ),
                                      SCENE_RUN,
                                      std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::runScene( const BaseRemoteInterface::SceneId& id,
                                      bool isGlobal,
                                      const std::string& block,
                                      const std::string& group ) {
    return m_hostService.sendRequest(
        GroupsNames::SCENE_MANAGER,
        JSON( "sceneId" << id << "global" << isGlobal << "block" << block << "group" << group ),
        SCENE_RUN,
        std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::stopScene( const SceneId& id, bool isGlobal, const std::string& block ) {
    return m_hostService.sendRequest( GroupsNames::SCENE_MANAGER,
                                      JSON( "sceneId" << id << "global" << isGlobal << "block" << block ),
                                      SCENE_STOP,
                                      std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::assignRoomToScene( const BaseRemoteInterface::SceneId& id,
                                               const BaseRemoteInterface::RoomId& roomId ) {
    return m_hostService.sendRequest( GroupsNames::SCENE_MANAGER,
                                      JSON( "sceneId" << id << "roomId" << roomId ),
                                      SCENE_ROOM_SET,
                                      std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::assignIconToScene( const BaseRemoteInterface::SceneId& id,
                                               const BaseRemoteInterface::Icon& icon ) {
    return m_hostService.sendRequest(
        GroupsNames::SCENE_MANAGER, JSON( "id" << id << "icon" << icon ), SCENE_ICON_SET, std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::addUserForNotification( const BaseRemoteInterface::SceneId& id,
                                                    const BaseRemoteInterface::UserId& userId ) {
    return m_hostService.sendRequest( GroupsNames::SCENE_MANAGER,
                                      JSON( "sceneId" << id << "userId" << userId ),
                                      SCENE_NOTIFICATION_ADD,
                                      std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::removeUserFromNotifications( const BaseRemoteInterface::SceneId& id,
                                                         const BaseRemoteInterface::UserId& userId ) {
    return m_hostService.sendRequest( GroupsNames::SCENE_MANAGER,
                                      JSON( "sceneId" << id << "userId" << userId ),
                                      SCENE_NOTIFICATION_REMOVE,
                                      std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::setSceneName( const BaseRemoteInterface::SceneId& id,
                                          const BaseRemoteInterface::Name& name ) {
    return m_hostService.sendRequest( GroupsNames::SCENE_MANAGER,
                                      JSON( "sceneId" << id << "name" << name ),
                                      SCENE_NAME_SET,
                                      std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::setMeta( const BaseRemoteInterface::SceneId& id, const BaseRemoteInterface::Meta& meta ) {
    return m_hostService.sendRequest( GroupsNames::SCENE_MANAGER,
                                      JSON( "sceneId" << id << "meta" << meta ),
                                      SCENE_META_SET,
                                      std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::setBlockMeta( const BaseRemoteInterface::SceneId& id,
                                          const BaseRemoteInterface::BlockId& blockId,
                                          const BaseRemoteInterface::Meta& meta ) {
    return m_hostService.sendRequest( GroupsNames::SCENE_MANAGER,
                                      JSON( "sceneId" << id << "blockId" << blockId << "meta" << meta ),
                                      SCENE_BLOCK_META_SET,
                                      std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::setHouseModes( const BaseRemoteInterface::SceneId& id,
                                           const BaseRemoteInterface::HouseModes& houseModes ) {
    return m_hostService.sendRequest( GroupsNames::SCENE_MANAGER,
                                      JSON( "sceneId" << id << "houseModes" << houseModes ),
                                      SCENE_HOUSE_MODES_SET,
                                      std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::setBlockEnabled( const SceneId& id, const BlockId& blockId, bool enabled ) {
    return m_hostService.sendRequest( GroupsNames::SCENE_MANAGER,
                                      JSON( "sceneId" << id << "blockId" << blockId << "enabled" << enabled ),
                                      SCENE_BLOCK_ENABLED_SET,
                                      std::chrono::seconds( 60 ) );
}

bool RemoteInterface::setSceneBlocksSource( const std::string& sourceId ) {
    const auto response =
        m_hostService.sendRequest( GroupsNames::SCENE_MANAGER, JSON( "sourceId" << sourceId ), SCENE_SET_BLOCKS );
    if( hasError( response ) ) {
        LOGGER_TRACE( "Error: " + cc::j( response ) );
        return false;
    }
    return true;
}

bool RemoteInterface::removeSceneBlocksSource( const std::string& sourceId ) {
    const auto response =
        m_hostService.sendRequest( GroupsNames::SCENE_MANAGER, JSON( "sourceId" << sourceId ), SCENE_REMOVE_BLOCKS );
    if( hasError( response ) ) {
        LOGGER_TRACE( "Error: " + cc::j( response ) );
        return false;
    }
    return true;
}

JsonObject RemoteInterface::sceneAllBlocksStatusReset( const SceneId& id ) {
    return m_hostService.sendRequest(
        GroupsNames::SCENE_MANAGER, JSON( "sceneId" << id ), SCENE_BLOCK_STATUS_RESET, std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::sceneBlockStatusReset( const SceneId& id, const BlockId& blockId ) {
    return m_hostService.sendRequest( GroupsNames::SCENE_MANAGER,
                                      JSON( "sceneId" << id << "blockId" << blockId ),
                                      SCENE_BLOCK_STATUS_RESET,
                                      std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::getSceneStatus( const SceneId& id ) {
    return m_hostService.sendRequest(
        GroupsNames::SCENE_MANAGER, JSON( "sceneId" << id ), SCENE_STATUS_GET, std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::getSceneState( const SceneId& id ) {
    namespace DbFields = SceneEngine::StateRegistryDbFields;

    auto record = m_dbClient.findOne( HaTempSceneStatesDBName, JSON( DbFields::Id << id ), nullptr );

    Json result;
    copyFieldIfPresent( record, DbFields::LastChangeTime, result );
    copyFieldIfPresent( record, DbFields::State, result );
    return result;
}

JsonObject RemoteInterface::getBlockState( const SceneId& sceneId, const std::string& blockId ) {
    namespace DbFields = SceneEngine::StateRegistryDbFields;

    auto record = m_dbClient.findOne( HaTempSceneBlockStatesDBName, JSON( DbFields::Id << blockId ), nullptr );

    Json result;
    copyFieldIfPresent( record, DbFields::LastChangeTime, result );
    copyFieldIfPresent( record, DbFields::State, result );
    return result;
}

bool RemoteInterface::modifyScenes( const JsonArray& commands, const JsonArray& blockIds ) {
    const auto response = m_hostService.sendRequest(
        GroupsNames::SCENE_MANAGER, JSON( "commands" << commands << "blockIds" << blockIds ), SCENE_MODIFY_SCENES );
    if( hasError( response ) ) {
        LOGGER_TRACE( "Error: " + cc::j( response ) );
        return false;
    }
    return true;
}

JsonObject RemoteInterface::getExpressionValue( const std::string& name, const JsonObject& options ) {
    auto response = m_hostService.sendRequest(
        GroupsNames::SCENE_MANAGER, JSON( "name" << name << "options" << options ), SCENE_EXPRESSION_VALUE_GET );
    return response;
}

JsonObject RemoteInterface::updateExpression( const std::string& name,
                                              const JsonObject& params,
                                              const std::string& code,
                                              const std::string& valueType,
                                              const JsonObject& metadata ) {
    return m_hostService.sendRequest(
        GroupsNames::SCENE_MANAGER,
        JSON( "name" << name << "params" << params << "code" << code << "valueType" << valueType << "updateExpression"
                     << true << "metadata" << metadata ),
        SCENE_EXPRESSIONS_SET,
        std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::updateExpressionVariable( const std::string& name,
                                                      const JsonObject& value,
                                                      const std::string& valueType,
                                                      const JsonObject& metadata ) {
    return m_hostService.sendRequest( GroupsNames::SCENE_MANAGER,
                                      JSON( "name" << name << "value" << value << "valueType" << valueType << "metadata"
                                                   << metadata << "setVariable" << true ),
                                      SCENE_EXPRESSIONS_SET,
                                      std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::getScenesInfo( const std::string& id ) {
    JsonObject response;
    auto scene = getScene( id );

    std::string name;
    if( !scene.empty() ) {
        const std::string errorMessage = "Wrong format of a parameter";
        if( scene.find( "name" ) != scene.end() ) {
            response[ "name" ] = json_get_str( scene, "name" );
        }

        if( scene.find( "enabled" ) != scene.end() ) {
            response[ "enabled" ] = json_get_bool( scene, "enabled" );
        }

        if( scene.find( "parent_id" ) != scene.end() ) {
            response[ "room_id" ] = json_get_str( scene, "parent_id" );
        }

        if( scene.find( "exec_policy" ) != scene.end() ) {
            response[ "exec_policy" ] = json_get_str( scene, "exec_policy" );
        }
    }
    else {
        response[ "error" ] = "The scene doesn't exist";
    }
    return response;
}

void RemoteInterface::actionBlockTest( const JsonObject& actionBlock, SendResponseCallback sendResponseCallback ) {
    m_hostService.sendAsyncRequest( GroupsNames::SCENE_MANAGER,
                                    actionBlock,
                                    SCENE_ACTION_BLOCK_TEST,
                                    std::move( sendResponseCallback ),
                                    std::chrono::seconds( 90 ) );
}

std::string RemoteInterface::temperatureScale() const {
    std::string defaultScale = CoreD::Model::ValueScales::Fahrenheit;
    Hub::SettingsReader reader( m_dbClient );
    auto scaleSetting = reader.findSetting( "scale.temperature" );
    if( scaleSetting && !scaleSetting->getTokenValue().empty() )
        defaultScale = scaleSetting->getTokenValue();
    return defaultScale;
}

std::pair< bool, Json > RemoteInterface::modifySceneScales( const Json& scene, const std::string& defaultScale ) {
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

void RemoteInterface::copyFieldIfPresent( const Json& source, const std::string& key, Json& destination ) {
    auto it = source.find( key );
    if( it != source.end() )
        destination[ key ] = it.value();
}

JsonObject RemoteInterface::upsertSceneExpression( const std::string& name,
                                                   const JsonObject& params,
                                                   const std::string& code,
                                                   const std::string& valueType,
                                                   const Json& metadata,
                                                   bool variable,
                                                   const JsonObject& value ) {
    return m_hostService.sendRequest(
        GroupsNames::SCENE_MANAGER,
        JSON( "name" << name << "params" << params << "code" << code << "valueType" << valueType << "metadata"
                     << metadata << "variable" << variable << "value" << value ),
        SCENE_EXPRESSIONS_SET,
        std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::deleteSceneExpression( const std::string& expressionName ) {
    return m_hostService.sendRequest( GroupsNames::SCENE_MANAGER,
                                      JSON( "name" << expressionName ),
                                      SCENE_EXPRESSIONS_DELETE,
                                      std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::listSceneExpressions( bool showCode,
                                                  bool strict,
                                                  const std::vector< std::string >& filterTypes,
                                                  bool hasVariable,
                                                  bool variable,
                                                  const std::vector< std::string >& names,
                                                  bool showValue ) {
    return m_hostService.sendRequest( GroupsNames::SCENE_MANAGER,
                                      Json{ { "showCode", showCode },
                                            { "strict", strict },
                                            { "filterTypes", filterTypes },
                                            { "hasVariable", hasVariable },
                                            { "variable", variable },
                                            { "names", names },
                                            { "showValue", showValue } },
                                      SCENE_EXPRESSIONS_LIST,
                                      std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::stopScene( const BaseRemoteInterface::SceneId& id,
                                       bool isGlobal,
                                       const std::string& block,
                                       const std::string& groupId ) {
    return m_hostService.sendRequest(
        GroupsNames::SCENE_MANAGER,
        JSON( "sceneId" << id << "global" << isGlobal << "block" << block << "group" << groupId ),
        SCENE_STOP,
        std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::checkScenesFilterParams( const JsonObject& params ) {
    JsonObject answer;
    if( !params.empty() ) {
        try {
            jsonCheckObject( params, "filters", true );
            jsonCheckObject( params[ "filters" ], "include", true );
            jsonCheckObject( params[ "filters" ][ "include" ], "generator", true );
            jsonCheckString( params[ "filters" ][ "include" ][ "generator" ], "role", true );
        }
        catch( const WrongParamsError& ex ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, "wrong datatype ", ex.what() );
            return answer;
        }
        catch( const WrongFieldError& ex ) {
            setErrorWithField( answer, Errors::WRONG_FIELD, "field missing", ex.what() );
            return answer;
        }
    }
    return answer;
}

}  // namespace ScenesManager
}  // namespace HubPlatform
