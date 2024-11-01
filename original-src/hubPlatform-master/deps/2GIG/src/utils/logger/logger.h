#pragma once

#include "2gig/instrumentation_client.h"

#include <string>

namespace TwoGig {

class Logger {
public:
    Logger( InstrumentationClient* instrumentationClient = nullptr );

    void trace( const std::string& message ) const;
    void debug( const std::string& message ) const;
    void info( const std::string& message ) const;
    void warn( const std::string& message ) const;
    void error( const std::string& message ) const;

    InstrumentationClient::LogLevel getLogLevel() const;

    static std::string bytesToString( const uint8_t* data, size_t size );

private:
    InstrumentationClient* m_instrumentationClient;
};

}  // namespace TwoGig
