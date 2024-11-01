#include "scenes_manager/scenes_processor/scene_request_handler.h"
#include "scenes_manager/scenes_processor/actions/action_factory.h"
#include "scenes_manager/scenes_processor/resources.h"
#include "scenes_manager/scenes_processor/response_builders/scenes_block_data_list/scenes_data_list_response_builder.h"
#include "scenes_manager/scenes_processor/scene_clone_manager.h"

#include "scenes_manager/scenes_engine/scene_manager.h"
#include "scenes_manager/scenes_engine/event_container.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "scenes_manager/scenes_engine/helpers/json_helper.h"
#include "scenes_manager/scenes_engine/helpers/clock.h"
#include "scenes_manager/scenes_engine/operators/scene_operator.h"
#include "scenes_manager/modify_scene/scene_scale_modifier.h"
#include "scenes_manager/modify_scene/modify_scene.h"
#include "scenes_manager/modify_scene/modify_scene_on_create.h"
#include "scenes_manager/modify_scene/modify_scene_on_fetch.h"
#include "scenes_manager/modify_scene/default_items_transport.h"
#include "cored/group_operations/device_item_group_manager.h"
#include "scenes_manager/scenes_engine/expressions/expression.h"
#include "scenes_manager/scenes_engine/expressions/expression_value_provider.h"
#include "scenes_manager/scenes_engine/events/generic_item_value_provider.h"
#include "scenes_manager/scenes_engine/value_source_key.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"
#include "cored/model/resources/cored_resources.h"

#include "database_client/dbqueryparser.h"
#include "database_client/db_tables_names.h"

#include "base_utils/acl.h"
#include "base_utils/ezlodb.h"

#include "jsonrpc/jsonrpc.h"
#include "common_utils/hub_info_helper.h"
#include "scenes_manager/scenes_engine/actions/test_action_executor.h"
#include "scenes_manager/scenes_engine/operators/operator_exceptions.h"
#include <utility>
#include "cored/model/values/value_factory.h"
#include "cored/model/exceptions.h"
#include "scenes_manager/scenes_engine/expressions/resources.h"
#include <typeinfo>
#include <memory.h>
#include "cored/model/resources/cored_resources.h"
#include "extensions_manager/common/anonymous_plugin_utils.h"
#include "extensions_manager/resources/em_resources.h"
#include "scenes_manager/scenes_processor/resources.h"
#include "cored/group_operations/device_group_find_handler.h"
#include "scenes_manager/scenes_engine/actions/action_block.h"
#include "crypto_utils/digests.h"

#include <functional>

namespace HubPlatform {
namespace ScenesManager {

constexpr const char EO_ENTRY[] = "eo";
const auto ID = "_id";

namespace Limits {
const int MAX_EXPRESSION_NAME_SIZE = 30;
const int MAX_EXPRESSION_CODE_SIZE = 5000;
const int MAX_VALUE_CHUNK_SIZE = 500000;
}  // namespace Limits

namespace {
// Could be refactored with json::get_ptr
JsonObject removeFields( const JsonObject& obj, const std::vector< std::string >& fields ) {
    JsonObject result( obj );
    for( const auto& field : fields ) {
        if( result.count( field ) ) {
            result.erase( field );
        }
    }
    return result;
}
void handleException( JsonObject& answer, const SceneEngine::SceneException& ex ) {
    setError( answer, ex.getError(), ex.what() );
    const auto& badField = ex.getBadField();
    if( !badField.empty() ) {
        auto& data = answer[ std::string( Errors::ERROR_FIELD ) ][ "data" ];
        data = data.get< std::string >() + "." + ex.getBadField();
    }
}

void handleException( JsonObject& answer, const std::exception& ex ) {
    setError( answer, Errors::INTERNAL_OPERATION_ERROR, ex.what() );
}

}  // namespace

ScenesRequestHandler::ScenesRequestHandler( IDbClient& client,
                                            const ControllerId& controllerId,
                                            SceneEngine::SceneManager& sceneManager,
                                            SceneEngine::IClock& clock,
                                            CoreDRemoteInterface::CoreDInterface& coreInterface,
                                            ExtensionsManager::RemoteInterface& extensionsManagerInterface,
                                            SceneEngine::Expressions::ExpressionManager& expressions,
                                            CoreD::DeviceItemGroupManager& deviceItemGroupManager,
                                            BroadcastCallback sendBroadcast,
                                            AddServiceTask addServiceTask )
: m_dbClient( client )
, m_manager( sceneManager )
, m_clock( clock )
, m_core( coreInterface )
, m_extensionsManager( extensionsManagerInterface )
, m_expressions( expressions )
, m_deviceItemGroupManager( deviceItemGroupManager )
, m_sendBroadcast( std::move( sendBroadcast ) )
, m_addServiceTask( std::move( addServiceTask ) )
, m_blocks( [this]( const SceneEngine::SceneBlocks::ItemToSave& itemToSave ) {
    auto& dbClient = getDBClient();
    JsonObject dbItem = dbClient.findOne( HaTempScenesItemsDBName, JSON( "_id" << itemToSave.item.id ), nullptr );
    if( dbItem.empty() ) {
        dbItem[ "_id" ] = itemToSave.item.id;
        dbItem[ "name" ] = itemToSave.item.name;
        dbItem[ "deviceId" ] = itemToSave.item.deviceId;
        dbItem[ "enum" ] = itemToSave.item.enumList;
        dbItem[ "sourceIds" ].push_back( itemToSave.sourceId );
        dbItem[ "deviceCategory" ] = itemToSave.item.device.category;
        dbItem[ "deviceSubcategory" ] = itemToSave.item.device.subcategory;
        dbItem[ "parentCategory" ] = itemToSave.item.parent.category;
        dbItem[ "parentSubcategory" ] = itemToSave.item.parent.subcategory;
        dbItem[ "type" ] = itemToSave.item.type;
        dbClient.insert( HaTempScenesItemsDBName, dbItem );
    }
    else {
        dbClient.update( HaTempScenesItemsDBName,
                         JSON( "_id" << itemToSave.item.id ),
                         JSON( "$addToSet" << JSON( "sourceIds" << itemToSave.sourceId ) ) );
    }
    const SceneEngine::SceneBlocks::OptionItem& optionItem = itemToSave.optionItem;
    if( !optionItem.name.empty() ) {
        LOGGER_TRACE_RAW( "id: " + itemToSave.item.id + " blockId: " + itemToSave.blockId );
        const JsonObject query = JSON( "itemId" << itemToSave.item.id << "blockId" << itemToSave.blockId );
        if( dbClient.count( HaTempScenesOptionItemsDBName, query ) == 0 ) {
            LOGGER_TRACE_RAW( "insert" );
            dbClient.insert( HaTempScenesOptionItemsDBName,
                             JSON( "itemId" << itemToSave.item.id << "blockId" << itemToSave.blockId << "sourceId"
                                            << itemToSave.sourceId << "deviceId" << itemToSave.item.deviceId
                                            << "optionItemName" << optionItem.name << "optionItemValue"
                                            << optionItem.value << "optionFieldName" << optionItem.fieldName ) );
        }
        else {
            LOGGER_TRACE_RAW( "update value: " + optionItem.value );
            dbClient.update(
                HaTempScenesOptionItemsDBName, query, JSON( "$set" << JSON( "optionItemValue" << optionItem.value ) ) );
        }
    }
} ) {
    m_expressions.onValueChanged( [this]( const std::string& expressionName, const Json& expressionValue ) {
        m_manager.processExpressionValueChanged( expressionName, expressionValue, JSON_EMPTY_OBJ );
    } );

    auto makeExpressionProvider = [this]( const std::string& expressionName ) -> SceneEngine::ItemValueProviderPtr {
        return makeExpressionValueProvider( m_expressions, expressionName );
    };

    auto makeGenericValueProvider = [&]( SceneEngine::ValueSourceKey valueKey ) -> SceneEngine::ItemValueProviderPtr {
        return makeGenericItemValueProvider( m_manager, std::move( valueKey ) );
    };

    auto geoPositionGetter = [this]() { return m_core.getGeoPosition().first; };
    auto clockGetter = [this]() -> SceneEngine::IClock& { return m_clock.get(); };

    m_sceneParser = std::make_unique< SceneParser >( coreInterface );
    m_actionParser = std::make_unique< ActionParser >(
        controllerId, coreInterface, extensionsManagerInterface, makeExpressionProvider );
    m_eventParser = EventParserFactory::makeEventParser(
        controllerId, geoPositionGetter, clockGetter, makeGenericValueProvider, m_manager.getTaskLoop() );

    m_anonymousPluginOperations = std::make_unique< AnonymousPluginOperations >( m_dbClient, m_manager );
}

void ScenesRequestHandler::safeCreateScene( JsonObject& scene ) {
    LOGGER_TRACE( cc::trace( "try create scene" ) );

    SceneParserInterface::ScenePtr scenePtr;
    try {
        createSceneInternal( scenePtr, scene, false );
    }
    catch( const SceneEngine::SceneException& ex ) {
        JsonObject error;
        Errors::get( ex.getError(), error );

        addEmptyScene( scenePtr, scene );

        std::string errorMessage;
        if( scenePtr ) {
            LOGGER_ERROR( cc::error( "Scene loaded with error, id: " ) + cc::note( scenePtr->getId() ) +
                          cc::error( ", error : " ) + cc::j( error ) );
            errorMessage = "Scene loaded with error, id: " + scenePtr->getId() + ", error: " + error.dump();
        }
        else {
            LOGGER_ERROR( cc::error( "Scene parsed with error: " ) + cc::j( error ) );
            errorMessage = "Scene parsed with error: " + error.dump();
        }

        setSceneError( scenePtr, errorMessage );
    }
    catch( const std::exception& ex ) {
        std::string exceptionName = typeid( ex ).name();
        std::string errorMessage = "Scene parsing failed ( " + exceptionName + " ): " + ex.what();
        LOGGER_ERROR( cc::error( std::string( "Scene parsing failed " ) ) +
                      cc::note( "( " + exceptionName + " ): " + ex.what() ) );

        addEmptyScene( scenePtr, scene );
        setSceneError( scenePtr, errorMessage );
    }
    catch( ... ) {
        const std::string errorMessage = "Caught unknown error during scene parsing";
        LOGGER_ERROR( cc::error( errorMessage ) );

        addEmptyScene( scenePtr, scene );
        setSceneError( scenePtr, errorMessage );
    }
}

void ScenesRequestHandler::createSceneInternal( SceneParserInterface::ScenePtr& scenePtr,
                                                JsonObject& scene,
                                                bool isEditing ) {
    scenePtr = m_sceneParser->parse( scene, isEditing );
    scene[ "generator" ] = scenePtr->getGeneratorData();

    IdUtils::PlaceholdersMapping placeholdersMapping;
    auto triggers = m_eventParser->parseTriggers( scene, placeholdersMapping );
    auto exceptions = m_eventParser->parseExceptions( scene, placeholdersMapping );
    auto globalRestriction = m_eventParser->parseGlobalRestriction( scenePtr, scene, placeholdersMapping );
    auto actions = parseActions( scene, placeholdersMapping, triggers.m_actionGroupMappings );
    checkEventsWithExpressions( triggers );
    checkEventsWithExpressions( exceptions );
    checkEventsWithExpressions( globalRestriction );
    m_manager.addScene( scenePtr,
                        std::move( actions ),
                        std::move( triggers ),
                        std::move( exceptions ),
                        std::move( globalRestriction ),
                        isEditing );
}

void ScenesRequestHandler::updatePluginsAcquiredCountForScene( Json& scene ) {
    IdUtils::PlaceholdersMapping placeholdersMapping;
    auto triggers = m_eventParser->parseTriggers( scene, placeholdersMapping );
    auto actions = parseActions( scene, placeholdersMapping, triggers.m_actionGroupMappings );
    auto sceneId = json_get_str( scene, "_id" );

    auto pluginsInfo = m_anonymousPluginOperations->getAnonymousPluginCountInScene( sceneId, actions );
    for( auto& info : pluginsInfo ) {
        auto pluginId = info.first;
        auto count = info.second;
        m_anonymousPluginOperations->updatePluginAcquiredCountForScene( pluginId, count, sceneId );
    }
}

void ScenesRequestHandler::setSceneError( SceneParserInterface::ScenePtr& scenePtr, const std::string& errorMessage ) {
    if( scenePtr ) {
        scenePtr->setErrorData( { { "error", errorMessage } } );
    }
}

void ScenesRequestHandler::addEmptyScene( SceneParserInterface::ScenePtr& scenePtr, const JsonObject& scene ) {
    if( !scenePtr ) {
        scenePtr = m_sceneParser->create( scene );
    }

    if( scenePtr ) {
        m_manager.addScene( scenePtr,
                            SceneEngine::ActionBlocks{},
                            SceneEngine::EventContainer{},
                            SceneEngine::EventContainer{},
                            SceneEngine::EventContainer{},
                            false );
    }
}

bool ScenesRequestHandler::processCreateScene( const JsonObject& params, JsonObject& answer ) {
    std::string name;
    bool isGlobal = json_get_bool( params, "global" );
    if( !isGlobal && !getNonEmptyField( params, "name", answer, name, true ) ) {
        return false;
    }
    SceneEngine::Scene::SceneID id;
    if( isGlobal ) {
        if( !getNonEmptyField( params, "_id", answer, id, false ) ) {
            return false;
        }
        if( m_manager.isSceneExist( id ) ) {
            setError( answer, Errors::SCENE_IS_OVERLAPPING );
            return false;
        }
    }

    if( id.empty() ) {
        id = Db::genOid();
        for( std::size_t i = 0; m_manager.isSceneExist( id ); ++i ) {
            if( i >= 3 ) {
                setErrorWithField( answer, Errors::IMPOSSIBLE_TO_GENERATE_UNIQUE_ID, "", id );
                LOGGER_ERROR( "The unique id can't be generated" );
                return false;
            }
            id = Db::genOid();
        }
    }
    bool enabled;
    if( !getField( params, "enabled", answer, enabled ) ) {
        return false;
    }

    JsonObject sceneID = Db::getIdObj( id );

    // TODO: Might not need to remove the temperatureUnits and use them later for conversion
    JsonObject scene = removeFields( params, { "permission", "serial", "sender", "temperatureUnits" } );
    scene[ db::ID_FIELD_NAME ] = id;

    std::pair< bool, Json > prepared = prepareCreateSafe( scene );
    if( prepared.first ) {
        scene = prepared.second;
        LOGGER_INFO( "prepareCreate modified the scene: " + cc::j( id ) );
    }
    else {
        LOGGER_TRACE( cc::trace( "prepareScene: Scene left unchanged: " ).append( id ) );
    }

    SceneEngine::Scene::ParentId parentId;
    if( !isGlobal && !getOptionalField( scene, "parent_id", parentId ) ) {
        scene[ "parent_id" ] = "";
    }

    try {
        SceneParserInterface::ScenePtr scenePtr;
        createSceneInternal( scenePtr, scene, false );
        LOGGER_TRACE( cc::j( scene ) );
        getDBClient().insert( HaScenesDBName, JSON( sceneID << scene ) );

        updatePluginsAcquiredCountForScene( scene );

        Acl::create( id, Resource::SCENES, params );
        if( scenePtr->isGlobal() ) {
            m_sendBroadcast( GLOBAL_SCENE_ADDED, scene );
        }
        else {
            scene[ HubPlatform::CoreD::Model::Resources::System::SyncNotification ] = true;
            m_sendBroadcast( SCENE_ADDED, scene );
        }
    }
    catch( const SceneEngine::SceneException& ex ) {
        handleException( answer, ex );
        LOGGER_ERROR( cc::error( "Create scene failed: " ) + cc::j( answer ) + cc::error( ", exception: " ) +
                      cc::note( ex.what() ) );
        return false;
    }
    catch( const std::exception& ex ) {
        handleException( answer, ex );
        LOGGER_ERROR( cc::error( std::string( "Create scene failed: " ) ) +
                      cc::note( "( " + std::string( typeid( ex ).name() ) + " ): " ) + cc::note( ex.what() ) );

        return false;
    }

    return true;
}

bool ScenesRequestHandler::processEditScene( const JsonObject& params, JsonObject& answer ) {
    JsonObject editedScene = JSON_EMPTY_OBJ;
    std::string id;
    if( !getField( params, db::ID_FIELD_NAME, answer, id ) ) {
        return false;
    }
    bool isGlobal = json_get_bool( params, "global" );
    if( isGlobal ) {
        if( !getField( params, "scene", answer, editedScene ) ) {
            return false;
        }
        editedScene[ "global" ] = true;
    }
    else {
        if( !getField( params, EO_ENTRY, answer, editedScene ) ) {
            return false;
        }
        std::string name;
        if( !getNonEmptyField( editedScene, "name", answer, name, true ) ) {
            return false;
        }
        SceneEngine::Scene::ParentId parentId;
        if( !getOptionalField( editedScene, "parent_id", parentId ) ) {
            editedScene[ "parent_id" ] = "";
        }
    }
    bool enabled;
    if( !getField( editedScene, "enabled", answer, enabled ) ) {
        return false;
    }

    id = tiny::tools::trim_copy( id );
    if( id.empty() ) {
        setError( answer, Errors::ID_IS_EMPTY );
        return false;
    }
    editedScene[ db::ID_FIELD_NAME ] = id;

    const SceneEngine::SceneSharedPtr& originalScenePtr = m_manager.findSceneById( id );
    if( !originalScenePtr ) {
        setErrorWithField( answer, Errors::SCENE_DOES_NOT_EXIST, "", id );
        return false;
    }
    const std::string originalSceneName = originalScenePtr->getName();
    JsonObject originalGeneratorData = originalScenePtr->getGeneratorData();
    if( !json_contains( editedScene, "generator" ) ) {
        if( !originalGeneratorData.empty() ) {
            originalGeneratorData[ "role" ] = "user";
            editedScene[ "generator" ] = originalGeneratorData;
        }
    }

    auto oldScene = getDBClient().findOne( HaScenesDBName, JSON( db::ID_FIELD_NAME << id ), nullptr );
    if( !m_manager.deleteScene( id, isGlobal, true ) ) {
        setErrorWithField( answer, Errors::SCENE_DOES_NOT_EXIST, "", id );
        return false;
    }

    std::pair< bool, Json > prepared = prepareCreateSafe( editedScene );
    if( prepared.first ) {
        editedScene = prepared.second;
        LOGGER_INFO( "prepareCreate modified the scene: " + cc::j( id ) );
    }
    else {
        LOGGER_TRACE( cc::trace( "prepareScene: Scene left unchanged: " ).append( id ) );
    }

    auto restoreOldScene = [this, oldScene, id]() mutable {
        try {
            safeCreateScene( oldScene );
            m_anonymousPluginOperations->resetPluginAcquiredInfoForScene( id );
            updatePluginsAcquiredCountForScene( oldScene );
        }
        catch( const SceneEngine::SceneException& ex ) {
            Json error = JSON_EMPTY_OBJ;
            handleException( error, ex );
            Log::error( cc::error( "restore failed: " ) + cc::j( error ) );
        }
        catch( const std::exception& ex ) {
            Log::error( cc::error( "restore failed: " ) + cc::note( ex.what() ) );
        }
    };

    try {
        SceneParserInterface::ScenePtr scenePtr;
        createSceneInternal( scenePtr, editedScene, true );
        m_anonymousPluginOperations->resetPluginAcquiredInfoForScene( id );
        updatePluginsAcquiredCountForScene( editedScene );

        getDBClient().update( HaScenesDBName, JSON( db::ID_FIELD_NAME << id ), editedScene );

        if( isGlobal ) {
            m_sendBroadcast( GLOBAL_SCENE_CHANGED, editedScene );
        }
        else {
            std::string sceneName;
            if( json_get( editedScene, "name", sceneName ) ) {
                using HubPlatform::CoreD::Model::Resources::System::SyncNotification;
                json_add_bool( editedScene, SyncNotification, sceneName != originalSceneName );
            }
            m_sendBroadcast( SCENE_CHANGED, editedScene );
        }
    }
    catch( const SceneEngine::SceneException& ex ) {
        restoreOldScene();
        handleException( answer, ex );
        Log::error( cc::error( "Edit of a scene failed: " ) + cc::j( answer ) );
        return false;
    }
    catch( const std::exception& ex ) {
        restoreOldScene();
        setError( answer, Errors::INTERNAL_OPERATION_ERROR, ex.what() );
        Log::error( cc::error( std::string( "Edit of a scene failed: " ) ) + cc::note( ex.what() ) );
        return false;
    }

    return true;
}

bool ScenesRequestHandler::processDeleteScene( const JsonObject& params, JsonObject& answer ) {
    std::string id;
    if( !getField( params, db::ID_FIELD_NAME, answer, id ) ) {
        return false;
    }

    if( m_manager.isSceneExist( id ) ) {
        m_anonymousPluginOperations->resetPluginAcquiredInfoForScene( id );
    }

    bool isGlobal = json_get_bool( params, "global" );
    if( !m_manager.deleteScene( id, isGlobal ) ) {
        setErrorWithField( answer, Errors::SCENE_DOES_NOT_EXIST, "", id );
        Log::warn( cc::warn( "Delete scene failed: " ) + cc::j( params ) );
        return false;
    }
    getDBClient().remove( HaScenesDBName, JSON( db::ID_FIELD_NAME << id ) );
    answer[ db::ID_FIELD_NAME ] = id;
    if( isGlobal ) {
        m_sendBroadcast( GLOBAL_SCENE_DELETED, answer );
    }
    else {
        answer[ HubPlatform::CoreD::Model::Resources::System::SyncNotification ] = true;
        m_sendBroadcast( SCENE_DELETED, answer );
    }
    return true;
}

bool ScenesRequestHandler::processDeleteAllScenes( const JsonObject& params, JsonObject& answer ) {
    bool isGlobal = json_get_bool( params, "global" );
    Json searchCondition = JSON_EMPTY_OBJ;
    if( isGlobal ) {
        searchCondition = JSON( "global" << true );
    }
    else {
        searchCondition = JSON( "global" << false << JSON( "global" << JSON( "$exists" << false ) ) );
    }

    auto& dbClient = getDBClient();
    auto fieldsToReturn = JSON( db::ID_FIELD_NAME << 1 << "global" << 1 );
    auto scenes = dbClient.findAll( HaScenesDBName, searchCondition, &fieldsToReturn );
    for( const auto& scene : scenes ) {
        auto id = json_get_str( scene, db::ID_FIELD_NAME );
        if( !m_manager.deleteScene( id, isGlobal ) ) {
            Log::warn( cc::warn( "Delete scene failed: " ) + cc::j( params ) );
            continue;
        }
        dbClient.remove( HaScenesDBName, JSON( db::ID_FIELD_NAME << id ) );
        auto broadcast = JSON( db::ID_FIELD_NAME << id );
        if( isGlobal ) {
            m_sendBroadcast( GLOBAL_SCENE_DELETED, broadcast );
        }
        else {
            broadcast[ HubPlatform::CoreD::Model::Resources::System::SyncNotification ] = true;
            m_sendBroadcast( SCENE_DELETED, broadcast );
        }
    }
    return true;
}

bool ScenesRequestHandler::processEnableScene( const JsonObject& params, JsonObject& answer ) {
    bool enabled = false;
    SceneEngine::Scene::SceneID sceneId;
    if( !getField( params, db::ID_FIELD_NAME, answer, sceneId ) || !getField( params, "enabled", answer, enabled ) ) {
        return false;
    }

    if( !m_manager.isSceneExist( sceneId ) ) {
        setErrorWithField( answer, Errors::SCENE_DOES_NOT_EXIST, "", sceneId );
        return false;
    }

    bool isGlobal = json_get_bool( params, "global" );
    JsonObject updatedScene = updateSceneParam( sceneId, "enabled", enabled, isGlobal );
    if( updatedScene.empty() || !m_manager.setSceneEnabled( sceneId, enabled ) ) {
        setErrorWithField( answer, Errors::SCENE_DOES_NOT_EXIST, "", sceneId );
        Log::warn( cc::warn( "Set enable failed: " ) + cc::j( answer ) );
        return false;
    }
    if( isGlobal ) {
        m_sendBroadcast( GLOBAL_SCENE_CHANGED, updatedScene );
    }
    else {
        m_sendBroadcast( SCENE_CHANGED, updatedScene );
    }

    return true;
}

bool ScenesRequestHandler::processCloneScene( const JsonObject& params, JsonObject& answer ) {
    bool enabled = false;
    SceneEngine::Scene::SceneID sceneId;
    if( !getField( params, db::ID_FIELD_NAME, answer, sceneId ) ) {
        return false;
    }

    if( !m_manager.isSceneExist( sceneId ) ) {
        setErrorWithField( answer, Errors::SCENE_DOES_NOT_EXIST, "", sceneId );
        return false;
    }

    std::string name;
    getField( params, "name", answer, name );

    bool isGlobal = json_get_bool( params, "global" );
    JsonObject clonedScene = cloneScene( sceneId, name, isGlobal );
    if( clonedScene.empty() ) {
        setErrorWithField( answer, Errors::SCENE_DOES_NOT_EXIST, "", sceneId );
        Log::warn( cc::warn( "Set enable failed: " ) + cc::j( answer ) );
        return false;
    }

    return true;
}

bool ScenesRequestHandler::processRunScene( const JsonObject& params, JsonObject& answer ) {
    SceneEngine::Scene::SceneID sceneId;
    if( !getField( params, "sceneId", answer, sceneId ) ) {
        return false;
    }
    if( !m_manager.isSceneExist( sceneId ) ) {
        setErrorWithField( answer, Errors::SCENE_DOES_NOT_EXIST, "", sceneId );
        return false;
    }
    auto blockType = [&] {
        std::string blockTypeStr;
        getOptionalField( params, ScenesManager::ActionFields::Fields::Block, blockTypeStr );
        using BlockType = SceneEngine::RunSceneActionBlockType;
        return blockTypeStr == ActionBlockTypes::Then ?
                   BlockType::Then :
                   blockTypeStr == ActionBlockTypes::Else ?
                   BlockType::Else :
                   blockTypeStr == ActionBlockTypes::ThenGroups ?
                   BlockType::ThenGroups :
                   blockTypeStr == ActionBlockTypes::ElseGroups ? BlockType::ElseGroups : BlockType::Empty;
    }();
    bool isGlobal = json_get_bool( params, "global" );
    std::string group = json_get_str( params, ScenesManager::ActionFields::Fields::Group );
    if( !m_manager.runScene( sceneId, isGlobal, blockType, group ) ) {
        setErrorWithField( answer, Errors::SCENE_DOES_NOT_EXIST, "", sceneId );
        return false;
    }
    return true;
}

bool ScenesRequestHandler::processStopScene( const JsonObject& params, JsonObject& answer ) {
    SceneEngine::Scene::SceneID sceneId;
    if( !getField( params, "sceneId", answer, sceneId ) ) {
        return false;
    }
    if( !m_manager.isSceneExist( sceneId ) ) {
        setErrorWithField( answer, Errors::SCENE_DOES_NOT_EXIST, "The scene doesn't exist", sceneId );
        return false;
    }
    std::string blockTypeStr;
    std::string groupId;
    getOptionalField( params, ScenesManager::ActionFields::Fields::Block, blockTypeStr );
    getOptionalField( params, ScenesManager::ActionFields::Fields::Group, groupId );

    if( !blockTypeStr.empty() )
        if( groupId.empty() && ( blockTypeStr != ActionBlockTypes::Then ) &&
            ( blockTypeStr != ActionBlockTypes::Else ) ) {
            setErrorWithField(
                answer, Errors::SCENE_UNSUPPORTED_ACTION_BLOCK, "Invalid action block type", blockTypeStr );
            return false;
        }

    auto blockType = [&] {
        std::string blockTypeStr;
        getOptionalField( params, ScenesManager::ActionFields::Fields::Block, blockTypeStr );
        using BlockType = SceneEngine::RunSceneActionBlockType;
        return blockTypeStr == ActionBlockTypes::Else ?
                   BlockType::Else :
                   blockTypeStr == ActionBlockTypes::ThenGroups ?
                   BlockType::ThenGroups :
                   blockTypeStr == ActionBlockTypes::ElseGroups ? BlockType::ElseGroups : BlockType::Empty;
    }();
    bool isGlobal = json_get_bool( params, "global" );
    if( !groupId.empty() ) {
        if( !m_manager.checkActionGroupIDForAll( blockType, groupId ) ) {
            setErrorWithField( answer, Errors::SCENE_ACTION_GROUP_NOT_EXIST, "A group doesn't exist", groupId );
            return false;
        }
        else {
            if( !m_manager.checkActionGroupID( sceneId, blockType, groupId ) ) {
                setErrorWithField( answer,
                                   Errors::SCENE_ACTION_GROUP_EXIST_WRONG_SCENE,
                                   "The group doesn't belong to the scene",
                                   groupId );
                return false;
            }
        }
    }
    if( !m_manager.stopScene( sceneId, isGlobal, blockType ) ) {
        setErrorWithField( answer, Errors::SCENE_DOES_NOT_EXIST, "", sceneId );
        return false;
    }
    return true;
}

bool ScenesRequestHandler::processSetRoom( const JsonObject& params, JsonObject& answer ) {
    std::string sceneId, roomId;
    if( !getField( params, "sceneId", answer, sceneId ) || !getField( params, "roomId", answer, roomId ) ) {
        return false;
    }

    SceneEngine::SceneSharedPtr scenePtr = m_manager.findSceneById( sceneId );
    if( !scenePtr ) {
        wrongParam( "sceneId", answer );
        return false;
    }
    std::string roomName;
    if( roomId == "0" ) {
        roomId = "";
    }
    else {
        JsonObject room = m_core.getRoom( roomId );
        if( room.empty() ) {
            wrongParam( "roomId", answer );
            return false;
        }
        roomName = json_get_str( room, "name" );
    }

    scenePtr->setParentId( roomId );
    scenePtr->setRoomName( roomName );

    JsonObject updatedScene = updateSceneParam( sceneId, "parent_id", roomId );
    if( updatedScene.empty() ) {
        return false;
    }
    m_sendBroadcast( SCENE_CHANGED, updatedScene );

    return true;
}

JsonObject ScenesRequestHandler::processRoomWasRemoved( const JsonObject& params ) {
    JsonObject answer = JSON_EMPTY_OBJ;
    std::string roomId;
    if( !getField( params, "_id", answer, roomId ) ) {
        return answer;
    }

    // Enumerate all scenes and clear room id.
    std::vector< SceneEngine::SceneSharedPtr > scenes;
    m_manager.foreachScenes( [&roomId, &scenes]( const SceneEngine::SceneSharedPtr& sceneObj ) {
        if( sceneObj->getParentId() == roomId ) {
            scenes.emplace_back( sceneObj );
        }
        return false;
    } );

    for( const auto& scenePtr : scenes ) {
        scenePtr->setParentId( std::string() );
        JsonObject updatedScene = updateSceneParam( scenePtr->getId(), "parent_id", std::string() );
        m_sendBroadcast( SCENE_CHANGED, updatedScene );
    }

    return answer;
}

JsonObject ScenesRequestHandler::processAddNotification( const JsonObject& params ) {
    JsonObject answer = JSON_EMPTY_OBJ;
    SceneEngine::Scene::SceneID sceneId;
    std::string userId;
    if( !getField( params, "sceneId", answer, sceneId ) || !getField( params, "userId", answer, userId ) ) {
        return answer;
    }
    SceneEngine::SceneSharedPtr scenePtr = m_manager.findSceneById( sceneId );
    if( !scenePtr ) {
        wrongParam( "sceneId", answer );
        return answer;
    }
    if( !scenePtr->addUserNotification( userId ) ) {
        wrongParam( "userId", answer );
        return answer;
    }
    JsonObject updatedScene =
        updateSceneParam( sceneId, "user_notifications", JsonArray( scenePtr->getUserNotifications() ) );
    if( updatedScene.empty() ) {
        setErrorWithField( answer, Errors::SCENE_DOES_NOT_EXIST, "", sceneId );
        Log::warn( cc::warn( "Add notification failed: " ) + cc::j( answer ) );
        return answer;
    }
    m_sendBroadcast( SCENE_CHANGED, updatedScene );

    return answer;
}

JsonObject ScenesRequestHandler::processRemoveNotification( const JsonObject& params ) {
    JsonObject answer = JSON_EMPTY_OBJ;
    SceneEngine::Scene::SceneID sceneId;
    std::string userId;
    if( !getField( params, "sceneId", answer, sceneId ) || !getField( params, "userId", answer, userId ) ) {
        return answer;
    }

    SceneEngine::SceneSharedPtr scenePtr = m_manager.findSceneById( sceneId );
    if( !scenePtr ) {
        wrongParam( "sceneId", answer );
        return answer;
    }
    if( !scenePtr->removeUserNotification( userId ) ) {
        wrongParam( "userId", answer );
        return answer;
    }
    JsonObject updatedScene =
        updateSceneParam( sceneId, "user_notifications", JsonArray( scenePtr->getUserNotifications() ) );
    if( updatedScene.empty() ) {
        setErrorWithField( answer, Errors::SCENE_DOES_NOT_EXIST, "", sceneId );
        Log::warn( cc::warn( "Remove notification failed: " ) + cc::j( answer ) );
        return answer;
    }
    m_sendBroadcast( SCENE_CHANGED, updatedScene );

    return answer;
}

JsonObject ScenesRequestHandler::processTimeList( const JsonObject& /*params*/ ) {
    auto timeScenesList = m_manager.getTimeScenesList();
    JsonArray timeScenesArray = JSON_EMPTY_ARR;
    for( const auto& scene : timeScenesList ) {
        JsonObject sceneInfo = JSON_EMPTY_OBJ;
        json_add_str( sceneInfo, "sceneId", scene.id );
        json_add_str( sceneInfo, "sceneName", scene.name );
        json_add_str( sceneInfo, "methodName", scene.method );
        json_add_str( sceneInfo, "executionDate", scene.date );
        timeScenesArray.push_back( sceneInfo );
    }
    return timeScenesArray;
}

JsonObject ScenesRequestHandler::processSetBlocks( const JsonObject& params ) {
    std::string sourceId;
    JsonObject answer = JSON_EMPTY_OBJ;
    if( !getField( params, "sourceId", answer, sourceId ) ) {
        return answer;
    }
    try {
        using namespace CoreDRemoteInterface;
        auto items = m_core.getItems();
        std::map< CoreDInterface::DeviceID, std::vector< JsonObject* > > sortedItemsByDeviceId;
        for( auto& item : items ) {
            auto deviceId = json_get_str( item, "deviceId" );
            sortedItemsByDeviceId[ deviceId ].push_back( &item );
        }
        const auto devices = m_core.getDevices();
        std::map< CoreDInterface::DeviceID, const JsonObject* > sortedDevicesByDeviceId;
        for( auto& device : devices ) {
            auto deviceId = json_get_str( device, "_id" );
            sortedDevicesByDeviceId[ deviceId ] = &device;
        }
        for( const auto& device : devices ) {
            const auto deviceId = json_get_str( device, "_id" );
            auto deviceIdItemIt = sortedItemsByDeviceId.find( deviceId );
            if( deviceIdItemIt == sortedItemsByDeviceId.end() ) {
                LOGGER_TRACE( "Device Id not found: " + deviceId );
                continue;
            }
            const auto parentId = json_get_str( device, "parentDeviceId" );
            std::string parentCategory;
            std::string parentSubcategory;
            if( !parentId.empty() ) {
                auto it = sortedDevicesByDeviceId.find( parentId );
                if( it != sortedDevicesByDeviceId.end() ) {
                    parentCategory = json_get_str( *it->second, "category" );
                    parentSubcategory = json_get_str( *it->second, "subcategory" );
                }
            }
            auto& items = deviceIdItemIt->second;
            for( auto* itemPtr : items ) {
                auto& item = *itemPtr;
                item[ "deviceCategory" ] = json_get_str( device, "category" );
                item[ "deviceSubcategory" ] = json_get_str( device, "subcategory" );
                item[ "parentCategory" ] = parentCategory;
                item[ "parentSubcategory" ] = parentSubcategory;
            }
        }
        bool result =
            m_blocks.setBlockSourceAndFilterItems( sourceId, *m_extensionsManager.getPluginFile( sourceId ), items );
        if( !result ) {
            std::string errorMessage = "invalid block";
            LOGGER_ERROR( cc::error( errorMessage ) );
            setError( answer, Errors::INTERNAL_OPERATION_ERROR, errorMessage );
            return answer;
        }
    }
    catch( std::exception& ex ) {
        LOGGER_ERROR( cc::error( ex.what() ) );
        setError( answer, Errors::INTERNAL_OPERATION_ERROR, ex.what() );
        return answer;
    }

    auto& dbClient = getDBClient();
    if( dbClient.findOne( HaScenesBlocksDBName, params, nullptr ).empty() ) {
        dbClient.insert( HaScenesBlocksDBName, params );
    }
    else {
        dbClient.update( HaScenesBlocksDBName, params, JSON( "$set" << params ) );
    }
    return answer;
}

JsonObject ScenesRequestHandler::processSetIcon( const JsonObject& params ) {
    std::string sceneId, icon;
    JsonObject answer = JSON_EMPTY_OBJ;
    if( !getField( params, "id", answer, sceneId ) || !getField( params, "icon", answer, icon ) ) {
        return answer;
    }
    if( !isInvalidValueRange( "_id", sceneId, SceneParserInterface::SceneIdMaxSize, answer ) ) {
        return answer;
    }
    if( !isInvalidValueRange( "icon", icon, SceneParserInterface::IconMaxSize, answer ) ) {
        return answer;
    }
    SceneEngine::SceneSharedPtr scenePtr = m_manager.findSceneById( sceneId );
    if( !scenePtr ) {
        setError( answer, Errors::SCENE_DOES_NOT_EXIST );
        return answer;
    }
    scenePtr->setIcon( icon );
    JsonObject updatedScene = updateSceneParam( sceneId, "icon", icon );
    if( updatedScene.empty() ) {
        return answer;
    }
    m_sendBroadcast( SCENE_CHANGED, updatedScene );
    return answer;
}

JsonObject ScenesRequestHandler::processRemoveBlocks( const JsonObject& params ) {
    std::string sourceId;
    JsonObject answer = JSON_EMPTY_OBJ;
    if( !getField( params, "sourceId", answer, sourceId ) ) {
        return answer;
    }

    auto& dbClient = getDBClient();
    dbClient.remove( HaScenesBlocksDBName, params );
    dbClient.remove( HaTempScenesItemsDBName, JSON( "$pull" << JSON( "sourceIds" << sourceId ) ) );
    dbClient.remove( HaTempScenesItemsDBName, JSON( "sourceIds" << JSON_EMPTY_ARR ) );
    dbClient.remove( HaTempScenesOptionItemsDBName, JSON( "sourceId" << sourceId ) );
    m_blocks.removeSceneBlocksSource( sourceId );
    return answer;
}

JsonObject ScenesRequestHandler::processGetBlocksDataList( const JsonObject& params ) {
    JsonObject answer = JSON_EMPTY_OBJ;
    bool all = json_get_bool( params, "all" );
    bool valueTypes = json_get_bool( params, "valueTypes" );
    bool scalableValueTypes = json_get_bool( params, "scalableValueTypes" );
    bool valueScales = json_get_bool( params, "valueScales" );
    bool sceneValueTypes = json_get_bool( params, "scenesValueTypes" );
    bool valueTypeFamilies = json_get_bool( params, "valueTypeFamilies" );
    bool comparisonOperators = json_get_bool( params, "comparisonOperators" );
    bool comparisonMethods = json_get_bool( params, "comparisonMethods" );
    bool actionMethods = json_get_bool( params, "actionMethods" );
    bool advancedScenesVersion = json_get_bool( params, "advancedScenesVersion" );

    ScenesDataListResponseBuilder responseBuilder( m_core );
    responseBuilder.setAll( all );
    responseBuilder.setValueTypes( valueTypes );
    responseBuilder.setScalableValueTypes( scalableValueTypes );
    responseBuilder.setValueScales( valueScales );
    responseBuilder.setSceneValueTypes( sceneValueTypes );
    responseBuilder.setValueTypeFamilies( valueTypeFamilies );
    responseBuilder.setComparisonOperators( comparisonOperators );
    responseBuilder.setComparisonMethods( comparisonMethods );
    responseBuilder.setActionMethods( actionMethods );

    return responseBuilder.getDataWithFilter();
}

JsonObject ScenesRequestHandler::processGetBlocks( const JsonObject& params ) {
    JsonArray devices;
    std::string blockType;
    JsonObject answer = JSON_EMPTY_OBJ;
    if( !getFieldAsArray( params, "devices", answer, devices ) ) {
        return answer;
    }
    if( !getField( params, "blockType", answer, blockType ) ) {
        return answer;
    }
    if( blockType == "when" ) {
        SceneEngine::SceneBlocks::ItemsToGet items;
        {
            auto& dbClient = getDBClient();
            auto query = JSON( "deviceId" << JSON( "$in" << devices ) );
            auto dbItems = dbClient.findAll( HaTempScenesItemsDBName, query, nullptr );
            auto dbOptionItems = dbClient.findAll( HaTempScenesOptionItemsDBName, query, nullptr );
            items = convertToBlockItems( dbItems, dbOptionItems );
        }
        std::vector< SceneEngine::SceneBlocks::StreamUPtr > blockStreams;
        for( const auto& sourceId : m_blocks.getBlockSourceIds() ) {
            try {
                blockStreams.emplace_back( m_extensionsManager.getPluginFile( sourceId ) );
            }
            catch( std::exception& ex ) {
                LOGGER_ERROR( cc::error( ex.what() ) );
            }
        }
        return m_blocks.getBlockList( items, blockStreams );
    }
    else if( blockType == "then" ) {
        return JSON_EMPTY_ARR;
    }
    else {
        setError( answer, Errors::WRONG_PARAMS, "Value should be ether when or then" );
    }
    return answer;
}

void ScenesRequestHandler::wrongParam( const std::string& fieldName, JsonObject& answer ) {
    JsonObject error_mes;
    Errors::getWrongParams( fieldName, error_mes );
    setErrorObject( answer, error_mes );
}

bool ScenesRequestHandler::cloneScene( const SceneEngine::Scene::SceneID& sceneId,
                                       JsonObject& clonedScene,
                                       const std::string& sceneName ) {
    JsonObject answer = JSON_EMPTY_OBJ;
    std::string id;

    const SceneEngine::SceneSharedPtr& originalScenePtr = m_manager.findSceneById( sceneId );
    if( !originalScenePtr ) {
        setErrorWithField( answer, Errors::SCENE_DOES_NOT_EXIST, "", id );
        return false;
    }

    ScenesCloneManager cloneManager( getDBClient(), clonedScene, m_manager );
    if( !cloneManager.run( sceneName ) ) {
        LOGGER_ERROR( cc::error( "Clone scene failed" ) );
        setErrorWithField( answer, Errors::SCENE_CANT_BE_CLONED, "", id );
        return false;
    }

    std::pair< bool, Json > prepared = prepareCreateSafe( clonedScene );
    if( prepared.first ) {
        clonedScene = prepared.second;
        LOGGER_INFO( "prepareCreate modified the scene: " + cc::j( id ) );
        LOGGER_INFO( "prepareCreate modified the prepared.second: " + cc::j( prepared.second ) );
    }
    else {
        LOGGER_TRACE( cc::trace( "prepareScene: Scene left unchanged: " ).append( id ) );
    }

    try {
        SceneParserInterface::ScenePtr scenePtr;
        createSceneInternal( scenePtr, clonedScene, false );
        LOGGER_TRACE( cc::j( clonedScene ) );

        auto sceneId = json_get_str( clonedScene, "id" );
        getDBClient().insert( HaScenesDBName, JSON( sceneId << clonedScene ) );

        updatePluginsAcquiredCountForScene( clonedScene );
        if( scenePtr->isGlobal() ) {
            m_sendBroadcast( GLOBAL_SCENE_ADDED, clonedScene );
        }
        else {
            clonedScene[ HubPlatform::CoreD::Model::Resources::System::SyncNotification ] = true;

            LOGGER_TRACE( "clonedScene: " + cc::j( clonedScene ) );

            m_sendBroadcast( SCENE_ADDED, clonedScene );
        }
    }
    catch( const SceneEngine::SceneException& ex ) {
        handleException( answer, ex );
        LOGGER_ERROR( cc::error( "Clone scene failed: " ) + cc::j( answer ) + cc::error( ", exception: " ) +
                      cc::note( ex.what() ) );
        return false;
    }
    catch( const std::exception& ex ) {
        handleException( answer, ex );
        LOGGER_ERROR( cc::error( std::string( "Clone scene failed: " ) ) +
                      cc::note( "( " + std::string( typeid( ex ).name() ) + " ): " ) + cc::note( ex.what() ) );
        return false;
    }
    return true;
}

JsonObject ScenesRequestHandler::cloneScene( const SceneEngine::Scene::SceneID& sceneId,
                                             const std::string& sceneName,
                                             bool isGlobal ) {
    JsonObject query;
    if( isGlobal ) {
        query = JSON( db::ID_FIELD_NAME << sceneId << "global" << true );
    }
    else {
        query = JSON( db::ID_FIELD_NAME << sceneId << "$or"
                                        << JSON_ARRAY_( JSON( "global" << false ),
                                                        JSON( "global" << JSON( "$exists" << false ) ) ) );
    }

    auto& dbClient = getDBClient();
    JsonObject sceneToClone = dbClient.findOne( HaScenesDBName, query, nullptr );
    if( sceneToClone.empty() ) {
        LOGGER_TRACE( "scene not found" );
        return sceneToClone;
    }

    return cloneScene( sceneId, sceneToClone, sceneName );
}

JsonObject ScenesRequestHandler::updateSceneParam( const SceneEngine::Scene::SceneID& sceneId,
                                                   const std::string& fieldName,
                                                   const Json& value,
                                                   bool isGlobal ) {
    JsonObject query;
    if( isGlobal ) {
        query = JSON( db::ID_FIELD_NAME << sceneId << "global" << true );
    }
    else {
        query = JSON( db::ID_FIELD_NAME << sceneId << "$or"
                                        << JSON_ARRAY_( JSON( "global" << false ),
                                                        JSON( "global" << JSON( "$exists" << false ) ) ) );
    }
    auto& dbClient = getDBClient();
    JsonObject sceneToAltering = dbClient.findOne( HaScenesDBName, query, nullptr );
    if( sceneToAltering.empty() ) {
        LOGGER_TRACE( "scene not found" );
        return sceneToAltering;
    }
    sceneToAltering[ fieldName ] = value;
    dbClient.update( HaScenesDBName, JSON( db::ID_FIELD_NAME << sceneId ), sceneToAltering );
    if( isGlobal ) {
        return JSON( fieldName << value );
    }
    return sceneToAltering;
}

JsonObject ScenesRequestHandler::enableSceneBlock( const SceneEngine::Scene::SceneID& sceneId,
                                                   const std::string& blockId,
                                                   bool enabled,
                                                   bool isGlobal ) {
    JsonObject query;
    if( isGlobal ) {
        query = JSON( db::ID_FIELD_NAME << sceneId << "global" << true );
    }
    else {
        query = JSON( db::ID_FIELD_NAME << sceneId << "$or"
                                        << JSON_ARRAY_( JSON( "global" << false ),
                                                        JSON( "global" << JSON( "$exists" << false ) ) ) );
    }
    auto& dbClient = getDBClient();
    JsonObject sceneToAltering = dbClient.findOne( HaScenesDBName, query, nullptr );
    if( sceneToAltering.empty() ) {
        LOGGER_TRACE( "scene not found" );
        return sceneToAltering;
    }
    auto blockPtr = findNestedBlock( sceneToAltering[ "when" ], blockId );
    if( !blockPtr ) {
        throw SceneEngine::SceneException( Errors::SCENE_BLOCK_DOES_NOT_EXIST );
    }
    blockPtr->operator[]( "enabled" ) = enabled;
    return sceneToAltering;
}

JsonObject* ScenesRequestHandler::findNestedBlock( Json& blocks, const std::string& blockId ) {
    std::string method;
    if( blocks.is_array() ) {
        for( auto& block : blocks ) {
            if( block[ "_id" ] == blockId ) {
                return &block;
            }
            else {
                json_get( block, std::vector< Json >{ "blockOptions", "method", "name" }, method );
                if( SceneEngine::Model::isLogicOperator( method ) ) {
                    auto result = findNestedBlock( block[ "fields" ][ 0 ][ "value" ], blockId );
                    if( result ) {
                        return result;
                    }
                }
            }
        }
    }
    else if( blocks[ "_id" ] == blockId ) {
        return &blocks;
    }
    return nullptr;
}

SceneEngine::SceneBlocks::ItemsToGet ScenesRequestHandler::convertToBlockItems(
    const std::vector< JsonObject >& dbItems,
    const std::vector< JsonObject >& dbOptionItems ) {
    using namespace SceneEngine;
    using SortedOptionItemsByItemId = std::map< SceneBlocks::ItemId, SceneBlocks::SortedOptionItemsByBlockId >;
    SortedOptionItemsByItemId sortedOptionItemsByItemId;
    for( const auto& dbOptionItem : dbOptionItems ) {
        auto itemId = json_get_str( dbOptionItem, "itemId" );
        auto blockId = json_get_str( dbOptionItem, "blockId" );
        SceneBlocks::OptionItem optionItem;
        optionItem.name = json_get_str( dbOptionItem, "optionItemName" );
        optionItem.value = json_get_str( dbOptionItem, "optionItemValue" );
        optionItem.fieldName = json_get_str( dbOptionItem, "optionFieldName" );
        sortedOptionItemsByItemId[ itemId ][ blockId ] = optionItem;
    }
    SceneBlocks::ItemsToGet itemsList;
    for( const auto& dbItem : dbItems ) {
        try {
            SceneBlocks::ItemToGet itemToGet;
            SceneBlocks::Item& item = itemToGet.item;
            item.id = json_get_str( dbItem, "_id" );
            item.name = json_get_str( dbItem, "name" );
            item.deviceId = json_get_str( dbItem, "deviceId" );
            item.device.category = json_get_str( dbItem, "deviceCategory" );
            item.device.subcategory = json_get_str( dbItem, "deviceSubcategory" );
            item.parent.category = json_get_str( dbItem, "parentCategory" );
            item.parent.subcategory = json_get_str( dbItem, "parentSubcategory" );
            item.type = json_get_str( dbItem, "type" );
            const JsonArray& dbEnumList = dbItem[ "enum" ];
            for( const auto& dbEnum : dbEnumList ) {
                item.enumList.insert( dbEnum.get< std::string >() );
            }
            auto itemIdIt = sortedOptionItemsByItemId.find( item.id );
            if( itemIdIt != sortedOptionItemsByItemId.end() ) {
                auto& sortedOptionItemsByBlockId = itemIdIt->second;
                itemToGet.sortedOptionItemsByBlockId.insert( sortedOptionItemsByBlockId.begin(),
                                                             sortedOptionItemsByBlockId.end() );
            }
            itemsList.emplace_back( std::move( itemToGet ) );
        }
        catch( std::exception& ex ) {
            LOGGER_ERROR( cc::error( "Exception message: " ) + cc::j( ex.what() ) );
            LOGGER_ERROR( cc::error( "Item format: " ) + cc::j( dbItem ) );
            return SceneBlocks::ItemsToGet();
        }
    }
    return itemsList;
}

bool ScenesRequestHandler::isInvalidValueRange( const std::string& fieldName,
                                                const std::string& field,
                                                std::size_t maxSize,
                                                JsonObject& answer ) const {
    if( field.size() > maxSize ) {
        JsonObject error = JSON_EMPTY_OBJ;
        Errors::getInvalidValueRange(
            fieldName, error, fieldName + " field is more than " + std::to_string( maxSize ) + " symbols" );
        answer[ std::string( Errors::ERROR_FIELD ) ] = std::move( error );
        return false;
    }
    return true;
}

void ScenesRequestHandler::setUp() {
    LOGGER_TRACE( "load scenes" );
    loadScenes();
}

void ScenesRequestHandler::tearDown() {}

void ScenesRequestHandler::checkEvents( const std::vector< SceneEngine::EventContainer::ItemEventBaseEvent >& events ) {
    for( const auto& event : events ) {
        const auto& itemEvent = event.second;
        const auto& searchKey = event.first;
        checkEvent( itemEvent, searchKey );
    }
}

void ScenesRequestHandler::checkEvent( const SceneEngine::ItemEventBasePtr& event,
                                       const SceneEngine::ValueSourceKey& searchKey ) {
    using namespace SceneEngine;

    for( const ValueSourceKey& eventValueKey : event->getValueKeys() ) {
        if( searchKey == eventValueKey )
            throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK,
                                  "Event value source is compared against itself: " + searchKey.getSourceId() );

        if( !eventValueKey.isExpression() )
            continue;

        auto& expressionName = eventValueKey.getSourceId();
        if( !m_expressions.hasExpression( expressionName ) ) {
            LOGGER_ERROR( cc::error( "Expression does not exist" ) );
            throw SceneException( Errors::SCENES_EXPRESSION_DOES_NOT_EXIST,
                                  "Expression does not exist: " + expressionName );
        }

        auto eventSourceValue = m_manager.fetchSourceValue( searchKey );
        if( !eventSourceValue.isValid() )
            throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "Invalid event source value type" );

        auto expressionValue = m_manager.fetchSourceValue(
            SceneEngine::ValueSourceKey( "", expressionName, SceneEngine::ValueSourceKey::SourceType::Expression ) );
        if( !expressionValue.isValid() )
            throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "Invalid expression value type" );

        try {
            event->checkValue( eventSourceValue );
        }
        catch( const SceneException& ex ) {
            LOGGER_ERROR( cc::error( std::string( "Event failed: " ) ) +
                          cc::note( "( " + std::string( typeid( ex ).name() ) + " ): " ) + cc::note( ex.what() ) );
            throw SceneException( Errors::SCENE_WHEN_TYPE_MISMATCH, "Expression value type mismatch" );
        }
        catch( const std::exception& ex ) {
            LOGGER_ERROR( cc::error( std::string( "Event failed: " ) ) +
                          cc::note( "( " + std::string( typeid( ex ).name() ) + " ): " ) + cc::note( ex.what() ) );
            throw SceneException( Errors::SCENE_WHEN_TYPE_MISMATCH, "Expression value type mismatch" );
        }
        catch( ... ) {
            LOGGER_ERROR( cc::error( "Expression another error" ) );
            throw SceneException( Errors::SCENES_EXPRESSION_ERROR, "Expression other error" );
        }
    }
}

void ScenesRequestHandler::checkItemEvents( const SceneEngine::EventContainer::ItemEvents& events ) {
    for( const auto& event : events ) {
        const auto& itemEvent = event.second;
        const auto& searchKey = event.first;
        checkItemEvent( itemEvent, searchKey );
    }
}

void ScenesRequestHandler::checkItemEvent( const SceneEngine::ItemEventPtr& event,
                                           const SceneEngine::ValueSourceKey& searchKey ) {
    using namespace SceneEngine;

    for( const ValueSourceKey& eventValueKey : event->getValueKeys() ) {
        if( !eventValueKey.isExpression() )
            continue;

        auto& expressionName = eventValueKey.getSourceId();
        if( !m_expressions.hasExpression( expressionName ) ) {
            LOGGER_ERROR( cc::error( "Expression does not exist" ) );
            throw SceneException( Errors::SCENES_EXPRESSION_DOES_NOT_EXIST, "Expression does not exist" );
        }

        auto eventSourceValue = m_manager.fetchSourceValue( searchKey );
        if( !eventSourceValue.isValid() )
            throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "Invalid event source value type" );

        auto expressionValue = m_manager.fetchSourceValue(
            SceneEngine::ValueSourceKey( "", expressionName, SceneEngine::ValueSourceKey::SourceType::Expression ) );
        if( !expressionValue.isValid() )
            throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "Invalid expression value type" );

        try {
            event->checkValue( eventSourceValue );
        }
        catch( const SceneException& ex ) {
            LOGGER_ERROR( cc::error( std::string( "Item event failed: " ) ) +
                          cc::note( "( " + std::string( typeid( ex ).name() ) + " ): " ) + cc::note( ex.what() ) );
            throw SceneException( Errors::SCENE_WHEN_TYPE_MISMATCH, "Expression value type mismatch" );
        }
        catch( const std::exception& ex ) {
            LOGGER_ERROR( cc::error( std::string( "Item event failed: " ) ) +
                          cc::note( "( " + std::string( typeid( ex ).name() ) + " ): " ) + cc::note( ex.what() ) );
            throw SceneException( Errors::SCENES_EXPRESSION_VALUE_ERROR, ex.what() );
        }
        catch( ... ) {
            LOGGER_ERROR( cc::error( "Expression another error" ) );
            throw SceneException( Errors::SCENES_EXPRESSION_VALUE_ERROR, "Expression other error" );
        }
    }
}

void ScenesRequestHandler::checkEventsWithExpressions( const SceneEngine::EventContainer& events ) {
    checkItemEvents( events.getItemEvents() );
    checkEvents( events.getCompareNumbersEvents() );
    checkEvents( events.getCompareStringsEvents() );
    checkEvents( events.getInArrayEvents() );
    checkEvents( events.getStringOperationEvents() );
    checkEvents( events.getCompareValuesEvents() );
    checkEvents( events.getDeviceItemGroupsEvents() );
}

void ScenesRequestHandler::loadScenes() {
    LOGGER_TRACE( cc::error( "loadScenes" ) );
    auto& dbClient = getDBClient();
    fillBlocksSources( dbClient );
    auto scenes = dbClient.findAll( HaScenesDBName, JSON_EMPTY_OBJ, nullptr );
    moveScenesWithIsSceneStateWhenBlockToBottom( scenes );

    m_anonymousPluginOperations->resetAllPluginAcquiredInfo();

    for( auto& scene : scenes ) {
        try {
            safeCreateScene( scene );
            updatePluginsAcquiredCountForScene( scene );
        }
        catch( const SceneEngine::SceneException& ex ) {
            std::string id;
            if( json_get( scene, db::ID_FIELD_NAME, id ) ) {
                JsonObject error_mes;
                Errors::get( ex.getError(), error_mes );
                Log::error( cc::error( "can't load scene with id : " ) + cc::note( id ) +
                            ", error : " + cc::j( error_mes ) );
            }
            else {
                JsonObject error_mes;
                Errors::get( ex.getError(), error_mes );
                Log::error( cc::error( "can't load scene with error : " ) + cc::j( error_mes.dump() ) );
            }
        }
        catch( const std::exception& ex ) {
            Log::error( cc::error( "Something terrible has happened : " ) + cc::note( ex.what() ) );
        }
    }
}

void ScenesRequestHandler::setClock( SceneEngine::IClock& clock ) {
    m_clock = clock;
    auto clockGetter = [this]() -> SceneEngine::IClock& { return m_clock.get(); };
    m_eventParser->setClockGetter( std::move( clockGetter ) );
}

void ScenesRequestHandler::itemAddedHandler( const JsonObject& message ) {
    JsonObject item = message;
    auto controllerId = json_get_str( item, "controllerId" );
    if( !controllerId.empty() && controllerId != HubInfoHelper::getPkDevice() ) {
        return;
    }
    m_blocks.checkForOptionItemUpdate( item );
    const auto blockSources = m_blocks.getBlockSourceIds();
    for( const auto& sourceId : blockSources ) {
        try {
            auto deviceId = json_get_str( item, "deviceId" );
            const auto device = m_core.getSingleDevice( deviceId );
            auto parentId = json_get_str( device, "parentDeviceId" );
            if( !parentId.empty() ) {
                const auto parent = m_core.getSingleDevice( parentId );
                item[ "parentCategory" ] = json_get_str( parent, "category" );
                item[ "parentSubcategory" ] = json_get_str( parent, "subcategory" );
            }
            item[ "deviceCategory" ] = json_get_str( device, "category" );
            item[ "deviceSubcategory" ] = json_get_str( device, "subcategory" );
            item[ "type" ] = json_get_str( item, "valueType" );
            auto possibleOptionItems = m_core.getItemsByDeviceId( deviceId );
            m_blocks.filterOutNewItem(
                item, possibleOptionItems, sourceId, *m_extensionsManager.getPluginFile( sourceId ) );
        }
        catch( std::exception& ex ) {
            Log::error( cc::error( "itemAddedHandler: " ) + cc::note( ex.what() ) );
        }
    }

    try {
        m_expressions.handleItemAdded( message );
    }
    catch( const std::exception& ex ) {
        Log::error( cc::error( "itemAddedHandler/expressions: " ) + cc::note( ex.what() ) );
    }
}

void ScenesRequestHandler::itemRemovedHandler( const JsonObject& message ) {
    auto itemId = json_get_str( message, "_id" );
    if( !m_expressions.handleItemRemoved( message ) ) {
        auto controllerId = json_get_str( message, "controllerId" );
        if( !controllerId.empty() && controllerId != HubInfoHelper::getPkDevice() ) {
            return;
        }
        m_expressions.itemCache().removeItem( SceneEngine::ValueSourceKey( controllerId, itemId ) );
    }

    auto& dbClient = getDBClient();
    dbClient.remove( HaTempScenesItemsDBName, JSON( "_id" << itemId ) );
    dbClient.remove( HaTempScenesOptionItemsDBName, JSON( "itemId" << itemId ) );
}

void ScenesRequestHandler::itemUpdatedHandler( const JsonObject& message ) {
    m_expressions.handleItemUpdated( message );
}

void ScenesRequestHandler::deviceUpdatedHandler( const JsonObject& message ) {
    m_expressions.handleDeviceUpdated( message );
}

void ScenesRequestHandler::deviceRemovedHandler( const JsonObject& message ) {}

JsonObject ScenesRequestHandler::modifyScenes( const JsonObject& params ) {
    JsonObject answer;
    auto& dbClient = getDBClient();
    auto scenes = dbClient.findAll( HaScenesDBName, JSON_EMPTY_OBJ, nullptr );
    std::set< std::string > blockIds;
    auto blockIdsIt = params.find( "blockIds" );
    if( blockIdsIt != params.end() ) {
        for( const auto& blockId : *blockIdsIt ) {
            blockIds.insert( json_get_str( blockId ) );
        }
    }
    auto commandsIt = params.find( "commands" );
    if( commandsIt == params.end() ) {
        Errors::getEmptyString( "commands", answer );
        return answer;
    }
    using ReplaceCommands = std::map< std::string, std::string >;
    ReplaceCommands replaceTokenCommands;
    ReplaceCommands replaceLanguageTextCommands;
    const JsonArray& commands = *commandsIt;
    for( const auto& command : commands ) {
        const auto commandName = json_get_str( command, "command" );
        if( commandName.empty() ) {
            Errors::getEmptyString( "command", answer );
            return answer;
        }
        if( commandName == "replace_token" ) {
            auto tokenToReplace = json_get_str( command, "to_replace" );
            auto replaceByToken = json_get_str( command, "replace_by" );
            replaceTokenCommands.insert( { std::move( tokenToReplace ), std::move( replaceByToken ) } );
        }
        else if( commandName == "replace_option_label" ) {
            auto languageTag = json_get_str( command, "language_tag" );
            auto replaceByText = json_get_str( command, "replace_by_text" );
            replaceLanguageTextCommands.insert( { std::move( languageTag ), std::move( replaceByText ) } );
        }
        else {
            Errors::getWrongParams( "command", answer );
            return answer;
        }
    }
    std::function< bool( JsonObject& ) > updateWhenBlock =
        [&replaceTokenCommands, &replaceLanguageTextCommands, &blockIds, &updateWhenBlock]( JsonObject& whenBlock ) {
            auto replaceTokenValue = [&replaceTokenCommands]( Json& obj ) {
                auto valueIt = obj.find( "value" );
                if( valueIt != obj.end() ) {
                    auto replaceByIt = replaceTokenCommands.find( json_get_str( *valueIt ) );
                    if( replaceByIt != replaceTokenCommands.end() ) {
                        *valueIt = replaceByIt->second;
                        return true;
                    }
                }
                return false;
            };
            std::string methodName;
            if( !json_get( whenBlock, { "blockOptions", "method", "name" }, methodName ) ) {
                Log::error( cc::error( "updateWhenBlock: empty method name" ) );
                return false;
            }
            auto fieldsIt = whenBlock.find( "fields" );
            if( fieldsIt == whenBlock.end() ) {
                Log::error( cc::error( "updateWhenBlock: empty fields" ) );
                return false;
            }
            using namespace SceneEngine::Model::Resources;
            if( SceneEngine::Model::isLogicOperator( methodName ) &&
                !SceneEngine::Model::isOperatorNot( methodName ) ) {
                auto blocksFieldIt = SceneEngine::Utils::findField( "blocks", *fieldsIt );
                if( blocksFieldIt == fieldsIt->end() ) {
                    Log::error( cc::error( "updateWhenBlock: empty blocks field" ) );
                    return false;
                }
                Log::trace( cc::j( *blocksFieldIt ) );
                auto valueIt = blocksFieldIt->find( "value" );
                if( valueIt == blocksFieldIt->end() ) {
                    Log::error( cc::error( "updateWhenBlock: empty value field" ) );
                    return false;
                }
                bool isUpdated = false;
                for( auto& block : *valueIt ) {
                    isUpdated |= updateWhenBlock( block );
                }
                return isUpdated;
            }
            if( SceneEngine::Model::isOperatorNot( methodName ) ) {
                auto blockFieldIt = SceneEngine::Utils::findField( "block", *fieldsIt );
                if( blockFieldIt == fieldsIt->end() ) {
                    Log::error( cc::error( "updateWhenBlock: empty block field" ) );
                    return false;
                }
                Log::trace( cc::j( *blockFieldIt ) );
                auto valueIt = blockFieldIt->find( "value" );
                if( valueIt == blockFieldIt->end() ) {
                    Log::error( cc::error( "updateWhenBlock: empty value field" ) );
                    return false;
                }
                return updateWhenBlock( *valueIt );
            }
            if( !blockIds.empty() ) {
                auto blockIdIt = whenBlock.find( "blockId" );
                if( blockIdIt == whenBlock.end() ) {
                    return false;
                }
                if( blockIds.find( json_get_str( *blockIdIt ) ) == blockIds.end() ) {
                    return false;
                }
            }
            bool isUpdated = false;
            auto valueFieldIt = SceneEngine::Utils::findField( "value", *fieldsIt );
            if( valueFieldIt == fieldsIt->end() ) {
                Log::error( cc::error( "updateWhenBlock: empty value field" ) );
                return false;
            }
            isUpdated |= replaceTokenValue( *valueFieldIt );
            auto optionsIt = valueFieldIt->find( "options" );
            if( optionsIt != valueFieldIt->end() ) {
                for( auto& option : *optionsIt ) {
                    isUpdated |= replaceTokenValue( option );
                    auto labelIt = option.find( "label" );
                    if( labelIt == option.end() ) {
                        continue;
                    }
                    auto replaceByIt = replaceLanguageTextCommands.find( json_get_str( *labelIt, "lang_tag" ) );
                    if( replaceByIt == replaceLanguageTextCommands.end() ) {
                        continue;
                    }
                    ( *labelIt )[ "text" ] = replaceByIt->second;
                    isUpdated = true;
                }
            }
            return isUpdated;
        };
    for( auto& scene : scenes ) {
        auto whenIt = scene.find( "when" );
        if( whenIt == scene.end() ) {
            LOGGER_ERROR( cc::error( "empty when block" ) );
            continue;
        }
        bool isUpdated = false;
        for( auto& when : *whenIt ) {
            isUpdated |= updateWhenBlock( when );
        }
        if( isUpdated ) {
            auto id = json_get_str( scene, "_id" );
            dbClient.update( HaScenesDBName, JSON( "_id" << id ), JSON( "$set" << scene ) );
        }
    }
    return answer;
}

JsonObject ScenesRequestHandler::getExpressionValue( const JsonObject& params ) {
    JsonObject answer;
    int size = Limits::MAX_VALUE_CHUNK_SIZE;
    int offset = 0;
    const bool compactDefault = true;
    bool compact = true;
    JsonObject result = JSON_EMPTY_OBJ;

    if( params.find( "options" ) != params.end() ) {
        if( params.find( "options" ) != params.end() ) {
            JsonObject options;
            json_get( params, "options", options );

            size = json_get_int( options, "size", 0 );
            if( size <= 0 || size > Limits::MAX_VALUE_CHUNK_SIZE ) {
                size = Limits::MAX_VALUE_CHUNK_SIZE;
            }

            offset = json_get_int( options, "offset", 0 );
            compact = json_get_bool( options, "compact", compactDefault );
        }
    }

    bool setMetadata = !compact;
    bool isLast = true;
    std::string error;

    if( params.find( "name" ) != params.end() ) {
        const auto name = json_get_str( params, "name" );
        const auto expression = m_expressions.findByName( name );
        if( !expression.empty() ) {
            JsonObject value = JSON_EMPTY_OBJ;
            json_get( expression, "value", value );

            std::string valueDump;
            if( value.is_string() ) {
                valueDump = value.get< std::string >();
            }
            else {
                valueDump = value.dump();
            }

            int valueSize = valueDump.size();

            // TODO: Add test for offset = -1, offset >= data.size
            if( offset < 0 || offset >= valueSize ) {
                setError( answer, Errors::WRONG_PARAMS, "Wrong format of a parameter 'data.offset'" );
                return answer;
            }

            isLast = ( offset + size >= valueSize );
            if( isLast && offset == 0 ) {
                result[ "value" ] = valueDump;
            }
            else {
                auto count = std::min( valueSize - offset, size );
                std::string part( valueDump.data() + offset, count );
                result[ "value" ] = part;
            }

            if( json_get( expression, "error", value ) ) {
                result[ "error" ] = value;
            }

            if( json_get( expression, "valueType", value ) ) {
                result[ "valueType" ] = json_get_str( value );
            }

            if( json_get( expression, "variable", value ) ) {
                result[ "variable" ] = json_get_bool( expression, "variable" );
            }

            if( setMetadata && json_get( expression, "metadata", value ) ) {
                result[ "metadata" ] = value;
            }

            result[ "last" ] = isLast;
        }
        else {
            setError( answer, Errors::SCENES_EXPRESSION_DOES_NOT_EXIST, "Expression doesn't exist" );
            return answer;
        }
    }
    else {
        setError( answer, Errors::WRONG_PARAMS, "Wrong format of a parameter 'name'" );
        return answer;
    }

    answer[ "result" ] = result;
    return answer;
}

JsonObject ScenesRequestHandler::setSceneMeta( const JsonObject& params ) {
    JsonObject answer = JSON_EMPTY_OBJ;
    std::string sceneId;
    JsonObject meta;
    if( !getField( params, "sceneId", answer, sceneId ) || !getField( params, "meta", answer, meta ) ) {
        return answer;
    }

    SceneEngine::SceneSharedPtr scenePtr = m_manager.findSceneById( sceneId );
    if( !scenePtr ) {
        setError( answer, Errors::SCENE_DOES_NOT_EXIST );
        return answer;
    }
    if( scenePtr->getMeta() != meta ) {
        scenePtr->setMeta( meta );
        JsonObject updatedScene = updateSceneParam( sceneId, "meta", meta );
        m_sendBroadcast( SCENE_CHANGED, updatedScene );
    }
    return answer;
}

JsonObject ScenesRequestHandler::setSceneBlockMeta( const JsonObject& params ) {
    JsonObject answer = JSON_EMPTY_OBJ;
    std::string sceneId;
    std::string blockId;
    JsonObject meta;
    if( !getField( params, "sceneId", answer, sceneId ) || !getField( params, "blockId", answer, blockId ) ||
        !getField( params, "meta", answer, meta ) ) {
        return answer;
    }

    JsonObject query = JSON(
        db::ID_FIELD_NAME << sceneId << "$or"
                          << JSON_ARRAY_( JSON( "global" << false ), JSON( "global" << JSON( "$exists" << false ) ) ) );
    auto& dbClient = getDBClient();
    JsonObject sceneToAlter = dbClient.findOne( HaScenesDBName, query, nullptr );
    if( sceneToAlter.empty() ) {
        LOGGER_TRACE( "scene not found" );
        return sceneToAlter;
    }

    auto blockPtr = findNestedBlock( sceneToAlter[ "when" ], blockId );
    if( !blockPtr ) {
        blockPtr = findNestedBlock( sceneToAlter[ "then" ], blockId );
    }
    if( !blockPtr ) {
        blockPtr = findNestedBlock( sceneToAlter[ "exceptions" ], blockId );
    }
    if( !blockPtr ) {
        blockPtr = findNestedBlock( sceneToAlter[ "else" ], blockId );
    }

    std::string blockName;
    JsonObject* groupsBlockPtr = nullptr;
    if( !blockPtr ) {
        auto tryProcessGroupBlock = [&]( std::string blockName ) -> bool {
            Json blocks = sceneToAlter[ blockName ];
            for( auto it = blocks.begin(); it != blocks.end(); ++it ) {
                const auto& key = it.key();
                blockPtr = findNestedBlock( sceneToAlter[ blockName ][ key ], blockId );
                break;
            }
            return blockPtr != nullptr;
        };

        if( !tryProcessGroupBlock( "thenGroups" ) ) {
            tryProcessGroupBlock( "elseGroups" );
        }
    }
    if( blockPtr ) {
        const char BlockMeta[] = "blockMeta";
        auto blockMeta = blockPtr->operator[]( BlockMeta );
        if( blockPtr->operator[]( BlockMeta ) != meta ) {
            blockPtr->operator[]( BlockMeta ) = meta;
            dbClient.update( HaScenesDBName, JSON( db::ID_FIELD_NAME << sceneId ), sceneToAlter );
            m_sendBroadcast( SCENE_CHANGED, sceneToAlter );
        }
    }

    return sceneToAlter;
}

JsonObject ScenesRequestHandler::setSceneName( const JsonObject& params ) {
    JsonObject answer = JSON_EMPTY_OBJ;
    std::string sceneId, name;
    if( !getField( params, "sceneId", answer, sceneId ) || !getField( params, "name", answer, name ) ) {
        return answer;
    }
    if( !isInvalidValueRange( "_id", sceneId, SceneParserInterface::SceneIdMaxSize, answer ) ) {
        return answer;
    }
    if( !isInvalidValueRange( "name", name, SceneParserInterface::NameMaxSize, answer ) ) {
        return answer;
    }
    SceneEngine::SceneSharedPtr scenePtr = m_manager.findSceneById( sceneId );
    if( !scenePtr ) {
        setError( answer, Errors::SCENE_DOES_NOT_EXIST );
        return answer;
    }
    scenePtr->setName( name );
    JsonObject updatedScene = updateSceneParam( sceneId, "name", name );
    m_sendBroadcast( SCENE_CHANGED, updatedScene );
    return answer;
}

JsonObject ScenesRequestHandler::setHouseModes( const JsonObject& params ) {
    JsonObject answer = JSON_EMPTY_OBJ;
    std::string sceneId;
    JsonArray houseModes;
    if( !getField( params, "sceneId", answer, sceneId ) ||
        !getFieldAsArray( params, "houseModes", answer, houseModes ) ) {
        return answer;
    }
    if( !isInvalidValueRange( "_id", sceneId, SceneParserInterface::SceneIdMaxSize, answer ) ) {
        return answer;
    }
    SceneEngine::SceneSharedPtr scenePtr = m_manager.findSceneById( sceneId );
    if( !scenePtr ) {
        setError( answer, Errors::SCENE_DOES_NOT_EXIST );
        return answer;
    }
    scenePtr->setHouseModes( houseModes );
    JsonObject updatedScene = updateSceneParam( sceneId, "house_modes", houseModes );
    m_sendBroadcast( SCENE_CHANGED, updatedScene );
    return answer;
}

JsonObject ScenesRequestHandler::setBlockEnabled( const JsonObject& params ) {
    JsonObject answer = JSON_EMPTY_OBJ;
    std::string sceneId;
    std::string blockId;
    bool enabled;
    if( !getField( params, "sceneId", answer, sceneId ) || !getField( params, "blockId", answer, blockId ) ||
        !getField( params, "enabled", answer, enabled ) ) {
        return answer;
    }
    try {
        m_manager.setSceneBlockEnabled( sceneId, blockId, enabled );
        JsonObject updatedScene = enableSceneBlock( sceneId, blockId, enabled );
        m_sendBroadcast( SCENE_CHANGED, updatedScene );
    }
    catch( const SceneEngine::SceneException& ex ) {
        handleException( answer, ex );
        Log::error( cc::error( "Set block enabled failed: " ) + cc::j( answer ) );
    }
    catch( const std::exception& ex ) {
        setError( answer, Errors::INTERNAL_OPERATION_ERROR, ex.what() );
        Log::error( cc::error( std::string( "Set block enabled failed: " ) ) + cc::note( ex.what() ) );
    }
    return answer;
}

void ScenesRequestHandler::actionBlockTest( const JsonObject& params,
                                            Services::Service& hostService,
                                            SceneEngine::ControllerFactoryInterface& controllerFactory,
                                            Callback callback ) {
    try {
        Json thenBlock = JSON( "then" << JSON_ARRAY_( params ) );
        IdUtils::PlaceholdersMapping placeholdersMapping;
        auto actions = m_actionParser->parse( thenBlock, ActionParserInterface::BlockType::Then, placeholdersMapping );
        if( actions.empty() ) {
            throw SceneEngine::SceneException( Errors::SCENE_WRONG_THEN_BLOCK );
        }
        auto actionExecutor = std::make_shared< SceneEngine::TestActionExecutor >(
            [callback]( const Json& response ) { callback( response ); } );
        auto method = SceneActions::getActionForMethod(
            actions.front()->getName(), hostService, controllerFactory, m_expressions, *this );
        method->run( *actions.front(), actionExecutor );
    }
    catch( const SceneEngine::SceneException& ex ) {
        JsonObject answer = JSON_EMPTY_OBJ;
        JsonObject error_mes;
        Errors::get( ex.getError(), error_mes );
        setErrorObject( answer, error_mes, ex.what() );
        Log::error( cc::error( "can't parse action : " ) + cc::j( error_mes ) );
        callback( answer );
    }
    catch( const std::exception& ex ) {
        JsonObject answer = JSON_EMPTY_OBJ;
        JsonObject error_mes;
        Errors::get( Errors::SCENE_WRONG_THEN_BLOCK, error_mes );
        setErrorObject( answer, error_mes, ex.what() );
        Log::error( cc::error( "error : " ) + cc::j( ex.what() ) );
        callback( answer );
    }
}

/** To use with processCreateScene/processEditScene. Modifies the scene passed if needed to contain scales of an items.
 */
std::pair< bool, Json > ScenesRequestHandler::prepareCreateSafe( const Json& scene ) {
    try {
        return prepareCreate( scene );
    }
    catch( const std::domain_error& e ) {
        LOGGER_ERROR( cc::error( "Exception: " ).append( e.what() ).append( ", domain_error" ) );
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( cc::error( "Exception: " ).append( e.what() ).append( typeid( e ).name() ) );
    }
    return std::make_pair( false, Json{} );
}

std::pair< bool, Json > ScenesRequestHandler::prepareCreate( const Json& scene ) {
    std::string defaultScale = fetchTemperatureScale();

    return prepareCreate( scene, defaultScale );
}

std::string ScenesRequestHandler::fetchTemperatureScale() const {
    const std::string ScaleTemperatureSetting = "scale.temperature";
    auto settingUPtr = m_core.findSetting( ScaleTemperatureSetting );
    if( !settingUPtr ) {
        // TODO: what to do in case there is no scale.temperature setting?
        throw SceneEngine::SceneException( Errors::EMPTY_FIELD, ScaleTemperatureSetting, "No field in settings" );
    }

    auto defaultScale = settingUPtr->getTokenValue();
    if( defaultScale.empty() ) {
        defaultScale = CoreD::Model::ValueScales::Fahrenheit;
    }
    return defaultScale;
}

std::pair< bool, Json > ScenesRequestHandler::prepareCreate( const Json& scene, const std::string& defaultScale ) {
    LOGGER_TRACE(
        cc::trace( "Default scale: " ).append( defaultScale ).append( ", scene: " ).append( cc::j( scene ) ) );

    DefaultItemsTransport itemsTransport( m_core );
    ModifyScene doIt( factoryModifySceneOnCreate( itemsTransport ) );
    return doIt.processScene( scene, defaultScale );
}

/** Prepares showing of scenes in commands like hub.scenes.list, hub.scenes.get: converts all scales to predefined
 * single scale */
std::pair< bool, Json > ScenesRequestHandler::prepareFetch( const Json& scene, const std::string& defaultScale ) {
    LOGGER_TRACE( cc::trace( "Default scale: " )
                      .append( defaultScale )
                      .append( ", scene: " )
                      .append( json_get_str( scene, "_id" ) ) );

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
    return std::make_pair( false, Json{} );
}

void ScenesRequestHandler::moveScenesWithIsSceneStateWhenBlockToBottom( std::vector< Json >& scenes ) {
    const auto searchPath = std::vector< Json >{ "blockOptions", "method", "name" };
    std::function< bool( const Json&, const std::string& ) > isSceneStateBlockPresent =
        [&searchPath, &isSceneStateBlockPresent]( const Json& block, const std::string& sceneId ) {
            std::string methodName;
            json_get( block, searchPath, methodName );
            bool isLogicOperatorBlock = SceneEngine::Model::isLogicOperator( methodName );
            if( isLogicOperatorBlock ) {
                auto& fields = block[ "fields" ];
                if( fields.empty() ) {
                    Log::error( cc::error( "fields not found in scene: " ) + cc::note( sceneId ) );
                    return false;
                }
                auto& blocksField = fields[ 0 ];
                if( blocksField.empty() ) {
                    Log::error( cc::error( "firstField not found in scene: " ) + cc::note( sceneId ) );
                    return false;
                }
                auto& value = blocksField[ "value" ];
                if( value.empty() ) {
                    Log::error( cc::error( "value not found in scene: " ) + cc::note( sceneId ) );
                    return false;
                }
                if( SceneEngine::Model::isOperatorNot( methodName ) ) {
                    return isSceneStateBlockPresent( value, sceneId );
                }
                else {
                    bool result = false;
                    for( const auto& subBlock : value ) {
                        result |= isSceneStateBlockPresent( subBlock, sceneId );
                    }
                    return result;
                }
            }
            else {
                return methodName == SceneEngine::Model::Resources::MethodNames::isSceneState;
            }
        };
    std::partition(
        scenes.begin(),
        scenes.end(),
        [&isSceneStateBlockPresent]( const Json& scene ) {  // move scenes with isSceneState when block to bottom
            auto sceneId = json_get_str( scene, "_id" );
            auto& whenBlocks = scene[ "when" ];
            bool result = false;
            for( const auto& block : whenBlocks ) {
                result |= isSceneStateBlockPresent( block, sceneId );
            }
            return !result;
        } );
}

JsonObject ScenesRequestHandler::getSceneById( const JsonObject& message ) {
    auto id = json_get_str( message, "_id" );
    auto result = m_dbClient.findOne( HaScenesDBName, JSON( db::ID_FIELD_NAME << id ), nullptr );

    std::string defaultScale = fetchTemperatureScale();
    std::pair< bool, Json > prepared = prepareFetch( result, defaultScale );
    if( prepared.first ) {
        LOGGER_TRACE( cc::trace( "result modified by prepareFetch: " ) +
                      cc::trace( json_get_str( prepared.second, "_id" ) ) );
        return prepared.second;
    }

    return result;
}

JsonArray ScenesRequestHandler::getScenesList( const JsonObject& params ) {
    auto results = m_dbClient.findAll(
        HaScenesDBName,
        JSON( "$or" << JSON_ARRAY_( JSON( "global" << false ), JSON( "global" << JSON( "$exists" << false ) ) ) ),
        nullptr );

    if( !params.empty() ) {
        if( !checkScenesListParams( params ) ) {
            return JSON_EMPTY_OBJ;
        }
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

    std::string defaultScale = fetchTemperatureScale();
    int updated = 0;
    for( auto& scene : results ) {
        std::pair< bool, Json > prepared = prepareFetch( scene, defaultScale );
        if( prepared.first ) {
            updated++;
            LOGGER_TRACE( "result modified by prepareFetch: " + cc::trace( json_get_str( prepared.second, "_id" ) ) );

            scene = prepared.second;
        }
    }

    return results;
}

JsonArray ScenesRequestHandler::getGlobalScenesList( const JsonObject& message ) {
    LOGGER_TRACE( cc::j( message ) );
    bool onlyId = json_get_bool( message, "onlyId", false );

    if( onlyId ) {
        auto fieldToReturn = JSON( "_id" << onlyId );
        return m_dbClient.findAll( HaScenesDBName, JSON( "global" << true ), &fieldToReturn );
    }
    else {
        auto results = m_dbClient.findAll( HaScenesDBName, JSON( "global" << true ), nullptr );

        std::string defaultScale = fetchTemperatureScale();
        int updated = 0;
        for( auto& scene : results ) {
            std::pair< bool, Json > prepared = prepareFetch( scene, defaultScale );
            if( prepared.first ) {
                updated++;
                LOGGER_TRACE( "result modified by prepareFetch: " +
                              cc::trace( json_get_str( prepared.second, "_id" ) ) );

                scene = prepared.second;
            }
        }

        return results;
    }
}

IDbClient& ScenesRequestHandler::getDBClient() {
    return m_dbClient;
}

const IDbClient& ScenesRequestHandler::getDBClient() const {
    return m_dbClient;
}

JsonObject setError() {
    JsonObject answer = JSON_EMPTY_OBJ;
    setError( answer, Errors::INTERNAL_OPERATION_ERROR );
    return answer;
}

/** @brief Creates or modifies an expression by name and using parameters passed.
 * @param message - object {name: string, code: string, params: object}, params = { items: object[], device_item_names[]
 * } */
JsonObject ScenesRequestHandler::expressionSet( const JsonObject& message ) {
    try {
        Json variableObj;
        bool hasVariable = json_get( message, "variable", variableObj );
        bool variable = json_get_bool( message, "variable" );

        bool setVariable = json_get_bool( message, "setVariable" );
        bool updateExpression = json_get_bool( message, "updateExpression" );

        Json value;
        bool hasValue = json_get( message, "value", value );

        auto code = json_get_str( message, "code" );

        if( updateExpression ) {
            JsonObject answer = JSON_EMPTY_OBJ;
            std::string name = json_get_str( message, "name" );
            auto prevValue = m_expressions.findByName( name );
            bool updateMode = !prevValue.empty();

            if( !updateMode ) {
                LOGGER_ERROR( "update expression error: expression doesn't exist" );
                setErrorWithField( answer, Errors::SCENES_EXPRESSION_DOES_NOT_EXIST, "", name );
                return answer;
            }

            return doExpressionSet( message );
        }
        else if( setVariable ) {
            return doVariableExpressionSet( message );
        }
        else if( variable == true && code.empty() && hasValue ) {
            JsonObject answer = JSON_EMPTY_OBJ;
            if( !json_get( message, "value", value ) ) {
                LOGGER_ERROR( "Update value is not passed" );
                setError( answer, Errors::SCENES_EXPRESSION_SET_VALUE_ERROR, "Update value is not allowed" );
                return answer;
            }
            return doVariableExpressionSet( message );
        }
        else if( variable == false ) {
            return doExpressionSet( message );
        }
        else {
            LOGGER_ERROR( cc::error( "Expression error: undefined expression type" ) );
            JsonObject answer = JSON_EMPTY_OBJ;
            setError( answer, Errors::WRONG_PARAMS, "Undefined expression type" );
            return answer;
        }
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Expression handling failed: " ) + cc::note( ex.what() ) );
        JsonObject answer = JSON_EMPTY_OBJ;
        setError( answer, Errors::INTERNAL_OPERATION_ERROR, ex.what() );
        return answer;
    }
}

JsonObject ScenesRequestHandler::doExpressionSet( const JsonObject& message ) {
    using namespace CoreD;
    LOGGER_TRACE( cc::j( message ) );

    JsonObject answer = JSON_EMPTY_OBJ;
    std::string name = json_get_str( message, "name" );
    std::string code = json_get_str( message, "code" );

    Json valueTypeObj;
    bool hasValueType = json_get( message, "valueType", valueTypeObj );
    std::string valueType = json_get_str( message, "valueType" );

    Json metadata;
    bool hasMetadata = json_get( message, "metadata", metadata );

    bool variableObj;
    bool hasVariable = json_get( message, "variable", variableObj );
    bool variable = json_get_bool( message, "variable" );

    Json params;
    json_get( message, "params", params );  // optional

    if( name.empty() ) {
        wrongParam( "name", answer );
        return answer;
    }
    if( name.size() >= Limits::MAX_EXPRESSION_NAME_SIZE ) {
        setError( answer,
                  Errors::TOO_LONG,
                  std::string{ "Field 'name' is too long:. Length is limited to " +
                               std::to_string( Limits::MAX_EXPRESSION_NAME_SIZE ) } );
        return answer;
    }
    if( code.empty() ) {
        wrongParam( "code", answer );
        return answer;
    }
    if( code.size() >= Limits::MAX_EXPRESSION_CODE_SIZE ) {
        setError( answer,
                  Errors::TOO_LONG,
                  std::string{ "Field 'code' is too long:. Length is limited to " +
                               std::to_string( Limits::MAX_EXPRESSION_CODE_SIZE ) } );
        return answer;
    }

    SceneEngine::Expressions::ItemsIndex itemsIndex;
    SceneEngine::Expressions::TypedExpressionUPtr expressionPtr(
        std::make_unique< SceneEngine::Expressions::TypedExpression >(
            m_expressions, name, code, params, valueType, metadata, variable ) );
    if( !expressionPtr->indexItems( answer, itemsIndex ) ) {
        return answer;
    }

    // fetch expression item ids and save it in expressions (expr.name -> itemIds[]) map
    if( !itemsIndex.empty() ) {
        itemsIndex.fetchAllItemIds(
            [&expressionPtr]( const std::string& itemId ) { expressionPtr->addItemId( itemId ); } );
    }

    bool isOk = false;
    bool isExpressionCalculateError = false;

    Json result;
    std::tie( isOk, result ) = m_expressions.calculateExpression( answer, code, itemsIndex );
    JsonArrayBuilder valueErrorArr;

    auto prevValue = m_expressions.findByName( name );
    bool updateMode = !prevValue.empty();

    if( hasValueType && !valueType.empty() ) {
        auto valueTypeEnum = CoreD::Model::ValueType::fromString( valueType );
        try {
            Model::ValueFactory::createItemValue( valueTypeEnum, result );
        }
        catch( const CoreD::Model::UnexpectedJsonValueFormatError& ex ) {
            setError( answer, Errors::SCENES_EXPRESSION_TYPE_MISMATCH, ex.what() );
            isExpressionCalculateError = true;
        }
        catch( const CoreD::Model::UnexpectedValueType& ex ) {
            setError( answer, Errors::SCENES_ITEM_VALUE_TYPE_ERROR, ex.what() );
            isExpressionCalculateError = true;
        }
        catch( const CoreD::Model::InvalidValueScale& ex ) {
            setError( answer, Errors::INVALID_VALUE_SCALE, ex.what() );
            isExpressionCalculateError = true;
        }
        catch( const CoreD::Model::ScalesNotFoundError& ex ) {
            setError( answer, Errors::SCALE_NOT_FOUND, ex.what() );
            isExpressionCalculateError = true;
        }
        catch( const CoreD::Model::CoreDModelError& ex ) {
            setError( answer, Errors::SCENES_ITEM_VALUE_TYPE_ERROR, ex.what() );
            isExpressionCalculateError = true;
        }
        catch( const std::exception& ex ) {
            setError( answer, Errors::SCENES_ITEM_VALUE_TYPE_ERROR, ex.what() );
            isExpressionCalculateError = true;
        }
        catch( ... ) {
            setError( answer, Errors::SCENES_EXPRESSION_SET_VALUE_ERROR, "Other error" );
            isExpressionCalculateError = true;
        }

        if( hasVariable && variable && !params.empty() ) {
            setError( answer,
                      Errors::SCENES_EXPRESSION_SET_VALUE_ERROR,
                      "Set expression is not allowed because parameters is not empty" );
        }

        if( !valueErrorArr.empty() ) {
            expressionPtr->setValueError( valueErrorArr );
        }

        if( answer.find( Errors::ERROR_FIELD ) != answer.end() ) {
            return answer;
        }
    }

    if( updateMode ) {
        if( isOk && !isExpressionCalculateError ) {
            bool savePreviousParams = false;
            const auto SaveParamsKey = "save_previous_params";
            if( json_get( message, SaveParamsKey, valueTypeObj ) ) {
                savePreviousParams = json_get_bool( message, SaveParamsKey, valueTypeObj );
            }

            if( savePreviousParams ) {
                params = prevValue[ "params" ];
            }

            expressionPtr->value( result );
            m_expressions.setExpression( name, std::move( expressionPtr ) );

            Json prevValueTypeValue;
            bool hasPrevValueType = json_get( prevValue, "valueType", prevValueTypeValue );
            Json prevPrevMetadataValue;
            bool hasPrevMetadata = json_get( prevValue, "metadata", prevPrevMetadataValue );
            Json prevVariable;
            bool hasPrevVariable = json_get( prevValue, "variable", prevVariable );

            bool isChanged = prevValue[ "code" ] != code || prevValue[ "params" ] != params;
            bool isValueTypeChanged =
                ( hasValueType && hasPrevValueType && json_get_str( prevValue, "valueType" ) != valueType ) ||
                hasValueType != hasPrevValueType;
            Json prevMetadata;
            bool isMetadataChanged = ( hasMetadata && hasPrevMetadata &&
                                       json_get( prevValue, "metadata", prevMetadata ) && prevMetadata != metadata ) ||
                                     hasMetadata != hasPrevMetadata;
            bool isVariableChanged =
                ( hasVariable && hasPrevVariable && json_get_bool( prevValue, "variable" ) != variable ) ||
                hasVariable != hasPrevVariable;

            if( isChanged || isValueTypeChanged || isMetadataChanged || isVariableChanged ) {
                if( m_sendBroadcast != nullptr ) {
                    auto broadcast = JSON( "name" << name << "code" << code );
                    if( hasVariable && isVariableChanged ) {
                        broadcast[ "variable" ] = variable;
                    }
                    if( hasValueType && isValueTypeChanged ) {
                        broadcast[ "valueType" ] = valueType;
                    }
                    if( hasMetadata && isMetadataChanged ) {
                        broadcast[ "metadata" ] = metadata;
                    }

                    m_sendBroadcast( SCENE_EXPRESSION_CHANGED, JSON( broadcast ) );
                }
            }
            LOGGER_TRACE( cc::trace( "Storing expression '" ) + cc::notice( name ) + cc::trace( "' value: " ) +
                          cc::j( result ) );
        }
        else {
            std::string errorMessage = result.is_object() ? json_get_str( result, "error" ) : json_get_str( result );
            LOGGER_ERROR( cc::error( "Expression evaluation failed for '" + name + "', error description: " ) +
                          cc::note( errorMessage ) );
            expressionPtr->error( errorMessage );
            setErrorWithField( answer, Errors::WRONG_PARAMS, errorMessage, name );
        }
    }
    else {
        if( isExpressionCalculateError ) {
            LOGGER_ERROR( cc::error( "Could not create an expression: " ) + cc::j( answer ) );
        }
        else if( isOk ) {
            expressionPtr->value( result );
            m_expressions.setExpression( name, std::move( expressionPtr ) );

            auto broadcast = JSON( "name" << name << "code" << code );

            if( hasVariable ) {
                broadcast[ "variable" ] = variable;
            }
            if( hasValueType && !valueType.empty() ) {
                broadcast[ "valueType" ] = valueType;
            }
            if( hasMetadata && !metadata.empty() ) {
                broadcast[ "metadata" ] = metadata;
            }

            if( m_sendBroadcast != nullptr ) {
                m_sendBroadcast( SCENE_EXPRESSION_ADDED, broadcast );
            }

            LOGGER_TRACE( cc::trace( "Storing expression '" ) + cc::notice( name ) + cc::trace( "' value: " ) +
                          cc::j( result ) );
        }
        else {
            std::string errorMessage = result.is_object() ? json_get_str( result, "error" ) : json_get_str( result );
            LOGGER_ERROR( cc::error( "Expression evaluation failed for '" + name + "', error description: " ) +
                          cc::note( errorMessage ) );
            setErrorWithField( answer, Errors::WRONG_PARAMS, errorMessage, name );
        }
    }

    return answer;
}

bool checkName( const std::string& name, Json& answer ) {
    if( name.size() >= Limits::MAX_EXPRESSION_NAME_SIZE ) {
        setErrorWithField( answer,
                           Errors::TOO_LONG,
                           std::string{ "Field is too long: name. Length is limited to " +
                                        std::to_string( Limits::MAX_EXPRESSION_NAME_SIZE ) },
                           name );
        return true;
    }

    return false;
}

JsonObject ScenesRequestHandler::doVariableExpressionSet( const JsonObject& message ) {
    using namespace CoreD;
    LOGGER_TRACE( cc::j( message ) );
    JsonObject answer = JSON_EMPTY_OBJ;
    std::string name = json_get_str( message, "name" );

    Json valueTypeObj;
    bool hasValueType = json_get( message, "valueType", valueTypeObj );
    std::string newValueType = json_get_str( message, "valueType" );

    Json newMetadata;
    bool hasMetadata = json_get( message, "metadata", newMetadata );

    Json value;
    bool hasValue = json_get( message, "value", value );
    json_get( message, "value", value );

    Json error;
    bool hasError = json_get( message, "error", error );
    json_get( message, "error", error );

    Json params;
    json_get( message, "params", params );  // optional

    Json variableObj;
    bool hasVariable = json_get( message, "variable", variableObj );
    bool variable = json_get_bool( message, "variable" );
    bool setVariable = json_get_bool( message, "setVariable" );

    if( name.empty() ) {
        wrongParam( "name", answer );
        return answer;
    }
    if( checkName( name, answer ) ) {
        return answer;
    }

    auto prevValue = m_expressions.findByName( name );
    bool updateMode = !prevValue.empty();

    if( setVariable ) {
        if( !updateMode ) {
            LOGGER_ERROR( "setVariable action error: Previous variable doesn't exist" );
            setError( answer, Errors::SCENES_EXPRESSION_DOES_NOT_EXIST, "Previous variable doesn't exist" );
            return answer;
        }

        variable = true;
        bool hasPrevVariable = prevValue.count( "variable" );
        if( !hasPrevVariable || ( hasPrevVariable && json_get_bool( prevValue, "variable" ) == false ) ) {
            LOGGER_ERROR( "Set variable error: previous 'variable' is false" );
            setError( answer, Errors::SCENES_EXPRESSION_SET_VALUE_ERROR, "Previous 'variable' is false" );
            return answer;
        }
    }

    Json prevValueTypeValue;
    bool hasPrevValueType = json_get( prevValue, "valueType", prevValueTypeValue );

    auto prevValueTypeString = json_get_str( prevValueTypeValue );
    Json prevMetadata;
    json_get( prevValue, "metadata", prevMetadata );

    auto valueType = setVariable && hasError ? prevValueTypeString : newValueType;
    auto metadata = setVariable && hasError ? prevMetadata : newMetadata;

    if( setVariable && hasError ) {
        hasValueType = false;
        hasMetadata = false;
    }

    SceneEngine::Expressions::TypedVariableExpressionUPtr expressionPtr(
        std::make_unique< SceneEngine::Expressions::TypedVariableExpression >(
            m_expressions, name, value, params, valueType, metadata, variable, error ) );

    if( setVariable && !updateMode ) {
        LOGGER_ERROR( "setVariable action error: Previous variable doesn't exist" );
        setError( answer, Errors::SCENES_EXPRESSION_DOES_NOT_EXIST, "Previous variable doesn't exist" );
    }

    using namespace CoreD::Model;

    if( hasValueType && !valueType.empty() ) {
        auto valueTypeEnum = ValueType::fromString( valueType );
        try {
            if( valueTypeEnum == ValueType::Enum::Dictionary ) {
                auto dictionaryString = ValueType::makeDictionaryType( ValueType::Enum::String );
                Model::ValueFactory::createItemValue( dictionaryString, value );
            }
            else {
                Model::ValueFactory::createItemValue( valueTypeEnum, value );
            }
        }
        catch( const CoreD::Model::UnexpectedJsonValueFormatError& ex ) {
            LOGGER_ERROR( "Expression value type mismatch error: " + ex.what() );
            setError( answer, Errors::SCENES_EXPRESSION_TYPE_MISMATCH, ex.what() );
            return answer;
        }
        catch( const CoreD::Model::UnexpectedValueType& ex ) {
            LOGGER_ERROR( "Expression item value type error: " + ex.what() );
            setError( answer, Errors::SCENES_ITEM_VALUE_TYPE_ERROR, ex.what() );
            return answer;
        }
        catch( const CoreD::Model::InvalidValueScale& ex ) {
            setError( answer, Errors::INVALID_VALUE_SCALE, ex.what() );
            return answer;
        }
        catch( const CoreD::Model::ScalesNotFoundError& ex ) {
            setError( answer, Errors::SCALE_NOT_FOUND, ex.what() );
            return answer;
        }
        catch( const CoreD::Model::CoreDModelError& ex ) {
            setError( answer, Errors::SCENES_ITEM_VALUE_TYPE_ERROR, ex.what() );
            return answer;
        }
        catch( const std::exception& ex ) {
            LOGGER_ERROR( "Expression value error: " + ex.what() );
            setError( answer, Errors::SCENES_EXPRESSION_SET_VALUE_ERROR, ex.what() );
            return answer;
        }

        catch( ... ) {
            LOGGER_ERROR( "Expression value other error" );
            setError( answer, Errors::SCENES_EXPRESSION_SET_VALUE_ERROR, "Other error" );
            return answer;
        }
    }

    if( updateMode ) {
        Json empty = JSON_EMPTY_OBJ;

        expressionUpdate( name,
                          hasValueType ? &valueType : nullptr,
                          hasMetadata ? &metadata : nullptr,
                          hasValue ? &value : &empty,
                          hasVariable ? &variable : nullptr,
                          setVariable,
                          prevValue,
                          expressionPtr,
                          hasError ? &error : &empty );
    }
    else {
        expressionPtr->value( value );
        m_expressions.setVariableExpression( name, std::move( expressionPtr ) );

        auto broadcast = JSON( "name" << name << "value" << value );

        broadcast[ "variable" ] = true;

        if( hasValueType && !valueType.empty() ) {
            broadcast[ "valueType" ] = valueType;
        }
        if( hasMetadata && !metadata.empty() ) {
            broadcast[ "metadata" ] = metadata;
        }
        if( hasValue && !value.empty() ) {
            broadcast[ "value" ] = value;
        }

        if( m_sendBroadcast != nullptr ) {
            m_sendBroadcast( SCENE_EXPRESSION_ADDED, broadcast );
        }

        LOGGER_TRACE( cc::trace( "Storing expression '" ) + cc::notice( name ) + cc::trace( " value: " ) +
                      cc::j( value ) );
    }

    return answer;
}

void ScenesRequestHandler::expressionUpdate( const std::string& name,
                                             const std::string* valueType,
                                             const Json* metadata,
                                             const Json* value,
                                             const bool* variable,
                                             bool setVariable,
                                             const Json& prevValue,
                                             SceneEngine::Expressions::TypedVariableExpressionUPtr& expressionPtr,
                                             const Json* error ) {
    bool hasPrevVariable = prevValue.count( "variable" );
    bool hasValue = value != nullptr;
    bool hasError = error != nullptr;
    if( setVariable == false ||
        ( setVariable == true && hasPrevVariable && json_get_bool( prevValue, "variable" ) == true ) ) {
        if( hasValue )
            expressionPtr->value( *value );
        m_expressions.setVariableExpression( name, std::move( expressionPtr ) );
        Json prevValueTypeValue;
        bool hasPrevValueType = json_get( prevValue, "valueType", prevValueTypeValue );
        Json prevPrevMetadataValue;
        bool hasPrevMetadata = json_get( prevValue, "metadata", prevPrevMetadataValue );
        bool isValueChanged = hasValue && prevValue[ "value" ] != *value;
        bool isErrorChanged = hasError && prevValue[ "error" ] != *error;
        bool hasValueType = valueType != nullptr;
        bool isValueTypeChanged =
            ( hasValueType && hasPrevValueType && json_get_str( prevValueTypeValue ) != *valueType ) ||
            hasValueType != hasPrevValueType;
        Json prevMetadata;
        bool hasMetadata = metadata != nullptr;
        bool isMetadataChanged = ( hasMetadata && hasPrevMetadata && json_get( prevValue, "metadata", prevMetadata ) &&
                                   prevMetadata != *metadata ) ||
                                 hasMetadata != hasPrevMetadata;

        bool hasVariable = variable != nullptr;
        bool isVariableChanged =
            ( hasVariable && hasPrevVariable && json_get_bool( prevValue, "variable" ) != *variable ) ||
            hasVariable != hasPrevVariable;

        if( isValueChanged || isValueTypeChanged || isMetadataChanged || isVariableChanged || isErrorChanged ) {
            sendChangedBroadcast( name,
                                  ( isValueTypeChanged ) ? valueType : nullptr,
                                  ( isMetadataChanged ) ? metadata : nullptr,
                                  ( isValueChanged ) ? value : nullptr,
                                  ( isVariableChanged ) ? variable : nullptr,
                                  ( isErrorChanged ) ? error : nullptr );
        }
        LOGGER_TRACE( cc::trace( "Storing expression '" ) + cc::notice( name ) + cc::trace( "' value: " ) +
                      cc::j( ( hasValue ) ? *value : JSON_EMPTY_OBJ ) );
    }
}

void ScenesRequestHandler::sendChangedBroadcast( const std::string& name,
                                                 const std::string* valueType,
                                                 const Json* metadata,
                                                 const Json* value,
                                                 const bool* variable,
                                                 const Json* error ) {
    bool hasChanges = false;
    if( m_sendBroadcast != nullptr ) {
        auto broadcast = JSON( "name" << name );
        if( variable != nullptr ) {
            hasChanges = true;
            broadcast[ "variable" ] = *variable;
        }
        if( valueType != nullptr ) {
            hasChanges = true;
            broadcast[ "valueType" ] = *valueType;
        }
        if( metadata != nullptr ) {
            hasChanges = true;
            broadcast[ "metadata" ] = *metadata;
        }
        if( value != nullptr ) {
            hasChanges = true;
            broadcast[ "value" ] = *value;
        }

        if( error != nullptr ) {
            hasChanges = true;
            broadcast[ "error" ] = *error;
        }

        if( hasChanges ) {
            m_sendBroadcast( SCENE_EXPRESSION_CHANGED, broadcast );
        }
    }
}

JsonObject ScenesRequestHandler::expressionDelete( const JsonObject& message ) {
    JsonObject answer = JSON_EMPTY_OBJ;
    Json name = json_get_str( message, "name" );
    if( name.empty() ) {
        wrongParam( "name", answer );
        return answer;
    }
    LOGGER_TRACE( cc::j( message ) );

    if( m_expressions.hasExpression( name ) ) {
        m_expressions.removeExpression( name );

        if( m_sendBroadcast != nullptr ) {
            m_sendBroadcast( SCENE_EXPRESSION_DELETED, JSON( "name" << name ) );
        }
    }
    else {
        setErrorWithField( answer, Errors::DOES_NOT_EXIST, "", name );
    }
    return answer;
}

JsonObject ScenesRequestHandler::expressionList( const JsonObject& message ) {
    using FetchResult = SceneEngine::Expressions::ExpressionManager::FetchResult;

    JsonObject answer = JSON_EMPTY_OBJ;
    bool showCode = json_get_bool( message, "showCode", false );
    bool strict = json_get_bool( message, "strict", false );
    std::vector< std::string > filterTypes;

    Json variablesRef;
    bool hasVariableFilter = false;
    bool variableFilterValue = false;

    if( json_get( message, "hasVariable", variablesRef ) ) {
        hasVariableFilter = json_get_bool( message, "hasVariable" );
    }

    if( hasVariableFilter ) {
        variableFilterValue = json_get_bool( message, "variable" );
    }

    Json filterTypeRef;

    if( json_get( message, "filterType", filterTypeRef ) ) {
        auto filterType = json_get_str( filterTypeRef );
        if( !filterType.empty() )
            filterTypes.push_back( filterType );
    }

    if( json_get( message, "filterTypes", filterTypeRef ) ) {
        JsonArray filterTypesArr;
        if( json_get_arr( message, "filterTypes", filterTypesArr ) ) {
            for( const auto& filterItem : filterTypesArr ) {
                auto filterType = json_get_str( filterItem );
                if( !filterType.empty() )
                    filterTypes.push_back( filterType );
            }
        }
    }

    std::set< std::string > names;
    if( message.find( "names" ) != message.end() ) {
        JsonArray namesArr;
        if( json_get_arr( message, "names", namesArr ) ) {
            for( const auto& nameItem : namesArr ) {
                auto name = json_get_str( nameItem );
                if( !name.empty() ) {
                    names.insert( std::move( name ) );
                }
            }
        }
    }

    bool showValue = json_get_bool( message, "showValue", true );

    Json expressions = JSON_EMPTY_ARR;
    auto resultSet = m_expressions.findExpressionsBy( JSON_EMPTY_OBJ );
    LOGGER_TRACE( cc::j( resultSet ) );
    for( const auto& expression : resultSet ) {
        Json entry = JSON_EMPTY_OBJ;
        entry[ "params" ] = JSON_EMPTY_OBJ;

        std::string expressionName = json_get_str( expression, db::ID_FIELD_NAME );

        if( names.size() && !names.count( expressionName ) ) {
            continue;
        }

        entry[ "name" ] = expressionName;

        auto expressionVariable = false;
        Json variableObj;

        if( json_get( expression, "variable", variableObj ) ) {
            expressionVariable = json_get_bool( expression, "variable" );
        }

        showCode = !expressionVariable;

        Json params;
        if( !expressionVariable && json_get( expression, "params", params ) ) {
            entry[ "params" ] = params;
        }

        if( showCode ) {
            entry[ "code" ] = json_get_str( expression, "code" );
        }

        auto valueType = json_get_str( expression, "valueType" );

        if( !valueType.empty() ) {
            if( filterTypes.empty() ) {
                Json valueTypeObj;
                if( json_get( expression, "valueType", valueTypeObj ) ) {
                    entry[ "valueType" ] = valueType;
                }
            }
            else {
                bool checkType = false;
                for( const auto& type : filterTypes ) {
                    if( type == valueType ) {
                        entry[ "valueType" ] = valueType;
                        checkType = true;
                        break;
                    }
                }
                if( !checkType ) {
                    continue;
                }
            }
        }
        else {
            if( !filterTypes.empty() ) {
                continue;
            }
        }

        if( hasVariableFilter ) {
            if( expressionVariable == variableFilterValue ) {
                entry[ "variable" ] = expressionVariable;
            }
            else {
                continue;
            }
        }
        else {
            entry[ "variable" ] = expressionVariable;
        }

        Json metadata;
        if( json_get( expression, "metadata", metadata ) ) {
            entry[ "metadata" ] = metadata;
        }

        auto fetchResult = m_expressions.fetchValue( expressionName );

        if( !fetchResult.isOk() ) {
            entry[ "error" ] = fetchResult.error;
        }

        if( fetchResult.hasValue && showValue ) {
            entry[ "value" ] = std::move( fetchResult.value );
        }
        else {
            LOGGER_WARN( cc::warn( "No value found for expression name = " ) + cc::note( expressionName ) );
        }

        expressions.push_back( entry );
    }
    answer[ "expressions" ] = expressions;
    return answer;
}

SceneEngine::ActionBlocks ScenesRequestHandler::parseActions(
    JsonObject& scene,
    IdUtils::PlaceholdersMapping& placeholdersMapping,
    const SceneEngine::EventContainer::ActionGroupMappings& mappings ) {
    using Block = ActionParserInterface::BlockType;

    const bool hasThenBlock = scene.count( ActionBlockTypes::Then ) != 0;
    const bool hasElseBlock = scene.count( ActionBlockTypes::Else ) != 0;
    if( !hasThenBlock ) {
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_THEN_BLOCK );
    }

    SceneEngine::ActionBlocks blocks;
    if( hasThenBlock ) {
        blocks.m_then = m_actionParser->parse( scene, Block::Then, placeholdersMapping );
        for( const auto& action : blocks.m_then ) {
            action->setBlockType( SceneEngine::RunSceneActionBlockType::Then );
        }
    }
    if( hasElseBlock ) {
        blocks.m_else = m_actionParser->parse( scene, Block::Else, placeholdersMapping );
        for( const auto& action : blocks.m_then ) {
            action->setBlockType( SceneEngine::RunSceneActionBlockType::Else );
        }
    }
    // Transforms a dictionary of action groups from <groupID:groupObj> to <whenBlockID:groupObj>
    auto remapActionGroupsToWhenBlockIDs = [&mappings, &scene]( const std::string& blockName ) {
        auto groupDictIter = scene.find( blockName );
        if( groupDictIter == scene.end() ) {
            return false;
        }
        auto result = JSON_EMPTY_OBJ;
        for( auto& mappingEntry : mappings ) {
            auto& groupID = mappingEntry.first;
            auto& whenBlockID = mappingEntry.second;

            auto groupIter = groupDictIter->find( groupID );
            if( groupIter != groupDictIter->end() ) {
                result[ whenBlockID ] = std::move( *groupIter );
                groupDictIter->erase( groupIter );
            }
        }
        if( !groupDictIter->empty() ) {
            throw SceneEngine::SceneException( Errors::SCENE_ACTION_GROUP_UNUSED );
        }
        *groupDictIter = std::move( result );
        return true;
    };
    if( remapActionGroupsToWhenBlockIDs( ActionBlockTypes::ThenGroups ) ) {
        blocks.m_thenGroups = m_actionParser->parseGroups( scene, Block::Then, placeholdersMapping );
        // Todo: add action block type info to action
    }
    if( remapActionGroupsToWhenBlockIDs( ActionBlockTypes::ElseGroups ) ) {
        blocks.m_elseGroups = m_actionParser->parseGroups( scene, Block::Else, placeholdersMapping );
    }
    return blocks;
}

JsonObject ScenesRequestHandler::processBlockStatusReset( const JsonObject& params ) {
    Json answer;
    SceneEngine::Scene::SceneID sceneId;
    if( !getField( params, "sceneId", answer, sceneId ) ) {
        return answer;
    }
    if( !m_manager.isSceneExist( sceneId ) ) {
        setError( answer, Errors::SCENE_DOES_NOT_EXIST, sceneId );
        return answer;
    }

    std::string blockId;
    const bool blockIdSpecified = json_get( params, "blockId", blockId );

    try {
        if( blockIdSpecified )
            m_manager.resetLatch( sceneId, blockId );
        else
            m_manager.resetSceneLatches( sceneId );
    }
    catch( const SceneEngine::LatchException& ex ) {
        LOGGER_ERROR( cc::error( "Reset Latch exception: " ) + cc::note( ex.what() ) );
        setError( answer, Errors::SCENE_WRONG_WHEN_BLOCK, "The block is not a Latch" );
    }
    catch( const SceneEngine::SceneException& ex ) {
        LOGGER_ERROR( cc::error( "Reset latch scene exception: " ) + cc::note( ex.what() ) );
        setError( answer, ex.getError(), ex.what() );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Reset latch common exception : " ) + cc::note( ex.what() ) );
        setError( answer, Errors::SCENE_WRONG_WHEN_BLOCK, std::string{ "Common error: " }.append( ex.what() ) );
    }

    return answer;
}

JsonObject ScenesRequestHandler::processScenesStatusGet( const JsonObject& params ) {
    Json answer;

    SceneEngine::Scene::SceneID sceneId;
    if( !getFieldVerbose( params, "sceneId", answer, sceneId ) ) {
        return answer;
    }
    if( !m_manager.isSceneExist( sceneId ) ) {
        setErrorWithField( answer, Errors::SCENE_DOES_NOT_EXIST, "", sceneId );
        return answer;
    }

    return m_manager.buildSceneStatus( sceneId );
}

void ScenesRequestHandler::fillBlocksSources( const IDbClient& dbClient ) {
    const auto blocksSources = dbClient.findAll( HaScenesBlocksDBName, JSON_EMPTY_OBJ, nullptr );
    for( const auto& source : blocksSources ) {
        std::string sourceId = json_get_str( source, "sourceId" );
        m_blocks.setSceneBlockSource( sourceId );
    }
}

void ScenesRequestHandler::saveHubBackupRunCallbackPtr( const ActionInterface::Action& action,
                                                        ActionInterface::ActionExecutorPtr actionExecutorPtr ) {
    m_manager.saveHubBackupRunCallbackPtr( action, actionExecutorPtr );
}

void ScenesRequestHandler::setHubBackupRunState( bool flag ) {
    m_manager.setHubBackupRunState( flag );
}

bool ScenesRequestHandler::checkScenesListParams( const JsonObject& params ) {
    try {
        jsonCheckObject( params, "filters", true );
        jsonCheckObject( params[ "filters" ], "include", true );
        jsonCheckObject( params[ "filters" ][ "include" ], "generator", true );
    }
    catch( const WrongParamsError& ex ) {
        LOGGER_ERROR( cc::error( "not an object field: " ) + cc::note( ex.what() ) );
        return false;
    }
    catch( const WrongFieldError& ex ) {
        LOGGER_ERROR( cc::error( "field not found: " ) + cc::note( ex.what() ) );
        return false;
    }
    try {
        jsonCheckString( params[ "filters" ][ "include" ][ "generator" ], "role", true );
    }
    catch( const WrongParamsError& ex ) {
        LOGGER_ERROR( cc::error( "not an string field: " ) + cc::note( ex.what() ) );
        return false;
    }
    catch( const WrongFieldError& ex ) {
        LOGGER_ERROR( cc::error( "field not found: " ) + cc::note( ex.what() ) );
        return false;
    }
    return true;
}

}  // namespace ScenesManager
}  // namespace HubPlatform
