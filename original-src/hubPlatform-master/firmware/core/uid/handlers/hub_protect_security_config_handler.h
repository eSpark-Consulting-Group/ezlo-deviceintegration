#include "uid/ui_service.h"
#include "jsonrpc/message.h"

namespace HubPlatform {
namespace Uid {

class HubProtectSecurityConfigHandler final {
public:
    explicit HubProtectSecurityConfigHandler( Uid::Service& service );

    void handleSecurityConfigList( rpc::Message& rpcMessage );

    bool isBypassSet( const std::string& securityConfigId );

private:
    Uid::Service& m_service;
};

}  // namespace Uid
}  // namespace HubPlatform
