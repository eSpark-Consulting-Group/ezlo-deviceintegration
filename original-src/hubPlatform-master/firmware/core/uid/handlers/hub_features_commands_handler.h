#pragma once

#include "cored/remote_interface/cored_remote_interface.h"
#include "service_interfaces/service.h"
#include "jsonrpc/message.h"

namespace HubPlatform {
namespace Uid {

class Service;

class HubFeaturesCommandsHandler final {
public:
    HubFeaturesCommandsHandler( Uid::Service& hostService );

    void handleHubFeaturesList( rpc::Message& rpcMessage ) const;
    void handleHubFeatureStatusSet( rpc::Message& rpcMessage );

    Json getFeaturesList() const;

private:
    void emitEvent( MessageSubClasses msgClass, const JsonObject& payload ) const;

private:
    Service& m_service;
    CoreDRemoteInterface::CoreDRemoteInterface m_core;
};

}  // namespace Uid
}  // namespace HubPlatform