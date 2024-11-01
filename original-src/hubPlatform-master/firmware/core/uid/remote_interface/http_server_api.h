#pragma once
#include "uid/remote_interface/http_server_remote_interface.h"
#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace Services {
class Service;
}
namespace Uid {

class HttpServerApi : public HttpServerRemoteInterface {
public:
    explicit HttpServerApi( Services::Service& hostService );
    ~HttpServerApi() override = default;

public:
    Json createLinkUrl( std::string pluginId, Json params ) override;
    bool deleteLinkUrl( std::string pluginId, std::string linkId ) override;
    Json getLinkInfo( std::string pluginId, std::string linkId ) override;

    bool closeConnection( std::string pluginId, std::string connectionId ) override;
    void sendData( std::string pluginId, Json params ) override;
    Json getConnectionInfo( std::string pluginId, std::string connectionId ) override;

private:
    Services::Service& m_hostService;
};

}  // namespace Uid
}  // namespace HubPlatform
