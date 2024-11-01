#pragma once

#include "uid/ui_service.h"
#include "jsonrpc/message.h"

#include <string>

namespace HubPlatform {
namespace Uid {

class HubWalkTestCommandsHandler final {
public:
    HubWalkTestCommandsHandler( Uid::Service& service );

    void invoke( const std::string& method, rpc::Message& rpcMessage );

private:
    void handleProtectSystemTest( rpc::Message& rpcMessage );
    void handleProtectSystemTestStop( rpc::Message& rpcMessage );
    void handleProtectSystemTestExtend( rpc::Message& rpcMessage );

private:
    Service& getService();

private:
    Uid::Service& m_service;
};

}  // namespace Uid
}  // namespace HubPlatform
