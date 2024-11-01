#pragma once

#include "scenes_manager/scenes_engine/events/event_base.h"

namespace SceneEngine {

/**
 * Execute scene if another scene's when block changes its state (true/false/changed)
 */
class GroupStateEvent : public EventBase {
public:
    enum class State { True, False, Changed };

    GroupStateEvent( std::string otherSceneId, std::string groupId, State state );

    std::string getMethodName() const override;
    const std::string& getSceneIdArg() const;
    const std::string& getGroupIdArg() const;

    bool isReferencedBlock( const std::string& sceneId, const std::string& groupId ) const;
    bool compare( AbstractOperator::OperatorState previousState, AbstractOperator::OperatorState newState ) const;

private:
    std::string m_sceneId;
    std::string m_groupId;
    State m_state;
};

using GroupStateEventPtr = std::shared_ptr< GroupStateEvent >;

}  // namespace SceneEngine
