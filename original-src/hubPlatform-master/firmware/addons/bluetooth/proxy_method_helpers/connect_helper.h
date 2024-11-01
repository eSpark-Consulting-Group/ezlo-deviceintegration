#pragma once

#include "bluetooth/proxy_method_helpers/proxy_async_method_helper.h"

namespace HubPlatform {

class ConnectHelper : public ProxyAsyncMethodHelper {
public:
    explicit ConnectHelper( Proxy& proxy )
    : ProxyAsyncMethodHelper( proxy, "Connect" ) {}
};

}  // namespace HubPlatform
