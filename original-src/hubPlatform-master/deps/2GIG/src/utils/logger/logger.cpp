#include "utils/logger/logger.h"

#include "2gig/instrumentation_client.h"

#include <sstream>
#include <iomanip>

namespace TwoGig {

Logger::Logger( InstrumentationClient* instrumentationClient )
: m_instrumentationClient( instrumentationClient ) {}

void Logger::info( const std::string& message ) const {
    if( m_instrumentationClient )
        m_instrumentationClient->info( message );
}

void Logger::warn( const std::string& message ) const {
    if( m_instrumentationClient )
        m_instrumentationClient->warn( message );
}

void Logger::error( const std::string& message ) const {
    if( m_instrumentationClient )
        m_instrumentationClient->error( message );
}

void Logger::debug( const std::string& message ) const {
    if( m_instrumentationClient )
        m_instrumentationClient->debug( message );
}

void Logger::trace( const std::string& message ) const {
    if( m_instrumentationClient )
        m_instrumentationClient->trace( message );
}

InstrumentationClient::LogLevel Logger::getLogLevel() const {
    if( m_instrumentationClient ) {
        return m_instrumentationClient->getLogLevel();
    }

    return InstrumentationClient::LogLevel::Debug;
}

std::string Logger::bytesToString( const uint8_t* data, size_t size ) {
    std::stringstream str;

    str << std::setfill( '0' ) << std::hex;
    for( size_t i = 0; i != size; ++i )
        str << std::setw( 2 ) << (unsigned int) data[ i ] << " ";

    char lastSpace;
    str >> lastSpace;

    return str.str();
}

}  // namespace TwoGig
