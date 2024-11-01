#pragma once

#include "scenes_manager/scenes_engine/events/date_event_base.h"
#include <tiny/tiny_dispatch.h>

namespace SceneEngine {

class IntervalEvent : public DateEventBase {
public:
    IntervalEvent() = default;
    IntervalEvent( const JsonObject& intervalValue );

    std::string getMethodName() const override;
    OptionalClock getNextDate( const GregorianCalendar::TimePoint& localNow, TimeZoneOffset offset ) const override;

    void setStartTime( const GregorianCalendar::ClockDuration& startTime );

private:
    // In the general case the service could be stopped in any time.
    // need to record the startTime.
    // When the scene loads need to check the following:
    // 1) how many times does the interval fit to this pause and when actions should be fired
    //   -------HubOff---------HubOn---------
    //               |__|__|__|__|__|
    // So we should calc the remainder of time and then repeat just by constant timeout.

    GregorianCalendar::ClockDuration m_duration{ 0 };
    GregorianCalendar::ClockDuration m_startTime{ 0 };
};

using IntervalEventPtr = std::shared_ptr< IntervalEvent >;

}  // namespace SceneEngine
