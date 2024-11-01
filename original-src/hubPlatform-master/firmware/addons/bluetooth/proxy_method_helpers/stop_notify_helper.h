#pragma once

#include "bluetooth/proxy_method_helpers/proxy_async_method_helper.h"

namespace HubPlatform {

class StopNotifyHelper final : public ProxyAsyncMethodHelper {
public:
    explicit StopNotifyHelper( Proxy& proxy )
    : ProxyAsyncMethodHelper( proxy, "StopNotify" ) {}
};

}  // namespace HubPlatform
