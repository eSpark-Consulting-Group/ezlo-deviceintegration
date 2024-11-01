#pragma once

#include <string>
#include <time.h>
#include <cstring>
#include <chrono>

class TimeUtils {
public:
    static std::string timestampToHReadable( const time_t rawtime );
    static std::string timestampToIsoDateUtc( const time_t rawtime );
    static std::string timestampToIsoDate( const time_t rawtime );
    static time_t hReadableToTimestamp( const std::string& time );
    static time_t isoDateToTimestamp( const std::string& ISODate );
    static std::string getCurrentHReadableTime();
    static std::string getCurrentIsoDateUtc();
    static std::string getCurrentIsoDate();
    static time_t getCurrentTime();
    static time_t getDiffTime( const time_t& time );
    static int getCurrentOffset();
    static double difftimeHReadable( const std::string& t1, const std::string& t2 = getCurrentHReadableTime() );
    static double difftimeISODate( const std::string& t1, const std::string& t2 = getCurrentIsoDate() );
    static double difftimeISODateUtc( const std::string& t1, const std::string& t2 = getCurrentIsoDateUtc() );
    static time_t getMonotonicTime();
    static time_t getDiffMonotonicTime( const time_t& time );
    static struct tm stringToTimeStruct( const std::string& time, const std::string& format = "%Y-%m-%d:%H:%M:%S" );
    static std::string timeStructToString( const struct tm& time, const std::string& format = "%FT%T%z" );
    static std::string chronoTimePointToString( const std::chrono::system_clock::time_point& time,
                                                const std::string& format = "%FT%T%z" );

    static std::string formatUptime( time_t uptime );
    static long getSystemUptime();
    static std::string convertTimeToHttpExpires( time_t timeValue );
    static time_t convertHttpExpiresToTime( const std::string& timeValue );
    static std::chrono::seconds currentSystemTime();
    static std::chrono::seconds currentSteadyTime();
};
