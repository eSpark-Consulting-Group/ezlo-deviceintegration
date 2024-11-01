#pragma once

#include "bus/messagesclasses.h"
#include "log_utils/ezlo_log.h"

#include <chrono>
#include <condition_variable>
#include <functional>
#include <queue>
#include <sstream>
#include <vector>
#include <sys/types.h>
#include <future>
#include <thread>

using DSKAsInts = std::vector< int >;

std::string time_str( time_t now = time( 0 ) );
std::string hexToString( const unsigned char* data, size_t size, bool upperCase = false );
std::string getHexString( const std::vector< unsigned char >& data, bool upperCase = false );
std::string intToHexString( int value, int width = 8 );
std::string ullToHexString( unsigned long long value, int width = 8 );
std::string zwaveDskToString( const uint8_t* data );
DSKAsInts getDSKAsArrayOfInts( const unsigned char* keyData, std::size_t size );
std::string getHostName();
std::string cutUriPort( const std::string& uri );
std::string toUpperFront( const std::string& str );
int64_t versionCalc( const std::string& version );
bool hasSpecialChars( const std::string& input );
std::string toUpper( const std::string& str );
std::string toLower( const std::string& str );
int hexToDec( unsigned char c );
bool getBinaryFromHex( const std::string& data, std::string& result );
std::string getLogLevelQuery( const std::string& logLevel );

std::string escapeSymbolsByFirst( const std::string& source, const char* symbols, size_t escapesCount = 1u );

static inline std::string quotes( const std::string& str ) {
    return ( std::string( "\"" ).append( str ).append( "\"" ) );
}

// TODO fixit
static inline void unquote( std::string& str ) {
    if( str.front() == '"' && str.back() == '"' ) {
        str.erase( 0, 1 );
        str.erase( str.length() - 1 );
    }
}

static inline std::string field( const std::string& fieldName,
                                 const std::string& fieldValue,
                                 bool quote = true,
                                 bool comma = true ) {
    return ( std::string( comma ? "," : "" )
                 .append( quotes( fieldName ) )
                 .append( ":" )
                 .append( quote ? quotes( fieldValue ) : fieldValue ) );
}

void tryToCall( std::function< void() > f, int times, const std::string& message );
bool waitForCondition( std::function< bool() > condition,
                       const std::chrono::milliseconds& timeout,
                       const std::chrono::milliseconds& checkInterval );

template < class T >
std::future< T > createDetachedTask( std::function< T() > f ) {
    std::packaged_task< T() > task( std::move( f ) );
    auto future = task.get_future();
    std::thread( std::move( task ) ).detach();
    return future;
}

void replaceAll( std::string& str, const std::string& before, const std::string& after );

void delayMilliseconds( int milliseconds );

namespace utils {

template < class IntegralType >
std::string to_string( const IntegralType& value ) {
    std::ostringstream out;
    out << value;
    return out.str();
}

// Composes an std::string object from arbitrary arguments.
// Accepts everything operator<< (member and non-member) does.
// Usage:
//      // intentionally assigned with const ref
//      // see
//      https://herbsutter.com/2008/01/01/gotw-88-a-candidate-for-the-most-important-const/
//      auto const& msg = stringify("1st = ", 11, ", 2nd = ", 22.22, ", 3rd = ",
//      std::string("VALUE"), ", 4th = ", std::boolalpha, false); log(msg);

inline std::ostream& operator<<( std::ostream& out, char const c ) {
    out << static_cast< int const >( c );
    return out;
}

inline std::ostream& operator<<( std::ostream& out, unsigned char const c ) {
    out << static_cast< unsigned int const >( c );
    return out;
}

template < typename T >
inline void stringify_helper( std::ostringstream& oss, T const& last ) {
    oss << last;
}

template < typename T, typename... Args >
inline void stringify_helper( std::ostringstream& oss, T const& head, Args const&... tail ) {
    oss << head;
    stringify_helper( oss, tail... );
}

template < typename... Args >
std::string stringify( Args const&... args ) {
    std::ostringstream oss;
    stringify_helper( oss, args... );
    return oss.str();
}

template < typename Message >
class MailBoxOne {
public:
    class MessageOpt {
    public:
        MessageOpt() = default;
        MessageOpt( Message&& message )
        : m_message( std::move( message ) )
        , m_isInit( true ) {}

        const Message& operator*() const {
            // TODO: handle uninitialized state.
            return m_message;
        }

        const Message* operator->() const {
            return &m_message;
        }

        Message* operator->() {
            return &m_message;
        }

        explicit operator bool() const {
            return m_isInit;
        }

    private:
        bool m_isInit = false;
        Message m_message;
    };

    void send( Message message ) {
        std::unique_lock< std::mutex > locker( m_mutex );
        m_queue.emplace( std::move( message ) );
        locker.unlock();
        m_conditionVariable.notify_one();
    }

    template < class Rep, class Period >
    MessageOpt wait( const std::chrono::duration< Rep, Period >& real_time ) {
        std::unique_lock< std::mutex > locker( m_mutex );
        if( m_conditionVariable.wait_for( locker, real_time, [this]() { return !m_queue.empty(); } ) ) {
            Message message = std::move( m_queue.front() );
            m_queue.pop();
            return MessageOpt( std::move( message ) );
        }
        else {
            return MessageOpt();
        }
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_conditionVariable;
    std::queue< Message > m_queue;
};

bool writeToFile( const std::string& fileName, const std::string& text );
std::string readPipe( const char* command );
std::string readPipe( const std::string& command );

template < class Iter >
std::string getHexString( const Iter& first, const Iter& last, bool upperCase ) {
    std::ostringstream result;
    result << std::hex << std::setfill( '0' ) << ( upperCase ? std::uppercase : std::nouppercase );
    for( Iter it = first; it != last; it++ ) {
        auto data = *it;
        result << std::setw( 2 ) << (int) data;
    }
    return result.str();
}

struct AfterReturn {
    using Action = std::function< void() >;
    AfterReturn( std::function< void() > action );
    ~AfterReturn();

private:
    Action m_action;
};

}  // namespace utils

namespace details {
template < typename T >
T& processArg( T& arg ) {
    return arg;
}

inline const char* processArg( const std::string& arg ) {
    return arg.c_str();
}

}  // namespace details
