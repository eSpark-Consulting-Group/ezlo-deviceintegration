#pragma once

#include "scenes_manager/scenes_engine/scene.h"
#include "scenes_manager/scenes_engine/events/scene_state_event.h"
#include "scenes_manager/scenes_engine/events/group_state_event.h"

#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace SceneEngine {

class EventContainer;

class SceneGraphCyclesDetector {
public:
    using SceneStateEvents = std::vector< SceneStateEventPtr >;
    using GroupStateEvents = std::vector< GroupStateEventPtr >;
    // source scene => scene executed by "runScene" action
    using RunSceneActions = std::unordered_multimap< Scene::SceneID, Scene::SceneID >;

    struct SceneConnection {
        SceneConnection( Scene::SceneID sceneId, std::string connectionMethod );
        Scene::SceneID m_sceneId;
        std::string m_connectionMethod;
    };
    using ScenesConnections = std::vector< SceneConnection >;
    using RunDetectionResult = std::pair< bool /* cycleDetected */, ScenesConnections >;

public:
    SceneGraphCyclesDetector( SceneStateEvents sceneStateEvents,
                              GroupStateEvents groupStateEvents,
                              RunSceneActions runSceneActions,
                              const Scene& newScene,
                              const EventContainer& sceneEvents );

    RunDetectionResult runDetection();

private:
    struct EventCacheData {
        EventCacheData( Scene::SceneID sceneId, EventBase::EventId eventId );
        bool operator==( const EventCacheData& right ) const;

        Scene::SceneID m_sceneId;
        EventBase::EventId m_eventId;
    };
    struct EventCacheDataHash {
        std::size_t operator()( const EventCacheData& eventCacheData ) const;
    };

    // source scene => linked isSceneState event
    using SceneStateEventsCache = std::unordered_multimap< Scene::SceneID, EventCacheData >;
    // source event => linked isGroupState event
    using GroupStateEventsCache = std::unordered_multimap< EventCacheData, EventCacheData, EventCacheDataHash >;

    SceneStateEventsCache buildSceneStateEventsCache() const;
    GroupStateEventsCache buildGroupStateEventsCache() const;

    bool checkSceneStateEvents( const Scene::SceneID& sourceSceneId );
    bool checkGroupStateEvents( const EventCacheData& sourceEventData );
    bool checkRunSceneActions( const Scene::SceneID& sourceSceneId );

    bool markSceneVisited( const Scene::SceneID& sceneId );

private:
    SceneStateEvents m_sceneStateEvents;
    GroupStateEvents m_groupStateEvents;
    RunSceneActions m_runSceneActions;

    Scene::SceneID m_newSceneId;
    const EventContainer& m_sceneEvents;

    GroupStateEventsCache m_groupStateEventsCache;
    SceneStateEventsCache m_sceneStateEventsCache;

    ScenesConnections m_connections;
    std::unordered_set< Scene::SceneID > m_visitedScenes;
};

}  // namespace SceneEngine
