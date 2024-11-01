#include "scenes_manager/scenes_engine/events/weeks_of_period_event.h"

#include "base_utils/gregorian_calendar_weeks.h"
#include "base_utils/gregorian_calendar.h"

#include "tiny/tiny_consolecolors.h"
#include "log_utils/ezlo_log.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"
#include "common_utils/cpp14.h"

namespace SceneEngine {

WeeksOfPeriodEvent::WeeksOfPeriodEvent( Period period, std::set< Week > weeks )
: m_weekList( std::move( weeks ) )
, m_period( period ) {}

std::string WeeksOfPeriodEvent::getMethodName() const {
    return Model::Resources::MethodNames::isDate;
}

DateEventBase::OptionalClock WeeksOfPeriodEvent::getNextDate( const GregorianCalendar::TimePoint& localNow,
                                                              TimeZoneOffset offset ) const {
    using namespace GregorianCalendar;
    auto date = Date::fromLocalTime( localNow, offset );
    switch( m_period ) {
    case Period::Month:
        for( int i = 0; i <= 1; ++i, date.incrementMonth() ) {
            for( const auto& weekday : m_weekList ) {
                date.setWeekday( weekday );
                if( auto clock = findNextDate( localNow, offset, date ) ) {
                    return clock;
                }
            }
        }
    case Period::Year:
        for( int i = 0; i <= 1; ++i, date.incrementYear() ) {
            for( const auto& weekday : m_weekList ) {
                date.setWeekday( weekday );
                if( auto clock = findNextDate( localNow, offset, date ) ) {
                    return clock;
                }
            }
        }
    }

    LOGGER_ERROR( "next date not found" );
    return nullptr;
}

bool WeeksOfPeriodEvent::isTimePointInRange( const GregorianCalendar::TimePoint& localNow,
                                             TimeZoneOffset offset,
                                             bool verbose ) const {
    GregorianCalendar::Date date( localNow );
    const uint8_t currentWeekNumber = getWeekOfPeriod( date );
    bool weekInRange = m_weekList.count( static_cast< Week >( currentWeekNumber ) ) > 0;

    constexpr int lastWeekMarker = -1;
    if( !weekInRange && m_weekList.count( lastWeekMarker ) > 0 ) {
        weekInRange = isLastWeekOfPeriod( date );
    }

    return weekInRange;
}

uint8_t WeeksOfPeriodEvent::getWeekOfPeriod( const GregorianCalendar::Date& date ) const {
    switch( m_period ) {
    case Period::Month:
        return GregorianCalendar::getWeekOfMonth( date );
    case Period::Year:
        return GregorianCalendar::getWeekOfYear( date );
    }

    return 0;
}

bool WeeksOfPeriodEvent::isLastWeekOfPeriod( const GregorianCalendar::Date& date ) const {
    switch( m_period ) {
    case Period::Month:
        return isLastWeekOfMonth( date );
    case Period::Year:
        return isLastWeekOfYear( date );
    }

    return false;
}

bool WeeksOfPeriodEvent::isLastWeekOfMonth( GregorianCalendar::Date date ) {
    const auto monthNow = date.getMonth();
    date.moveToWeekday( 1 );
    date.incrementWeek();
    return monthNow != date.getMonth();
}

bool WeeksOfPeriodEvent::isLastWeekOfYear( GregorianCalendar::Date date ) {
    const auto yearNow = date.getYear();
    date.moveToWeekday( 1 );
    date.incrementWeek();
    return yearNow != date.getYear();
}

DateEventBase::OptionalClock WeeksOfPeriodEvent::findNextDate( const GregorianCalendar::TimePoint& localNow,
                                                               TimeZoneOffset offset,
                                                               const GregorianCalendar::Date& calculatedDate ) const {
    LocalTimePoint tp( localNow.time_since_epoch(), offset, true );
    auto calculatedUtc = calculatedDate.toClockDuration();
    using namespace GregorianCalendar;

    if( calculatedUtc > tp.toUtc() ) {
        auto targetDate = GregorianCalendar::Date::toLocalTime( calculatedDate.toClockDuration(),
                                                                calculatedDate.getTimeZoneOffset() );

        return std::make_unique< LocalTimePoint >( targetDate, calculatedDate.getTimeZoneOffset(), true );
    }
    return nullptr;
}

}  // namespace SceneEngine
