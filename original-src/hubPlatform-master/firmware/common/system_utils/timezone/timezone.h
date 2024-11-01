#pragma once

#include <string>
#include <stdexcept>

namespace HubPlatform {
namespace system_utils {

class TimeZoneError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
    explicit TimeZoneError( const std::string& msg )
    : runtime_error( msg ){};
};

class TimeZone {
public:
    virtual ~TimeZone() = default;
    virtual void setTimeZone( const std::string& zoneName ) = 0;
    virtual std::string getTimeZone() = 0;
    virtual bool isTimeZoneDefault() = 0;
};

}  // namespace system_utils
}  // namespace HubPlatform
