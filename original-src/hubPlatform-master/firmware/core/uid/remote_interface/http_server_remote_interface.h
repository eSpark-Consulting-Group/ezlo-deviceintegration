#pragma once
#include "json_utils/json_fwd.h"
#include <stdexcept>
#include <string>

namespace HubPlatform {
namespace Uid {

class HttpServerError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

// http_server api
class HttpServerRemoteInterface {
public:
    virtual ~HttpServerRemoteInterface() = default;

public:
    virtual Json createLinkUrl( std::string pluginId, Json params ) = 0;
    virtual bool deleteLinkUrl( std::string pluginId, std::string linkId ) = 0;
    virtual Json getLinkInfo( std::string pluginId, std::string linkId ) = 0;

    virtual bool closeConnection( std::string pluginId, std::string connectionId ) = 0;
    virtual void sendData( std::string pluginId, Json params ) = 0;
    virtual Json getConnectionInfo( std::string pluginId, std::string connectionId ) = 0;
};

}  // namespace Uid
}  // namespace HubPlatform
