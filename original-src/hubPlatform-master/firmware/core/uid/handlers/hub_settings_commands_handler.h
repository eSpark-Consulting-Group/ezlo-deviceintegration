#pragma once

#include "cored/remote_interface/cored_interface.h"
#include "service_interfaces/service.h"
#include "jsonrpc/message.h"

namespace HubPlatform {
namespace Uid {

class Service;

class HubSettingsCommandsHandler final {
public:
    HubSettingsCommandsHandler( Uid::Service& hostService );

    void handleHubSettingsList( rpc::Message& rpcMessage );
    void handleSettingValueSet( rpc::Message& rpcMessage );

    std::vector< Json > getSettingsList() const;

private:
    CoreDRemoteInterface::CoreDInterfaceUPtr m_core;
};

}  // namespace Uid
}  // namespace HubPlatform