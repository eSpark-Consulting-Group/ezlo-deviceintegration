#pragma once

#include "2gig/instrumentation_client.h"

namespace HubPlatform {
namespace TwoGig {

class InstrumentationClientImpl : public ::TwoGig::InstrumentationClient {
public:
    void trace( const std::string& message ) override;

    void debug( const std::string& message ) override;

    void info( const std::string& message ) override;

    void warn( const std::string& message ) override;

    void error( const std::string& message ) override;

    LogLevel getLogLevel() const override;
};

}  // namespace TwoGig
}  // namespace HubPlatform
