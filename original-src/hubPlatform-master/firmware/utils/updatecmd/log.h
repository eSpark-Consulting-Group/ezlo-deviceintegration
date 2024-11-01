#pragma once

#define LOG( severity, entry )                                               \
    {                                                                        \
        time_t rawTime = 0;                                                  \
        struct tm* timeInfo = nullptr;                                       \
        char buffer[ 80 ];                                                   \
        memset( buffer, '\0', sizeof( buffer ) );                            \
                                                                             \
        time( &rawTime );                                                    \
        timeInfo = localtime( &rawTime );                                    \
        strftime( buffer, sizeof( buffer ), "%Y-%m-%d %H:%M:%S", timeInfo ); \
                                                                             \
        log << buffer << " " << severity << ": " << entry << std::endl;      \
        log.flush();                                                         \
    }

#define LOG_ERROR( entry ) LOG( "ERROR", entry )
#define LOG_INFO( entry ) LOG( "INFO ", entry )
#define LOG_WARN( entry ) LOG( "WARN ", entry )
#define LOG_DEBUG( entry ) LOG( "DEBUG ", entry )
