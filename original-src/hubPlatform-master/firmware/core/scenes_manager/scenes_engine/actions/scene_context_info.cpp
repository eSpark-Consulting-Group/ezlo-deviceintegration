#include "scenes_manager/scenes_engine/actions/scene_context_info.h"
#include "log_utils/ezlo_log.h"
#include "database_client/dbid.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

namespace SceneEngine {

using namespace SceneEngine::Model::Resources;

SceneExecutionContext::SceneExecutionContext( const std::string& id,
                                              const std::string& sceneId,
                                              int64_t timestamp,
                                              IDbClient& dbClient,
                                              const JsonObject& message )
: m_id( id )
, m_sceneId( sceneId )
, m_timestamp( timestamp )
, m_dbClient( dbClient )
, m_message( message ) {
    createDbItem();
}

void SceneExecutionContext::remove() {
    auto contexts = m_dbClient.findAll( HaTempDBSceneExecutionContextDBName, JSON_EMPTY_OBJ, nullptr );
    m_dbClient.remove( HaTempDBSceneExecutionContextDBName, JSON( db::ID_FIELD_NAME << m_id ) );
}

std::string SceneExecutionContext::getSceneId() const {
    return m_sceneId;
}

void SceneExecutionContext::setSceneId( const std::string& sceneId ) {
    m_sceneId = sceneId;
}

JsonObject SceneExecutionContext::getMessage() const {
    return m_message;
}

int64_t SceneExecutionContext::getTimestamp() const {
    return m_timestamp;
}

void SceneExecutionContext::addActionInfo( SingleSceneContextPtr actionInfo ) {
    if( actionInfo ) {
        m_singleActionInfos[ std::to_string( actionInfo->getId() ) ] = actionInfo;
        updateDbItem();
    }
}

void SceneExecutionContext::removeActionInfo( int actionId ) {
    m_singleActionInfos.erase( std::to_string( actionId ) );
    updateDbItem();
}

SingleSceneContextPtr SceneExecutionContext::getActionInfo( const std::string actionId ) {
    return m_singleActionInfos[ actionId ];
}

std::map< std::string, SingleSceneContextPtr > SceneExecutionContext::getActionInfos() const {
    return m_singleActionInfos;
}

void SceneExecutionContext::createDbItem() {
    auto query = toJson();
    query[ db::ID_FIELD_NAME ] = m_id;
    m_dbClient.insert( HaTempDBSceneExecutionContextDBName, query, true );
}

void SceneExecutionContext::updateDbItem() {
    auto updateObj = toJson();
    updateObj[ db::ID_FIELD_NAME ] = m_id;
    m_dbClient.update( HaTempDBSceneExecutionContextDBName, JSON_EMPTY_OBJ, updateObj );
}

void SceneExecutionContext::loadFromDb() {
    auto records = m_dbClient.findAll( HaTempDBSceneExecutionContextDBName, JSON( "_id" << m_id ), nullptr );
    fromJson( records );
}

void SceneExecutionContext::fromJson( const JsonObject& data ) {
    m_id = json_get_str( data, "_id" );
    m_sceneId = json_get_str( data, "sceneId" );
    m_timestamp = json_get_int( data, "timestamp" );
    json_get_obj( data, "message", m_message );

    JsonArray actions;
    json_get_arr( data, "actions", actions );

    for( auto action : actions ) {
        SingleSceneContextPtr actionInfo = std::make_shared< SingleSceneContext >( m_sceneId, "", 0 );
        auto key = std::to_string( actionInfo->getId() );
        actionInfo->fromJson( action );
        if( m_singleActionInfos.find( key ) == m_singleActionInfos.end() ) {
            m_singleActionInfos[ key ] = actionInfo;
        }
    }
}

JsonObject SceneExecutionContext::toJson() {
    JsonArrayBuilder actions;
    for( auto actionInfo : m_singleActionInfos ) {
        auto actionData = actionInfo.second->toJson();
        actions << actionData;
    }

    // DB record structure:
    // context record: id | sceneId | timestamp | [action]
    // action:  id | sceneId | status | blockId | blockName | taskPosition

    JsonObject res =
        JSON( "timestamp" << m_timestamp << "sceneId" << m_sceneId << "actions" << actions << "message" << m_message );

    return res;
}

void SceneExecutionContext::stopAllActions( RunSceneActionBlockType blockType ) {
    m_stopped = true;
    m_stopBlockType = blockType;
    for( auto actionInfo : m_singleActionInfos ) {
        if( blockType == RunSceneActionBlockType::Empty || actionInfo.second->getBlockType() == blockType ) {
            actionInfo.second->stop();
            actionInfo.second->setStatus( ScenesState::Stopped );
        }
    }
}

bool SceneExecutionContext::isStoppedActions() {
    bool isStopped = false;
    for( auto actionInfo : m_singleActionInfos ) {
        isStopped = actionInfo.second->getStatus() == ScenesState::Stopped;
        if( !isStopped ) {
            return false;
        }
    }

    return isStopped;
}

void SceneExecutionContext::printDB() {
    auto recordsObjs = m_dbClient.findAll( HaTempDBSceneExecutionContextDBName, JSON_EMPTY_OBJ, nullptr );
    int size = recordsObjs.size();
    LOGGER_INFO( "DB size:" + cc::num( size ) );
    for( auto record : recordsObjs ) {
        auto recordStr = record.dump();
        LOGGER_INFO( recordStr );
    }
}

bool SceneExecutionContext::isStopped( RunSceneActionBlockType blockType ) {
    if( m_stopBlockType != RunSceneActionBlockType::Empty ) {
        return m_stopped && m_stopBlockType == blockType;
    }

    return m_stopped;
}

bool SceneExecutionContext::isStopWasSent() {
    return m_stopWasSent;
}

void SceneExecutionContext::stopWasSent( bool stopWasSent ) {
    m_stopWasSent = stopWasSent;
}

}  // namespace SceneEngine
