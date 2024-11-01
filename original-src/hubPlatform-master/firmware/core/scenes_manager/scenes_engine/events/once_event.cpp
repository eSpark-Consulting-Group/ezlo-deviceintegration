// Extracted from date_event.h/cpp
#include "scenes_manager/scenes_engine/events/once_event.h"
#include "common_utils/cpp14.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

namespace SceneEngine {

OnceEvent::OnceEvent( const GregorianCalendar::Date& date )
: m_date( date ) {}

std::string OnceEvent::getMethodName() const {
    return Model::Resources::MethodNames::isOnce;
}

DateEvent::OptionalClock OnceEvent::getNextDate( const GregorianCalendar::TimePoint& /* localNow */,
                                                 TimeZoneOffset offset ) const {
    // Make local time point with respect to current offset (DTS/no DST)
    if( offset != m_date.getTimeZoneOffset() ) {
        const_cast< GregorianCalendar::Date& >( m_date ).setTimeZoneOffset( offset );
    }

    auto targetDate = GregorianCalendar::Date::toLocalTime( m_date.toClockDuration(), m_date.getTimeZoneOffset() );
    return std::make_unique< LocalTimePoint >( targetDate, m_date.getTimeZoneOffset(), true );
}

}  // namespace SceneEngine
