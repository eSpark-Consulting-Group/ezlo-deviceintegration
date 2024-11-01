#pragma once

#include "service_interfaces/service.h"

#include "notifications/remote_interface/notifications_remote_interface.h"

namespace HubPlatform {
namespace Uid {

class Service;

class HubNotificationsHandler final {
public:
    explicit HubNotificationsHandler( Uid::Service& hostService );

    void handleSMTPSettingsSet( rpc::Message& rpcMessage );
    void handleSMTPSettingsGet( rpc::Message& rpcMessage );
    void handleSMTPSend( rpc::Message& rpcMessage );

private:
    Notifications::RemoteInterface m_notifications;
};

}  // namespace Uid
}  // namespace HubPlatform
