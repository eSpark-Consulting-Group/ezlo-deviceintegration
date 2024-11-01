#pragma once

#include "scenes_manager/scenes_engine/operators/visitors/operator_visitor.h"
#include "scenes_manager/scenes_engine/events/event_base.h"
#include "scenes_manager/scenes_engine/states_registry.h"

#include "json_utils/json_wrapper.h"

#include <string>
#include <set>

namespace SceneEngine {

namespace UT {
class SceneStatusGetRequestTest;
}  // namespace UT

class OperatorStatusVisitor : public OperatorVisitor {
public:
    using EventMap = std::map< std::string, EventBasePtr >;

    OperatorStatusVisitor( Scene::SceneID sceneId, const EventMap& sceneEvents, const StatesRegistry& statesRegistry );

    const Json& getStatus() const;

    void visit( EventOperator& op ) override;
    void visit( NotOperator& op ) override;
    void visit( MultipleChildrenOperator& op ) override;
    void visit( FunctionOperator& op ) override {}
    void visit( OrOperator& op ) override;
    void visit( XorOperator& op ) override;
    void visit( AndOperator& op ) override;
    void visit( ForLessFunctionOperator& op ) override;
    void visit( ForAtLeastFunctionOperator& op ) override;
    void visit( RepeatFunctionOperator& op ) override;
    void visit( FollowFunctionOperator& op ) override;
    void visit( PulseFunctionOperator& op ) override;
    void visit( LatchFunctionOperator& op ) override;

private:
    bool markOperatorVisited( const AbstractOperator& op );

    Json getOperatorStatus( AbstractOperator& op );
    Json collectChildrenStatus( MultipleChildrenOperator& op );

    static Json createStatusJson( const std::string& id,
                                  const std::string& name,
                                  const std::string& blockName,
                                  const StatesRegistry::Record& registryRecord );

private:
    Scene::SceneID m_sceneId;
    const EventMap& m_sceneEvents;
    const StatesRegistry& m_statesRegistry;

    Json m_fullStatus;
    Json* m_parent;

    std::set< std::string > m_visitedOperators;

    friend class UT::SceneStatusGetRequestTest;
};

}  // namespace SceneEngine