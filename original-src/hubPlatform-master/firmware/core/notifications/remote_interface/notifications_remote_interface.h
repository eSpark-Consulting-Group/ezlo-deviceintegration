#pragma once

#include "notifications/remote_interface/notifications_base_remote_interface.h"
#include "service_interfaces/service.h"

#include <memory>

namespace HubPlatform {
namespace Services {
class Service;
}
namespace Notifications {

class RemoteInterface final : public BaseRemoteInterface {
public:
    RemoteInterface( Services::Service& hostService );

    JsonObject smtpSettingsSet( const JsonObject& params ) override;
    JsonObject smtpSettingsGet() override;
    JsonObject sendEmail( const JsonObject& params ) override;

private:
    Services::Service& m_hostService;
};

}  // namespace Notifications
}  // namespace HubPlatform
