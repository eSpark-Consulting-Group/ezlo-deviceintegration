#include "scenes_manager/scenes_engine/date_scheduler/date_scheduler.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

namespace SceneEngine {

void DateScheduler::setTime( ClockDuration localNow, std::chrono::seconds tzOffset ) {
    ClockDuration delta( std::abs( localNow.count() - m_localTime.duration().count() ) );
    if( m_recalculationEnabled && delta > std::chrono::seconds( 6 ) ) {  // legacy, retained to avoid breaking anything
        recalculateDates( localNow, tzOffset );
    }

    m_localTime.setLocalTime( localNow, tzOffset );
}

void DateScheduler::enableRecalculationOnBigTimeShift( bool enabled ) {
    m_recalculationEnabled = enabled;
}

bool DateScheduler::empty() const {
    return m_queue.empty();
}

void DateScheduler::push( DateEventBasePtr event ) {
    EventEntry entry;
    entry.event = std::move( event );
    entry = recalculateEntry( entry, m_localTime.duration(), m_localTime.offset() );
    m_queue.insert( std::move( entry ) );
}

void DateScheduler::eraseBySceneId( const Scene::SceneID& id ) {
    for( auto it = m_queue.begin(); it != m_queue.end(); ) {
        if( it->event->getSceneId() == id ) {
            it = m_queue.erase( it );
            continue;
        }
        ++it;
    }
}

const DateScheduler::Queue& DateScheduler::getSchedule() const {
    return m_queue;
}

std::vector< DateEventBasePtr > DateScheduler::collectActivatedEvents() {
    using namespace GregorianCalendar;

    std::vector< DateEventBasePtr > result;
    auto it = m_queue.begin();
    while( it != m_queue.end() ) {
        const EventEntry& eventEntry = *it;
        // Compare how localtime and next time look like here for isDate and isSunState
        auto localTimeToUtc = m_localTime.toUtc();
        auto nextTimeToUtc = eventEntry.m_nextEvent.toUtc();

        if( nextTimeToUtc > localTimeToUtc ) {
            break;
        }

        result.push_back( it->event );
        EventEntry newEntry = recalculateEntry( *it, m_localTime.duration(), m_localTime.offset() );
        it = m_queue.erase( it );
        m_queue.insert( std::move( newEntry ) );
    }

    return result;
}

void DateScheduler::recalculateDates( ClockDuration localNow, std::chrono::seconds offset ) {
    Queue queue;
    while( !m_queue.empty() ) {
        auto it = m_queue.begin();
        EventEntry newEntry = recalculateEntry( *it, localNow, offset );
        m_queue.erase( it );
        queue.insert( newEntry );
    }
    m_queue = std::move( queue );
}

/** Creates a copy of entry parameter, resets nextDate and nextDateTzOffset fields.
 * Calls getNextDate with localNow and offset parameters.
 * If new next date is returned, saves it in this new entry.
 * @returns new EventEntry with next date provided by the event.*/
DateScheduler::EventEntry DateScheduler::recalculateEntry( const EventEntry& entry,
                                                           ClockDuration localNow,
                                                           std::chrono::seconds offset ) {
    using namespace GregorianCalendar;

    EventEntry newEntry = entry;
    newEntry.m_nextEvent.setLocalTime( ClockDuration::max(), std::chrono::seconds( 0 ), false );

    if( auto date = newEntry.event->getNextDate( TimePoint( localNow ), offset ) ) {
        newEntry.m_lastEvent = *date;
        // Compare with respect to local time and offset
        const LocalTimePoint localNowPoint = LocalTimePoint( localNow, offset, true );
        if( date->toUtc() > localNowPoint.toUtc() ) {
            newEntry.m_nextEvent = *date;
        }
    }

    // 2023-10-19: Changed isDate::getNextDate() to return local time with offset, not utc offset !
    Date newEntryDate =
        Date::fromLocalTime( TimePoint( newEntry.m_lastEvent.duration() ), newEntry.m_lastEvent.offset() );
    Date newEntryNextDate =
        Date::fromLocalTime( TimePoint( newEntry.m_nextEvent.duration() ), newEntry.m_nextEvent.offset() );
    LOGGER_DEBUG( cc::debug( "Recalculated date event entry, event id: " + newEntry.event->getEventId() +
                             cc::debug( ", date: " ) + newEntryDate.toString() + cc::debug( ", next date: " ) +
                             newEntryNextDate.toString() ) );

    return newEntry;
}

bool DateScheduler::EventEntry::operator<( const EventEntry& other ) const {
    using namespace std::chrono;
    return duration_cast< seconds >( m_nextEvent.duration() ) - this->m_nextEvent.offset() <
           duration_cast< seconds >( other.m_nextEvent.duration() ) - other.m_nextEvent.offset();
}

}  // namespace SceneEngine
