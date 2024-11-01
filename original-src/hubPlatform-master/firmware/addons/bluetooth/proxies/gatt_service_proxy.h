#pragma once

#include "bluetooth/proxies/gatt_attribute_proxy.h"

namespace HubPlatform {

class GattServiceProxy final : public GattAttributeProxy {
public:
    explicit GattServiceProxy( GDBusProxy* proxy );

    std::string getDevicePath() const;
};

}  // namespace HubPlatform
