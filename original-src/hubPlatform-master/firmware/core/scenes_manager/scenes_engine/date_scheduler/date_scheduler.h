#pragma once

#include "scenes_manager/scenes_engine/events/date_event.h"
#include "base_utils/gregorian_calendar.h"

#include <set>
#include <functional>

namespace SceneEngine {

class DateScheduler {
public:
    using ClockDuration = GregorianCalendar::ClockDuration;

    /** Holds event pointer, next local time and timezone offset calculated by this event. */
    struct EventEntry {
        DateEventBasePtr event;
        LocalTimePoint m_nextEvent;
        LocalTimePoint m_lastEvent;  //!< result of last getNextEvent for the event

        bool operator<( const EventEntry& other ) const;
    };
    using Queue = std::multiset< EventEntry >;

public:
    void setTime( ClockDuration localNow, std::chrono::seconds tzOffset );
    void enableRecalculationOnBigTimeShift( bool enabled );

    bool empty() const;
    void push( DateEventBasePtr event );
    void eraseBySceneId( const Scene::SceneID& id );

    std::vector< DateEventBasePtr > collectActivatedEvents();

    const Queue& getSchedule() const;

private:
    void recalculateDates( ClockDuration localNow, std::chrono::seconds offset );
    static DateScheduler::EventEntry recalculateEntry( const EventEntry& entry,
                                                       ClockDuration localNow,
                                                       std::chrono::seconds offset );

private:
    Queue m_queue;
    LocalTimePoint m_localTime;
    bool m_recalculationEnabled{ true };
};

}  // namespace SceneEngine
