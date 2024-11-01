#include "bluetooth/proxies/gatt_descriptor_proxy.h"

namespace HubPlatform {

GattDescriptorProxy::GattDescriptorProxy( GDBusProxy* proxy )
: GattAttributeProxy( proxy ) {}

std::string GattDescriptorProxy::getCharacteristicPath() const {
    return getProperty< std::string >( "Characteristic" );
}

}  // namespace HubPlatform
