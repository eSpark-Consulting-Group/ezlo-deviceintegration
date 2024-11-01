#pragma once

#include "bluetooth/proxies/gatt_attribute_proxy.h"

#include <vector>

namespace HubPlatform {

class GattCharacteristicProxy : public GattAttributeProxy {
public:
    explicit GattCharacteristicProxy( GDBusProxy* proxy );

    void startNotify();
    void stopNotify();
    void writeValue( const std::vector< std::uint8_t >& data );
    std::vector< std::uint8_t > readValue();
    std::string getServicePath() const;
    std::vector< std::string > getFlags() const;
    std::vector< std::uint8_t > getCachedValue() const;
    bool isNotificationSupported() const;

public:
    static const char* NotifyFlag;
    static const char* IndicateFlag;
    static const char* ValuePropertyName;
};

}  // namespace HubPlatform
