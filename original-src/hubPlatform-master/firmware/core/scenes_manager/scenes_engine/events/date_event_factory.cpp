#include "scenes_manager/scenes_engine/events/date_event_factory.h"
#include "scenes_manager/scenes_engine/events/daily_event.h"
#include "scenes_manager/scenes_engine/events/weekly_event.h"
#include "scenes_manager/scenes_engine/events/monthly_event.h"
#include "scenes_manager/scenes_engine/events/once_event.h"

namespace SceneEngine {

DateEventPtr DateEventFactory::makeDailyEvent( DateEvent::TimeList dailyTime ) {
    return std::make_shared< DailyEvent >( std::move( dailyTime ) );
}

DateEventPtr DateEventFactory::makeWeeklyEvent( DateEvent::TimeList dailyTime, std::set< DateEvent::Day > days ) {
    return std::make_shared< WeeklyEvent >( std::move( dailyTime ), std::move( days ) );
}

DateEventPtr DateEventFactory::makeMonthlyEvent( DateEvent::TimeList dailyTime, std::set< DateEvent::Day > days ) {
    return std::make_shared< MonthlyEvent >( std::move( dailyTime ), std::move( days ) );
}

DateEventPtr DateEventFactory::makeOnceEvent( const GregorianCalendar::Date& date ) {
    return std::make_shared< OnceEvent >( date );
}

}  // namespace SceneEngine