#include "2gig_instrumentation_client.h"

#include "log_utils/ezlo_log.h"

namespace HubPlatform {
namespace TwoGig {

void InstrumentationClientImpl::trace( const std::string& message ) {
    Log::trace( message );
}

void InstrumentationClientImpl::debug( const std::string& message ) {
    Log::debug( message );
}

void InstrumentationClientImpl::info( const std::string& message ) {
    Log::info( message );
}

void InstrumentationClientImpl::warn( const std::string& message ) {
    Log::warn( message );
}

void InstrumentationClientImpl::error( const std::string& message ) {
    Log::error( message );
}

InstrumentationClientImpl::LogLevel InstrumentationClientImpl::getLogLevel() const {
    switch( Log::getLogLevel() ) {
    case Log::TRACE:
        return LogLevel::Trace;

    case Log::DEBUG:
        return LogLevel::Debug;

    case Log::INFO:
        return LogLevel::Info;

    case Log::WARN:
        return LogLevel::Warning;

    case Log::ERROR:
    case Log::FATAL:
        return LogLevel::Error;

    default:
        break;
    }

    return LogLevel::Info;
}

}  // namespace TwoGig
}  // namespace HubPlatform