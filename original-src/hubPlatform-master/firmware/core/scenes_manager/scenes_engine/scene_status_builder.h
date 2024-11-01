#pragma once

#include "scenes_manager/scenes_engine/scene.h"
#include "scenes_manager/scenes_engine/operators/visitors/operator_status_visitor.h"

namespace SceneEngine {

class StatesRegistry;

class SceneStatusBuilder {
public:
    using EventMap = OperatorStatusVisitor::EventMap;

    explicit SceneStatusBuilder( const StatesRegistry& statesRegistry );

    Json build( const Scene::SceneID& sceneId,
                const EventMap& triggerEvents,
                const EventMap& exceptionEvents,
                const EventMap& globalRestrictionEvents,
                const Json& sceneErrorInfo );

private:
    Json buildEventsStatus( const Scene::SceneID& sceneId, const EventMap& sceneEvents ) const;

    const StatesRegistry& m_statesRegistry;
};

}  // namespace SceneEngine
