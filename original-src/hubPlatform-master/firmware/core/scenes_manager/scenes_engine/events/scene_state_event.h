#pragma once

#include "scenes_manager/scenes_engine/events/event_base.h"
#include "scenes_manager/scenes_engine/scene.h"

namespace SceneEngine {

class SceneStateEvent : public EventBase {
public:
    enum class Action {
        AnyResult,
        FailedResult,
        FinishedResult,
        PartiallyFinishedResult,
        SceneEnabled,
        SceneDisabled,
        StoppedResult
    };
    enum class SceneResult { Finished, PartiallyFinished, Failed, Stopped };
    enum class CompareResult { True, False, NotApplicable };

    SceneStateEvent( Scene::SceneID sceneId, Action action );

    std::string getMethodName() const override;
    const Scene::SceneID& getActivatingSceneId() const;

    CompareResult compare( const Scene::SceneID& sceneId, bool isSceneEnabled ) const;
    CompareResult compare( const Scene::SceneID& sceneId, SceneResult result ) const;

private:
    bool isScene( const Scene::SceneID& sceneId ) const;
    static CompareResult toCompareResult( bool value );

    Scene::SceneID m_sceneId;
    Action m_action;
};

using SceneStateEventPtr = std::shared_ptr< SceneStateEvent >;

}  // namespace SceneEngine
