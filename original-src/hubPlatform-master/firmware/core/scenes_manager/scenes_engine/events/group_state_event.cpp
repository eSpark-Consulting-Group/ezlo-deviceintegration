#include "scenes_manager/scenes_engine/events/group_state_event.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

namespace SceneEngine {

GroupStateEvent::GroupStateEvent( std::string otherSceneId,
                                  std::string groupId,
                                  SceneEngine::GroupStateEvent::State state )
: m_sceneId( std::move( otherSceneId ) )
, m_groupId( std::move( groupId ) )
, m_state( state ) {}

std::string GroupStateEvent::getMethodName() const {
    return Model::Resources::MethodNames::isGroupState;
}

const std::string& GroupStateEvent::getSceneIdArg() const {
    return m_sceneId;
}

const std::string& GroupStateEvent::getGroupIdArg() const {
    return m_groupId;
}

bool GroupStateEvent::isReferencedBlock( const std::string& sceneId, const std::string& groupId ) const {
    return m_sceneId == sceneId && m_groupId == groupId;
}

bool GroupStateEvent::compare( AbstractOperator::OperatorState previousState,
                               AbstractOperator::OperatorState newState ) const {
    return m_state == State::True && newState == AbstractOperator::OperatorState::True ||
           m_state == State::False && newState == AbstractOperator::OperatorState::False ||
           m_state == State::Changed && newState != previousState;
}

}  // namespace SceneEngine
