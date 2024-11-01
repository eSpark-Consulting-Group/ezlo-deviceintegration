#pragma once

#include "bluetooth/proxies/proxy.h"

namespace HubPlatform {

class GattAttributeProxy : public Proxy {
public:
    explicit GattAttributeProxy( GDBusProxy* proxy );

    std::string getUUID() const;
};

}  // namespace HubPlatform
