#pragma once

#include "update_manager/remote_interface/um_remote_interface.h"
#include "hardware_manager/remote_interface/hm_remote_interface.h"
#include "cored/remote_interface/cored_interface.h"

#include "jsonrpc/message.h"

#include <string>

namespace HubPlatform {
namespace Uid {

class HubSoftwareCommandsHandler final {
public:
    HubSoftwareCommandsHandler( Services::Service& hostService );

    void invoke( const std::string& method, rpc::Message& rpcMessage );

private:
    void handleInfoGetRequest( rpc::Message& rpcMessage );
    void handleUpdatePlanRequest( rpc::Message& rpcMessage );
    void handleUpdateExecuteRequest( rpc::Message& rpcMessage );

    bool isUpdateEnabled() const;

private:
    UpdateManager::RemoteInterface m_updateManager;
    HardwareManager::RemoteInterface m_hardwareManager;
    CoreDRemoteInterface::CoreDInterfaceUPtr m_core;
};

}  // namespace Uid
}  // namespace HubPlatform
