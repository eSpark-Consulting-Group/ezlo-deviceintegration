#include "scenes_manager/scenes_engine/date_scheduler/date_range_scheduler.h"
#include "scenes_manager/scenes_engine/events/transactional_events_updater.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

namespace SceneEngine {

void DateRangeScheduler::push( DateRangeEventBasePtr event ) {
    m_events.push_back( std::move( event ) );
}

void DateRangeScheduler::eraseBySceneId( const Scene::SceneID& id ) {
    auto it = std::remove_if( m_events.begin(), m_events.end(), [&id]( const DateRangeEventBasePtr& event ) {
        return event->getSceneId() == id;
    } );
    m_events.erase( it, m_events.end() );
}

const DateRangeScheduler::DateRangeEvents& DateRangeScheduler::getEvents() const {
    return m_events;
}

void DateRangeScheduler::update( GregorianCalendar::TimePoint localNow,
                                 std::chrono::seconds tzOffset,
                                 TransactionalEventsUpdater& updater,
                                 bool verbose ) {
    for( auto& event : m_events ) {
        const bool crossedRangeEdge = event->update( localNow, tzOffset, verbose );
        if( crossedRangeEdge ) {
            if( event->isInRange() ) {
                LOGGER_DEBUG_RAW( cc::debug( "Entered time range, event id: " + event->getEventId() ) );
                updater.requestTrigger( event );
            }
            else {
                LOGGER_DEBUG_RAW( cc::debug( "Exited time range, event id: " + event->getEventId() ) );
                updater.requestReset( event );
            }
        }
    }
}

}  // namespace SceneEngine
