#pragma once

#include "uid/ui_service.h"
#include "jsonrpc/message.h"

#include <string>

namespace HubPlatform {
namespace Uid {

class HubSwingerShutdownCommandsHandler final {
public:
    HubSwingerShutdownCommandsHandler( Uid::Service& service );

    void invoke( const std::string& method, rpc::Message& rpcMessage );

private:
    void handleSwingerShutdownList( rpc::Message& rpcMessage );
    void handleSwingerShutdownReset( rpc::Message& rpcMessage );
    void handleSwingerShutdownDisableAdd( rpc::Message& rpcMessage );
    void handleSwingerShutdownDisableRemove( rpc::Message& rpcMessage );
    void handleSwingerShutdownLimitsSet( rpc::Message& rpcMessage );
    void handleSwingerShutdownLimitsReset( rpc::Message& rpcMessage );

private:
    Service& getService();

private:
    Uid::Service& m_service;
};

}  // namespace Uid
}  // namespace HubPlatform
