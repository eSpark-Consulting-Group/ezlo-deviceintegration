#include "scenes_manager/scenes_engine/scene_status_builder.h"
#include "scenes_manager/scenes_engine/scene_status_resources.h"

#include "base_utils/timeutils.h"

namespace SceneEngine {

SceneStatusBuilder::SceneStatusBuilder( const StatesRegistry& statesRegistry )
: m_statesRegistry( statesRegistry ) {}

Json SceneStatusBuilder::build( const Scene::SceneID& sceneId,
                                const EventMap& triggerEvents,
                                const EventMap& exceptionEvents,
                                const EventMap& globalRestrictionEvents,
                                const Json& sceneErrorInfo ) {
    using namespace SceneStatusFields;

    Json sceneStatus;

    const auto state = m_statesRegistry.readSceneState( sceneId );
    sceneStatus[ WhenBlocks ] = buildEventsStatus( sceneId, triggerEvents );
    sceneStatus[ Exceptions ] = buildEventsStatus( sceneId, exceptionEvents );
    sceneStatus[ GlobalRestriction ] = buildEventsStatus( sceneId, globalRestrictionEvents );

    if( state.isValid )
        sceneStatus[ State ] = operatorStateToString( state.state );
    else
        sceneStatus[ State ] = UnknownState;

    if( state.lastChangeTime != StatesRegistry::TimePoint() )
        sceneStatus[ LastChange ] = TimeUtils::chronoTimePointToString( state.lastChangeTime );
    else
        sceneStatus[ LastChange ] = Json();

    if( !sceneErrorInfo.empty() )
        sceneStatus[ Error ] = sceneErrorInfo;

    Json result;
    result[ Id ] = sceneId;
    result[ Status ] = std::move( sceneStatus );
    return result;
}

Json SceneStatusBuilder::buildEventsStatus( const Scene::SceneID& sceneId, const EventMap& sceneEvents ) const {
    OperatorStatusVisitor visitor( sceneId, sceneEvents, m_statesRegistry );

    for( const auto& eventData : sceneEvents ) {
        const EventBasePtr& event = eventData.second;
        AbstractOperator::SharedOperator op = event->getOperator();
        while( true ) {
            AbstractOperator::SharedOperator parent = op->getParent();
            if( !parent )
                break;
            op = parent;
        }
        op->accept( visitor );
    }

    return visitor.getStatus();
}

}  // namespace SceneEngine
