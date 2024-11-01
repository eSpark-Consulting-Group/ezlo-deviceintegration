#pragma once

#include "bluetooth/proxy_method_helpers/proxy_async_method_helper.h"

namespace HubPlatform {

class StartNotifyHelper final : public ProxyAsyncMethodHelper {
public:
    explicit StartNotifyHelper( Proxy& proxy )
    : ProxyAsyncMethodHelper( proxy, "StartNotify" ) {}
};

}  // namespace HubPlatform
