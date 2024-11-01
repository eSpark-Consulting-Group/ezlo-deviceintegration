#pragma once
#include "scenes_manager/scenes_engine/events/date_range_event_base.h"
#include "cored/remote_interface/system_attribute_types.h"
#include "base_utils/gregorian_calendar.h"
#include "scenes_manager/scenes_engine/helpers/clock_interface.h"

#include <chrono>
#include <functional>

class SunState;

namespace SceneEngine {

class SunStateEvent : public DateRangeEventBase {
public:
    enum class State : int { SUN_RISE, SUN_SET };
    enum class OffsetType : int { IN_TIME, BEFORE, AFTER };
    enum class Weekday : int { MON = 1, TUE, WED, THU, FRI, SAT, SUN };
    enum class RangeType : int { TillMidnight, FromMidnight, AtMoment };

    using GetGeoPositionCallback = std::function< HubPlatform::CoreDRemoteInterface::GeoPosition() >;
    using ClockGetter = std::function< SceneEngine::IClock&() >;

    using Weekdays = std::vector< Weekday >;
    using Days = std::vector< Day >;

public:
    static State stateTypeFrom( const std::string& value );
    static OffsetType valueTypeFrom( const std::string& value );

    SunStateEvent( State type, OffsetType offsetType, GetGeoPositionCallback getGeoPosition, ClockGetter clockGetter );

    std::string getMethodName() const override;
    OptionalClock getNextDate( const GregorianCalendar::TimePoint& localNow, TimeZoneOffset offset ) const override;

    bool isTimePointInRange( const GregorianCalendar::TimePoint& localNow,
                             TimeZoneOffset offset,
                             bool verbose ) const override;

    void setTimeOffset( const JsonObject& value );
    void setDays( const JsonArray& value );
    void setWeekdays( const JsonArray& value );
    void setRangeType( RangeType rangeType );
    static GregorianCalendar::Date getCurrentDayMidnight( const GregorianCalendar::Date& date );

    bool isRangeEvent() const;

    // @brief format is the following: "hh:mm:ss.mmm" look to the ISO 8106
    // possible values "hh:mm:ss" / "mm:ss" / "ss"
    static std::chrono::seconds toDuration( const std::string& timeLiteral );
    static bool isSameDays( const GregorianCalendar::Date& date1, const GregorianCalendar::Date& date2 );

private:
    std::unique_ptr< GregorianCalendar::Date > calcNextSunStateDate( const GregorianCalendar::Date& now,
                                                                     bool verbose ) const;

    void setSunStateDay( GregorianCalendar::Date& sunstateDate ) const;
    void setSunStateWeekday( GregorianCalendar::Date& sunstateDate ) const;
    static void setMidnight( GregorianCalendar::Date& date );
    void addDelay( GregorianCalendar::Date& sunstateDate, bool verbose ) const;
    double calculateOffsetFromMidnight( const SunState& sunState ) const;
    double calculateOffsetFromMidnightUTC( const SunState& sunState ) const;

    static GregorianCalendar::Date switchToAnotherTimeZone( GregorianCalendar::Date dateToUpdate,
                                                            std::chrono::seconds timeZoneOffsetDelta );
    bool fixDaylightSavingsTimeOffset( const GregorianCalendar::Date& now,
                                       GregorianCalendar::Date& sunstateDate ) const;

private:
    State m_sunStateType;
    OffsetType m_valueType;

    Weekdays m_weekdays;
    Days m_days;
    std::chrono::seconds m_timeOffset;
    ClockGetter m_clockGetter;
    GetGeoPositionCallback m_getGeoPosition;
    RangeType m_rangeType;
    bool m_hasDaylightSavingsTime{ false };
};

using SunStateEventPtr = std::shared_ptr< SunStateEvent >;

}  // namespace SceneEngine
