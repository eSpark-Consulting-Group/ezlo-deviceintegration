#pragma once

#include "bluetooth/proxies/gatt_attribute_proxy.h"

namespace HubPlatform {

class GattDescriptorProxy : public GattAttributeProxy {
public:
    explicit GattDescriptorProxy( GDBusProxy* proxy );

    std::string getCharacteristicPath() const;
};

}  // namespace HubPlatform
