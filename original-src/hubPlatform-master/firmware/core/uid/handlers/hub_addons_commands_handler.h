#pragma once

#include "uid/ui_service.h"
#include "jsonrpc/message.h"

#include <string>

namespace HubPlatform {
namespace Uid {

class HubAddonsCommandsHandler final {
public:
    HubAddonsCommandsHandler( Uid::Service& service );

    void invoke( const std::string& method, rpc::Message& rpcMessage );

private:
    Uid::Service& m_service;
};

}  // namespace Uid
}  // namespace HubPlatform