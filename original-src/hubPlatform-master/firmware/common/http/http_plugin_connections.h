#pragma once

#include "http/connection.h"
#include <string>
#include <curl/curl.h>

namespace HubPlatform {
namespace CommonHttp {

class HttpPluginConnections {
public:
    using PluginId = std::string;
    using ConnectionId = std::string;

public:
    virtual ~HttpPluginConnections() = default;

    virtual ConnectionPtr findConnection( const PluginId& pluginId, const ConnectionId& connId ) = 0;
    virtual ConnectionPtr findByHandle( CURL* handle ) const = 0;
    virtual bool closeConnection( const PluginId& pluginId,
                                  const ConnectionId& connId,
                                  bool removeFromMulti,
                                  bool throwIfConnectionIdInvalid = true ) = 0;

    virtual bool isStopped() const = 0;
};

}  // namespace CommonHttp
}  // namespace HubPlatform