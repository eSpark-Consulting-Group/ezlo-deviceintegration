#pragma once

#include "bluetooth/proxy_method_helpers/proxy_async_method_helper.h"

namespace HubPlatform {

class RegisterAdvertismentHelper final : public ProxyAsyncMethodHelper {
public:
    RegisterAdvertismentHelper( Proxy& proxy, const std::string& path );

protected:
    void setup( DBusMessageIter* iter ) override;

private:
    std::string m_path;
};

}  // namespace HubPlatform
