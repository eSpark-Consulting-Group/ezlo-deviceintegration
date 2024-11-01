#pragma once

#include <stdexcept>

namespace HubPlatform {
namespace Notifications {

class NotificationsError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
    explicit NotificationsError( const std::string& msg )
    : runtime_error( msg ){};
};

class SettingsError : public NotificationsError {
public:
    explicit SettingsError( const std::string& msg )
    : NotificationsError( msg ){};
};

class NotificationsManagerError : public NotificationsError {
public:
    explicit NotificationsManagerError( const std::string& msg )
    : NotificationsError( msg ){};
};

}  // namespace Notifications
}  // namespace HubPlatform
