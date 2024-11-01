#pragma once

#include "jsonrpc/message.h"

namespace HubPlatform {
namespace Uid {

class Service;

class HubVidooCommandsHandler final {
public:
    HubVidooCommandsHandler( Uid::Service& hostService );

    void handleHubVidooElementsEvent( rpc::Message& rpcMessage );

private:
    Uid::Service& m_service;
};

}  // namespace Uid
}  // namespace HubPlatform
