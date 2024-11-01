#pragma once

#include "scenes_manager/scenes_engine/operators/scene_operator.h"
#include "scenes_manager/scenes_engine/actions/iscene_action_execute.h"
#include "scenes_manager/scenes_engine/value_source_key.h"
#include <string>

namespace SceneEngine {

class EventBase {
public:
    using EventId = std::string;
    using ValueKeys = std::vector< std::reference_wrapper< const ValueSourceKey > >;

    EventBase();

    virtual ~EventBase() = default;
    virtual std::string getMethodName() const = 0;
    virtual ValueKeys getValueKeys() const;

    void setEventId( const EventId& eventId );
    const EventId& getEventId() const;
    void setBlockName( std::string name );
    const std::string& getBlockName() const;

    const Scene& getScene() const;
    Scene::SceneID getSceneId() const;
    void setSceneActionExecutor( SceneActionExecutePtr actionExecutor );
    bool hasSceneActionExecutor() const;
    void setOperator( AbstractOperator::SharedOperator parentOperatorPtr );
    AbstractOperator::SharedOperator getOperator() const;

    /// @brief If event is disabled then it is TRUE all the time.
    /// State setting does not change anything.
    void setEnabled( bool enabled );

    // new API of triggering events
    void setOperatorState( bool state );
    void notifyOperatorState();
    AbstractOperator::OperatorState evaluateSubtree();

    // legacy API, should be removed when all events will be re-written to new approach
    bool runActionsIfSucceededCondition( bool state );
    bool runActionsIfTriggeredCondition();

private:
    void triggerOperatorAndCheckCondition();
    void setOperatorAndCheckCondition( bool state );

private:
    EventId m_eventId;
    std::shared_ptr< EventOperator > m_eventOperatorPtr;
    SceneActionExecutePtr m_actionExecutorPtr;
};

using EventBasePtr = std::shared_ptr< EventBase >;

}  // namespace SceneEngine
