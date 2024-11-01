#pragma once

#include "update_manager/remote_interface/um_remote_interface.h"

#include "jsonrpc/message.h"

#include <string>

namespace HubPlatform {
namespace Uid {

// NOTE: legacy, do not use
class HubFirmwareCommandsHandler final {
public:
    HubFirmwareCommandsHandler( Services::Service& hostService );

    void invoke( const std::string& method, rpc::Message& rpcMessage );

private:
    void handleLuupSwitchRequest( rpc::Message& rpcMessage );

private:
    UpdateManager::RemoteInterface m_updateManager;
};

}  // namespace Uid
}  // namespace HubPlatform
