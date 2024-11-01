#include "bluetooth/proxies/gatt_attribute_proxy.h"
#include "gatt_attribute_proxy.h"

namespace HubPlatform {

GattAttributeProxy::GattAttributeProxy( GDBusProxy* proxy )
: Proxy( proxy ) {}

std::string GattAttributeProxy::getUUID() const {
    return getProperty< std::string >( "UUID" );
}
}  // namespace HubPlatform
