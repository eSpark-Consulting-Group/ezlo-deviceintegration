#include "scenes_manager/scenes_engine/scene_graph_cycles_detector.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"
#include "scenes_manager/scenes_engine/event_container.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "base_utils/hash_utils.h"

namespace SceneEngine {

namespace MethodNames = Model::Resources::MethodNames;

SceneGraphCyclesDetector::SceneGraphCyclesDetector( SceneStateEvents sceneStateEvents,
                                                    GroupStateEvents groupStateEvents,
                                                    RunSceneActions runSceneActions,
                                                    const Scene& newScene,
                                                    const EventContainer& sceneEvents )
: m_sceneStateEvents( std::move( sceneStateEvents ) )
, m_groupStateEvents( std::move( groupStateEvents ) )
, m_runSceneActions( std::move( runSceneActions ) )
, m_newSceneId( newScene.getId() )
, m_sceneEvents( sceneEvents ) {}

SceneGraphCyclesDetector::RunDetectionResult SceneGraphCyclesDetector::runDetection() {
    //  currently, cycles are detected only for "isSceneState" and "isGroupState" and "runScene" actions
    const bool hasRunSceneAction = m_runSceneActions.find( m_newSceneId ) != m_runSceneActions.end();
    const bool hasEventsCausingCycles =
        !m_sceneEvents.sceneStateEvents.empty() || !m_sceneEvents.groupStateEvents.empty();
    if( !hasRunSceneAction && !hasEventsCausingCycles ) {
        return RunDetectionResult( false, ScenesConnections() );
    }

    LOGGER_TRACE_RAW( cc::trace( "Starting detection of cycles in scene graph" ) );

    m_groupStateEventsCache = buildGroupStateEventsCache();
    m_sceneStateEventsCache = buildSceneStateEventsCache();
    m_connections.clear();
    m_visitedScenes.clear();

    m_visitedScenes.emplace( m_newSceneId );

    bool cycleWillBeCreated = checkSceneStateEvents( m_newSceneId ) || checkRunSceneActions( m_newSceneId );
    if( !cycleWillBeCreated ) {
        for( const auto& sourceEventLinkedEventPair : m_groupStateEventsCache ) {
            const EventCacheData& sourceEventData = sourceEventLinkedEventPair.first;
            if( sourceEventData.m_sceneId == m_newSceneId ) {
                cycleWillBeCreated = checkGroupStateEvents( sourceEventData );
                if( cycleWillBeCreated )
                    break;
            }
        }
    }

    if( !cycleWillBeCreated )
        m_connections.clear();

    return RunDetectionResult( cycleWillBeCreated, std::move( m_connections ) );
}

SceneGraphCyclesDetector::GroupStateEventsCache SceneGraphCyclesDetector::buildGroupStateEventsCache() const {
    GroupStateEventsCache cache;

    for( const GroupStateEventPtr& event : m_groupStateEvents ) {
        EventCacheData sourceEventData( event->getSceneIdArg(), event->getGroupIdArg() );
        EventCacheData linkedEventData( event->getSceneId(), event->getEventId() );
        cache.emplace( std::move( sourceEventData ), std::move( linkedEventData ) );
    }

    for( const GroupStateEventPtr& event : m_sceneEvents.groupStateEvents ) {
        EventCacheData sourceEventData( event->getSceneIdArg(), event->getGroupIdArg() );
        EventCacheData linkedEventData( m_newSceneId, event->getEventId() );
        cache.emplace( std::move( sourceEventData ), std::move( linkedEventData ) );
    }

    return cache;
}

SceneGraphCyclesDetector::SceneStateEventsCache SceneGraphCyclesDetector::buildSceneStateEventsCache() const {
    SceneStateEventsCache cache;

    for( const SceneStateEventPtr& event : m_sceneStateEvents ) {
        EventCacheData linkedEventData( event->getSceneId(), event->getEventId() );
        cache.emplace( event->getActivatingSceneId(), std::move( linkedEventData ) );
    }

    for( const SceneStateEventPtr& event : m_sceneEvents.sceneStateEvents ) {
        EventCacheData linkedEventData( m_newSceneId, event->getEventId() );
        cache.emplace( event->getActivatingSceneId(), std::move( linkedEventData ) );
    }

    return cache;
}

bool SceneGraphCyclesDetector::checkSceneStateEvents( const Scene::SceneID& sourceSceneId ) {
    auto predicate = [this]( const SceneStateEventsCache::value_type& entry ) {
        const EventCacheData& linkedEventData = entry.second;
        m_connections.emplace_back( linkedEventData.m_sceneId, MethodNames::isSceneState );
        if( !markSceneVisited( linkedEventData.m_sceneId ) )
            return true;

        const bool targetSceneReachable = checkSceneStateEvents( linkedEventData.m_sceneId ) ||
                                          checkGroupStateEvents( linkedEventData ) ||
                                          checkRunSceneActions( linkedEventData.m_sceneId );

        if( !targetSceneReachable && !m_connections.empty() )
            m_connections.pop_back();

        return targetSceneReachable;
    };

    auto eventsRange = m_sceneStateEventsCache.equal_range( sourceSceneId );
    auto itEventCreatingCycle = std::find_if( eventsRange.first, eventsRange.second, predicate );
    return itEventCreatingCycle != eventsRange.second;
}

bool SceneGraphCyclesDetector::checkGroupStateEvents( const EventCacheData& sourceEventData ) {
    auto predicate = [this]( const GroupStateEventsCache::value_type& entry ) {
        const EventCacheData& linkedEventData = entry.second;
        m_connections.emplace_back( linkedEventData.m_sceneId, MethodNames::isGroupState );
        if( !markSceneVisited( linkedEventData.m_sceneId ) )
            return true;

        const bool targetSceneReachable = checkSceneStateEvents( linkedEventData.m_sceneId ) ||
                                          checkGroupStateEvents( linkedEventData ) ||
                                          checkRunSceneActions( linkedEventData.m_sceneId );

        if( !targetSceneReachable && !m_connections.empty() )
            m_connections.pop_back();

        return targetSceneReachable;
    };

    auto eventsRange = m_groupStateEventsCache.equal_range( sourceEventData );
    auto itEventCreatingCycle = std::find_if( eventsRange.first, eventsRange.second, predicate );
    return itEventCreatingCycle != eventsRange.second;
}

bool SceneGraphCyclesDetector::checkRunSceneActions( const Scene::SceneID& sourceSceneId ) {
    auto predicate = [this]( const RunSceneActions::value_type& entry ) {
        const Scene::SceneID& executedSceneId = entry.second;
        m_connections.emplace_back( executedSceneId, "runScene" );
        if( !markSceneVisited( executedSceneId ) )
            return true;

        const bool targetSceneReachable =
            checkRunSceneActions( executedSceneId ) || checkSceneStateEvents( executedSceneId );

        if( !targetSceneReachable && !m_connections.empty() )
            m_connections.pop_back();

        return targetSceneReachable;
    };

    auto actionsRange = m_runSceneActions.equal_range( sourceSceneId );
    auto itActionCreatingCycle = std::find_if( actionsRange.first, actionsRange.second, predicate );
    return itActionCreatingCycle != actionsRange.second;
}

bool SceneGraphCyclesDetector::markSceneVisited( const Scene::SceneID& sceneId ) {
    bool added = false;
    std::tie( std::ignore, added ) = m_visitedScenes.emplace( sceneId );
    return added;
}

SceneGraphCyclesDetector::SceneConnection::SceneConnection( Scene::SceneID sceneId, std::string connectionMethod )
: m_sceneId( std::move( sceneId ) )
, m_connectionMethod( std::move( connectionMethod ) ) {}

SceneGraphCyclesDetector::EventCacheData::EventCacheData( Scene::SceneID sceneId, EventBase::EventId eventId )
: m_sceneId( std::move( sceneId ) )
, m_eventId( std::move( eventId ) ) {}

bool SceneGraphCyclesDetector::EventCacheData::operator==( const EventCacheData& right ) const {
    return m_sceneId == right.m_sceneId && m_eventId == right.m_eventId;
}

std::size_t SceneGraphCyclesDetector::EventCacheDataHash::operator()( const EventCacheData& eventCacheData ) const {
    std::size_t result = 0;
    result = common::hash_combine( result, common::hash_value( eventCacheData.m_sceneId ) );
    result = common::hash_combine( result, common::hash_value( eventCacheData.m_eventId ) );
    return result;
}

}  // namespace SceneEngine
