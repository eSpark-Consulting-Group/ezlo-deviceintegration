#include "log_utils/ezlo_log.h"

#include "bus/messagesclasses.h"

#include "json_utils/json_utils.h"
#include "base_utils/comparators.h"
#include "config_reader/config_keys.h"
#include "config_reader/config_reader.h"

#include <tiny/tiny_consolecolors.h>

#include <chrono>
#include <iostream>
#include <cstdarg>

#ifdef __LOGGER_LOG_THREAD_ID__
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#endif

std::atomic_bool Log::m_alwaysLogToConsole{ true };
std::atomic_int Log::m_logLevel{ -1 };
std::map< const Log::PriorityLevel, std::atomic_int > Log::m_logCounter = Log::getLogCounter();

const char* Log::toString( PriorityLevel logLevel ) {
    switch( logLevel ) {
    case PriorityLevel::FATAL:
        return "FATAL";

    case PriorityLevel::ERROR:
        return "ERROR";

    case PriorityLevel::WARN:
        return "WARN";

    case PriorityLevel::INFO:
        return "INFO";

    case PriorityLevel::DEBUG:
        return "DEBUG";

    case PriorityLevel::TRACE:
        return "TRACE";
    };

    return "UNKNOWN";
}

int Log::toPriorityLevel( const char* priority ) {
    if( !priority )
        return -1;

    if( strcasecmp( priority, "FATAL" ) == 0 )
        return FATAL;
    if( strcasecmp( priority, "ERROR" ) == 0 )
        return ERROR;
    if( strcasecmp( priority, "WARN" ) == 0 )
        return WARN;
    if( strcasecmp( priority, "INFO" ) == 0 )
        return INFO;
    if( strcasecmp( priority, "TRACE" ) == 0 )
        return TRACE;
    if( strcasecmp( priority, "DEBUG" ) == 0 )
        return DEBUG;

    return -1;
}

MessageSubClasses Log::toMessageSubClass( PriorityLevel logLevel ) {
    switch( logLevel ) {
    case PriorityLevel::FATAL:
        return HA_LOG_FATAL;

    case PriorityLevel::ERROR:
        return HA_LOG_ERROR;

    case PriorityLevel::WARN:
        return HA_LOG_WARN;

    case PriorityLevel::INFO:
        return HA_LOG_INFO;

    case PriorityLevel::DEBUG:
        return HA_LOG_DEBUG;

    case PriorityLevel::TRACE:
        return HA_LOG_MSG;
    };

    return HA_LOG_DEBUG;
}

void Log::printAllLogsToConsole( bool status ) {
    m_alwaysLogToConsole = status;
}

namespace {
std::ostream& formatLogLevel( std::ostream& ostream, MessageSubClasses msgSubclass ) {
    switch( msgSubclass ) {
    case HA_LOG_FATAL:
        ostream << cc::fatal( "FATAL" );
        break;
    case HA_LOG_ERROR:
        ostream << cc::error( "ERROR" );
        break;
    case HA_LOG_WARN:
        ostream << cc::warn( "WARN " );
        break;
    case HA_LOG_MSG:
        ostream << cc::normal( "MSG  " );
        break;
    case HA_LOG_INFO:
        ostream << cc::info( "INFO " );
        break;
    case HA_LOG_DEBUG:
        ostream << cc::debug( "DEBUG" );
        break;
    default:
        ostream << cc::normal( "MSG  " );
        break;
    }  // switch( msgSubclass )
    return ostream;
}

std::ostream& formatLogThreadId( std::ostream& os ) {
#ifdef __LOGGER_LOG_THREAD_ID__
    pid_t threadId = syscall( SYS_gettid );
    pid_t processId = getpid();
    os << "[" << cc::num10( processId ) << ":" << cc::num10( threadId ) << "] ";
#endif
    return os;
}

std::ostream& selectStream( bool useCErr ) {
    // DONE: EFWR-4000: for tests and dev - separate streams, leave cerr for live builds
#ifdef __HUB_TYPE_LOCAL__
    auto& os = useCErr ? std::cerr : std::cout;
#else
    auto& os = std::cerr;
    (void) useCErr;
#endif

    return os;
}
}  // namespace

void Log::outputToStream( const std::string& message, MessageSubClasses msgSubclass, bool useCErr ) {
    std::stringstream ss;
    ss << cc::time2string( std::chrono::high_resolution_clock::now() ) << " ";

    formatLogLevel( ss, msgSubclass );

    ss << ": ";

    formatLogThreadId( ss );

    ss << message;

    size_t cnt = message.length();
    if( cnt > 0 && message[ cnt - 1 ] != '\n' )
        ss << '\n';

    auto& ostream = selectStream( useCErr );
    ostream << ss.str();
    ostream.flush();
}

void Log::print( PriorityLevel level, const char* format, ... ) {
    if( getLogLevel() >= level ) {
        va_list args;
        va_start( args, format );

        printList( level, format, args );

        va_end( args );
    }
}

void Log::printList( PriorityLevel level, const char* format, va_list args, const std::string& module ) {
    if( getLogLevel() >= level ) {
        std::vector< char > message( 1024 );
        while( true ) {
            va_list args2;
            va_copy( args2, args );
            int res = vsnprintf( message.data(), message.size(), format, args2 );
            if( ( res >= 0 ) && ( res < static_cast< int >( message.size() ) ) ) {
                va_end( args2 );
                break;
            }
            size_t size;
            if( res < 0 ) {
                size = message.size() * 2;
            }
            else {
                size = static_cast< size_t >( res ) + 1;
            }

            message.clear();
            message.resize( size );
            va_end( args2 );
        }

        if( module.empty() ) {
            outputToStream( std::string( message.data() ), toMessageSubClass( level ) );
        }
        else {
            std::string messageStr( message.data() );
            messageStr.pop_back();
            switch( level ) {
            case PriorityLevel::DEBUG:
                ++m_logCounter[ level ];
                outputToStream( cc::debug( "[" + module + "] " + messageStr ), toMessageSubClass( level ), false );
                break;
            case PriorityLevel::ERROR:
                ++m_logCounter[ level ];
                outputToStream( cc::error( "[" + module + "] " + messageStr ), toMessageSubClass( level ), true );
                break;
            case PriorityLevel::FATAL:
                ++m_logCounter[ level ];
                outputToStream( cc::fatal( "[" + module + "] " + messageStr ), toMessageSubClass( level ), true );
                break;
            case PriorityLevel::INFO:
                ++m_logCounter[ level ];
                outputToStream( cc::info( "[" + module + "] " + messageStr ), toMessageSubClass( level ), false );
                break;
            case PriorityLevel::TRACE:
                ++m_logCounter[ level ];
                outputToStream( cc::trace( "[" + module + "] " + messageStr ), toMessageSubClass( level ), false );
                break;
            case PriorityLevel::WARN:
                ++m_logCounter[ level ];
                outputToStream( cc::warn( "[" + module + "] " + messageStr ), toMessageSubClass( level ), true );
                break;
            }
        }
    }
}

void Log::print( PriorityLevel level, const std::string& message ) {
    if( getLogLevel() >= level ) {
        outputToStream( message, toMessageSubClass( level ) );
    }
}

void Log::trace( const std::string& message ) {
    if( getLogLevel() >= TRACE ) {
        ++m_logCounter[ TRACE ];
        outputToStream( message, toMessageSubClass( TRACE ), false );
    }
}

void Log::debug( const std::string& message, bool isRawMessage /*= false*/ ) {
    if( getLogLevel() >= DEBUG ) {
        ++m_logCounter[ DEBUG ];
        outputToStream( message, toMessageSubClass( DEBUG ), false );
    }
}

void Log::info( const std::string& message ) {
    if( getLogLevel() >= INFO ) {
        ++m_logCounter[ INFO ];
        outputToStream( message, toMessageSubClass( INFO ), false );
    }
}

void Log::warn( const std::string& message ) {
    if( getLogLevel() >= WARN ) {
        ++m_logCounter[ WARN ];
        outputToStream( message, toMessageSubClass( WARN ), true );
    }
}

void Log::error( const std::string& message ) {
    if( getLogLevel() >= ERROR ) {
        ++m_logCounter[ ERROR ];
        outputToStream( message, toMessageSubClass( ERROR ), true );
    }
}

void Log::fatal( const std::string& message ) {
    if( getLogLevel() >= FATAL ) {
        ++m_logCounter[ FATAL ];
        outputToStream( message, toMessageSubClass( FATAL ), true );
    }
}

void Log::setLocalLogLevel( const int logLevel ) {
    m_logLevel.store( logLevel );
}

void Log::setLocalColors( bool useColors ) {
    cc::_on_.store( useColors );
}

bool Log::getLocalColors() {
    return cc::_on_.load();
}

void Log::setLocalIndent( int indent ) {
    cc::_default_json_indent_.store( indent );
}

int Log::getLocalIndent() {
    return cc::_default_json_indent_.load();
}

int Log::getLogLevel() {
    if( m_logLevel == -1 ) {
        return requestLogLevel();
    }
    return m_logLevel;
}

void Log::processLogCommand( const int msgSubclass, const JsonObject& object ) {
    int logLevel = -1;
    JsonObject result;
    if( !json_get_obj( object, "result", result ) ) {
        result = object;
    }

    switch( msgSubclass ) {
    case SET_HA_LOG_LEVEL:
    case GET_HA_LOG_LEVEL:
        if( !json_get( result, "logLevel", logLevel ) ) {
            error( "SET_LOG_LEVEL no field logLevel: " + json_write( object ) );
            return;
        }
        m_logLevel = logLevel;
        info( std::string( "Log::getLogLevel " ) + toString( static_cast< PriorityLevel >( m_logLevel.load() ) ) + "(" +
              std::to_string( m_logLevel ) + ")" );
        break;

    default:
        warn( "processLogCommand: Unknown msg_subclass: " + std::to_string( msgSubclass ) );
    }
}

int Log::requestLogLevel() {
    HubPlatform::ConfigReader configReader;
    if( configReader.hasValue( HubPlatform::ConfigKeys::LogLevel ) ) {
        m_logLevel = Log::toPriorityLevel( ( configReader.getString( HubPlatform::ConfigKeys::LogLevel ) ).c_str() );
        return m_logLevel;
    }
    return INFO;
}

std::string Log::stat_getLogLevelName( int epl ) {
    return toString( static_cast< PriorityLevel >( epl ) );
}

std::string Log::cutMessage( const std::string& msg, size_t limit ) {
    if( msg.size() > limit ) {
        std::string result = "[cut: " + std::to_string( limit ) + "] " + msg;
        result.resize( limit );
        return result;
    }

    return msg;
}

void Log::printLogCounter() {
    JsonObject logCounter;
    for( const auto& entry : m_logCounter ) {
        std::string logLevel = toString( entry.first );
        logCounter[ logLevel ] = entry.second.load();
    }
    info( cc::info( "Log Counter: " ) + cc::j( logCounter ) );
}

JsonObject Log::logCounterToJson() {
    JsonObject logCounter;
    for( const auto& entry : m_logCounter ) {
        std::string logLevel = toString( entry.first );
        logCounter[ logLevel ] = entry.second.load();
    }
    return logCounter;
}
