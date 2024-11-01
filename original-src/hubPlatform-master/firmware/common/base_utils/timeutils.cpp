#include "timeutils.h"
#include "utils.h"
#include <tiny/tiny_consolecolors.h>
#include <assert.h>
#include <sys/sysinfo.h>

// l_sergiy: remove this redundant TimeUtils class

std::string TimeUtils::timestampToHReadable( const time_t rawtime ) {
    //"Tue Jul 10 00:11:51 1974"
    char buff[ 32 ] = { 0 };
    char* timeStr = ctime_r( &rawtime, buff );
    return ( ( timeStr && strlen( timeStr ) > 0 ) ? std::string( timeStr, strlen( timeStr ) - 1 ) : "" );
}

std::string TimeUtils::timestampToIsoDateUtc( const time_t rawtime ) {
    //    struct tm * dt;
    //    char buffer [30];
    //    dt = gmtime(&rawtime);
    //    strftime(buffer, sizeof(buffer), "%FT%T.000Z", dt );
    //    //"1999-05-09T10:25:22.777Z"
    //    return std::string(buffer);
    return cc::time2string( rawtime,
                            uint64_t( 0 ),  // nMicroSeconds
                            true,           // isUTC
                            false           // isColored
    );
}
std::string TimeUtils::timestampToIsoDate( const time_t rawtime ) {
    //    struct tm * dt;
    //    char buffer [30];
    //    dt = localtime(&rawtime);
    //    strftime(buffer, sizeof(buffer), "%FT%T.000Z", dt);
    //    return std::string(buffer);
    return cc::time2string( rawtime,
                            uint64_t( 0 ),  // nMicroSeconds
                            false,          // isUTC
                            false           // isColored
    );
}

//"Tue Jul 10 00:11:51 1974"
time_t TimeUtils::hReadableToTimestamp( const std::string& time ) {
    time_t rawtime = 1;
    struct tm* dt;
    dt = localtime( &rawtime );
    strptime( time.c_str(), "%c", dt );
    return mktime( dt );
}

//"1999-05-09T10:25:22.777Z" - old
//"1999:05:09 10:25:22.012345" - new, higher precission, UTC only, no time shifts
time_t TimeUtils::isoDateToTimestamp( const std::string& ISODate ) {
    //    time_t rawtime = 1;
    //    struct tm *dt;
    //    dt = localtime(&rawtime);
    //    strptime(ISODate.c_str(), "%FT%T", dt);
    //    return mktime(dt);
    std::tm aTm;
    uint64_t nMicroSeconds;
    bool bOK = cc::string2time( ISODate.c_str(), aTm, nMicroSeconds );
    if( !bOK )
        return 0;
    return mktime( &aTm );
}

std::string TimeUtils::getCurrentHReadableTime() {
    time_t current;
    time( &current );
    return timestampToHReadable( current );
}

std::string TimeUtils::getCurrentIsoDateUtc() {
    //    time_t current;
    //    time(&current);
    //    return timestampToIsoDateUtc(current);
    return cc::now2string( true,   // isUTC
                           false,  // isDaysInsteadOfYMD
                           false   // isColored
    );
}
std::string TimeUtils::getCurrentIsoDate() {
    //    time_t current;
    //    time(&current);
    //    return timestampToIsoDate(current);
    return cc::now2string( false,  // isUTC
                           false,  // isDaysInsteadOfYMD
                           false   // isColored
    );
}

time_t TimeUtils::getCurrentTime() {
    time_t current;
    time( &current );
    return current;
}

time_t TimeUtils::getMonotonicTime() {
    struct timespec monotonicTime;
    clock_gettime( CLOCK_MONOTONIC, &monotonicTime );
    return monotonicTime.tv_sec;
}

time_t TimeUtils::getDiffMonotonicTime( const time_t& time ) {
    return ( getMonotonicTime() - time );
}

struct tm TimeUtils::stringToTimeStruct( const std::string& time, const std::string& format ) {
    struct tm tm;
    memset( &tm, 0, sizeof( struct tm ) );
    strptime( time.c_str(), format.c_str(), &tm );
    return tm;
}

std::string TimeUtils::timeStructToString( const tm& time, const std::string& format ) {
    constexpr const size_t DateTimeLen = 100;
    std::string buffer( DateTimeLen, '\0' );
    const auto timeLen = strftime( &buffer[ 0 ], buffer.capacity(), format.c_str(), &time );
    assert( timeLen <= DateTimeLen + 1 );
    if( timeLen == 0 ) {
        return std::string();
    }
    buffer.resize( strlen( buffer.c_str() ) );
    return buffer;
}

std::string TimeUtils::chronoTimePointToString( const std::chrono::system_clock::time_point& time,
                                                const std::string& format ) {
    std::time_t tt = std::chrono::system_clock::to_time_t( time );
    std::tm tm = *std::localtime( &tt );  // Locale time-zone, usually UTC by default.
    return timeStructToString( tm, format );
}

time_t TimeUtils::getDiffTime( const time_t& time ) {
    return ( getCurrentTime() - time );
}

double TimeUtils::difftimeHReadable( const std::string& t1, const std::string& t2 ) {
    return difftime( hReadableToTimestamp( t2 ), hReadableToTimestamp( t1 ) );
}

double TimeUtils::difftimeISODate( const std::string& t1, const std::string& t2 ) {
    return static_cast< int >( difftime( isoDateToTimestamp( t2 ), isoDateToTimestamp( t1 ) ) );
}

double TimeUtils::difftimeISODateUtc( const std::string& t1, const std::string& t2 ) {
    return static_cast< int >( difftime( isoDateToTimestamp( t2 ), isoDateToTimestamp( t1 ) ) );
}

int TimeUtils::getCurrentOffset() {
    time_t currentTime = getCurrentTime();
    time( &currentTime );
    struct tm* timeinfo = localtime( &currentTime );
    return static_cast< int >( timeinfo->tm_gmtoff );
}

std::string TimeUtils::formatUptime( time_t uptime ) {
    time_t rest = uptime;
    auto getTime = [&rest]( std::uint64_t factor ) {
        auto result = rest % factor;
        rest /= factor;
        return result;
    };

    const auto seconds = getTime( 60u );
    const auto minutes = getTime( 60u );
    const auto hours = getTime( 24u );

    return utils::stringify( rest, "d ", hours, "h ", minutes, "m ", seconds, "s" );
}

long TimeUtils::getSystemUptime() {
    struct sysinfo info = {};
    int error = sysinfo( &info );
    if( error != 0 ) {
        LOGGER_ERROR( cc::error( "Error = " + cc::num10( error ) ) );
    }
    return info.uptime;
}

std::string TimeUtils::convertTimeToHttpExpires( time_t timeValue ) {
    struct tm* timeInfo;
    timeInfo = gmtime( &timeValue );
    char buffer[ 80 ];
    strftime( buffer, sizeof( buffer ), "%a, %d %b %Y %H:%M:%S GMT", timeInfo );

    return std::string( buffer );
}

time_t TimeUtils::convertHttpExpiresToTime( const std::string& httpExpires ) {
    struct tm timeInfo;
    if( strptime( httpExpires.c_str(), "%a, %d %b %Y %H:%M:%S GMT", &timeInfo ) == nullptr ) {
        return -1;
    }
    timeInfo.tm_isdst = -1;
    time_t result = mktime( &timeInfo );
    time_t currentTimestamp = time( nullptr );
    struct tm* localTimeInfo = localtime( &currentTimestamp );
    long int timezoneOffset = localTimeInfo->tm_gmtoff;
    return result + timezoneOffset;
}

std::chrono::seconds TimeUtils::currentSystemTime() {
    return std::chrono::duration_cast< std::chrono::seconds >( std::chrono::system_clock::now().time_since_epoch() );
}

std::chrono::seconds TimeUtils::currentSteadyTime() {
    return std::chrono::duration_cast< std::chrono::seconds >( std::chrono::steady_clock::now().time_since_epoch() );
}