#pragma once

#include <string>
#include <stdexcept>

namespace HubPlatform {
namespace CloudDevices {
namespace LocalNotification {

class LocalNotificationsError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
    explicit LocalNotificationsError( const std::string& msg )
    : runtime_error( msg ){};
};

class DeserializationError : public LocalNotificationsError {
public:
    explicit DeserializationError( const std::string& msg )
    : LocalNotificationsError( msg ){};
};

class ArchiveError : public LocalNotificationsError {
public:
    explicit ArchiveError( const std::string& msg )
    : LocalNotificationsError( msg ){};
};

}  // namespace LocalNotification
}  // namespace CloudDevices
}  // namespace HubPlatform
