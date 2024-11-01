#pragma once

#include "bluetooth/proxy_method_helpers/proxy_async_method_helper.h"

namespace HubPlatform {

class StopDiscoveryHelper final : public ProxyAsyncMethodHelper {
public:
    explicit StopDiscoveryHelper( Proxy& proxy )
    : ProxyAsyncMethodHelper( proxy, "StopDiscovery" ) {}
};

}  // namespace HubPlatform
