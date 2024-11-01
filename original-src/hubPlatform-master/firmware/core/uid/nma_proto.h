#pragma once

#include "uid/proto.h"
#include "hardware_manager/remote_interface/hm_remote_interface.h"

namespace HubPlatform {
namespace Uid {

namespace UT {
class NmaProtoTestSuite;
}

class NmaProto : public Proto {
public:
    using Proto::Proto;

protected:
    bool invokeHandler( const std::string& method, rpc::Message& message ) override;

private:
    void registered( rpc::Message& rpcMessage );
    void sendLogToServer( rpc::Message& rpcMessage );
    void getLogTail( rpc::Message& rpcMessage );
    void updateUsers( rpc::Message& rpcMessage );
    void handleCloudAccessKeysControllerSync( rpc::Message& rpcMessage );

    friend class UT::NmaProtoTestSuite;
};

}  // namespace Uid
}  // namespace HubPlatform
