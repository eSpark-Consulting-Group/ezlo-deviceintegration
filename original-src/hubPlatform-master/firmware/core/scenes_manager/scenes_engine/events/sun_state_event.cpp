#include "scenes_manager/scenes_engine/events/sun_state_event.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "scenes_manager/scenes_engine/sunstate_processor/sun_state.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "common_utils/cpp14.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"
#include "scenes_manager/scenes_engine/helpers/dst.h"

#include <tiny/tiny_utils.h>
#include <unordered_map>
#include <utility>

namespace SceneEngine {

std::chrono::seconds SunStateEvent::toDuration( const std::string& timeLiteral ) {
    // Producer FST Hours -> Minutes -> Seconds(milliseconds)
    // start state of FST is the amount of part !!!

    using PartProducerFunc = std::chrono::seconds ( * )( const std::string& part );
    using ProducersFSTType = std::vector< PartProducerFunc >;
    static const ProducersFSTType producers = { []( const std::string& part ) -> std::chrono::seconds {
                                                   int hours = std::stoi( part );
                                                   std::chrono::seconds value( hours * 60 * 60 );
                                                   return value;
                                               },
                                                []( const std::string& part ) {
                                                    int minutes = std::stoi( part );
                                                    std::chrono::seconds value( minutes * 60 );
                                                    return value;
                                                },
                                                []( const std::string& part ) {
                                                    int seconds = std::stoi( part );
                                                    std::chrono::seconds value( seconds );
                                                    return value;
                                                } };

    tiny::string_vector_t vec = tiny::tools::split2vec( timeLiteral, ":" );
    // trick is here !!!
    if( producers.size() < vec.size() ) {
        // Make concrete type of the Error;
        throw SceneException( Errors::WRONG_FIELD );
    }
    auto it = producers.begin() + ( producers.size() - vec.size() );
    std::chrono::seconds res( 0 );
    for( int partIt = 0; it != producers.end(); ++it ) {
        res += ( *it )( vec[ partIt ] );
        ++partIt;
    }
    return res;
}

SunStateEvent::State SunStateEvent::stateTypeFrom( const std::string& value ) {
    using mapType = std::unordered_map< std::string, State >;
    static const mapType stateMap = { { "sunset", State::SUN_SET }, { "sunrise", State::SUN_RISE } };

    auto it = stateMap.find( value );
    if( it != stateMap.cend() ) {
        return it->second;
    }
    throw SceneException( Errors::WRONG_PARAMS );
}

SunStateEvent::OffsetType SunStateEvent::valueTypeFrom( const std::string& value ) {
    using mapType = std::unordered_map< std::string, OffsetType >;
    static const mapType valueMap = {
        { "intime", OffsetType::IN_TIME }, { "before", OffsetType::BEFORE }, { "after", OffsetType::AFTER }
    };
    auto it = valueMap.find( value );
    if( it != valueMap.cend() ) {
        return it->second;
    }
    throw SceneException( Errors::WRONG_PARAMS );
}

SunStateEvent::SunStateEvent( State type,
                              OffsetType offsetType,
                              GetGeoPositionCallback getGeoPosition,
                              ClockGetter clockGetter )
: m_sunStateType( type )
, m_valueType( offsetType )
, m_weekdays()
, m_days()
, m_timeOffset( 0 )
, m_clockGetter( std::move( clockGetter ) )
, m_getGeoPosition( std::move( getGeoPosition ) )
, m_rangeType( RangeType::AtMoment ) {
    if( m_getGeoPosition == nullptr )
        throw std::invalid_argument( "SunStateEvent ctor: getGeoPosition getter is null" );
    if( m_clockGetter == nullptr )
        throw std::invalid_argument( "SunStateEvent ctor: clock getter is null" );

    // TODO: In case of timezone will be changed on the fly without reboot, recalculate this value (now reboot is done)
    m_hasDaylightSavingsTime = DaylightSavingsTime::hasDSTChange( m_clockGetter() );
}

std::string SunStateEvent::getMethodName() const {
    return Model::Resources::MethodNames::isSunState;
}

void SunStateEvent::setTimeOffset( const JsonObject& value ) {
    if( value.is_string() ) {
        m_timeOffset = toDuration( value.get< std::string >() );
    }
    else {
        throw SceneException( Errors::WRONG_FIELD );
    }
}

void SunStateEvent::setDays( const JsonArray& value ) {
    if( !value.is_array() ) {
        throw SceneException( Errors::WRONG_FIELD );
    }
    m_days.clear();
    for( const auto& day : value ) {
        if( day.is_number_integer() ) {
            int dayValue = day.get< int >();
            if( dayValue > 31 || dayValue < 1 ) {
                throw SceneException( Errors::WRONG_FIELD );
            }
            m_days.push_back( dayValue );
        }
        else {
            throw SceneException( Errors::WRONG_FIELD );
        }
    }
    std::sort( m_days.begin(), m_days.end() );
}

void SunStateEvent::setWeekdays( const JsonArray& value ) {
    if( !value.is_array() ) {
        throw SceneException( Errors::WRONG_FIELD );
    }
    m_weekdays.clear();
    for( const auto& weekDay : value ) {
        if( weekDay.is_number_integer() ) {
            Weekday weekDayValue = static_cast< Weekday >( weekDay.get< int >() );
            if( weekDayValue < Weekday::MON || weekDayValue > Weekday::SUN ) {
                // out of range
                throw SceneException( Errors::WRONG_FIELD );
            }
            m_weekdays.push_back( static_cast< Weekday >( weekDayValue ) );
        }
        else {
            throw SceneException( Errors::WRONG_FIELD );
        }
    }
    std::sort( m_weekdays.begin(), m_weekdays.end() );
}

void SunStateEvent::setRangeType( SunStateEvent::RangeType rangeType ) {
    m_rangeType = rangeType;
}

DateEventBase::OptionalClock SunStateEvent::getNextDate( const GregorianCalendar::TimePoint& localNow,
                                                         TimeZoneOffset offset ) const {
    using namespace GregorianCalendar;
    const auto nowDate = Date::fromLocalTime( localNow, offset );

    auto sunStateDate = calcNextSunStateDate( nowDate, true );
    if( !sunStateDate ) {
        LOGGER_ERROR( cc::error( "Sun state date is not calculated" ) );
        return nullptr;
    }

    Date nextDate;
    if( m_rangeType == RangeType::AtMoment ) {
        nextDate = *sunStateDate;
    }
    else if( m_rangeType == RangeType::FromMidnight || m_rangeType == RangeType::TillMidnight ) {
        const bool sameDays = isSameDays( nowDate, *sunStateDate );
        nextDate = sameDays ? *sunStateDate : getCurrentDayMidnight( *sunStateDate );
    }
    else {
        LOGGER_ERROR( cc::error( "Unhandled range type: " ) + cc::num10( (int) m_rangeType ) );
        return nullptr;
    }

    const bool isLocal = true;
    ClockDuration localTime = Date::toLocalTime( nextDate.toClockDuration(), nextDate.getTimeZoneOffset() );
    return std::make_unique< LocalTimePoint >( localTime, nextDate.getTimeZoneOffset(), isLocal );
}

GregorianCalendar::Date SunStateEvent::getCurrentDayMidnight( const GregorianCalendar::Date& date ) {
    GregorianCalendar::Date midnight( date );
    setMidnight( midnight );
    return midnight;
}

bool SunStateEvent::isRangeEvent() const {
    return m_rangeType == RangeType::FromMidnight || m_rangeType == RangeType::TillMidnight;
}

bool SunStateEvent::isTimePointInRange( const GregorianCalendar::TimePoint& localNow,
                                        TimeZoneOffset offset,
                                        bool verbose ) const {
    using namespace GregorianCalendar;
    if( m_rangeType == RangeType::AtMoment ) {
        return false;
    }
    // Set Date from utc, and offset separately
    const auto nowDate = Date::fromLocalTime( localNow, offset );
    const auto sunStateDate = calcNextSunStateDate( nowDate, verbose );
    if( !sunStateDate ) {
        LOGGER_ERROR( cc::error( "Sun state date is not calculated" ) );
        return false;
    }

    const bool sameDays = isSameDays( nowDate, *sunStateDate );
    if( m_rangeType == RangeType::FromMidnight ) {
        return sameDays;
    }
    else if( m_rangeType == RangeType::TillMidnight ) {
        return !sameDays;
    }

    LOGGER_ERROR( cc::error( "Unhandled range type: " ) + cc::num10( (int) m_rangeType ) );
    return false;
}

bool SunStateEvent::isSameDays( const GregorianCalendar::Date& date1, const GregorianCalendar::Date& date2 ) {
    return date1.getYear() == date2.getYear() && date1.getMonth() == date2.getMonth() &&
           date1.getDay() == date2.getDay();
}

void SunStateEvent::setMidnight( GregorianCalendar::Date& date ) {
    date.setHour( 0 );
    date.setMinute( 0 );
    date.setSecond( 0 );
    date.setMillisecond( 0 );
}

std::unique_ptr< GregorianCalendar::Date > SunStateEvent::calcNextSunStateDate( const GregorianCalendar::Date& now,
                                                                                bool verbose ) const {
    const auto pos = m_getGeoPosition();
    SunState sunState( pos.latitude, pos.longitude, m_clockGetter );

    if( verbose ) {
        LOGGER_TRACE( cc::trace( "now:.........................." ) + cc::note( now.toString() ) );
    }
    GregorianCalendar::Date sunstateMidnight( getCurrentDayMidnight( now ) );
    GregorianCalendar::Date sunstateDate( getCurrentDayMidnight( now ) );
    if( verbose ) {
        LOGGER_TRACE( cc::trace( "midnight:....................." ) + cc::note( sunstateDate.toString() ) );
    }

    const int calcNextDayTries = 3;
    int iteration = 0;
    for( ; iteration < calcNextDayTries; ++iteration ) {
        if( verbose ) {
            LOGGER_TRACE( cc::trace( "iteration number: " ) + cc::note( std::to_string( iteration ) ) );
        }
        if( !m_weekdays.empty() ) {
            setSunStateWeekday( sunstateDate );
        }
        else if( !m_days.empty() ) {
            setSunStateDay( sunstateDate );
        }
        if( verbose ) {
            LOGGER_TRACE( cc::trace( "sunstate day:................." ) + cc::note( sunstateDate.toString() ) );
        }
        sunState.setCurrentDate( sunstateDate.getYear(), sunstateDate.getMonth(), sunstateDate.getDay() );

        double minutesFromMidnightShift = calculateOffsetFromMidnight( sunState );
        if( std::isnan( minutesFromMidnightShift ) ) {
            LOGGER_ERROR( cc::error( "sunstate is not calculated" ) );
            return nullptr;
        }

        sunstateDate.addMinutes( static_cast< int64_t >( minutesFromMidnightShift ) );
        if( verbose ) {
            LOGGER_TRACE( cc::trace( "sunstate date:................" ) + cc::note( sunstateDate.toString() ) );
        }
        addDelay( sunstateDate, verbose );
        if( verbose ) {
            LOGGER_TRACE( cc::trace( "sunstate date plus delay:....." ) + cc::note( sunstateDate.toString() ) );
        }
        if( sunstateDate.toClockDuration() < now.toClockDuration() ) {
            setMidnight( sunstateDate );
            sunstateDate.incrementDay();
            if( iteration == 1 ) {
                sunstateDate.incrementDay();
            }
            if( verbose ) {
                LOGGER_TRACE( cc::trace( "sunstate next date midnight:..." ) + cc::note( sunstateDate.toString() ) );
            }
            continue;
        }
        // check offset of calculated time point here
        fixDaylightSavingsTimeOffset( now, sunstateDate );
        break;
    }
    if( iteration >= calcNextDayTries ) {
        LOGGER_ERROR( cc::error( "sunstate not found" ) );
        return nullptr;
    }
    return std::make_unique< GregorianCalendar::Date >( sunstateDate );
}

bool SunStateEvent::fixDaylightSavingsTimeOffset( const GregorianCalendar::Date& now,
                                                  GregorianCalendar::Date& sunstateDate ) const {
    if( m_hasDaylightSavingsTime ) {
        auto& clocks = m_clockGetter();

        auto nowTZOffset = clocks.timeZoneOffset( now.toTimePoint() );
        auto localSunEventTZOffset = clocks.timeZoneOffset( sunstateDate.toTimePoint() );

        if( nowTZOffset != localSunEventTZOffset ) {
            std::chrono::seconds timeZoneOffsetDelta = localSunEventTZOffset - nowTZOffset;
            sunstateDate = switchToAnotherTimeZone( sunstateDate, timeZoneOffsetDelta );
            return true;
        }
    }
    return false;
}

GregorianCalendar::Date SunStateEvent::switchToAnotherTimeZone( GregorianCalendar::Date dateToUpdate,
                                                                std::chrono::seconds timeZoneOffsetDelta ) {
    dateToUpdate.addSeconds( timeZoneOffsetDelta.count() );

    auto currentTzOffset = dateToUpdate.getTimeZoneOffset();
    dateToUpdate.setTimeZoneOffset( currentTzOffset + timeZoneOffsetDelta );

    LOGGER_DEBUG( cc::debug( "SunState midnight and SunState offset difference: " ) +
                  cc::num10( timeZoneOffsetDelta.count() ) + cc::debug( ", date after update: " ) +
                  dateToUpdate.toString() );
    return dateToUpdate;
}

double SunStateEvent::calculateOffsetFromMidnight( const SunState& sunState ) const {
    double eventTime;
    switch( m_sunStateType ) {
    case State::SUN_SET: {
        eventTime = sunState.getSunsetMinutes();
    } break;
    case State::SUN_RISE: {
        eventTime = sunState.getSunriseMinutes();
    } break;
    }
    return eventTime;
}

double SunStateEvent::calculateOffsetFromMidnightUTC( const SunState& sunState ) const {
    double eventTime;
    switch( m_sunStateType ) {
    case State::SUN_SET: {
        eventTime = sunState.calcSunsetUTC();
    } break;
    case State::SUN_RISE: {
        eventTime = sunState.calcSunriseUTC();
    } break;
    }
    return eventTime;
}

void SunStateEvent::setSunStateDay( GregorianCalendar::Date& sunstateDate ) const {
    int currentDay = sunstateDate.getDay();
    auto it = std::lower_bound( m_days.cbegin(), m_days.cend(), currentDay );
    if( it == m_days.cend() ) {
        sunstateDate.setDay( 1 );
        sunstateDate.incrementMonth();
        int firstDay = static_cast< int >( *m_days.begin() );
        sunstateDate.setDay( firstDay );
    }
    else {
        sunstateDate.setDay( *it );
    }
}

void SunStateEvent::setSunStateWeekday( GregorianCalendar::Date& sunstateDate ) const {
    const auto currentWeekday = static_cast< Weekday >( sunstateDate.getWeekday() );
    auto it = std::lower_bound( m_weekdays.cbegin(), m_weekdays.cend(), currentWeekday );
    if( it == m_weekdays.cend() ) {
        sunstateDate.incrementWeek();
        int firstWeekday = static_cast< int >( *m_weekdays.begin() );
        sunstateDate.moveToWeekday( firstWeekday );
    }
    else {
        sunstateDate.moveToWeekday( static_cast< uint8_t >( *it ) );
    }
}

void SunStateEvent::addDelay( GregorianCalendar::Date& sunstateDate, bool verbose ) const {
    if( verbose ) {
        LOGGER_TRACE( cc::trace( "time offset: " ) + cc::note( std::to_string( m_timeOffset.count() ) ) );
    }
    switch( m_valueType ) {
    case OffsetType::IN_TIME:
        break;
    case OffsetType::AFTER:
        sunstateDate.addSeconds( m_timeOffset.count() );
        break;
    case OffsetType::BEFORE:
        sunstateDate.addSeconds( -m_timeOffset.count() );
        break;
    }
}

}  // namespace SceneEngine
