#pragma once

#include "bluetooth/proxies/proxy.h"

namespace HubPlatform {

class AgentManagerProxy final : public Proxy {
public:
    explicit AgentManagerProxy( GDBusProxy* proxy );

    void registerAgent( const std::string& path, const std::string& capability );
    void requestDefaultAgent( const std::string& path );
    void unregisterAgent( const std::string& path );
};

}  // namespace HubPlatform
