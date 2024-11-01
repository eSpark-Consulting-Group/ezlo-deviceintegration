#pragma once

#include <string>

namespace TwoGig {

struct InstrumentationClient {
    virtual ~InstrumentationClient() = default;

    enum class LogLevel { Trace = 0, Debug, Info, Warning, Error };

    virtual void trace( const std::string& message ) = 0;
    virtual void debug( const std::string& message ) = 0;
    virtual void info( const std::string& message ) = 0;
    virtual void warn( const std::string& message ) = 0;
    virtual void error( const std::string& message ) = 0;

    virtual LogLevel getLogLevel() const = 0;
};

}  // namespace TwoGig
