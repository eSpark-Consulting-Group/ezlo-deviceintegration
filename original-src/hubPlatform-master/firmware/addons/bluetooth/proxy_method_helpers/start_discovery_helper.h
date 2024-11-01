#pragma once

#include "bluetooth/proxy_method_helpers/proxy_async_method_helper.h"

namespace HubPlatform {

class StartDiscoveryHelper final : public ProxyAsyncMethodHelper {
public:
    explicit StartDiscoveryHelper( Proxy& proxy )
    : ProxyAsyncMethodHelper( proxy, "StartDiscovery" ) {}
};

}  // namespace HubPlatform
