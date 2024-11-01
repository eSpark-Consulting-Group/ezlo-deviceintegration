#pragma once

#include "bluetooth/proxy_method_helpers/proxy_async_method_helper.h"

namespace HubPlatform {

class PairHelper : public ProxyAsyncMethodHelper {
public:
    explicit PairHelper( Proxy& proxy )
    : ProxyAsyncMethodHelper( proxy, "Pair" ) {}
};

}  // namespace HubPlatform
