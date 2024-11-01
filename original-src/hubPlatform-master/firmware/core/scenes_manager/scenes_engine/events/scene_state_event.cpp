#include "scenes_manager/scenes_engine/events/scene_state_event.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

namespace SceneEngine {

SceneStateEvent::SceneStateEvent( Scene::SceneID sceneId, Action action )
: m_sceneId( std::move( sceneId ) )
, m_action( action ) {}

std::string SceneStateEvent::getMethodName() const {
    return Model::Resources::MethodNames::isSceneState;
}

const Scene::SceneID& SceneStateEvent::getActivatingSceneId() const {
    return m_sceneId;
}

SceneStateEvent::CompareResult SceneStateEvent::compare( const Scene::SceneID& sceneId, bool isSceneEnabled ) const {
    if( !isScene( sceneId ) )
        return CompareResult::NotApplicable;

    switch( m_action ) {
    case Action::SceneEnabled:
        return toCompareResult( isSceneEnabled );

    case Action::SceneDisabled:
        return toCompareResult( !isSceneEnabled );

    default:
        return CompareResult::NotApplicable;
    }
}

SceneStateEvent::CompareResult SceneStateEvent::compare( const Scene::SceneID& sceneId, SceneResult result ) const {
    if( !isScene( sceneId ) )
        return CompareResult::NotApplicable;

    switch( m_action ) {
    case Action::AnyResult:
        return CompareResult::True;

    case Action::FailedResult:
        return toCompareResult( result == SceneResult::Failed );

    case Action::FinishedResult:
        return toCompareResult( result == SceneResult::Finished );

    case Action::PartiallyFinishedResult:
        return toCompareResult( result == SceneResult::PartiallyFinished );

    case Action::StoppedResult:
        return toCompareResult( result == SceneResult::Stopped );

    default:
        return CompareResult::NotApplicable;
    }
}

bool SceneStateEvent::isScene( const Scene::SceneID& sceneId ) const {
    return sceneId == m_sceneId;
}

SceneStateEvent::CompareResult SceneStateEvent::toCompareResult( bool value ) {
    return value ? CompareResult::True : CompareResult::False;
}

}  // namespace SceneEngine
