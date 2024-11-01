#pragma once

#include "bluetooth/proxy_method_helpers/proxy_async_method_helper.h"

#include <string>

namespace HubPlatform {

class RegisterAgentHelper final : public ProxyAsyncMethodHelper {
public:
    RegisterAgentHelper( Proxy& proxy, const std::string& path, const std::string& capability );

protected:
    void setup( DBusMessageIter* iter ) override;

private:
    std::string m_path;
    std::string m_capability;
};

}  // namespace HubPlatform
