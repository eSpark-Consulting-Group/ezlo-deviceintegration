#pragma once

#include "bluetooth/proxy_method_helpers/proxy_async_method_helper.h"

namespace HubPlatform {

class RemoveDeviceHelper final : public ProxyAsyncMethodHelper {
public:
    RemoveDeviceHelper( Proxy& proxy, std::string deviceProxyPath );

protected:
    void setup( DBusMessageIter* iter ) override;

private:
    std::string m_deviceProxyPath;
};

}  // namespace HubPlatform
