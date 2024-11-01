#pragma once

#include "scenes_manager/scenes_engine/events/date_range_event_base.h"

#include <vector>

namespace SceneEngine {

class TransactionalEventsUpdater;

class DateRangeScheduler {
public:
    using DateRangeEvents = std::vector< DateRangeEventBasePtr >;

public:
    void push( DateRangeEventBasePtr event );
    void eraseBySceneId( const Scene::SceneID& sceneId );
    const DateRangeEvents& getEvents() const;

    void update( GregorianCalendar::TimePoint localNow,
                 std::chrono::seconds tzOffset,
                 TransactionalEventsUpdater& updater,
                 bool verbose );

private:
    DateRangeEvents m_events;
};

}  // namespace SceneEngine
