#pragma once

#include "jsonrpc/message.h"
#include "system_utils/ssh.h"
#include "service_interfaces/service.h"

#include <string>
#include <mutex>
#include <set>

namespace HubPlatform {
namespace Uid {

class HubSshTunnelCommandsHandler final {
public:
    HubSshTunnelCommandsHandler( Services::Service& hostService );

    void invoke( const std::string& method, rpc::Message& rpcMessage );

private:
    void handleCreateRequest( rpc::Message& rpcMessage );
    void handleDeleteAllRequest( rpc::Message& rpcMessage );

private:
    Services::Service& m_service;
};

}  // namespace Uid
}  // namespace HubPlatform
