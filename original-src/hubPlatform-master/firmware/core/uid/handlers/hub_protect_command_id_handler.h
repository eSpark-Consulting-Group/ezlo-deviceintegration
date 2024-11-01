#include "uid/ui_service.h"
#include "jsonrpc/message.h"

namespace HubPlatform {
namespace Uid {

class HubProtectCommandIdHandler final {
public:
    explicit HubProtectCommandIdHandler( Uid::Service& service );

    void handleCommandIdList( rpc::Message& rpcMessage );

private:
    Uid::Service& m_service;
};

}  // namespace Uid
}  // namespace HubPlatform
