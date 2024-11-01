#include "scenes_manager/scenes_engine/environment.h"
#include <chrono>

namespace SceneEngine {

EnvironmentTime::EnvironmentTimePimpl::EnvironmentTimePimpl()
: m_clock( []() { return std::time( nullptr ); } ) {}

std::time_t EnvironmentTime::EnvironmentTimePimpl::getTime() const {
    std::lock_guard< std::mutex > lock( m_mutex );
    return m_clock();
}

std::chrono::seconds EnvironmentTime::EnvironmentTimePimpl::timeFromMidnight( const std::time_t& tm ) const {
    std::tm* timeinfo = std::localtime( &tm );
    std::chrono::seconds result( timeinfo->tm_hour * 60 * 60 + timeinfo->tm_min * 60 + timeinfo->tm_sec );
    return result;
}

std::chrono::seconds EnvironmentTime::EnvironmentTimePimpl::timeToMidnight( const std::time_t& tm ) const {
    std::tm* timeinfo = std::localtime( &tm );
    std::chrono::seconds TOTAL_SECONDS_IN_DAY( 24 * 60 * 60 );
    std::chrono::seconds secondsLeft =
        TOTAL_SECONDS_IN_DAY -
        std::chrono::seconds( timeinfo->tm_hour * 60 * 60 + timeinfo->tm_min * 60 + timeinfo->tm_sec );
    return secondsLeft;
}

long int EnvironmentTime::EnvironmentTimePimpl::getTimeZoneOffset() const {
    std::time_t current_time;
    std::time( &current_time );
    struct std::tm* timeinfo = std::localtime( &current_time );
    return timeinfo->tm_gmtoff;
}

std::time_t EnvironmentTime::EnvironmentTimePimpl::getTomorrow( const std::time_t& value ) const {
    using day_t = std::chrono::duration< int, std::ratio< 60 * 60 * 24 > >;
    std::chrono::system_clock::time_point tpCurrent = std::chrono::system_clock::from_time_t( value );
    day_t one_day( 1 );
    std::chrono::system_clock::time_point tomorrow = tpCurrent + one_day;
    std::time_t time = std::chrono::system_clock::to_time_t( tomorrow );
    return time;
}

std::tm EnvironmentTime::EnvironmentTimePimpl::getTomorrow( const std::tm& value ) const {
    std::tm localVariable = value;
    std::time_t timeValue = std::mktime( &localVariable );
    std::time_t ret = getTomorrow( timeValue );
    std::tm* result = std::localtime( &ret );
    return *result;
}

std::time_t EnvironmentTime::EnvironmentTimePimpl::calcDaysOffset( const std::time_t& value, int days ) const {
    if( days <= 0 ) {
        return value;
    }
    using day_t = std::chrono::duration< int, std::ratio< 60 * 60 * 24 > >;
    std::chrono::system_clock::time_point tpCurrent = std::chrono::system_clock::from_time_t( value );
    day_t offsetDays( days );
    std::chrono::system_clock::time_point timePoint = tpCurrent + offsetDays;
    std::time_t time = std::chrono::system_clock::to_time_t( timePoint );
    return time;
}

int EnvironmentTime::EnvironmentTimePimpl::getDayOfWeek( const std::time_t& value ) const {
    std::tm* time = std::localtime( &value );
    int weekday = time->tm_wday;
    if( weekday == 0 ) {
        weekday = 7;
    }
    return weekday;
}

int EnvironmentTime::EnvironmentTimePimpl::getYear( const std::time_t& value ) const {
    constexpr long STD_TM_FEATURES = 1900;
    std::tm* result = std::localtime( &value );
    return result->tm_year + STD_TM_FEATURES;
}

int EnvironmentTime::EnvironmentTimePimpl::getMonth( const std::time_t& value ) const {
    std::tm* result = std::localtime( &value );
    return result->tm_mon + 1;
}

int EnvironmentTime::EnvironmentTimePimpl::getDayOfMonth( const std::time_t& value ) const {
    std::tm* result = std::localtime( &value );
    return result->tm_mday;
}

int EnvironmentTime::EnvironmentTimePimpl::getDaysInMonth( const std::time_t& value ) const {
    int year = getYear( value );
    int month = getMonth( value );

    // leap year condition, if month is 2
    if( month == 2 ) {
        if( ( year % 400 == 0 ) || ( year % 4 == 0 && year % 100 != 0 ) )
            return 29;
        else
            return 28;
    }
    // months which has 31 days
    else if( month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12 )
        return 31;
    else
        return 30;
}

void EnvironmentTime::EnvironmentTimePimpl::setClock( EnvironmentTime::Clock clock ) {
    std::lock_guard< std::mutex > lock( m_mutex );
    m_clock = clock;
}

EnvironmentTime::EnvironmentTime( std::unique_ptr< EnvironmentTimePimpl > customImpl )
: m_impl( std::move( customImpl ) ) {}

EnvironmentTime::EnvironmentTime()
: m_impl( new EnvironmentTimePimpl() ) {}

std::time_t EnvironmentTime::getTime() const {
    return m_impl->getTime();
}

std::chrono::seconds EnvironmentTime::getTimeFromMidnight( const std::time_t& tm ) const {
    return m_impl->timeFromMidnight( tm );
}

std::chrono::seconds EnvironmentTime::getTimeToMidnight( const std::time_t& tm ) const {
    return m_impl->timeToMidnight( tm );
}

long int EnvironmentTime::getTimeZoneOffset() const {
    return m_impl->getTimeZoneOffset();
}

std::time_t EnvironmentTime::getTomorrow( const std::time_t& value ) const {
    return m_impl->getTomorrow( value );
}

std::tm EnvironmentTime::getTomorrow( const std::tm& value ) const {
    return m_impl->getTomorrow( value );
}

std::time_t EnvironmentTime::calcDaysOffset( const std::time_t& value, int days ) const {
    return m_impl->calcDaysOffset( value, days );
}

int EnvironmentTime::getDayOfWeek( const std::time_t& value ) const {
    return m_impl->getDayOfWeek( value );
}

int EnvironmentTime::getYear( const std::time_t& value ) const {
    return m_impl->getYear( value );
}

int EnvironmentTime::getMonth( const std::time_t& value ) const {
    return m_impl->getMonth( value );
}

int EnvironmentTime::getDayOfMonth( const std::time_t& value ) const {
    return m_impl->getDayOfMonth( value );
}

int EnvironmentTime::getDaysInMonth( const std::time_t& value ) const {
    return m_impl->getDaysInMonth( value );
}

void EnvironmentTime::setClock( EnvironmentTime::Clock clock ) {
    m_impl->setClock( clock );
}

Environment::Environment()
: m_envTime( new EnvironmentTime ) {}

Environment::Environment( std::unique_ptr< EnvironmentTime > custom )
: m_envTime( std::move( custom ) ) {}

Environment::~Environment() {}

EnvironmentTime& Environment::getTimeEnv() {
    return *m_envTime.get();
}

const EnvironmentTime& Environment::getTimeEnv() const {
    return *m_envTime.get();
}

}  // namespace SceneEngine
