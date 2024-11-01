#pragma once

#include "cored/remote_interface/cored_remote_interface.h"
#include "service_interfaces/service.h"
#include "jsonrpc/message.h"

#include "cloud_devices/remote_interface/cloud_devices_remote_interface.h"

namespace HubPlatform {
namespace Uid {

class Service;

class HubCloudSubscriptionsHandler final {
public:
    explicit HubCloudSubscriptionsHandler( Uid::Service& hostService );

    void handleCloudSubscriptionCreate( rpc::Message& rpcMessage );
    void handleCloudSubscriptionUpdate( rpc::Message& rpcMessage );
    void handleCloudSubscriptionDelete( rpc::Message& rpcMessage );
    void handleCloudSubscriptionsList( rpc::Message& rpcMessage );
    void handleCloudSubscriptionDispatch( rpc::Message& rpcMessage );

private:
    CloudDevices::RemoteInterface m_cloudDevices;
};

}  // namespace Uid
}  // namespace HubPlatform
