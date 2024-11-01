#pragma once

#include "bluetooth/proxy_method_helpers/proxy_async_method_helper.h"

namespace HubPlatform {

class DisconnectHelper : public ProxyAsyncMethodHelper {
public:
    explicit DisconnectHelper( Proxy& proxy )
    : ProxyAsyncMethodHelper( proxy, "Disconnect" ) {}
};

}  // namespace HubPlatform
