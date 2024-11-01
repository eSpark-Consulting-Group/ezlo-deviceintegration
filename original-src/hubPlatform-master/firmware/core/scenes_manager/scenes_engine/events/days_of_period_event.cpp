#include "scenes_manager/scenes_engine/events/days_of_period_event.h"
#include "scenes_manager/scenes_engine/exceptions.h"

#include "base_utils/gregorian_calendar.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"
#include "common_utils/cpp14.h"

namespace SceneEngine {

DaysOfPeriodEvent::DaysOfPeriodEvent( Period period, std::set< uint8_t > days )
: m_daysList( std::move( days ) )
, m_period( period ) {
    verifyDaysValues();
}

std::string DaysOfPeriodEvent::getMethodName() const {
    return Model::Resources::MethodNames::isDate;
}

DateEventBase::OptionalClock DaysOfPeriodEvent::getNextDate( const GregorianCalendar::TimePoint& localNow,
                                                             TimeZoneOffset offset ) const {
    using namespace GregorianCalendar;
    auto date = Date::fromLocalTime( localNow, offset );
    switch( m_period ) {
    case Period::Month:
        for( int i = 0; i <= 1; ++i, date.incrementMonth() ) {
            for( const auto& day : m_daysList ) {
                date.setDay( day );
                if( auto clock = findNextDate( localNow, offset, date ) ) {
                    return clock;
                }
            }
        }
    case Period::Week:
        for( int i = 0; i <= 1; ++i, date.incrementWeek() ) {
            for( const auto& day : m_daysList ) {
                date.moveToWeekday( day );
                if( auto clock = findNextDate( localNow, offset, date ) ) {
                    return clock;
                }
            }
        }
    }

    LOGGER_ERROR( "next date not found" );
    return nullptr;
}

/** @brief Calculates whether time point belongs to a day of a week or a day of a month.
 * @returns true - local time point belongs to a day stored in the event object.
 * @returns false - local time doesn't belong to any day stored in the event object. */
bool DaysOfPeriodEvent::isTimePointInRange( const GregorianCalendar::TimePoint& localNow,
                                            TimeZoneOffset offset,
                                            bool verbose ) const {
    using namespace GregorianCalendar;
    Date date = Date::fromLocalTime( localNow, offset );
    const uint8_t day = getDayOfPeriod( date );
    return m_daysList.find( day ) != m_daysList.end();
}

uint8_t DaysOfPeriodEvent::getDayOfPeriod( const GregorianCalendar::Date& date ) const {
    switch( m_period ) {
    case Period::Month:
        return date.getDay();
    case Period::Week:
        return date.getWeekday();
    }

    LOGGER_ERROR( cc::error( "Unhandled period type: " ) + cc::num( (int) m_period ) );
    return 0;
}

void DaysOfPeriodEvent::verifyDaysValues() const {
    const uint8_t maxValue = [this]() {
        switch( m_period ) {
        case Period::Month:
            return 31;
        case Period::Week:
            return 7;
        }
        return 0;
    }();

    for( uint8_t day : m_daysList ) {
        if( day > maxValue )
            throw SceneException( Errors::INVALID_VALUE_RANGE, "Day number exceeded allowed range limit" );
    }
}

DateEventBase::OptionalClock DaysOfPeriodEvent::findNextDate( const GregorianCalendar::TimePoint& localNow,
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
