#include "extensions_manager/remote_interface/em_remote_interface.h"
#include "jsonrpc/message.h"
#include <string>

namespace HubPlatform {
namespace Uid {
class CustomNodesList {
public:
    CustomNodesList( Services::Service& service );
    ~CustomNodesList() = default;
    void handler( rpc::Message& rpcMessage );

private:
    void setWrongParamError( rpc::Message& rpcMessage, const std::string& key );

private:
    Services::Service& m_service;
};
}  // namespace Uid
}  // namespace HubPlatform