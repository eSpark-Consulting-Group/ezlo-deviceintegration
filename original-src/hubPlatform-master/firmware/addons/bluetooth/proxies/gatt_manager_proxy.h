#pragma once

#include "bluetooth/proxies/proxy.h"

#include <string>

namespace HubPlatform {

class GattManagerProxy final : public Proxy {
public:
    explicit GattManagerProxy( GDBusProxy* proxy );

    void registerApplication( const std::string& path );
};

}  // namespace HubPlatform
