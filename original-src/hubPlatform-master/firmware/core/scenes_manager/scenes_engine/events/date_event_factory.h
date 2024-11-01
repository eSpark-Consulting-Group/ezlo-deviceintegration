#pragma once
#include "scenes_manager/scenes_engine/events/date_event.h"

namespace SceneEngine {

class DateEventFactory {
public:
    static DateEventPtr makeDailyEvent( DateEvent::TimeList dailyTime );
    static DateEventPtr makeWeeklyEvent( DateEvent::TimeList dailyTime, std::set< DateEvent::Day > days );
    static DateEventPtr makeMonthlyEvent( DateEvent::TimeList dailyTime, std::set< DateEvent::Day > days );
    static DateEventPtr makeOnceEvent( const GregorianCalendar::Date& date );
};

}  // namespace SceneEngine
