#pragma once

#include "update_manager/remote_interface/um_remote_interface.h"

#include "jsonrpc/message.h"

#include <string>

namespace HubPlatform {
namespace Uid {

class Service;

class HubProvisionInfoCommandsHandler final {
public:
    HubProvisionInfoCommandsHandler( Service& hostService );

    void invoke( const std::string& method, rpc::Message& rpcMessage );

private:
    void handleSetRequest( rpc::Message& rpcMessage );

private:
    Service& m_hostService;
    UpdateManager::RemoteInterface m_updateManager;
};

}  // namespace Uid
}  // namespace HubPlatform