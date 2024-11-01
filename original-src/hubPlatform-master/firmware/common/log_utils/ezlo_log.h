#pragma once

#include "json_utils/json_fwd.h"
#include "bus/messagesclasses.h"

#include <string>
#include <atomic>
#include <cstdarg>

#define MAX_LOG_MESSAGE 1024

#define LOGGER_PRETTY_FUNCTION std::string( __PRETTY_FUNCTION__ )
#define LOGGER_FUNCTION std::string( __FUNCTION__ )
#define LOGGER_ADDON LOGGER_FUNCTION + ": "

#define LOGGER_TRACE_RAW( message )                       \
    if( Log::getLogLevel() >= Log::PriorityLevel::TRACE ) \
        do {                                              \
            Log::trace( message );                        \
    } while( false )

#define LOGGER_DEBUG_RAW( message )                       \
    if( Log::getLogLevel() >= Log::PriorityLevel::DEBUG ) \
        do {                                              \
            Log::debug( message );                        \
    } while( false )

#define LOGGER_INFO_RAW( message )                       \
    if( Log::getLogLevel() >= Log::PriorityLevel::INFO ) \
        do {                                             \
            Log::info( message );                        \
    } while( false )

#define LOGGER_WARN_RAW( message )                       \
    if( Log::getLogLevel() >= Log::PriorityLevel::WARN ) \
        do {                                             \
            Log::warn( message );                        \
    } while( false )

#define LOGGER_ERROR_RAW( message )                       \
    if( Log::getLogLevel() >= Log::PriorityLevel::ERROR ) \
        do {                                              \
            Log::error( message );                        \
    } while( false )

#define LOGGER_FATAL_RAW( message )                       \
    if( Log::getLogLevel() >= Log::PriorityLevel::FATAL ) \
        do {                                              \
            Log::fatal( message );                        \
    } while( false )

#define LOGGER_TRACE( message ) LOGGER_TRACE_RAW( LOGGER_ADDON + message )
#define LOGGER_DEBUG( message ) LOGGER_DEBUG_RAW( LOGGER_ADDON + message )
#define LOGGER_INFO( message ) LOGGER_INFO_RAW( LOGGER_ADDON + message )
#define LOGGER_WARN( message ) LOGGER_WARN_RAW( LOGGER_ADDON + message )
#define LOGGER_ERROR( message ) LOGGER_ERROR_RAW( LOGGER_ADDON + message )
#define LOGGER_FATAL( message ) LOGGER_FATAL_RAW( LOGGER_ADDON + message )

#define LOGGER_PRINT( logLevel, ... )            \
    do {                                         \
        if( Log::getLogLevel() >= logLevel )     \
            Log::print( logLevel, __VA_ARGS__ ); \
    } while( false )

#define LOGGER_TRACE_PRINT( ... ) LOGGER_PRINT( Log::PriorityLevel::TRACE, __VA_ARGS__ )
#define LOGGER_DEBUG_PRINT( ... ) LOGGER_PRINT( Log::PriorityLevel::DEBUG, __VA_ARGS__ )
#define LOGGER_INFO_PRINT( ... ) LOGGER_PRINT( Log::PriorityLevel::INFO, __VA_ARGS__ )
#define LOGGER_WARN_PRINT( ... ) LOGGER_PRINT( Log::PriorityLevel::WARN, __VA_ARGS__ )
#define LOGGER_ERROR_PRINT( ... ) LOGGER_PRINT( Log::PriorityLevel::ERROR, __VA_ARGS__ )
#define LOGGER_FATAL_PRINT( ... ) LOGGER_PRINT( Log::PriorityLevel::FATAL, __VA_ARGS__ )

class Log {
public:
    enum PriorityLevel { FATAL = 0, ERROR = 300, WARN = 400, INFO = 600, DEBUG = 700, TRACE = 799 };

    static const char* toString( PriorityLevel logLevel );

    static int toPriorityLevel( const char* priority );

    static MessageSubClasses toMessageSubClass( PriorityLevel logLevel );

public:
    static void trace( const std::string& message );
    static void debug( const std::string& message, bool isRawMessage = false );
    static void info( const std::string& message );
    static void warn( const std::string& message );
    static void error( const std::string& message );
    static void fatal( const std::string& message );
    static void setLocalLogLevel( const int logLevel );
    static void setLocalColors( bool useColors );
    static bool getLocalColors();
    static void setLocalIndent( int indent );
    static int getLocalIndent();

    static void print( PriorityLevel level, const char* format, ... );
    static void printList( PriorityLevel level, const char* format, va_list args, const std::string& module = "" );

    static void print( PriorityLevel level, const std::string& message );

    static void processLogCommand( const int msgSubclass, const JsonObject& object );

    static int getLogLevel();
    static std::string stat_getLogLevelName( int epl );

    static void printAllLogsToConsole( bool status );

    static std::string cutMessage( const std::string& msg, size_t limit = MAX_LOG_MESSAGE );
    static void printLogCounter();
    static JsonObject logCounterToJson();

private:
    using LogMap = std::map< const PriorityLevel, std::atomic_int >;
    static int requestLogLevel();

    static void outputToStream( const std::string& message,
                                MessageSubClasses msgSubclass = HA_LOG_MSG,
                                bool useCErr = true );
    static LogMap getLogCounter() {
        LogMap logCounter;
        logCounter[ FATAL ] = 0;
        logCounter[ ERROR ] = 0;
        logCounter[ WARN ] = 0;
        logCounter[ INFO ] = 0;
        logCounter[ DEBUG ] = 0;
        logCounter[ TRACE ] = 0;
        return logCounter;
    }

private:
    static std::atomic_int m_logLevel;
    static std::atomic_bool m_alwaysLogToConsole;
    static LogMap m_logCounter;
};
