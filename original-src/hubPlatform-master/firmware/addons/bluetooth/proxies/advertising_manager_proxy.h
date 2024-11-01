#pragma once

#include "bluetooth/proxies/proxy.h"

#include <string>

namespace HubPlatform {

class AdvertisingManagerProxy final : public Proxy {
public:
    explicit AdvertisingManagerProxy( GDBusProxy* proxy );

    void startAdvertising( const std::string& path );
    void stopAdvertising( const std::string& path );
};

}  // namespace HubPlatform
