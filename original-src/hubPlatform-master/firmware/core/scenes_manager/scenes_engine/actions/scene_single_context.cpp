#include "scene_context_info.h"
#include "service_interfaces/postponed_task.h"
#include "log_utils/ezlo_log.h"

#include <utility>

namespace SceneEngine {

SingleSceneContext::SingleSceneContext( std::string sceneId, std::string blockName, int actionId )
: m_id( actionId )
, m_sceneId( std::move( sceneId ) )
, m_blockName( std::move( blockName ) )
, m_actionId( actionId ) {}

Json SingleSceneContext::toJson() const {
    JsonObject result = JSON( "id" << m_id << "sceneId" << m_sceneId << "status" << m_status << "blockName"
                                   << m_blockName << "taskPosition" << m_taskPosition << "actionId" << m_actionId
                                   << "blockType" << static_cast< int >( m_blockType ) );
    return result;
}

void SingleSceneContext::fromJson( const JsonObject& data ) {
    m_id = json_get_int( data, "id" );
    m_sceneId = json_get_str( data, "sceneId" );
    m_status = json_get_str( data, "status" );
    m_blockName = json_get_str( data, "blockName" );
    m_taskPosition = json_get_int( data, "taskPosition" );
    m_blockType = static_cast< RunSceneActionBlockType >( json_get_int( data, "blockType" ) );
}

void SingleSceneContext::setId( int id ) {
    m_id = id;
}

int SingleSceneContext::getId() const {
    return m_id;
}

const std::string& SingleSceneContext::getSceneId() {
    return m_sceneId;
}

void SingleSceneContext::setSceneId( const std::string& id ) {
    m_sceneId = id;
}

void SingleSceneContext::setStatus( const std::string& status ) {
    m_status = status;
}

const std::string& SingleSceneContext::getStatus() {
    return m_status;
}

void SingleSceneContext::setCurrentTaskPosition( int position ) {
    m_taskPosition = position;
}

int SingleSceneContext::getCurrentTaskPosition() const {
    return m_taskPosition;
}

void SingleSceneContext::setTimestamp( int64_t timestamp ) {
    m_timestamp = timestamp;
}

int64_t SingleSceneContext::getTimestamp() const {
    return m_timestamp;
}

void SingleSceneContext::setBlockName( const std::string& blockName ) {
    m_blockName = blockName;
}

const std::string& SingleSceneContext::getBlockName() {
    return m_blockName;
}

void SingleSceneContext::setTask( HubPlatform::Services::PostponedTaskPtr& task ) {
    m_task = task;
}

void SingleSceneContext::setBlockType( RunSceneActionBlockType blockType ) {
    m_blockType = blockType;
}

RunSceneActionBlockType SingleSceneContext::getBlockType() const {
    return m_blockType;
}

void SingleSceneContext::stop() {
    if( m_task && m_task.get() ) {
        using namespace HubPlatform::Services;
        m_task->stop( PostponedTask::StoppingMode::NoWaiting );
    }
}

}  // namespace SceneEngine
