#pragma once
#include "uid/remote_interface/http_server_remote_interface.h"
#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace Services {
class Service;
}
namespace Uid {

class HttpServerError;
class HttpServerRemoteInterface;

class RemoteInterface final {
public:
    RemoteInterface( Services::Service& hostService );
    ~RemoteInterface();

    bool isRegisteredOnCloud() const;

    JsonObject getHubInfo() const;

    // http_server
    HttpServerRemoteInterface& httpServer();
    JsonObject fetchLocalServerPorts() const;

private:
    Services::Service& m_hostService;
    std::unique_ptr< HttpServerRemoteInterface > m_httpServerApiUPtr;
};

}  // namespace Uid
}  // namespace HubPlatform
