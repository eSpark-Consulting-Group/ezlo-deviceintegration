#pragma once

#include "bluetooth/proxy_method_helpers/proxy_async_method_helper.h"

#include <string>

namespace HubPlatform {

class UnregisterAgentHelper final : public ProxyAsyncMethodHelper {
public:
    UnregisterAgentHelper( Proxy& proxy, const std::string& path );

protected:
    void setup( DBusMessageIter* iter ) override;

private:
    std::string m_path;
};

}  // namespace HubPlatform
